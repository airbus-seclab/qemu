/*
 * QEMU American Fuzzy Lop board
 * child
 *
 * Copyright (c) 2019 S. Duverger Airbus
 * GPLv2
 */
#include "qemu/afl.h"

/* We need to give AFL a fake pid to SIGKILL on timeout.
 * But we don't fork().
 *
 * Pre-requisite: child_pid > 0
 *
 * Solution 1 : child_pid = 1 (init)
 * kill() will fail if we are not root
 * Solution 2: getpid() - XXX, as pid are incremented upon process creation
 * this let us a huge window before exhausting pid space and cycling.
 * The risk is on highly active system to reach it and kill a process
 * owned by our user (unlikely) in a race condition between "chosen pid"
 * process creation and AFL sending SIGKILL.
 */
#ifdef AFL_CONTACT
static int get_fake_pid(afl_t *afl)
{
   if (afl->euid)
      return 1;

   /* run as root (bad) so find unused pid */
   struct stat st;
   char   path[64];
   int    pid = afl->ppid;

__force_pid_search:
   while (--pid > 0) {
      if (pid != afl->pid && pid != afl->ppid) {
         snprintf(path, sizeof(path), "/proc/%d", pid);
         if (stat(path, &st) < 0 || !S_ISDIR(st.st_mode))
            return pid;
      }
   }

   pid = INT_MAX;
   goto __force_pid_search;
}

void afl_forward_child(afl_t *afl)
{
   int child_pid = get_fake_pid(afl);
   if (write(afl->config.afl.sts_fd, &child_pid, 4) != 4) {
      error_report("can't write pid to afl");
      exit(EXIT_FAILURE);
   }
}
#endif
