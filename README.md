# Mini Shell 

This is a mini Unix shell program that supports job control.

## Overview

A shell is an interactive command-line interpreter that runs programs on behalf of the user. A shell repeatedly prints a prompt, waits for a command line on stdin, and then carries out some action, as directed by the contents of the command line.

The command line is a sequence of ASCII text words delimited by whitespace. The ﬁrst word in the command line is either the name of a built-in command or the pathname of an executable ﬁle. The remaining words are command-line arguments.

If the ﬁrst word is a built-in command, the shell immediately executes the command in the current process. Otherwise, the word is assumed to be the pathname of an executable program. In this case, the shell forks a child process, then loads and runs the program in the context of the child. The child processes created as a result of interpreting a single command line are known collectively as a job. In general, a job can consist of multiple child processes connected by Unix pipes.

If the command line ends with an ampersand ”&”, then the job runs in the background, which means that the shell does not wait for the job to terminate before printing the prompt and awaiting the next command line. Otherwise, the job runs in the foreground, which means that the shell waits for the job to terminate before awaiting the next command line. Thus, at any point in time, at most one job can be running in the foreground. However, an arbitrary number of jobs can run in the background.

```bash
# built-in commands
minish> jobs

# executable
minish> /bin/ls -l -d
```

Following is an example of backgound jobs.

```bash
# run a program in the background
minish> /bin/ls -l -d &
```

Mini shell support the notion of job control, which allows users to move jobs back and forth between background and foreground, and to change the process state (running, stopped, or terminated) of the processes in a job.

- Typing `ctrl-c` causes a SIGINT signal to be delivered to each process in the foreground job.
- Similarly, typing `ctrl-z` causes a SIGTSTP signal to be delivered to each process in the foreground job.

## Features

- The command line typed by the user should consist of a name and zero or more arguments, all separated by one or more spaces. If name is a built-in command, then Minish should handle it immediately and wait for the next command line. Otherwise, Minish should assume that name is the path of an executable ﬁle, which it loads and runs in the context of an initial child process (In this context, the term job refers to this initial child process).
- Mini shell does not support pipes or I/O direction for the time being.
- Typing ctrl-c (ctrl-z) should cause a SIGINT (SIGTSTP) signal to be sent to the current foreground job, as well as any descendents of that job (e.g., any child processes that it forked). If there is no foreground job, then the signal should have no effect.
- If the command line ends with an ampersand &, then Minish should run the job in the background. Otherwise, it should run the job in the foreground.
- Each job can be identiﬁied by either a process ID (PID) or a job ID (JID). JIDs should be denoted on the command line by the preﬁx ’%’. For example, “%5” denotes JID 5, and “5” denotes PID 5.
- Minish supports the following built-in commands:
  - The `quit` command terminates the shell.
  - The `jobs` command lists all background jobs.
  - The `bg` <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in the background. The <job> argument can be either a PID or a JID.
  - The `fg` <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in the foreground. The <job> argument can be either a PID or a JID.
- Minish should reap all of its zombie children.

## Architecture

### Jobs

- `job_t` struct

`job_t` struct is defined as follows,

```c
struct job_t {
 pit_t pid;  /* process id */
 int jid;    /* job id */
 int state;  /* process state includes UNDEF, FG, BG, RUNNING */
 char cmdline[MAXLINE]  /* command line string */
}
```

- Job list 

Job list is defined in `shell.c` as a global variable.

```c
#define MAXJOBS 16
struct job_t jobs[MAXJOBS];
```

- Manipulation functions

Job-related manipulations are shown as follows.

```c
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
```

### Main Loop

The shell is basically an infinite while loop, terminated only when received a
termination signal (SIGINT or SIGKILL) or by `quit` built-in command.

```c
// pseudo code for main function
int main(int argc, char *argv[]) {
  init_jobs();
  install_signal_handlers();

  while (true) {
    read_command();
    eval_command();
  }

  return 0;
}
```

### Signal Handlers

- Sigchld

To support foreground and background jobs, the shell process should be able to
wait or not wait some child processes. For foreground job, shell process wait
the termination of foreground child processes and reap them when they terminated
(when the state changed to be ZOMBIE). For background job, shell process has no
bother waiting for the termination of the child process and is free to do its
own things. It reaps zombie processes when the background process finished and
send SIGCHLD signals.

Child processes are reaped by `sigchld_handler` for foreground and background
processes. The only difference is the shell process will be SUSPENDED until
there is no foreground job.

```c
// sigmast and sigsuspend
sigprocmask(SIG_BLOCK, &mask_chld, &prev_chld);
// suspend until no foreground job
while (fgPID(jobs) != 0) {
  sigsuspend(&prev_chld);
}
sigprocmask(SIG_SETMASK, &prev_chld, NULL);
```

```c
// sigchld_handler
while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) {
  // WNOHANG: waitpid will return when there is no zombie process
  // WUNTRACED: waitpid will return when a child process is stopped
  if (WIFEXITED(staeus) || WIFSIGNALED(status)) {
    sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    deletejob(jobs, pid);
    sigprocmask(SIG_SETMASK, &prev_all, NULL);
  }
}
```

### Continue with Builtin Commands

Mini shell provides two builtin commands to continue the execution of some jobs.

```bash
# pid is denoted by a number, '5'
# jid is denoted by the prefix '%' with a number, '%5'

# continue as a foreground job
fg <pid|jid>
# conitnue as a background job
bg <pid|jid>
```

## Contribution

Contributions are welcomed and please send it to email of project maintainer,
matt0xcc@gmail.com

Contact: quminzhi@gmail.com

Copyright (c) 2023 Minzhi. All Rights Reserved.
