#pragma once
#ifndef SHELL_H_
#define SHELL_H_

#include "common.h"
#include "job.h"

#define MAXARGS 128

extern char **environ;
struct job_t jobs[MAXJOBS];

typedef void (*handler_t)(int);
handler_t Signal(int signum, handler_t handler);

void sigquit_handler(int sig);
void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

void eval(char *cmdline);
int builtin_cmd(char *argv[]);
void do_bgfg(char *argv[]);
void waitfg(pid_t pid);

/* helper functions */

// return 1 if a bg job is requested, 0 otherwise
int parseline(const char *cmdline, char *argv[]);
void usage(void);
void unix_error(char *msg);
void app_error(char *msg);

#endif
