/*
 * QEMU American Fuzzy Lop board
 * generic components
 *
 * Copyright (c) 2019 S. Duverger Airbus
 * GPLv2
 */
#include "qemu/afl.h"

afl_t* afl_pre_init(void)
{
#ifdef AFL_ALTERNATE_STDOUT
   dup2(open("/tmp/qemu.out", O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR), 1);
#endif
#ifdef AFL_ALTERNATE_STDERR
   dup2(open("/tmp/qemu.err", O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR), 2);
#endif

   return (afl_t*)g_new0(afl_t, 1);
}

static void afl_init_ram(afl_t *afl, MachineState *mcs, MemoryRegion *sysmem)
{
   afl->ram_size = mcs->ram_size;
   debug("ram size %lu max %lu\n",
         mcs->ram_size, mcs->maxram_size);

   if (afl->ram_mr == NULL) {
      error_report("can't find RAM memory region");
      exit(EXIT_FAILURE);
   }

   afl->ram_ptr = memory_region_get_ram_ptr(afl->ram_mr);

   if (afl->ram_ptr == NULL) {
      error_report("can't get RAM pointer");
      exit(EXIT_FAILURE);
   }
}

static void afl_init_fuzz(afl_t *afl)
{
#ifndef AFL_CONTACT
   afl->trace_bits = mmap(NULL, afl->config.afl.trace_size,
                          PROT_READ|PROT_WRITE,
                          MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
   if (afl->trace_bits == MAP_FAILED) {
      error_report("AFL trace bits mmap() failed");
      exit(EXIT_FAILURE);
   }
#else
   char *env = getenv(afl->config.afl.trace_env);
   if (!env) {
      error_report("can't get AFL SHM env: '%s'", afl->config.afl.trace_env);
      exit(EXIT_FAILURE);
   }

   afl->shm_id = strtol(env, NULL, 10);
   if (afl->shm_id < 0) {
      error_report("can't get AFL SHM id");
      exit(EXIT_FAILURE);
   }

   afl->trace_bits = shmat(afl->shm_id, NULL, 0);
   if (afl->trace_bits == (void*)-1) {
      error_report("AFL SHM attach failed");
      exit(EXIT_FAILURE);
   }
#endif
   afl->euid = geteuid();
   afl->pid  = getpid();
   afl->ppid = getppid();

   debug("AFL trace bits %p\n", afl->trace_bits);

   /* prepare internal timer to ~sync with AFL user time-out */
   afl->user_timer = timer_new_ms(QEMU_CLOCK_VIRTUAL,
                                  afl_user_timeout_cb, afl);
}

static void afl_init_vm(afl_t *afl)
{
   strncpy(afl->vms_path, afl->config.qemu.vms_tpl, sizeof(afl->vms_path));
   afl->vms_path[sizeof(afl->vms_path) - 1] = 0;
   afl->vms_fd = mkstemp(afl->vms_path);
}

/*
 * Get Qemu CPU pointers and set initial breakpoint acting as
 * interesting starting point for fuzzing (kernel entry point).
 */
static void afl_init_cpu(afl_t *afl, MachineState *mcs)
{
   afl->vm_exit = afl->config.tgt.fuzz_ep;

   debug("%s: fuzz EP 0x"TARGET_FMT_lx"\n", __func__, afl->vm_exit);
   afl_insert_breakpoint(afl, afl->vm_exit);

#ifdef AFL_CONTROL_PANIC
   debug("%s: fuzz panic 0x"TARGET_FMT_lx"\n", __func__, afl->config.tgt.panic);
   afl_insert_breakpoint(afl, afl->config.tgt.panic);
#endif

#ifdef AFL_CONTROL_CSWITCH
   debug("%s: ctxt switch 0x"TARGET_FMT_lx"\n", __func__, afl->config.tgt.cswitch);
   afl_insert_breakpoint(afl, afl->config.tgt.cswitch);
#endif

   qemu_add_vm_change_state_handler(afl_vm_state_change, (void*)afl);
}

void afl_cleanup(afl_t *afl)
{
   debug("board cleanup\n");
   if (afl->user_timer)
      timer_del(afl->user_timer);

   close(afl->vms_fd);
   unlink(afl->vms_path);
   //XXX: unmap(), shmdt() ...
}

void afl_init(afl_t *afl, MachineState *mcs)
{
   MemoryRegion *sysmem = get_system_memory();

   afl_init_conf(afl);
   afl_init_arch(afl, mcs, sysmem);
   afl_init_cpu(afl, mcs);
   afl_init_ram(afl, mcs, sysmem);
   afl_init_vm(afl);
   afl_init_fuzz(afl);
   afl_init_trace_mem(afl);

   debug("board ready\n");
}
