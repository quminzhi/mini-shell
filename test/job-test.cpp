#include <gtest/gtest.h>
#include <string>

extern "C" {
#include <stdlib.h>
#include "job.h"
}

class JobTest: public ::testing::Test {
protected:

  struct job_t jobs[MAXJOBS]; /* job list */

  void SetUp() override {
    initjobs(jobs);
  }

  void TearDown() override {}
};

TEST_F(JobTest, TestInitialization) {
  std::string cmd1 = "tail -f &";
  char *ccmd1 = new char[cmd1.size() + 1];
  strcpy(ccmd1, cmd1.c_str());
  addjob(jobs, 12, FG, ccmd1);

  struct job_t *job = getjobPID(jobs, 12);
  EXPECT_EQ(job->jid, getNextJID() - 1);
  EXPECT_EQ(job->pid, 12);
  EXPECT_EQ(job->state, FG);
  // EXPECT_EQ(job->cmdline, "tail -f &");  // address comparison, not c string compare
  EXPECT_TRUE(strcmp(job->cmdline, "tail -f &") == 0);
}

TEST_F(JobTest, TestAddJobs) {
  std::string cmd1 = "sleep 3 &";
  char *ccmd1 = new char[cmd1.size() + 1];
  strcpy(ccmd1, cmd1.c_str());
  addjob(jobs, 2, BG, ccmd1);
  addjob(jobs, 3, FG, ccmd1);
  addjob(jobs, 4, BG, ccmd1);
  EXPECT_EQ(getNextJID(), 5);  // jobs is a global value
}
