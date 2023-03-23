#pragma once
#ifndef JOB_H_
#define JOB_H_

#include "common.h"
#include <unistd.h>

#define MAXLINE 1024   /* max line size */
#define MAXJOBS 16     /* max jobs at any time point */
#define MAXJID 1 << 16 /* max job id */

enum { UNDEF, FG, BG, ST, STATE_SIZE }; /* define job states */

struct job_t {
  pid_t pid;             /* process id */
  int jid;               /* job id */
  int state;             /* UNDEF, FG, BG, RUN */
  char cmdline[MAXLINE]; /* command line string */
};

void initjobs(struct job_t *jobs);
void listjobs(struct job_t *jobs);

// return max job id in the job list
int maxJID(struct job_t *jobs);
int getNextJID();

int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
void clearjob(struct job_t *jobs);

// return pid of currrent foreground job, 0 if no foreground job
pid_t fgPID(struct job_t *jobs, pid_t pid);
// return null on failure
struct job_t *getjobPID(struct job_t *jobs, pid_t pid);
// return null on failure
struct job_t *getjobJID(struct job_t *jobs, int jid);
// return 0 if pid does not exist
int PID2JID(struct job_t *jobs, pid_t pid);

#endif // JOB_H_
