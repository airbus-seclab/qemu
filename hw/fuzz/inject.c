/*
 * QEMU American Fuzzy Lop board
 * code injection
 *
 * Copyright (c) 2019 S. Duverger Airbus
 * GPLv2
 */
#include "qemu/afl.h"

/*
 * AFL generates input on stdin, translate it to partition code.
 * The code generators are target dependent.
 */
#if defined(AFL_DUMP_TESTCASE) || defined(AFL_DUMP_PARTITION)
static void afl_dump_mem(const char *log, uint8_t *mm, size_t size)
{
   size_t i;
   debug("-- START %s --\n", log);
#ifdef AFL_DEBUG
   for(i=0 ; i<size ; i++) {
      qemu_log("\\x%02x", mm[i]);
   }
   qemu_log("\n");
#endif
   debug("-- END %s --\n", log);
}
#endif

#ifdef AFL_INJECT_TESTCASE
size_t afl_inject_test_case(afl_t *afl)
{
   struct stat st; fstat(0, &st);
   void *mm = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, 0, 0);

   if (mm == MAP_FAILED) {
      error_report("Unable to mmap test case");
      exit(EXIT_FAILURE);
   }
#ifdef AFL_DUMP_TESTCASE
   afl_dump_mem("TEST CASE", mm, st.st_size);
#endif

   uint8_t *out = afl->ram_ptr + afl->config.tgt.fuzz_inj;
   ssize_t len = afl_gen_code(mm, st.st_size, out, afl->config.tgt.part_size);

   if (len > 0) {
      afl_mem_invalidate(afl->ram_mr, afl->config.tgt.fuzz_inj, len);
#ifdef AFL_DUMP_PARTITION
      afl_dump_mem("PART CODE", out, len);
#endif
   }

   munmap(mm, st.st_size);
   return len;
}
#endif // INJECT_TESTCASE


