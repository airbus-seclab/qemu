/*
 * QEMU American Fuzzy Lop board
 * trace
 *
 * Copyright (c) 2019 S. Duverger Airbus
 * GPLv2
 */
#include "qemu/afl.h"


/*
 * cf. afl-src/hash.h
 * cf. afl-src/config.h
 */
#ifdef AFL_TRACE_CHKSM

#define HASH_CONST     0xa5b35705
#define ROL64(_x, _r)  ((((u64)(_x)) << (_r)) | (((u64)(_x)) >> (64 - (_r))))

static inline u32 hash32(const void* key, u32 len, u32 seed) {

  const u64* data = (u64*)key;
  u64 h1 = seed ^ len;

  len >>= 3;

  while (len--) {

    u64 k1 = *data++;

    k1 *= 0x87c37b91114253d5ULL;
    k1  = ROL64(k1, 31);
    k1 *= 0x4cf5ad432745937fULL;

    h1 ^= k1;
    h1  = ROL64(h1, 27);
    h1  = h1 * 5 + 0x52dce729;

  }

  h1 ^= h1 >> 33;
  h1 *= 0xff51afd7ed558ccdULL;
  h1 ^= h1 >> 33;
  h1 *= 0xc4ceb9fe1a85ec53ULL;
  h1 ^= h1 >> 33;

  return h1;

}

void afl_trace_checksum(afl_t *afl, const char *log)
{
   debug("map %s hash32: 0x%x\n", log,
         hash32(afl->trace_bits, afl->config.afl.trace_size, HASH_CONST));
}
#endif

/*
 * cf. afl-src/afl-fuzz.c
 */
#ifdef AFL_TRACE_COUNT

#define FF(_b)  (0xff << ((_b) << 3))
static inline u32 count_bytes(u8* mem, u32 size) {

  u32* ptr = (u32*)mem;
  u32  i   = (size >> 2);
  u32  ret = 0;

  while (i--) {

    u32 v = *(ptr++);

    if (!v) continue;
    if (v & FF(0)) ret++;
    if (v & FF(1)) ret++;
    if (v & FF(2)) ret++;
    if (v & FF(3)) ret++;

  }

  return ret;
}

void afl_trace_count(afl_t *afl, const char *log)
{
   debug("map %s count: 0x%x\n", log,
         count_bytes(afl->trace_bits, afl->config.afl.trace_size));
}
#endif


#ifdef AFL_TRACE_MMIO
/*
 * Special IO Memory region used by AFL for code coverage
 */
static void afl_trace_write(void *opaque, hwaddr addr,
                            uint64_t data, unsigned size)
{
   afl_t   *afl = (afl_t*)opaque;
   uint8_t *ptr = (uint8_t*)afl->trace_bits;

   if (size != 1) {
      debug("%s(addr=0x"TARGET_FMT_plx", size=%d): "
            "unsupported write @ pc = 0x"TARGET_FMT_lx"\n"
            ,__func__, addr, size, afl_get_pc(&afl->arch));
      return;
   }

   debug("%p w %d\n", &ptr[addr], (uint8_t)data);
   ptr[addr] = data & 0xff;

#ifdef AFL_TRACE_CHKSM
   afl_trace_checksum(afl, "afl_trace_write");
#endif
}

static uint64_t afl_trace_read(void *opaque, hwaddr addr, unsigned size)
{
   afl_t   *afl = (afl_t*)opaque;
   uint8_t *ptr = (uint8_t*)afl->trace_bits;

   if (size != 1) {
      debug("%s(addr=0x"TARGET_FMT_plx", size=%d): "
            "unsupported read @ pc = 0x"TARGET_FMT_lx"\n"
            ,__func__, addr, size, afl_get_pc(&afl->arch));
      return 0;
   }

   /* debug("%p r %d\n", &ptr[addr], ptr[addr]); */
   return ptr[addr];
}

static const MemoryRegionOps afl_trace_ops = {
   .read = afl_trace_read,
   .write = afl_trace_write,
   .endianness = DEVICE_NATIVE_ENDIAN,
};

void afl_init_trace_mem(afl_t *afl)
{
   memory_region_init_io(
      &afl->trace_mr, NULL,
      &afl_trace_ops, afl,
      "afl_trace_bits", afl->config.afl.trace_size);

   memory_region_add_subregion(get_system_memory(),
                               afl->config.afl.trace_addr,
                               &afl->trace_mr);
}
#else
/*
 * Use a RAM MemoryRegion at specific host location
 */
void afl_init_trace_mem(afl_t *afl)
{
   if ((ulong)afl->trace_bits & (((ulong)sysconf(_SC_PAGE_SIZE))-1)) {
      error_report("AFL trace map must be page aligned");
      exit(EXIT_FAILURE);
   }

   memory_region_init_ram_ptr(
      &afl->trace_mr, NULL, "afl_trace_bits",
      afl->config.afl.trace_size, afl->trace_bits);

#ifdef AFL_CONTROL_CSWITCH
   /* fake trace bitmap */
   afl->fake_bits = mmap(NULL, afl->config.afl.trace_size,
                         PROT_READ|PROT_WRITE,
                         MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

   if (afl->fake_bits == MAP_FAILED) {
      error_report("AFL fake bits mmap() failed");
      exit(EXIT_FAILURE);
   }

   memory_region_init_ram_ptr(
      &afl->fake_mr, NULL,
      "afl_fake_bits", afl->config.afl.trace_size, afl->fake_bits);

   memory_region_add_subregion_overlap(get_system_memory(),
                                       afl->config.afl.trace_addr,
                                       &afl->fake_mr, 0);

   memory_region_add_subregion_overlap(get_system_memory(),
                                       afl->config.afl.trace_addr,
                                       &afl->trace_mr, 1);
#else
   memory_region_add_subregion(get_system_memory(),
                               afl->config.afl.trace_addr,
                               &afl->trace_mr);
#endif

   /* debug("AFL trace MR %p ram block %p offset 0x%lx\n", */
   /*       &afl->trace_mr, */
   /*       afl->trace_mr.ram_block, */
   /*       afl->trace_mr.ram_block->offset */
   /*    ); */
}
#endif
