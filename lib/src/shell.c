#include "shell.h"
#include "job.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

static int argc;
static char *argv[MAXARGS];

// Wrapper for the sigaction function
handler_t Signal(int signum, handler_t handler) {
  struct sigaction action, old_action;
  action.sa_handler = handler;
  sigemptyset(&action.sa_mask); /* block sigs of type being handled */
  action.sa_flags = SA_RESTART; /* restart syscalls if possible */
  if (sigaction(signum, &action, &old_action) < 0)
    unix_error("Signal error");
  return (old_action.sa_handler);
}

void sigquit_handler(int sig) {
  printf("Terminating after receipt of SIGQUIT signal\n");
  exit(1);
}

// synchronize with addjob by blocking signal masks
void sigchld_handler(int sig) {
  if (verbose) {
    printf("sigchld_handler: entering\n");
  }

  int olderrno = errno;
  pid_t pid;
  int status;
  while ((pid = waitpid(-1, &status, 0)) > 0) {
    int jid = PID2JID(jobs, pid);
    if (verbose) {
      printf("sigchld_handler: Job [%d] (%d) deleted\n", jid, pid);
    }

    // reap a zombie child
    deletejob(jobs, pid);

    if (verbose) {
      if (WIFEXITED(status)) {
        printf("sigchld_handler: Job [%d] (%d) terminates OK (status %d)\n",
               jid, pid, WEXITSTATUS(status));
      } else {
        printf("sigchld_handler: Job [%d] (%d) terminates abnormally\n", jid,
               pid);
      }
    }
  }

  errno = olderrno;
  if (verbose) {
    printf("sigchld_handler: exiting\n");
  }
}
void sigtstp_handler(int sig) {}
void sigint_handler(int sig) {}

void eval(char *cmdline) {
  // initialize argv
  for (int i = 0; i < MAXARGS; i++) {
    argv[i] = NULL;
  }

  int is_bg = parseline(cmdline, argv);
  if (argv[0] == NULL || strcmp(argv[0], "") == 0) {
    // empty line
    return;
  }

  int is_builtin;
  // if it is a builtin command
  if ((is_builtin = builtin_cmd(argv)) == 1)
    return;

  // if not, synchronize add and delete jobs
  pid_t pid;
  sigset_t mask_all, mask_one, prev;
  sigfillset(&mask_all);
  sigemptyset(&mask_one);
  sigaddset(&mask_one, SIGCHLD);

  // to make sure sigchld_handler triggered after job is added
  // block SIGCHLD for parent process and child process
  sigprocmask(SIG_BLOCK, &mask_one, &prev);

  /* child process */
  if ((pid = fork()) == 0) {
    sigprocmask(SIG_SETMASK, &prev, NULL);
    if (execve(argv[0], argv, environ) < 0) {
      fprintf(stderr, "%s: Command not found\n", argv[0]);
      exit(0);
    }
  }

  /* shell process */
  int p_state = is_bg ? BG : FG;
  // prevent any signal from interrupting the addjob routine
  sigprocmask(SIG_BLOCK, &mask_all, NULL);
  addjob(jobs, pid, p_state, argv[0]);
  // restore original mask state
  sigprocmask(SIG_SETMASK, &prev, NULL);

  if (!is_bg) {
    // not a backgroup request
    waitfg(pid);
  }
}

void waitfg(pid_t pid) {
  sigset_t mask_chld, prev;
  sigemptyset(&mask_chld);
  sigaddset(&mask_chld, SIGCHLD);

  sigprocmask(SIG_BLOCK, &mask_chld, &prev);
  while (fgPID(jobs)) {
    sigsuspend(&prev);
  }

  if (verbose) {
    printf("waitfg: Process (%d) no longer the foreground process\n", pid);
  }

  // restore mask
  sigprocmask(SIG_SETMASK, &prev, NULL);
}

// return 1 and execute builtin command immediately, and 0 otherwise
int builtin_cmd(char *argv[]) {
  // resolve builtin command if it is valid
  if (strcmp(*argv, "quit") == 0 && argc == 1) {
    exit(0);
    return 1;
  } else if (strcmp(*argv, "jobs") == 0 && argc == 1) {
    listjobs(jobs);
    return 1;
  } else if ((strcmp(*argv, "fg") == 0 && argc == 2) ||
             (strcmp(*argv, "bg") == 0 && argc == 2)) {
    do_bgfg(argv);
    return 1;
  }

  return 0;
}

void do_bgfg(char *argv[]) {}

/* Helper Functions */

int parseline(const char *cmdline, char *argv[]) {
  static char buf[MAXLINE]; /* cache cmdline */
  char *p = buf;            /* pointer that traverse the buffer */
  int bg = 0;               /* 1 if it is a background command, 0 otherwise */

  strcpy(buf, cmdline);
  // replace trailing \n with a space
  buf[strlen(buf) - 1] = ' ';

  // parse argument: treat '...' as a single argument
  argc = 0;
  char *delim;
  // determine delimeter
  if (*p == '\'') {
    p++;
    delim = strchr(p, '\'');
  } else {
    delim = strchr(p, ' ');
  }

  // two pointers
  while (delim) {
    argv[argc++] = p;
    // replace delimeter with null terminated sign '\0'
    *delim = '\0';
    p = delim + 1;

    // skip leading spaces and find delimeter of ith argument
    while (p && *p == ' ')
      p++;

    // determine delimeter
    if (*p == '\'') {
      p++;
      delim = strchr(p, '\'');
    } else {
      delim = strchr(p, ' ');
    }
  };

  // appending a null to the end of the argument list
  argv[argc] = NULL;

  // if cmd is a blank line, treat it as a bg job with no argv
  if (argc == 0)
    return 1;

  // bg is 1 if & is the last argument, 0 otherwise.
  // remove & from the argument list
  if ((bg = (*argv[argc - 1] == '&')) != 0) {
    argv[--argc] = NULL;
  }

  return bg;
}

void usage(void) {
  printf("Usage: shell [-hvp]\n");
  printf("   -h   print this message\n");
  printf("   -v   print additional diagnostic information\n");
  printf("   -p   do not emit a command prompt\n");
  exit(1);
}

void unix_error(char *msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

void app_error(char *msg) {
  fprintf(stdout, "%s\n", msg);
  exit(1);
}
