#include <gtest/gtest.h>
#include <string>

extern "C" {
#include "common.h"
#include <stdlib.h>
}

class CommTest : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(CommTest, TestInitialization) {}

TEST_F(CommTest, TestAddJobs) {}
