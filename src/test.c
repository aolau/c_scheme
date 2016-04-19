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

/* eat space */
TEST_EQ_STR("(1 2 3)", scheme_eat_space("  (1 2 3)"));
TEST_EQ_STR("(1 2 3)", scheme_eat_space("(1 2 3)"));

/* peek */
TEST_EQ('a', scheme_peek("abc"));

/* is_digit */
TEST_EQ(true, scheme_is_digit('0'));
TEST_EQ(true, scheme_is_digit('9'));

/* read numbers */ 
TEST_EQ(0, scheme_obj_as_num(scheme_read("0")));
TEST_EQ(5, scheme_obj_as_num(scheme_read("5")));

/* read strings */
TEST_EQ_STR("", scheme_obj_as_string(scheme_read("\"\"")));
TEST_EQ_STR("a b c", scheme_obj_as_string(scheme_read("\"a b c\"")));

/* read symbols */
TEST_EQ_STR("foo", scheme_obj_as_string(scheme_read("foo")));

/* print */
TEST_EQ_STR("1", scheme_print(scheme_read("1")));
TEST_EQ_STR("foo", scheme_print(scheme_read("foo")));
TEST_EQ_STR("\"foo\"", scheme_print(scheme_read("\"foo\"")));

TEST_END(scheme);

