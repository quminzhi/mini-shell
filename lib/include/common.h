#pragma once
#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <stdint.h>

#define SUCCESS 1
#define FAILURE 0
#define bool uint8_t

extern char **environ;
int verbose = 0;

int parseLine(const char *cmdline, char *argv[]);
void usage(void);
void unix_error(char *msg);
void app_error(char *msg);

#endif  // COMMON_H_
