/*
 * QEMU American Fuzzy Lop PC board
 * Copyright (c) 2018 S. Duverger Airbus
 * GPLv2
 */
#include "qemu/afl.h"


void afl_check_intercept(CPUX86State *env, int intno, int is_int,
                         int error_code, uintptr_t retaddr)
{
   CPUState *cs = CPU(x86_env_get_cpu(env));

   /* software interrupt or not Page Fault */
   if(is_int || intno != EXCP0E_PAGE)
      return;

   /* keep some detail level for AFL board processing (async_panic) */
   cs->exception_index = EXCP_INTERCEPT + intno;
   env->error_code = error_code;
   env->exception_is_int = is_int;

   /* remove any pending exception */
   env->old_exception = -1;
   cpu_loop_exit_restore(cs, retaddr);

   /* We try to keep the Qemu logic of vcpu loop exiting and async
    * main loop event triggering (vm_change_state). We will exit vcpu
    * loop and get back to cpu-exec(). We then enter
    * cpu_handle_exception() and exit rapidly because of our special
    * exception_index. We vm_stop() early in cpu_handle_exception() to
    * prevent implementing post exit treatment in cpus.c for each vCPU
    * specific implementation (tcg, kvm, ...). This will set
    * cpu->exit_request=1.
    *
    * However we still have to explicitly call afl_check_intercept() and
    * trigger vm_stop() into each vCPU exception/interrupts handling:
    * - tcg: raise_interrupt2()
    * - kvm/haxm/hvf: TODO
    */
}

/*
 * RAM region is created in piix PC init functions
 * look for its pointer after execution
 */
static void afl_ram_get_area(afl_t          *afl,
                             PCMachineState *pcms,
                             MemoryRegion   *sysmem)
{
   MemoryRegion *mr;

   QTAILQ_FOREACH(mr, &sysmem->subregions, subregions_link) {
      if (mr->size == pcms->below_4g_mem_size &&
          mr->alias &&
          mr->alias_offset == 0 &&
          mr->alias->ram)
      {
         afl->ram_mr = mr->alias;
         return;
      }
   }
}

#ifdef AFL_RAM_GUARD
void afl_arch_ram_guard_setup(afl_t *afl)
{
   uint32_t atop = QEMU_ALIGN_DOWN(afl->ram_size, PAGE_SIZE);
   uint32_t vtop = QEMU_ALIGN_UP(afl->config.tgt.size, PAGE_SIZE);
   uint32_t gpgd = atop - PAGE_SIZE;
   uint32_t *hpgd, *hptb, gptb;
   uint32_t f, i, m, n;

   if (atop < (vtop + 3*PAGE_SIZE)) {
      error_report("not enough RAM to inject PTB (0x%x/0x%x)", atop, vtop);
      exit(EXIT_FAILURE);
   }

   // map large
   n = pg_4M_nr(afl->config.tgt.size);
   hpgd = (uint32_t*)(afl->ram_ptr + gpgd);
   memset((void*)hpgd, 0, PAGE_SIZE);
   for(i=0 ; i<n ; i++)
      pg_set_large_entry(&hpgd[i], PG_FULL, i);

   debug("ramguard: idmap %d 4MB pg\n", n);

   // map fine-grain
   m = page_nr(afl->config.tgt.size % PG_4M_SIZE);
   if (m) {
      f = n << (PG_4M_SHIFT - PG_4K_SHIFT);
      gptb = atop - 2*PAGE_SIZE;
      hptb = (uint32_t*)(afl->ram_ptr + gptb);
      memset((void*)hptb, 0, PAGE_SIZE);
      for(i=0 ; i<m ; i++)
         pg_set_entry(&hptb[i], PG_FULL, f+i);

      pg_set_entry(&hpgd[n], PG_FULL, page_nr(gptb));
      debug("ramguard: (grain) idmap %d 4KB pg from frame 0x%x\n", m, f);
   }

   // map trace bitmap
   n = pd32_idx(afl->config.afl.trace_addr);
   m = pt32_idx(afl->config.afl.trace_addr);
   f = page_nr(afl->config.afl.trace_addr);
   gptb = atop - 3*PAGE_SIZE;
   hptb = (uint32_t*)(afl->ram_ptr + gptb);
   memset((void*)hptb, 0, PAGE_SIZE);
   for (i=0 ; i<afl->config.afl.trace_size/PAGE_SIZE ; i++)
      pg_set_entry(&hptb[m+i], PG_FULL, f+i);

   pg_set_entry(&hpgd[n], PG_FULL, page_nr(gptb));
   debug("ramguard: (trace) idmap %d 4KB pg from frame 0x%x\n",
         afl->config.afl.trace_size/PAGE_SIZE, f);

   // enable paging
   CPUX86State *env = &afl->cpu->env;
   env->cr[0] |= CR0_PG_MASK;
   env->cr[4] |= CR4_PGE_MASK;
   cpu_x86_update_cr3(env, gpgd);
   afl_mem_invalidate(afl->ram_mr, gpgd, 3*PAGE_SIZE);

   debug("ramguard: enabled\n");
}
#endif

void afl_init_arch(afl_t *afl, MachineState *mcs, MemoryRegion *sysmem)
{
   CPUState *cpu = first_cpu;
   PCMachineState *pcms = PC_MACHINE(mcs);

   /* debug("AFL - RAM B4G %lu A4G %lu MB4G %lu\n", */
   /*       pcms->below_4g_mem_size, */
   /*       pcms->above_4g_mem_size, */
   /*       pcms->max_ram_below_4g); */

   afl_ram_get_area(afl, pcms, sysmem);
   afl->arch.cpu = X86_CPU(cpu);
}

static void pc_afl_machine_options(MachineClass *m)
{
    PCMachineClass *pcmc = PC_MACHINE_CLASS(m);
    pcmc->default_nic_model = "e1000";

    m->family = "pc_piix";
    m->desc = "Intel AFL platform";
    m->default_machine_opts = "firmware=bios-256k.bin";
    m->default_display = "std";

    //m->alias = "pc";
    //m->is_default = 0;
}

// fixed static definition in pc-piix.c
extern void pc_init1(MachineState*, const char*, const char*);

static void pc_init_afl(MachineState *mcs)
{
   afl_t *afl = afl_pre_init();
   //re-use piix PC init function
   pc_init1(mcs, TYPE_I440FX_PCI_HOST_BRIDGE, TYPE_I440FX_PCI_DEVICE);
   afl_init(afl, mcs);
}

DEFINE_PC_MACHINE(afl, "afl", pc_init_afl, pc_afl_machine_options)
