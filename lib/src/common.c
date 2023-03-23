#include "common.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

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
