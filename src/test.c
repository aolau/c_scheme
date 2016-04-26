#include "minitest.h"
#include "scheme.h"

static scheme_context *context = 0;

#define SCHEME_R(expr_) scheme_read((expr_), context)

char * read_print(char *expr) {
    scheme_obj *o = scheme_read((expr), context);
    char *p = scheme_print(o);
    scheme_obj_mark(o, UNUSED);
    return p;
}

char * read_eval_print(char *expr) {
    scheme_obj *ro = scheme_read(expr, context);
    scheme_obj *eo = scheme_eval(ro, context);
    char *p = scheme_print(eo);
    scheme_obj_mark(ro, UNUSED);
    scheme_obj_mark(eo, UNUSED);
    
    return p;
}


#define SCHEME_RP(expr_) read_print((expr_))

#define SCHEME_REP(expr_)  read_eval_print((expr_))

TEST_SETUP(scheme) {
    context = scheme_init();
    scheme_context_push_env(context,
                            scheme_env_create(
                                scheme_read("(+ - * nil if a b c)", context),
                                scheme_read("(+ - * () if 1 2 3)", context),
                                context));
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
TEST_EQ(0, scheme_obj_as_num(SCHEME_R("0")));
TEST_EQ(5, scheme_obj_as_num(SCHEME_R("5")));

/* read strings */
TEST_EQ_STR("\"\"", SCHEME_RP("\"\""));
TEST_EQ_STR("\"a b c\"", SCHEME_RP("\"a b c\""));

/* read symbols */
TEST_EQ_STR("foo", SCHEME_RP("foo"));

/* read and print */
TEST_EQ_STR("1", SCHEME_RP("1"));
TEST_EQ_STR("foo", SCHEME_RP("foo"));
TEST_EQ_STR("\"foo\"", SCHEME_RP("\"foo\""));

TEST_EQ_STR("\'1", SCHEME_RP("\'1"));

/* lists */
TEST_EQ_STR("nil", scheme_print(scheme_obj_nil()));

TEST_EQ_STR("(1 2)", SCHEME_RP("(1 2)"));
TEST_EQ_STR("(foo 1 2)", SCHEME_RP("(foo 1 2)"));
TEST_EQ_STR("(\"bar\")", SCHEME_RP("(\"bar\")"));

TEST_EQ_STR("((1 2) foo)", SCHEME_RP("((1 2) foo)"));
TEST_EQ_STR("(foo (1 2))", SCHEME_RP("(foo (1 2))"));
TEST_EQ_STR("(foo (1 2 (bar \"baz\")))",
            SCHEME_RP("(foo (1 2 (bar \"baz\")))"));


/* eval */

/* Global variable lookup (a b c) (1 2 3)*/
TEST_EQ_STR("1", SCHEME_REP("a"));
TEST_EQ_STR("2", SCHEME_REP("b"));
TEST_EQ_STR("3", SCHEME_REP("c"));

/* self-evaluating */
TEST_EQ_STR("1", SCHEME_REP("1"));
TEST_EQ_STR("\"bar\"", SCHEME_REP("\"bar\""));

/* Empty list */
TEST_EQ_STR("nil", SCHEME_REP("()"));
            
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

/* if */
TEST_EQ_STR("5", SCHEME_REP("(if 1 5 6)"));
TEST_EQ_STR("6", SCHEME_REP("(if () 5 6)"));
TEST_EQ_STR("5", SCHEME_REP("(if (if 1 (- 2 1) ()) (+ 2 3) 6)"));
TEST_EQ_STR("nil", SCHEME_REP("(if () 4)"));

/* list */
TEST_EQ_STR("nil", SCHEME_REP("(list)"));
TEST_EQ_STR("(1 2 3)", SCHEME_REP("(list 1 2 3)"));
TEST_EQ_STR("(1 2 3)", SCHEME_REP("(list 1 (+ 1 1) (if 1 3))"));

/* let */
TEST_EQ_STR("1", SCHEME_REP("(let ((a 1) (b 2)) (- b a))"));
TEST_EQ_STR("1", SCHEME_REP("(let ((a (+ 2 1)) (b (+ 1 1))) (- a b))"));
TEST_EQ_STR("1", SCHEME_REP("(let ((a 1) (b (let ((a 2)) a))) (- b a))"));

/* GC */

for (int j = 0; j < 1000; j++) {
    TEST_EQ_STR("(+ 1)", SCHEME_RP("(+ 1)"));
}

TEST_END(scheme);
