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

// child processes may be terminated or stopped (WUNTRACED)
void sigchld_handler(int sig) {
  if (verbose) {
    printf("sigchld_handler: entering\n");
  }

  int olderrno = errno;
  pid_t pid;
  int status;
  sigset_t mask_all, prev_all;

  sigfillset(&mask_all);

  // NOTE:
  // WUNTRACED: return when a child process is stopped, like a foreground
  // process is stopped by SIGTSTP (ctrl-z)
  // WNOHANG: return when no zombie (terminated) process exist, allowing shell
  // main process to do some other stuff instead of waiting unterminated
  // processes.
  while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) {
    int jid = PID2JID(jobs, pid);

    // case 1 WUNTRACED: stop process that received SIGTSTP or SIGSTOP
    // NOTE: different from what we do in sigtstp_handler, we handle signals
    // from other sources instead of shell-delivered signals, which is handled
    // in sigtstp_handler
    if (WIFSTOPPED(status) &&
        (WSTOPSIG(status) == SIGTSTP || WSTOPSIG(status) == SIGSTOP)) {
      struct job_t *stpjob = getjobPID(jobs, pid);
      if (stpjob && stpjob->state != ST) {
        // has not been catched
        printf("sigchld_handler: Job [%d] (%d) stopped by signal %d\n",
               PID2JID(jobs, pid), pid, WSTOPSIG(status));
        stpjob->state = ST;
      }
    }

    // case 2: reap termination processes
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) {
      struct job_t *intjob = getjobPID(jobs, pid);
      if (intjob)
        // a child process is terminated by a SIGINT not from shell
        // intjob == NULL when it is handled in sigint_handler
        printf("sigchld_handler: Job [%d] (%d) terminated by signal %d\n",
               PID2JID(jobs, pid), pid, WTERMSIG(status));
    }

    // terminated voluntarily or forcibaly
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
      // deletejob may be called twice when received SIGINT from user
      deletejob(jobs, pid);
      if (verbose) {
        printf("sigchld_handler: Job [%d] (%d) deleted\n", jid, pid);
      }
      sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }

    if (verbose) {
      if (WIFEXITED(status)) {
        printf("sigchld_handler: Job [%d] (%d) terminates OK (status %d)\n",
               jid, pid, WEXITSTATUS(status));
      }
    }
  }

  errno = olderrno;
  if (verbose) {
    printf("sigchld_handler: exiting\n");
  }
}

// The kernel sends a SIGTSTP to the shell whenever the use type ctrl-z at the
// keyboard. Catch it and suspend foreground job by sending it a SIGTSTP.
void sigtstp_handler(int sig) {
  int olderrno = errno;
  sigset_t mask_all, prev_all;
  sigfillset(&mask_all);

  sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
  pid_t pid = fgPID(jobs);
  // send SIGTSTP to foreground job's process grounp
  kill(-pid, SIGTSTP);
  printf("sigtstp_handler: Job [%d] (%d) stopped by signal %d\n",
         PID2JID(jobs, pid), pid, sig);
  struct job_t *stpjob = getjobPID(jobs, pid);
  stpjob->state = ST;
  sigprocmask(SIG_SETMASK, &prev_all, NULL);

  errno = olderrno;
}

// deliver SIGINT handler from user to the foreground job through shell process.
void sigint_handler(int sig) {
  int olderrno = errno;
  sigset_t mask_all, prev_all;
  sigfillset(&mask_all);

  sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
  pid_t pid;
  pid = fgPID(jobs);
  if (pid == 0)
    return;
  kill(-pid, SIGINT);
  printf("sigint_handler: Job [%d] (%d) terminated by signal %d\n",
         PID2JID(jobs, pid), pid, sig);
  deletejob(jobs, pid);
  sigprocmask(SIG_SETMASK, &prev_all, NULL);

  errno = olderrno;
}

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
  sigset_t mask_all, mask_one, prev_one;
  sigfillset(&mask_all);
  sigemptyset(&mask_one);
  sigaddset(&mask_one, SIGCHLD);

  // to make sure sigchld_handler triggered after job is added
  // block SIGCHLD for parent process and child process
  sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

  /* child process */
  if ((pid = fork()) == 0) {
    // give the child process a new gid to handle SIGINT correctly
    setpgid(0, 0);
    sigprocmask(SIG_SETMASK, &prev_one, NULL);
    if (execve(argv[0], argv, environ) < 0) {
      fprintf(stderr, "%s: Command not found\n", argv[0]);
      exit(0);
    }
  }

  /* shell process */
  int p_state = is_bg ? BG : FG;
  // replace '\n' with '\0'
  cmdline[strlen(cmdline) - 1] = '\0';
  // prevent any signal from interrupting the addjob routine
  sigprocmask(SIG_BLOCK, &mask_all, NULL);
  addjob(jobs, pid, p_state, cmdline);
  // restore original mask state
  sigprocmask(SIG_SETMASK, &prev_one, NULL);

  if (!is_bg) {
    // not a backgroup request
    waitfg(pid);
  }
}

void waitfg(pid_t pid) {
  // prevent SIGCHLD from being received at <=
  // in that case, SIGCHLD won't be catched and it causes infinite loop
  sigset_t mask_chld, prev_chld;
  sigemptyset(&mask_chld);
  sigaddset(&mask_chld, SIGCHLD);

  sigprocmask(SIG_BLOCK, &mask_chld, &prev_chld);
  while (fgPID(jobs)) {
    // <=
    sigsuspend(&prev_chld);
  }
  // restore mask
  sigprocmask(SIG_SETMASK, &prev_chld, NULL);

  if (verbose) {
    printf("waitfg: Process (%d) no longer the foreground process\n", pid);
  }
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

// TODO: continue the process as a foreground job or a background job
void do_bgfg(char *argv[]) {
  char *cmd = (char *)malloc(strlen(argv[0]) + 1);
  strcpy(cmd, argv[0]);

  if (argc != 2) {
    fprintf(stderr, "do_bgfg: expected 1 argument, but got %d\n", argc);
    free(cmd);
    return;
  }

  if (strcmp(cmd, "fg") != 0 && strcmp(cmd, "bg") != 0) {
    fprintf(stderr, "do_bgfg: invalid command \'%s\'\n", cmd);
    return;
  }

  struct job_t *job;
  int num = 0;
  char *endptr = NULL;
  char *p = argv[1];
  if (*p == '%') {
    // handle jid
    p++;
    num = strtol(p, &endptr, 10);
    if (p == endptr) {
      printf("%s: argument must be a PID or %%jobid\n", argv[0]);
      return;
    }
    job = getjobJID(jobs, num);
    if (!job) {
      printf("%%%d: No such job\n", num);
      return;
    }
  } else {
    // handle pid
    num = strtol(p, &endptr, 10);
    if (p == endptr) {
      printf("%s: argument must be a PID or %%jobid\n", argv[0]);
      return;
    }
    job = getjobPID(jobs, num);
    if (!job) {
      printf("(%d): No such process\n", num);
      return;
    }
  }

  // change process state and update job list
  if (strcmp(cmd, "bg") == 0) {
    // change ST/BG to BG
    if (job->state == ST) {
      kill(-(job->pid), SIGCONT);
      printf("[%d] (%d) %s\n", job->jid, job->pid, job->cmdline);
      job->state = BG;
    }
  } else {
    // change ST/BG to FG
    if (job->state == ST) {
      kill(-(job->pid), SIGCONT);
      job->state = FG;
    }
    if (job->state == BG) {
      job->state = FG;
    }
    waitfg(job->pid);
  }

  free(cmd);
}

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
