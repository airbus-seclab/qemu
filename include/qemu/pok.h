/*
 * (c) 2018, Airbus A. Gantet
 */
#ifndef __POK_H__
#define __POK_H__

#include "qemu/pok_syscall.h"

/********* Generator operating mode ************/

/*
 * inputs are interpreted as syscall id, followed by syscall
 * arguments. Arguments which are user memory pointers are ensured to
 * be valid and afl fuzzed values are inserted into these user memory
 * areas
 */
#define GENERATOR_RAND_ID_ARGS_VALID_USERMEM

/*
 * Use a fixed system call ID and only interpret afl fuzzed input as
 * syscall arguments
 */
/* #define GENERATOR_FIXED_ID_RAND_ARGS */
/* #define GENERATOR_FIXED_ID 405 /\*POK_SYSCALL_PARTITION_GET_ID*\/ */

/*
 * Use given ID and random arguments values from afl fuzzed input
 */
/* #define GENERATOR_FIXED_RAND_ID_RAND_ARGS */

/* #define POK_GEN_DEBUG */
#define NO_SYS_TIME_RELATED 1
#define NO_SYS_CONSWRITE    1

size_t afl_gen_code(uint8_t*, size_t, uint8_t*, size_t);

/************** Implementation ***********************/

typedef struct {
   int         pok_syscall_id;
   const char *pok_syscall_name;
   uint8_t     pok_syscall_nbargs;
   uint32_t    pok_param_sizes[5];
   int         enabled;

} pok_syscall_generator_info_t;

static pok_syscall_generator_info_t pok_syscall_table[] = {

   {POK_SYSCALL_PARTITION_GET_PERIOD,
    "POK_SYSCALL_PARTITION_GET_PERIOD",
    2,
    {sizeof(uint64_t),0},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_QUEUEING_STATUS,
    "POK_SYSCALL_MIDDLEWARE_QUEUEING_STATUS",
    2,
    {0,sizeof(pok_port_queueing_status_t)},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_QUEUEING_ID,
    "POK_SYSCALL_MIDDLEWARE_QUEUEING_ID",
    2,
    {sizeof(char)*32,sizeof(pok_port_id_t)},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_QUEUEING_CREATE,
    "POK_SYSCALL_MIDDLEWARE_QUEUEING_CREATE",
    5,
    {sizeof(char)*32,0,0,0,sizeof(pok_port_id_t)} ,
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_QUEUEING_SEND,
    "POK_SYSCALL_MIDDLEWARE_QUEUEING_SEND",
    4,
    {0,sizeof(void)*32/*data max buf 256*/,0,0},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_VIRTUAL_CREATE,
    "POK_SYSCALL_MIDDLEWARE_VIRTUAL_CREATE",
    2,
    {sizeof(char)*32,sizeof(pok_port_id_t)},
    1,
   },

   {POK_SYSCALL_ERROR_GET,
    "POK_SYSCALL_ERROR_GET",
    2,
    {sizeof(pok_error_status_t),0},
    1,
   },

   {POK_SYSCALL_PARTITION_GET_ID,
    "POK_SYSCALL_PARTITION_GET_ID",
    2,
    {sizeof(uint8_t),0}    ,
    1,
   },

   {POK_SYSCALL_PARTITION_GET_OPERATING_MODE,
    "POK_SYSCALL_PARTITION_GET_OPERATING_MODE",
    2,
    {sizeof(pok_partition_mode_t),0},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_SAMPLING_READ,
    "POK_SYSCALL_MIDDLEWARE_SAMPLING_READ",
    4,
    {0,sizeof(void*)*32/*data max buf 2^32?*/,sizeof(pok_port_size_t),sizeof(bool_t)},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_SAMPLING_CREATE,
    "POK_SYSCALL_MIDDLEWARE_SAMPLING_CREATE",
    5,
    {sizeof(char)*32,0,0,0,0/*sizeof(pok_port_id_t)- out parameter?*/},
    1,
   },

   {POK_SYSCALL_THREAD_CREATE,
    "POK_SYSCALL_THREAD_CREATE",
    2,
    {0/*sizeof(uint32_t) thread id:out param?*/,sizeof(pok_thread_attr_t)},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_VIRTUAL_DESTINATION,
    "POK_SYSCALL_MIDDLEWARE_VIRTUAL_DESTINATION",
    3,
    {0,0,0/*sizeof(uint32_t)- out param*/},
    1,
   },

   {POK_SYSCALL_OUTB,
    "POK_SYSCALL_OUTB",
    2,
    {0/*data*/,0/*port*/},
    1,
   },

   {POK_SYSCALL_THREAD_STOPSELF,
    "POK_SYSCALL_THREAD_STOPSELF",
    2,
    {0,0}/*unused*/,
    1,
   },

   {POK_SYSCALL_THREAD_RESTART,
    "POK_SYSCALL_THREAD_RESTART",
    2,
    {0,0},
    1,
   },

   {POK_SYSCALL_THREAD_SET_PRIORITY,
    "POK_SYSCALL_THREAD_SET_PRIORITY",
    2,
    {0,0},
    1,
   },

   {POK_SYSCALL_ERROR_HANDLER_CREATE,
    "POK_SYSCALL_ERROR_HANDLER_CREATE",
    2,
    {0,sizeof(void)},
    1,
   },

   {POK_SYSCALL_THREAD_SLEEP,
    "POK_SYSCALL_THREAD_SLEEP",
    2,
    {sizeof(uint32_t),sizeof(uint32_t)},
#ifdef NO_SYS_TIME_RELATED
    0,
#else
    1,
#endif
   },

   {POK_SYSCALL_CONSWRITE,
    "POK_SYSCALL_CONSWRITE",
    2,
    {0,0},
#ifdef NO_SYS_CONSWRITE
    0,
#else
    1,
#endif

   },

   {POK_SYSCALL_THREAD_ID,
    "POK_SYSCALL_THREAD_ID",
    2,
    {0/*sizeof(uint32_t)- out param?*/,0},
    1,
   },

   {POK_SYSCALL_ERROR_RAISE_APPLICATION_ERROR,
    "POK_SYSCALL_ERROR_RAISE_APPLICATION_ERROR",
    2,
    {sizeof(char)*32,0},
    1,
   },

   {POK_SYSCALL_GETTICK,
    "POK_SYSCALL_GETTICK",
    2,
    {0/*sizeof(uint64_t)- out param*/,0},
    1,
   },

   {POK_SYSCALL_THREAD_SUSPEND_TARGET,
    "POK_SYSCALL_THREAD_SUSPEND_TARGET",
    2,
    {0,0},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_VIRTUAL_GET_GLOBAL,
    "POK_SYSCALL_MIDDLEWARE_VIRTUAL_GET_GLOBAL",
    2,
    {0,0/*sizeof(uint32_t)- out param*/},
    1,
   },

   {POK_SYSCALL_PARTITION_GET_LOCK_LEVEL,
    "POK_SYSCALL_PARTITION_GET_LOCK_LEVEL",
    2,
    {0/*sizeof(uint32_t)- out param*/,0},
    1,
   },

   {POK_SYSCALL_ERROR_HANDLER_SET_READY,
    "POK_SYSCALL_ERROR_HANDLER_SET_READY",
    2,
    {0,0}/*not implemented in kernel?!*/,
    1,
   },

   {POK_SYSCALL_INB,
    "POK_SYSCALL_INB",
    2,
    {0,0},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_VIRTUAL_NB_DESTINATIONS,
    "POK_SYSCALL_MIDDLEWARE_VIRTUAL_NB_DESTINATIONS",
    2,
    {0,0/*sizeof(uint32_t)- out param*/},
    1,
   },

   {POK_SYSCALL_THREAD_PERIOD,
    "POK_SYSCALL_THREAD_PERIOD",
    2,
    {0,0},
#ifdef NO_SYS_TIME_RELATED
    0,
#else
    1,
#endif
   },

   {POK_SYSCALL_PARTITION_SET_MODE,
    "POK_SYSCALL_PARTITION_SET_MODE",
    2,
    {0,0},
    1,
   },

   {POK_SYSCALL_THREAD_SLEEP_UNTIL,
    "POK_SYSCALL_THREAD_SLEEP_UNTIL",
    2,
    {0,0},
#ifdef NO_SYS_TIME_RELATED
    0,
#else
    1,
#endif
   },

   {POK_SYSCALL_MIDDLEWARE_SAMPLING_STATUS,
    "POK_SYSCALL_MIDDLEWARE_SAMPLING_STATUS",
    2,
    {0,0/*sizeof(uint32_t)- out param*/},
    1,
   },

   {POK_SYSCALL_THREAD_RESUME,
    "POK_SYSCALL_THREAD_RESUME",
    2,
    {0,0},
    1,
   },

   {POK_SYSCALL_THREAD_STOP,
    "POK_SYSCALL_THREAD_STOP",
    2,
    {0,0},
#ifdef NO_SYS_TIME_RELATED
    0,
#else
    1,
#endif
   },

   {POK_SYSCALL_PARTITION_GET_START_CONDITION,
    "POK_SYSCALL_PARTITION_GET_START_CONDITION",
    2,
    {0/*sizeof(pok_start_condition_t)- out param*/,0},
    1,
   },

   {POK_SYSCALL_THREAD_STATUS,
    "POK_SYSCALL_THREAD_STATUS",
    2,
    {0,0/*sizeof(pok_thread_attr_t)- out param*/},
    1,
   },

   {POK_SYSCALL_THREAD_SUSPEND,
    "POK_SYSCALL_THREAD_SUSPEND",
    2,
    {0,0},
#ifdef NO_SYS_TIME_RELATED
    0,
#else
    1,
#endif
   },

   {POK_SYSCALL_MIDDLEWARE_SAMPLING_ID,
    "POK_SYSCALL_MIDDLEWARE_SAMPLING_ID",
    2,
    {sizeof(char)*32,0/*sizeof(pok_port_id_t)- out param*/},
    1,
   },

   {POK_SYSCALL_LOCKOBJ_CREATE,
    "POK_SYSCALL_LOCKOBJ_CREATE",
    2,
    {sizeof(pok_lockobj_id_t),sizeof(pok_lockobj_attr_t)},
    1,
   },

   {POK_SYSCALL_PARTITION_GET_DURATION,
    "POK_SYSCALL_PARTITION_GET_DURATION",
    2,
    {0/*sizeof(uint64_t)- out param*/,0},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_SAMPLING_WRITE,
    "POK_SYSCALL_MIDDLEWARE_SAMPLING_WRITE",
    3,
    {0,sizeof(void)*32,0},
    1,
   },

   {POK_SYSCALL_THREAD_DELAYED_START,
    "POK_SYSCALL_THREAD_DELAYED_START",
    2,
    {0,0},
#ifdef NO_SYS_TIME_RELATED
    0,
#else
    1,
#endif
   },

   {POK_SYSCALL_LOCKOBJ_OPERATION,
    "POK_SYSCALL_LOCKOBJ_OPERATION",
    2,
    {0,sizeof(pok_lockobj_lockattr_t)},
    1,
   },

   {POK_SYSCALL_MIDDLEWARE_QUEUEING_RECEIVE,
    "POK_SYSCALL_MIDDLEWARE_QUEUEING_RECEIVE",
    5,
    {0,0/*warning: normal size 64b*/,0,0/*sizeof(void)32out*/,0/*sizeof(pok_port_size_t)- out param*/},
    1,
   }

};

#ifdef GENERATOR_RAND_ID_ARGS_VALID_USERMEM
static uint32_t compute_expected_size(uint8_t syscall_id)
{
   uint32_t size = 0;
   for (int i = 0; i < pok_syscall_table[syscall_id].pok_syscall_nbargs; i++) {
      if ((pok_syscall_table[syscall_id]).pok_param_sizes[i] == 0) {
         size += 4;
      }
      size += (pok_syscall_table[syscall_id]).pok_param_sizes[i];
   }
   return size;
}
#endif

#endif
