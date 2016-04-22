#include "minitest.h"
#include "scheme.h"

static scheme_context *context = 0;

#define SCHEME_REP(expr_)\
    scheme_print(scheme_eval(scheme_read(expr_), context))

TEST_SETUP(scheme) {
    context = scheme_init();
    scheme_context_set_env(context,
                           scheme_env_create(scheme_read("(a b c)"),
                                             scheme_read("(1 2 3)")));
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

/* read and print */
TEST_EQ_STR("1", scheme_print(scheme_read("1")));
TEST_EQ_STR("foo", scheme_print(scheme_read("foo")));
TEST_EQ_STR("\"foo\"", scheme_print(scheme_read("\"foo\"")));

TEST_EQ_STR("\'1", scheme_print(scheme_read("\'1")));

/* lists */
TEST_EQ_STR("()", scheme_print(scheme_obj_nil()));

TEST_EQ_STR("(1 2)", scheme_print(scheme_read("(1 2)")));
TEST_EQ_STR("(foo 1 2)", scheme_print(scheme_read("(foo 1 2)")));
TEST_EQ_STR("(\"bar\")", scheme_print(scheme_read("(\"bar\")")));

TEST_EQ_STR("((1 2) foo)", scheme_print(scheme_read("((1 2) foo)")));
TEST_EQ_STR("(foo (1 2))", scheme_print(scheme_read("(foo (1 2))")));
TEST_EQ_STR("(foo (1 2 (bar \"baz\")))",
            scheme_print(scheme_read("(foo (1 2 (bar \"baz\")))")));


/* eval */

/* Global variable lookup (a b c) (1 2 3)*/
TEST_EQ_STR("1", SCHEME_REP("a"));
TEST_EQ_STR("2", SCHEME_REP("b"));
TEST_EQ_STR("3", SCHEME_REP("c"));

/* self-evaluating */
TEST_EQ_STR("1", SCHEME_REP("1"));
TEST_EQ_STR("\"bar\"", SCHEME_REP("\"bar\""));

/* quote */
TEST_EQ_STR("1", SCHEME_REP("\'1"));


/* primitive operations */

TEST_EQ_STR("5", SCHEME_REP("(+ 1 2 2)"));
TEST_EQ_STR("5", SCHEME_REP("(+ (+ 1 2) 2)"));

TEST_EQ_STR("5", SCHEME_REP("(- 6 1)"));
TEST_EQ_STR("5", SCHEME_REP("(+ (- 4 1) 2)"));

TEST_EQ_STR("6", SCHEME_REP("(* 3 2)"));
TEST_EQ_STR("6", SCHEME_REP("(* (+ 1 2) (- 3 1))"));

/* Primitive operations and global variables */
TEST_EQ_STR("6", SCHEME_REP("(+ (+ a b) c)"));

TEST_END(scheme);
