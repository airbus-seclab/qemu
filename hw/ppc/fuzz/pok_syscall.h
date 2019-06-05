/*
 * (c) 2018, Airbus A. Gantet
 */


#include <stdint.h>
/* typedef unsigned short        uint8_t; */
/* typedef unsigned short        uint16_t; */
/* typedef unsigned int          uint32_t; */
/* typedef unsigned long long    uint64_t; */

/* typedef short                 int16_t; */
/* typedef signed long long      int64_t; */

/* typedef unsigned int          size_t; */
//typedef unsigned long int     intptr_t;

#define FALSE  0
#define TRUE   1
#define bool_t int
#define pok_bool_t int


#define POK_CONFIG_NB_THREADS 5
#define POK_ERROR_MAX_MSG_SIZE                  250

typedef uint32_t pok_port_size_t;
typedef uint8_t pok_port_direction_t;
typedef uint8_t pok_port_kind_t;
typedef uint8_t pok_queueing_discipline_t;
typedef uint8_t pok_port_id_t;
typedef uint8_t pok_size_t;
typedef uint8_t pok_range_t;
typedef uint8_t pok_buffer_id_t;
typedef uint8_t pok_blackboard_id_t;
typedef uint8_t pok_lockobj_id_t;
typedef uint8_t pok_sem_id_t;
typedef uint8_t pok_event_id_t;
typedef uint8_t pok_partition_id_t;
typedef uint16_t pok_sem_value_t;

typedef enum
{
      POK_ERRNO_OK                    =   0,
      POK_ERRNO_EINVAL                =   1,

      POK_ERRNO_UNAVAILABLE           =   2,
      POK_ERRNO_PARAM                           =   3,
      POK_ERRNO_TOOMANY               =   5,
      POK_ERRNO_EPERM                 =   6,
      POK_ERRNO_EXISTS                =   7,

      POK_ERRNO_ERANGE                =   8,
      POK_ERRNO_EDOM                  =   9,
      POK_ERRNO_HUGE_VAL              =  10,

      POK_ERRNO_EFAULT                =  11,

      POK_ERRNO_THREAD                =  49,
      POK_ERRNO_THREADATTR            =  50,

      POK_ERRNO_TIME                 =  100,

      POK_ERRNO_PARTITION_ATTR        = 200,

      POK_ERRNO_PORT                 =  301,
      POK_ERRNO_NOTFOUND             =  302,
      POK_ERRNO_DIRECTION            =  303,
      POK_ERRNO_SIZE                 =  304,
      POK_ERRNO_DISCIPLINE           =  305,
      POK_ERRNO_PORTPART             =  307,
      POK_ERRNO_EMPTY                =  308,
      POK_ERRNO_KIND                 =  309,
      POK_ERRNO_FULL                 =  311,
      POK_ERRNO_READY                =  310,
      POK_ERRNO_TIMEOUT              =  250,
      POK_ERRNO_MODE                 =  251,

      POK_ERRNO_LOCKOBJ_UNAVAILABLE  =  500,
      POK_ERRNO_LOCKOBJ_NOTREADY     =  501,
      POK_ERRNO_LOCKOBJ_KIND         =  502,
      POK_ERRNO_LOCKOBJ_POLICY       =  503,

      POK_ERRNO_PARTITION_MODE       =  601,

      POK_ERRNO_PARTITION            =  401
} pok_ret_t;


typedef enum
{
   POK_SYSCALL_CONSWRITE                           =  10,
   POK_SYSCALL_GETTICK                             =  20,
   POK_SYSCALL_INT_NUMBER                          =  42,
   POK_SYSCALL_THREAD_CREATE                       =  50,
   POK_SYSCALL_THREAD_SLEEP_UNTIL                  =  51,
   POK_SYSCALL_THREAD_SLEEP                        =  52,
   POK_SYSCALL_THREAD_SUSPEND                      =  53,
   POK_SYSCALL_THREAD_RESTART                      =  54,
   POK_SYSCALL_THREAD_STOP                         =  55,
   POK_SYSCALL_THREAD_PERIOD                       =  56,
   POK_SYSCALL_THREAD_STOPSELF                     =  57,
   POK_SYSCALL_THREAD_ID                           =  58,
   POK_SYSCALL_THREAD_STATUS                       =  59,
   POK_SYSCALL_THREAD_SET_PRIORITY								 =  60,
   POK_SYSCALL_THREAD_RESUME                       =  61,
   POK_SYSCALL_THREAD_SUSPEND_TARGET               =  62,
   //POK_SYSCALL_THREAD_DEADLINE                     =  63,
   //POK_SYSCALL_THREAD_STATE                        =  64,
   POK_SYSCALL_THREAD_DELAYED_START                =  65,
   POK_SYSCALL_MIDDLEWARE_SAMPLING_ID              = 101,
   POK_SYSCALL_MIDDLEWARE_SAMPLING_READ            = 102,
   POK_SYSCALL_MIDDLEWARE_SAMPLING_STATUS          = 103,
   POK_SYSCALL_MIDDLEWARE_SAMPLING_WRITE           = 104,
   POK_SYSCALL_MIDDLEWARE_SAMPLING_CREATE          = 105,
   POK_SYSCALL_MIDDLEWARE_QUEUEING_CREATE          = 110,
   POK_SYSCALL_MIDDLEWARE_QUEUEING_SEND            = 111,
   POK_SYSCALL_MIDDLEWARE_QUEUEING_RECEIVE         = 112,
   POK_SYSCALL_MIDDLEWARE_QUEUEING_ID              = 113,
   POK_SYSCALL_MIDDLEWARE_QUEUEING_STATUS          = 114,
   POK_SYSCALL_MIDDLEWARE_VIRTUAL_CREATE           = 150,
   POK_SYSCALL_MIDDLEWARE_VIRTUAL_NB_DESTINATIONS  = 151,
   POK_SYSCALL_MIDDLEWARE_VIRTUAL_DESTINATION      = 152,
   POK_SYSCALL_MIDDLEWARE_VIRTUAL_GET_GLOBAL       = 153,
   POK_SYSCALL_LOCKOBJ_CREATE                      = 201,
   POK_SYSCALL_LOCKOBJ_OPERATION                   = 202,
   POK_SYSCALL_ERROR_HANDLER_CREATE                = 301,
   POK_SYSCALL_ERROR_HANDLER_SET_READY             = 302,
   POK_SYSCALL_ERROR_RAISE_APPLICATION_ERROR       = 303,
   POK_SYSCALL_ERROR_GET                           = 304,
   POK_SYSCALL_PARTITION_SET_MODE                  = 404,
   POK_SYSCALL_PARTITION_GET_ID                    = 405,
   POK_SYSCALL_PARTITION_GET_PERIOD                = 406,
   POK_SYSCALL_PARTITION_GET_DURATION              = 407,
   POK_SYSCALL_PARTITION_GET_LOCK_LEVEL            = 408,
   POK_SYSCALL_PARTITION_GET_OPERATING_MODE        = 409,
   POK_SYSCALL_PARTITION_GET_START_CONDITION       = 410,
   POK_SYSCALL_INB                                 = 501,
   POK_SYSCALL_OUTB                                = 502,
   POK_SYSCALL_PCI_REGISTER                        = 601,
} pok_syscall_id_t;

typedef struct
{
   uint32_t             nargs;
   uint32_t             arg1;
   uint32_t             arg2;
   uint32_t             arg3;
   uint32_t             arg4;
   uint32_t             arg5;
} pok_syscall_args_t;

typedef struct
{
   pok_partition_id_t   partition;
   uint32_t             thread;
   uint32_t             base_addr;
}pok_syscall_info_t;

typedef struct
{
    pok_port_size_t      size;
    pok_port_direction_t direction;
    uint8_t              waiting_processes;
}pok_port_queueing_status_t;

typedef enum
{
    POK_PORT_QUEUEING_DISCIPLINE_FIFO      = 1,
    POK_PORT_QUEUEING_DISCIPLINE_PRIORITY  = 2
} pok_port_queueing_disciplines_t;

typedef enum
{
    POK_PORT_DIRECTION_IN   = 1,
    POK_PORT_DIRECTION_OUT  = 2
} pok_port_directions_t;

typedef pok_queueing_discipline_t pok_port_queueing_discipline_t;

typedef enum
{
    POK_PORT_KIND_QUEUEING  = 1,
    POK_PORT_KIND_SAMPLING  = 2,
#ifdef POK_NEEDS_PORTS_VIRTUAL
    POK_PORT_KIND_VIRTUAL   = 2,
#endif
    POK_PORT_KIND_INVALID   = 10
} pok_port_kinds_t;

typedef unsigned int pok_spinlock_t;

/* All kind of lock objects we have in the kernel */

typedef enum
{
   POK_LOCKOBJ_POLICY_STANDARD = 0,
   POK_LOCKOBJ_POLICY_PIP      = 1,
   POK_LOCKOBJ_POLICY_PCP      = 2
}pok_locking_policy_t;


typedef enum
{
   LOCKOBJ_STATE_LOCK = 0,
   LOCKOBJ_STATE_UNLOCK = 1,
   LOCKOBJ_STATE_WAITEVENT = 2
}pok_mutex_state_t;

typedef enum
{
   POK_LOCKOBJ_KIND_MUTEX = 1,
   POK_LOCKOBJ_KIND_SEMAPHORE = 2,
   POK_LOCKOBJ_KIND_EVENT = 3
}pok_lockobj_kind_t;

typedef struct
{
   pok_spinlock_t             spin;
   pok_spinlock_t             eventspin;
   /* spinlock to enfoce mutual exclusion */

   bool_t                     is_locked;
   /* Is the mutex locked ? */
   
   pok_mutex_state_t          thread_state[POK_CONFIG_NB_THREADS + 2];
   /* Describe which thread is blocked in the mutex */
   
   pok_locking_policy_t       locking_policy;
   /* Locking policy */

   pok_queueing_discipline_t  queueing_policy;
   /* Locking policy */

   pok_lockobj_kind_t         kind;
   
   bool_t                     initialized;
   /* Is the mutex initialized ? */
   
   uint16_t                   current_value;
   uint16_t                   max_value;
} pok_lockobj_t;


typedef struct
{
    pok_port_id_t                    identifier;
    pok_partition_id_t               partition;
    pok_port_size_t                  index;
    bool_t                           full;
    pok_port_size_t                  size;
    pok_port_size_t                  off_b; /* Offset of the beginning of the buffer */
    pok_port_size_t                  off_e; /* Offset of the end of the buffer */
    pok_port_direction_t             direction;
    pok_port_queueing_discipline_t   discipline;
    pok_bool_t                       ready;
    bool_t                           empty;
    uint8_t                          kind;
    uint64_t                         refresh;
    uint64_t                         last_receive;
    pok_lockobj_t                    lock;
    bool_t                           must_be_flushed;
}pok_port_t;

typedef struct
{
   uint8_t        error_kind;
   uint32_t       failed_thread;
   uint32_t       failed_addr;
   char           msg[POK_ERROR_MAX_MSG_SIZE];
   uint32_t       msg_size;
} pok_error_status_t;

typedef enum
{
   POK_PARTITION_MODE_INIT_COLD = 1, /**< Partition is in cold mode (init mode),
                                       only the main thread is active */
   POK_PARTITION_MODE_INIT_WARM = 2, /**< Partition is in warm init mode,
                                       only the main thread is active */
   POK_PARTITION_MODE_NORMAL    = 3, /**< Partition is currently being executed,
                                       main thread is disabled, other threads
                                       are running
                                     */
   POK_PARTITION_MODE_IDLE      = 4, /**< Idle mode, the partition is not executed */
   POK_PARTITION_MODE_RESTART   = 5, /**< Currently being restarted */
   POK_PARTITION_MODE_STOPPED   = 6, /**< Partition is stopped, it will no longer be executed */
}pok_partition_mode_t;

typedef enum
{
  POK_STATE_STOPPED = 0,
  POK_STATE_RUNNABLE = 1,
  POK_STATE_WAITING = 2,
  POK_STATE_LOCK = 3,
  POK_STATE_WAIT_NEXT_ACTIVATION = 4,
  POK_STATE_DELAYED_START = 5
} pok_state_t;

typedef struct
{
    uint8_t      priority;         /* Priority is from 0 to 255 */
    void*        entry;            /* entrypoint of the thread  */
    uint64_t     period;
    uint64_t     deadline;
    uint64_t     time_capacity;
    uint32_t     stack_size;
    pok_state_t  state;
} pok_thread_attr_t;

typedef enum
{
  NORMAL_START          = 0,
  PARTITION_RESTART     = 1,
  HM_MODULE_RESTART     = 2,
  HM_PARTITION_RESTART  = 3
}pok_start_condition_t;

typedef uint8_t pok_lockobj_id_t;


/* All kind of lock objects we have in the kernel */

typedef uint8_t pok_queueing_discipline_t;

typedef uint16_t pok_sem_value_t;

typedef struct
{
   pok_lockobj_kind_t         kind;
   pok_locking_policy_t       locking_policy;
   pok_queueing_discipline_t  queueing_policy;  
   pok_sem_value_t            initial_value;
   pok_sem_value_t            max_value;
}pok_lockobj_attr_t;

typedef enum
{
   LOCKOBJ_OPERATION_LOCK = 1,
   LOCKOBJ_OPERATION_UNLOCK = 2,
   LOCKOBJ_OPERATION_WAIT = 3,
   LOCKOBJ_OPERATION_SIGNAL = 4,
   LOCKOBJ_OPERATION_BROADCAST = 5
}pok_lockobj_operation_t;

typedef enum
{
   LOCKOBK_LOCK_REGULAR = 1,
   LOCKOBJ_LOCK_TIMED   = 2
}pok_lockobj_lock_kind_t;

typedef struct
{
   pok_lockobj_operation_t    operation;
   pok_lockobj_kind_t         obj_kind;
   pok_lockobj_lock_kind_t    lock_kind;
   uint64_t                   time;
}pok_lockobj_lockattr_t;



/**
 *  Function that performs the syscall. It is called by the 
 *  architecture interruption handler.
 *
 *  @param syscall_id
 *  This param correspond to the syscal which should be performed.
 *  The list of available syscalls is available in
 *  the definition of the pok_syscall_id_t type
 *
 *  @param args
 *  Arguments of the syscall. It corresponds to data useful
 *  to perform the syscall.
 *
 *  @param infos
 *  Informations about the syscall: which partition/thread
 *  initiates the syscall, etc ...
 *
 *  @return
 *  Returns an error code, which is defined in include/errno.h
 */

pok_ret_t	pok_core_syscall (const pok_syscall_id_t     syscall_id,
                              const pok_syscall_args_t*  args,
                              const pok_syscall_info_t*  infos);

/*
 * Initiate syscalls.
 * This part is defined in low-level layers, each architecture/bsp
 * initiate the syscall in a different way.
 */
pok_ret_t pok_syscall_init(void);

#define POK_CHECK_PTR_OR_RETURN(pid,ptr)	\
   if (!POK_CHECK_PTR_IN_PARTITION(pid,ptr))	\
{						\
    return POK_ERRNO_EINVAL;                        \
}
