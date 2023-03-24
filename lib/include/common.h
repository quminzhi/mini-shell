#pragma once
#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define SUCCESS 1
#define FAILURE 0
#define bool uint8_t
#define true 1
#define false 0

#define MAXLINE 1024   /* max line size */

int verbose = 0;

#endif  // COMMON_H_
