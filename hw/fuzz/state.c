/*
 * QEMU American Fuzzy Lop board
 * vm state handler
 *
 * Copyright (c) 2019 S. Duverger Airbus
 * GPLv2
 */
#include "qemu/afl.h"

static void afl_run_target(afl_t *afl);

static void afl_forward_status(afl_t *afl)
{
   /* relay waitpid() status to AFL */
#ifdef AFL_CONTACT
   debug("forward status (%d) to AFL\n", afl->status);
   if (write(afl->config.afl.sts_fd, &afl->status, 4) != 4) {
      error_report("can't write status to afl");
      exit(EXIT_FAILURE);
   }
#endif

#ifdef AFL_TRACE_CHKSM
   afl_trace_checksum(afl, "post-fwd-status");
#endif

#ifdef AFL_TRACE_COUNT
   afl_trace_count(afl, "forward status");
#endif
}

static void afl_reload_forward(afl_t *afl)
{
   afl_load_vm(afl, afl->vms_fd);
   afl_forward_status(afl);
   afl_run_target(afl);
}

/*
 * Forward fake child fork() and early failing status
 * to generate a new test case without any execution.
 * Used when partition code generator failed parsing.
 * We do not generate fake pid as usual (we get faster
 * to not waste time), because AFL will never have time
 * to SIGKILL it (upon timeout) between read(pid)
 * & read(status).
 */
#if defined(AFL_CONTACT) && defined(AFL_INJECT_TESTCASE)
static void afl_flash_forward(afl_t *afl)
{
   int child_pid = 1;
   if (write(afl->config.afl.sts_fd, &child_pid, 4) != 4) {
      error_report("can't write fast pid to afl");
      exit(EXIT_FAILURE);
   }

   debug("--> vm fast exit() !\n");
   afl->status = sts_exit(1);
   afl_forward_status(afl);
}
#endif

/*
 * Qemu AFL user-timeout implementation. We use VirtualClock
 * to only count time elapsed during target execution.
 * The callback is called during qemu main_loop()
 * thus we can't change VM state directly, because
 * the VM might be running and stopping it from here
 * might deadlock due to qemu virtual clock disabling
 * waiting for timer callback to return.
 *
 * So we act asynchronously, by requesting the VM to stop.
 * Out of timer callback execution, the qemu main_loop()
 * will analyse stop requests and change VM state.
 * This will raise our afl_vm_state_change() callback.
 *
 * We simulate SIGKILL status for AFL.
 */
void afl_user_timeout_cb(void *opaque)
{
   afl_t *afl = (afl_t*)opaque;

   debug("--> vm timeout()\n");
   afl->status = sts_kill();
   qemu_system_vmstop_request_prepare();
   qemu_system_vmstop_request(RUN_STATE_RESTORE_VM);
}

static inline void afl_setup_timer(QEMUTimer *timer, int64_t duration)
{
   int64_t  now    = qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL);
   int64_t  expire = now + duration;
   timer_mod(timer, expire);
}

/*
 * Enable AFL trace bitmap
 * only when scheduling attacker partition
 */
#ifdef AFL_CONTROL_CSWITCH
static void afl_handle_cswitch(afl_t *afl)
{
   target_ulong sp = afl_get_stack(&afl->arch);

   if (sp >= afl->config.tgt.part_kstack &&
       sp < (afl->config.tgt.part_kstack+afl->config.tgt.part_kstack_size)) {

      debug("scheduling target partition (0x"TARGET_FMT_lx")\n", sp);
      memory_region_set_enabled(&afl->fake_mr, false);
      memory_region_set_enabled(&afl->trace_mr, true);

      /* afl->fake_mr.priority  = 0; */
      /* afl->trace_mr.priority = 1; */
      /* memory_region_update_pending = true; */
      /* memory_region_transaction_commit(); */

      /* memory_region_del_subregion(get_system_memory(),&afl->fake_mr); */
      /* memory_region_add_subregion(get_system_memory(), */
      /*                             afl->config.afl.trace_addr, &afl->trace_mr); */
   } else {
      debug("scheduling other partition (0x"TARGET_FMT_lx")\n", sp);
      memory_region_set_enabled(&afl->fake_mr, true);
      memory_region_set_enabled(&afl->trace_mr, false);

      /* afl->fake_mr.priority  = 1; */
      /* afl->trace_mr.priority = 0; */
      /* memory_region_update_pending = true; */
      /* memory_region_transaction_commit(); */

      /* memory_region_del_subregion(get_system_memory(),&afl->trace_mr); */
      /* memory_region_add_subregion(get_system_memory(), */
      /*                             afl->config.afl.trace_addr, &afl->fake_mr); */
   }

   debug("<-- resume VM (scheduled)\n");

   /*
    * We could not use "env->hflags |= HF_RF_MASK" to ignore the
    * breakpoint and resume insn because GDB_BP are always triggered.
    * To speed up things, we should fix qemu breakpoints logic.
    */
   afl_remove_breakpoint(afl, afl->config.tgt.cswitch);
   cpu_single_step(CPU(afl->arch.cpu), true);
   vm_start();
}
#endif

/*
 * Qemu AFL standard execution (NO_FAULT)
 * raised on VM partition code end breakpoint
 * installed in afl_run_target()
 * We simulate exit(0) status for AFL.
 */
static void afl_handle_execution_end(afl_t *afl)
{
   debug("--> vm exit() !\n");
   afl->status = sts_exit(0);
   afl_reload_forward(afl);
}

static void afl_handle_abort(afl_t *afl)
{
   debug("--> vm abort() !\n");
   afl->status = sts_abort();
   afl_reload_forward(afl);
}

/*
 * Prepare the VM to be resumed :
 *  - wait for AFL to generate test case
 *  - inject it to the partition
 *  - install BP at new partition code end
 *  - spwan fake pid (no fork)
 *  - setup user time out for this test case
 *
 * Must be called from a Qemu VM callback (ie. vm_state_change())
 * to be able to resume VM upon return.
 */
#if defined(AFL_DEBUG) && defined(AFL_CONTACT)
static uint64_t test_nr = 1;
#endif

static void afl_run_target(afl_t *afl)
{
#if defined(AFL_INJECT_TESTCASE) ||                                     \
   (defined(AFL_CONTROL_EXECUTION) && !defined(AFL_CONTROL_EXEC_ZERO))
   ssize_t len = afl->config.tgt.nop_size;
#endif

#ifdef AFL_CONTACT
__wait_test_case:
   /* Wait for parent by reading from the pipe. Abort if read fails. */
   debug("waiting for test case [%lu]\n", test_nr++);

   int prev_timed_out; //last child did time out ?
   if (read(afl->config.afl.ctl_fd, &prev_timed_out, 4) != 4) {
      error_report("not ready to run test case");
      exit(EXIT_FAILURE);
   }
#endif // CONTACT

#ifdef AFL_TRACE_CHKSM
   afl_trace_checksum(afl, "pre-test-case");
#endif

   /* inject test case in VM partition */
#ifdef AFL_INJECT_TESTCASE
   len = afl_inject_test_case(afl);
#ifdef AFL_CONTACT
   if (len <= 0) {
      afl_flash_forward(afl);
      goto __wait_test_case;
   }
#endif // CONTACT
#endif // INJECT_TEST_CASE

   /* remove previously installed breakpoint */
   afl_remove_breakpoint(afl, afl->vm_exit);

   /* VM partition exit(0) point for current test case */
#ifdef AFL_CONTROL_EXECUTION

#ifdef AFL_CONTROL_EXEC_ZERO
   afl->vm_exit = afl->config.tgt.fuzz_ep_next;
   (*(u8*)afl->trace_bits)++;
#else
   afl->vm_exit = afl->config.tgt.fuzz_ep + len;
   debug("vm exec end @ 0x"TARGET_FMT_lx" (%ld)\n", afl->vm_exit, len);
#endif // EXEC_ZERO

   afl_insert_breakpoint(afl, afl->vm_exit);
#endif // CONTROL_EXECUTION

   /* Write fake PID to pipe to release AFL (run_target()) */
#ifdef AFL_CONTACT
   afl_forward_child(afl);
#endif

   /* At this point, AFL just received the child_pid, set user timeout
    * and wait for child_status from pipe. If user timeout expires,
    * AFL will SIGKILL(child_pid) and set child_timed_out = 1. After
    * receiveing child status, AFL disarms the user timeout and
    * proceed with fault analysis.
    *
    * AFL analyses child status based on waitpid() return.
    * WIFSIGNALED(status): child process was terminated by a signal.
    *
    * if child_pid > 0 && timer expired : child_timed_out = 1
    * if child_pid > 0 && ctrl+c        : stop_soon = 1
    *
    * if (WIFSIGNALED(status) && !stop_soon) {
    *    kill_signal = WTERMSIG(status)
    *    if (child_timed_out && kill_signal == SIGKILL)
    *       return FAULT_TMOUT
    *    return FAULT_CRASH
    * return FAULT_NONE
    */
#ifdef AFL_CONTROL_EXECUTION
   afl_setup_timer(afl->user_timer, afl->config.qemu.timeout - afl->config.qemu.overhead);
#endif

   /* Resume VM until memory fault, timeout or end of execution */
   debug("<-- resume vm (new test case)\n");
   vm_start();
}

/*
 * The VM has run enough code to boot, init devices,
 * setup partitions and now its interesting to start
 * fuzzing. Partition have not been started yet.
 */
static void afl_handle_start_fuzzing(afl_t *afl)
{
#ifdef AFL_DUMMY_CASE
   debug("DUMMY TEST CASE\n");
   void *out   = afl->ram_ptr + afl->config.tgt.fuzz_inj;
   ssize_t len = 2;
   //*((uint8_t*)out) = 0x90;
   *((uint16_t*)out) = 0xfeeb;
   afl_mem_invalidate(afl->ram_mr, afl->config.tgt.fuzz_inj, len);
#endif

#ifdef AFL_RAM_GUARD
   afl_arch_ram_guard_setup(afl);
#endif

#ifdef AFL_CONTACT
   /* release AFL init_forkserver() */
   debug("say hello to AFL\n");
   if (write(afl->config.afl.sts_fd, "hello", 4) != 4) {
      error_report("can't write hello to afl");
      exit(EXIT_FAILURE);
   }
#endif

   /* trace bitmap is reset at this stage */
   afl_save_vm(afl, afl->vms_fd);
   afl_run_target(afl);
}

/*
 * When running with KVM, it seems unsafe to change vm state inside a
 * vm_state_change_handler(). The vcpu is running very slowly. When
 * looking at kvm_stat, far less virq are injected per second. So we
 * update vm state asynchronously, in the vcpu thread with a queued
 * work. Notice that in TCG mode, we didn't encountered this issue.
 */
static void async_restore_vm(CPUState *cpu, run_on_cpu_data data)
{
    afl_t *afl = (afl_t*)data.host_ptr;
    afl_reload_forward(afl);
}

/*
 * Dedicated breakpoint handling VM change state
 */
static void async_debug_vm(CPUState *cpu, run_on_cpu_data data)
{
   afl_t         *afl = (afl_t*)data.host_ptr;
   target_ulong   pc  = afl_get_pc(&afl->arch);

   //debug("VM #debug event 0x%x\n", pc);
#ifdef AFL_CONTROL_PANIC
   /* VM kernel panic */
   if (pc == afl->config.tgt.panic)
      afl_handle_abort(afl);
   else
#endif

#ifdef AFL_CONTROL_CSWITCH
   /* VM kernel context switch */
   if (pc == afl->config.tgt.cswitch)
      afl_handle_cswitch(afl);
   else if (pc == afl->config.tgt.cswitch_next) {
      cpu_single_step(CPU(afl->arch.cpu), false);
      afl_insert_breakpoint(afl, afl->config.tgt.cswitch);
      vm_start();
   }
   else
#endif

   /* VM execution should hit last installed breakpoint */
   if (pc != afl->vm_exit)
      error_report("vm unhandled #debug event 0x"TARGET_FMT_lx  \
                   " (waiting 0x"TARGET_FMT_lx")\n"
                   "----> investigate with monitor", pc, afl->vm_exit);
   else if (afl->vm_exit == afl->config.tgt.fuzz_ep)
      afl_handle_start_fuzzing(afl);
   else
      afl_handle_execution_end(afl);
}

/*
 * Dedicated exception handling VM change state
 */
static void async_panic_vm(CPUState *cpu, run_on_cpu_data data)
{
   afl_t  *afl  = (afl_t*)data.host_ptr;
   int    intno = cpu->exception_index;

   /* coherency with default Qemu behavior */
   cpu->exception_index = -1;

   debug("excp #%d @ 0x"TARGET_FMT_lx"\n",
         intno, afl_get_pc(&afl->arch));

   afl_handle_abort(afl);
}

/*
 * This is our main VM state handler.
 * We trap any VM status change, ie. breakpoints, restore, ...
 * and proceed with corresponding action.
 */
void afl_vm_state_change(void *opaque, int running, RunState state)
{
   afl_t    *afl = (afl_t*)opaque;
   CPUState *cpu = CPU(afl->arch.cpu);

   debug("vm state %s\n", RunState_str(state));

   if (running || state == RUN_STATE_PAUSED) {
      return;
   }

   if (state == RUN_STATE_SHUTDOWN) {
      debug("vm is shutting down\n");
      afl_cleanup(afl);
      return;
   }

   // XXX: do it only on async handlers ?
   cpu_synchronize_state(cpu);
   if (!kvm_enabled()) {
      tb_flush(cpu);
   }

   /* VM async request RESTORE_VM, cf. afl_user_timeout_cb() */
   if (state == RUN_STATE_RESTORE_VM) {
      async_run_on_cpu(first_cpu, async_restore_vm,
                       RUN_ON_CPU_HOST_PTR(afl));
      return;
   }

   if (state == RUN_STATE_DEBUG) {
      async_run_on_cpu(first_cpu, async_debug_vm,
                       RUN_ON_CPU_HOST_PTR(afl));
      return;
   }

   if (state == RUN_STATE_GUEST_PANICKED) {
      async_run_on_cpu(first_cpu, async_panic_vm,
                       RUN_ON_CPU_HOST_PTR(afl));
      return;
   }

   if (runstate_needs_reset()) {
      error_report("vm runstate needs reset !");
      exit(EXIT_FAILURE);
   }

   debug("vm unhandled state %s\n", RunState_str(state));
   exit(EXIT_FAILURE);
}
