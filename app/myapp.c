#include "myapp.h"
#include "common.h"
#include "job.h"
#include "shell.h"
#include <signal.h>
#include <stdio.h>

char prompt[] = "mini> ";
struct job_t jobs[MAXJOBS];

int main(int argc, char *argv[]) {
  int emit_prompt = 1; /* emit prompt (default) */

  /* Redirect stderr to stdout (so that driver will get all output
  │* on the pipe connected to stdout) */
  dup2(1, 2);

  /* Parse the command line */
  char o;
  while ((o = getopt(argc, argv, "hvp")) != EOF) {
    switch (o) {
    case 'h': /* print help message */
      usage();
      break;
    case 'v': /* emit additional diagnostic info */
      verbose = 1;
      break;
    case 'p':          /* don't print a prompt */
      emit_prompt = 0; /* handy for automatic testing */
      break;
    default:
      usage();
    }
  }

  /* Install the signal handlers */
  Signal(SIGINT, sigint_handler);   /* ctrl-c */
  Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
  Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */
  Signal(SIGQUIT, sigquit_handler); /* Kill the shell */

  /* Initialize the job list */
  initjobs(jobs);

  /* Execute the shell's read/eval loop */
  char cmdline[MAXLINE];
  
  while (1) {
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }
    /* Read command line */
    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
      app_error("fgets error");
    if (feof(stdin)) { /* End of file (ctrl-d) */
      fflush(stdout);
      exit(0);
    }

    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
  }

  /* control should never reach here */
  exit(0);
}
