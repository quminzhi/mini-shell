#include "shell.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

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

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

void eval(char *cmdline);
int builtin_cmd(char *argv[]);
void do_bgfg(char *argv[]);
void waitfg(pid_t pid);

/* Helper Functions */

int parseLine(const char *cmdline, char *argv[]) {
  static char buf[MAXLINE]; /* cache cmdline */
  char *p = buf;            /* pointer that traverse the buffer */
  int bg = 0;               /* 1 if it is a background command, 0 otherwise */

  strcpy(buf, cmdline);
  // replace trailing \n with a space
  buf[strlen(buf) - 1] = ' ';

  // parse argument: treat '...' as a single argument
  int argc = 0;
  char *delim;
  // two pointers
  do {
    // skip leading spaces and find delimeter of ith argument
    while (p && *p == ' ')
      p++;
    if (*p == '\'') {
      p++;
      delim = strchr(buf, '\'');
    } else {
      delim = strchr(buf, ' ');
    }

    argv[argc++] = p;
    // replace delimeter with null terminated sign '\0'
    *delim = '\0';
    p = delim + 1;
  } while (delim);

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
