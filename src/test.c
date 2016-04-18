#include "minitest.h"
#include "scheme.h"

static scheme_context *context = 0;

TEST_SETUP(scheme) {
    context = scheme_init();
}

TEST_TEARDOWN(scheme) {
    scheme_shutdown(context);
}

TEST_BEGIN(scheme);

TEST_EQ(0.0, scheme_obj_as_num(scheme_read("0.0")));
TEST_EQ(5.0, scheme_obj_as_num(scheme_read("5.0")));
TEST_EQ_STR("", scheme_print(NULL));

TEST_END(scheme);

