/*
 * QEMU American Fuzzy Lop board
 * cpu mgmt
 *
 * Copyright (c) 2019 S. Duverger Airbus
 * GPLv2
 */
#include "qemu/afl.h"

/*
 * Initial breakpoint (EP_FUZZ) should not be of SW type because
 * memory will be overwritten while the kernel is loaded by its boot
 * loader.
 */
void afl_remove_breakpoint(afl_t *afl, target_ulong pc)
{
   CPUState *cpu = CPU(afl->arch.cpu);

   /* debug("%s: 0x%x\n", __func__, pc); */
   if (!kvm_enabled()) {
      cpu_breakpoint_remove(cpu, pc, BP_GDB);
   } else {
      kvm_remove_breakpoint(cpu, pc, 1, GDB_BREAKPOINT_HW);
      //kvm_remove_breakpoint(cpu, pc, 1, GDB_BREAKPOINT_SW);
   }
}

void afl_insert_breakpoint(afl_t *afl, target_ulong pc)
{
   CPUState *cpu = CPU(afl->arch.cpu);

   /* debug("%s: 0x%x\n", __func__, pc); */
   if (!kvm_enabled()) {
      cpu_breakpoint_insert(cpu, pc, BP_GDB, NULL);
   } else {
      kvm_insert_breakpoint(cpu, pc, 1, GDB_BREAKPOINT_HW);
      //kvm_insert_breakpoint(cpu, pc, 1, GDB_BREAKPOINT_SW);
   }
   cpu_synchronize_state(cpu);
}
