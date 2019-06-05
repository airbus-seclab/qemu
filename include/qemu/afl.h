/*
 * QEMU American Fuzzy Lop board
 * Copyright (c) 2019 S. Duverger Airbus
 * GPLv2
 */

#ifndef __AFL_H__
#define __AFL_H__


#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "qemu/coroutine.h"
#include "qemu/timer.h"

#include "qapi/error.h"
#include "qapi/qapi-commands-misc.h"
#include "qapi/qapi-events-run-state.h"
#include "qapi/qmp/qerror.h"

#include "cpu.h"
#include "translate-all.h"

#include "hw/hw.h"
#include "hw/loader.h"
#include "hw/boards.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_host.h"
#include "hw/sysbus.h"

#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "exec/ram_addr.h"
#include "exec/gdbstub.h"
#include "exec/semihost.h"
#include "exec/exec-all.h"

#include "sysemu/hw_accel.h"
#include "sysemu/arch_init.h"
#include "sysemu/numa.h"
#include "sysemu/kvm.h"

#include "migration/migration.h"
#include "migration/global_state.h"
#include "migration/misc.h"
#include "migration/vmstate.h"
#include "migration/qemu-file-types.h"
#include "migration/qemu-file.h"
#include "migration/qemu-file-channel.h"
#include "migration/savevm.h"
#include "io/channel-file.h"

#include <sys/types.h>
#include <sys/shm.h>


typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;


/*
 * AFL board configuration mode
 */
#define AFL_DEBUG                1
#define AFL_CONTACT              1
#define AFL_INJECT_TESTCASE      1
//#define AFL_DUMMY_CASE           1

#define AFL_FAST_RESTORE         1
#define AFL_CONTROL_EXECUTION    1
//#define AFL_CONTROL_EXEC_ZERO    1
#define AFL_CONTROL_PANIC        1
//#define AFL_CONTROL_CSWITCH      1
#define AFL_RAM_GUARD            1
//#define AFL_TRACE_MMIO           1
#define AFL_PRESERVE_TRACEMAP    1

#ifdef AFL_CONTACT
#ifndef AFL_INJECT_TESTCASE
#define AFL_INJECT_TESTCASE 1
#endif
#endif

#ifdef AFL_DEBUG
#define AFL_ALTERNATE_STDOUT     1
#define AFL_ALTERNATE_STDERR     1
//#define AFL_TRACE_CHKSM          1
//#define AFL_TRACE_COUNT          1
#ifdef AFL_INJECT_TESTCASE
//#define AFL_DUMP_TESTCASE        1
//#define AFL_DUMP_PARTITION       1
#endif
#define debug(fmt, ...) do {qemu_log("afl-"TARGET_NAME": "fmt, ## __VA_ARGS__);} while(0)

#else // NO DEBUG
#define debug(fmt, ...) do {} while(0)
#endif

#if defined(TARGET_X86_64) || defined(TARGET_I386)
#define AFL_TRACE_ADDR        0x80000000
#elif defined(TARGET_PPC) || defined(TARGET_PPC64)
#define AFL_TRACE_ADDR        0xe0000000
#endif

/*
 * Intel i386/x86_64 specific
 */
#if defined(TARGET_X86_64) || defined(TARGET_I386)

#include "hw/pci/pci_ids.h"
#include "hw/i386/pc.h"
#include "hw/i386/apic.h"
#include "hw/acpi/acpi.h"
#include "kvm_i386.h"
#include "hw/i2c/smbus.h"
#include "hw/xen/xen.h"

#define PG_P            (1)
#define PG_RW           (1<<1)
#define PG_USR          (1<<2)
#define PG_PS           (1<<7)
#define PG_GLB          (1<<8)
#define PG_FULL         (PG_USR|PG_GLB|PG_RW)

#define PG_4K_SHIFT      12
#define PG_4K_SIZE      (1<<PG_4K_SHIFT)
#define pg_4K_nr(addr)  ((addr)>>PG_4K_SHIFT)

#define PG_4M_SHIFT      22
#define PG_4M_SIZE      (1<<PG_4M_SHIFT)
#define pg_4M_nr(addr)  ((addr)>>PG_4M_SHIFT)

#define PAGE_SIZE       PG_4K_SIZE
#define page_nr(addr)   pg_4K_nr(addr)
#define pd32_idx(addr)  (((addr)>>PG_4M_SHIFT)&0x3ff)
#define pt32_idx(addr)  (((addr)>>PG_4K_SHIFT)&0x3ff)

#define pg_set_entry(_e_,_attr_,_pfn_)                  \
   ({                                                   \
      *(_e_) = ((_pfn_)<<PG_4K_SHIFT)|(_attr_)|PG_P;    \
   })

#define pg_set_large_entry(_e_,_attr_,_pfn_)                    \
   ({                                                           \
      *(_e_) = ((_pfn_)<<PG_4M_SHIFT)|(_attr_)|PG_PS|PG_P;      \
   })

void afl_check_intercept(CPUX86State *env, int intno, int is_int,
                         int error_code, uintptr_t retaddr);

typedef struct afl_x86_board
{
   X86CPU *cpu;

} afl_x86_t;

typedef afl_x86_t afl_arch_t;

static inline target_ulong afl_get_pc(afl_arch_t *arch)
{
   CPUX86State *env = &arch->cpu->env;
   return env->segs[R_CS].base + env->eip;
}

static inline void afl_set_pc(afl_arch_t *arch, target_ulong pc)
{
   CPUX86State *env = &arch->cpu->env;
   //env->segs[R_CS].base + env->eip;
   env->eip = pc; // XXX: set cs ?
}

static inline target_ulong afl_get_stack(afl_arch_t *arch)
{
   CPUX86State *env = &arch->cpu->env;
   return env->regs[R_ESP];
}

/*
 * PowerPC specific
 */
#elif defined(TARGET_PPC) || defined(TARGET_PPC64)

#include "hw/char/serial.h"
#include "hw/block/fdc.h"
#include "hw/isa/pc87312.h"

#include "hw/ppc/ppc.h"
#include "kvm_ppc.h"

typedef struct afl_powerpc_board
{
   PowerPCCPU *cpu;

} afl_ppc_t;

typedef afl_ppc_t afl_arch_t;

static inline target_ulong afl_get_pc(afl_arch_t *arch)
{
   CPUPPCState *env = &arch->cpu->env;
   return env->nip;
}

static inline void afl_set_pc(afl_arch_t *arch, target_ulong pc)
{
   CPUPPCState *env = &arch->cpu->env;
   env->nip = pc;
}

static inline target_ulong afl_get_stack(afl_arch_t *arch)
{
   CPUPPCState *env = &arch->cpu->env;
   return env->gpr[1];
}

#endif



/*
 * Generic AFL board
 */

/* waitpid() status format */
#define create_wait_status(code, signal)        \
   (((int)code)<<8 | (int)(signal & 0x7f))

#define sts_abort()      create_wait_status(0, SIGABRT)
#define sts_kill()       create_wait_status(0, SIGKILL)
#define sts_exit(code)   create_wait_status(code, 0)
#define sts_stopped(sig) create_wait_status(sig, 0x7f)

typedef struct afl_configuration
{
   /* QEMU / AFL interaction information */
   struct __afl_qemu_conf {
      int64_t     timeout;  /* AFL cmd line user time out in ms */
      int64_t     overhead; /* Estimated overhead for qemu/afl
                             * transitions used to setup timer */
      const char *vms_tpl;  /* vmstate template file path */
   } qemu;

   /* AFL internals */
   struct __afl_int_conf {
      int         ctl_fd;     /* AFL control file descriptor */
      int         sts_fd;     /* AFL status file descriptor */
      size_t      trace_size; /* AFL coverage bitmap size in bytes */
      uint64_t    trace_addr; /* AFL coverage bitmap target mmio
                               * address */
      const char *trace_env;  /* AFL coverage bitmap shared memory
                               * identifier environment variable
                               * name */
   } afl;

   /* Virtual Machine (Target) partition information */
   struct __afl_target_conf {
      target_ulong  part_base;        /* Partition base paddr */
      uint64_t      part_size;        /* Allocated partition size */
      target_ulong  part_kstack;      /* Partition thread allocated
                                       * kernel stack vaddr */
      uint64_t      part_kstack_size; /* Partition thread allocated
                                       * kernel stack size */
      uint64_t      nop_size;         /* NOP-sled size */
      target_ulong  part_off;         /* NOP-sled offset */
      target_ulong  fuzz_inj;         /* Generated code injection paddr */
      target_ulong  fuzz_ep;          /* Fuzzing starting point vaddr */
      target_ulong  fuzz_ep_next;     /* Insn vaddr following FUZZ_EP */
      target_ulong  size;             /* Effective target physical memory
                                       * used */
      target_ulong  panic;            /* Target 'kernel panic' vaddr */
      target_ulong  cswitch;          /* Target context switch vaddr */
      target_ulong  cswitch_next;     /* Insn vaddr follwing 'vm_cswitch_next' */
   } tgt;

   /* Runtime operating mode/strategy */
   /* struct __afl_running_mode { */
   /*    bool    fast_restore; */
   /*    bool    control_execution; */
   /*    bool    control_exec_zero; */
   /*    bool    control_panic; */
   /*    bool    control_cswitch; */
   /*    bool    ram_guard; */
   /*    bool    trace_mmio; */
   /*    bool    preserve_tracemap; */
   /*    bool    trace_chksm; */
   /*    bool    trace_count; */
   /*    bool    dump_test; */
   /*    bool    dump_part; */
   /* } mode; */

} afl_conf_t;

typedef struct afl_board
{
   afl_conf_t     config;
   QEMUTimer     *user_timer;

   // VM state
   char           vms_path[32];
   int            vms_fd;
   MemoryRegion  *ram_mr;
   void          *ram_ptr;
   uint32_t       ram_size;
   target_ulong   vm_exit;
   int            status;

   // AFL internals
   int            shm_id;
   MemoryRegion   trace_mr;
   void          *trace_bits;
#ifdef AFL_CONTROL_CSWITCH
   MemoryRegion   fake_mr;
   void          *fake_bits;
#endif
   uid_t          euid;
   int            pid, ppid;

   // Architecture specific
   afl_arch_t     arch;

} afl_t;

/*
 * Functions
 */
afl_t*  afl_pre_init(void);
void    afl_cleanup(afl_t*);

void    afl_init(afl_t*, MachineState*);
void    afl_init_conf(afl_t*);
void    afl_init_arch(afl_t*, MachineState*, MemoryRegion*);
void    afl_init_trace_mem(afl_t *afl);

void    afl_remove_breakpoint(afl_t*, uint32_t);
void    afl_insert_breakpoint(afl_t*, uint32_t);
void    afl_vm_state_change(void*, int, RunState);
void    afl_forward_child(afl_t*);
void    afl_user_timeout_cb(void*);
void    afl_save_vm(afl_t*, int);
void    afl_load_vm(afl_t*, int);
size_t  afl_inject_test_case(afl_t*);
void    afl_arch_ram_guard_setup(afl_t*);
ssize_t afl_gen_code(uint8_t*, size_t, uint8_t*, size_t);
void    afl_mem_invalidate(MemoryRegion*, hwaddr, hwaddr);

void    afl_trace_checksum(afl_t*, const char*);
void    afl_trace_count(afl_t*, const char*);

// __AFL_H__
#endif
