/*
 * QEMU American Fuzzy Lop board
 * memory mgmt
 *
 * Copyright (c) 2019 S. Duverger Airbus
 * GPLv2
 */
#include "qemu/afl.h"

/*
 * When accessing VM memory we should use cpu_physical_memory_rw().
 *
 * However, when we access RAM it ends by doing a memcpy() to the
 * RAMBlock and then invalidate_and_set_dirty(). We want to prevent
 * the unnecessary operations as we are able to directly access RAM
 * from host and exeactly what kind of memory we are modifying.
 *
 * The invalidate_and_set_dirty() function behaves like
 * memory_region_set_dirty() by calling
 * cpu_physical_memory_set_dirty_range() but is defined static so we
 * can't use it here. There is a slight difference in its code by
 * calling cpu_physical_memory_range_includes_clean() and testing if
 * the dirty log mask includes DIRTY_MEMORY_CODE so that it will
 * explicitly calls tb_invalidate_phys_range() and then clears its.
 *
 * When creating RAM memory region, the dirty_log_mask is set with
 * DIRTY_CODE and as we are modifying translated blocks (overwrite
 * code) we prefer write our own invalidation procedure.
 *
 * We will behave as in invalidate_and_set_dirty() but knowing that we
 * are operating with DIRTY_CODE. Thus we know by clearing dirty mask
 * after tb_invalidate_phys_range() that we don't need to call
 * cpu_physical_memory_set_dirty_range().However we still implement
 * the xen specific case present in this later function.
 */
#if defined(AFL_RAM_GUARD) ||                                   \
   defined(AFL_DUMMY_CASE) ||                                   \
   defined(AFL_INJECT_TESTCASE)
void afl_mem_invalidate(MemoryRegion *mr, hwaddr addr, hwaddr len)
{
   assert(mr->ram_block);
   addr += memory_region_get_ram_addr(mr);

   if (!kvm_enabled()) {
      assert(tcg_enabled());
      //tb_lock();
      tb_invalidate_phys_range(addr, addr + len);
      //tb_unlock();
   }

   if (xen_enabled())
      xen_hvm_modified_memory(addr, len);
}
#endif
