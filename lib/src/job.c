#include "job.h"
#include <stdio.h>
#include <string.h>

static int nextJID = 1;

int getNextJID() {
  return nextJID;
}

void initjobs(struct job_t *jobs) {
  for (int i = 0; i < MAXJOBS; i++) {
    clearjob(&jobs[i]);
  }
}

void clearjob(struct job_t *job) {
  job->pid = 0;
  job->jid = 0;
  job->state = UNDEF;
  job->cmdline[0] = '\0';
}

// return max job id in the job list
int maxJID(struct job_t *jobs) {
  int rc = 0;
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].jid > rc) {
      rc = jobs[i].jid;
    }
  }
  return rc;
}

int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) {
  if (pid < 1) {
    fprintf(stderr, "error: pid < 1\n");
    return FAILURE;
  }

  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == 0) {
      jobs[i] = (struct job_t){.pid = pid, .state = state, .jid = nextJID++};

      if (nextJID > MAXJID)
        nextJID = 1;
      strcpy(jobs[i].cmdline, cmdline);

      if (verbose) {
        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid,
               jobs[i].cmdline);
      }

      return SUCCESS;
    }
  }

  printf("Tried to create too many jobs\n");
  return FAILURE;
}

int deletejob(struct job_t *jobs, pid_t pid) {
  if (pid < 1) {
    fprintf(stderr, "error: pid < 1\n");
    return FAILURE;
  }

  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == pid) {
      clearjob(&jobs[i]);
      // update nextJID available
      nextJID = maxJID(jobs) + 1;
      return SUCCESS;
    }
  }

  return FAILURE;
}

pid_t fgPID(struct job_t *jobs) {
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].state == FG) {
      return jobs[i].pid;
    }
  }
  return 0;
}

struct job_t *getjobPID(struct job_t *jobs, pid_t pid) {
  if (pid < 1)
    return NULL;

  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == pid) {
      return &jobs[i];
    }
  }

  return NULL;
}

struct job_t *getjobJID(struct job_t *jobs, int jid) {
  if (jid < 1)
    return NULL;

  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].jid == jid) {
      return &jobs[i];
    }
  }

  return NULL;
}

int PID2JID(struct job_t *jobs, pid_t pid) {
  if (pid < 1) {
    return 0;
  }

  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == pid) {
      return jobs[i].jid;
    }
  }

  return 0;
}

void listjobs(struct job_t *jobs) {
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid != 0) {
      printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
      switch (jobs[i].state) {
      case BG:
        printf("Running ");
        break;
      case FG:
        printf("Foreground ");
        break;
      case ST:
        printf("Stopped ");
        break;
      default:
        fprintf(stderr, "listjobs: Internal error: job[%d].state=%d ", i,
                jobs[i].state);
      }

      printf("%s\n", jobs[i].cmdline);
    }
  }
}
