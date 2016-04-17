#include "minitest.h"

TEST_SETUP(scheme) {
}

TEST_TEARDOWN(scheme) {
}

TEST_BEGIN(scheme);

TEST_EQ(1, 1);
TEST_EQ(0, 0);
TEST_EQ_STR("hej", "hej");


TEST_END(scheme);

