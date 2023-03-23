#pragma once
#ifndef SHELL_H_
#define SHELL_H_

#include "common.h"

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

#endif
