#include "minitest.h"
#include "scheme.h"

TEST_SETUP(scheme) {
}

TEST_TEARDOWN(scheme) {
}

TEST_BEGIN(scheme);

TEST_EQ_STR("", scheme_print(NULL));

TEST_END(scheme);

