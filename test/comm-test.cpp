#include <cstdio>
#include <gtest/gtest.h>

extern "C" {
#include "csapp.h"
}

TEST(CSAPPTEST, INVOCATIONTEST) {
  EXPECT_EQ(sio_putl(10), 2);
}
