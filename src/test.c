#include "minitest.h"
#include "lsp.h"

static lsp_context *context = 0;

#define LSP_R(expr_) lsp_read((expr_), context)

char * read_print(char *expr) {
    lsp_obj *o = lsp_read((expr), context);
    char *p = lsp_print(o);
    lsp_obj_mark(o, UNUSED);
    return p;
}

char * read_eval_print(char *expr) {
    lsp_obj *ro = lsp_read(expr, context);
    lsp_obj *eo = lsp_eval(ro, context);
    char *p = lsp_print(eo);
    lsp_obj_mark(ro, UNUSED);
    lsp_obj_mark(eo, UNUSED);
    
    return p;
}


#define LSP_RP(expr_) read_print((expr_))

#define LSP_REP(expr_)  read_eval_print((expr_))

TEST_SETUP(lsp) {
    context = lsp_init();
    lsp_context_push_env(context,
                            lsp_env_create(
                                lsp_read("(a b c)", context),
                                lsp_read("(1 2 3)", context),
                                context));
}

TEST_TEARDOWN(lsp) {
    lsp_shutdown(context);
}

TEST_BEGIN(lsp);

/* eat space */
TEST_EQ_STR("(1 2 3)", lsp_eat_space("  (1 2 3)"));
TEST_EQ_STR("(1 2 3)", lsp_eat_space("(1 2 3)"));

/* peek */
TEST_EQ('a', lsp_peek("abc"));

/* is_digit */
TEST_EQ(true, lsp_is_digit('0'));
TEST_EQ(true, lsp_is_digit('9'));

/* read strings */
TEST_EQ_STR("\"\"", LSP_RP("\"\""));
TEST_EQ_STR("\"a b c\"", LSP_RP("\"a b c\""));

/* read symbols */
TEST_EQ_STR("foo", LSP_RP("foo"));

/* read and print */
TEST_EQ_STR("1", LSP_RP("1"));
TEST_EQ_STR("foo", LSP_RP("foo"));
TEST_EQ_STR("\"foo\"", LSP_RP("\"foo\""));

TEST_EQ_STR("\'1", LSP_RP("\'1"));

/* lists */
TEST_EQ_STR("nil", lsp_print(lsp_obj_nil()));

TEST_EQ_STR("(1 2)", LSP_RP("(1 2)"));
TEST_EQ_STR("(foo 1 2)", LSP_RP("(foo 1 2)"));
TEST_EQ_STR("(\"bar\")", LSP_RP("(\"bar\")"));

TEST_EQ_STR("((1 2) foo)", LSP_RP("((1 2) foo)"));
TEST_EQ_STR("(foo (1 2))", LSP_RP("(foo (1 2))"));
TEST_EQ_STR("(foo (1 2 (bar \"baz\")))",
            LSP_RP("(foo (1 2 (bar \"baz\")))"));


/* eval */

/* Global variable lookup (a b c) (1 2 3)*/
TEST_EQ_STR("1", LSP_REP("a"));
TEST_EQ_STR("2", LSP_REP("b"));
TEST_EQ_STR("3", LSP_REP("c"));

/* self-evaluating */
TEST_EQ_STR("1", LSP_REP("1"));
TEST_EQ_STR("\"bar\"", LSP_REP("\"bar\""));

/* Empty list */
TEST_EQ_STR("nil", LSP_REP("()"));
            
/* quote */
TEST_EQ_STR("1", LSP_REP("\'1"));
TEST_EQ_STR("foo", LSP_REP("'foo"));
TEST_EQ_STR("nil", LSP_REP("'()"));
TEST_EQ_STR("(1 2 3)", LSP_REP("'(1 2 3)"));
TEST_EQ_STR("'foo", LSP_REP("''foo"));


/* primitive operations */

TEST_EQ_STR("5", LSP_REP("(+ 1 2 2)"));
TEST_EQ_STR("5", LSP_REP("(+ (+ 1 2) 2)"));

TEST_EQ_STR("5", LSP_REP("(- 6 1)"));
TEST_EQ_STR("5", LSP_REP("(+ (- 4 1) 2)"));

TEST_EQ_STR("6", LSP_REP("(* 3 2)"));
TEST_EQ_STR("6", LSP_REP("(* (+ 1 2) (- 3 1))"));

/* Primitive operations and global variables */
TEST_EQ_STR("6", LSP_REP("(+ (+ a b) c)"));

/* if */
TEST_EQ_STR("5", LSP_REP("(if 1 5 6)"));
TEST_EQ_STR("6", LSP_REP("(if () 5 6)"));
TEST_EQ_STR("5", LSP_REP("(if (if 1 (- 2 1) ()) (+ 2 3) 6)"));
TEST_EQ_STR("nil", LSP_REP("(if () 4)"));

/* list */
TEST_EQ_STR("nil", LSP_REP("(list)"));
TEST_EQ_STR("(1 2 3)", LSP_REP("(list 1 2 3)"));
TEST_EQ_STR("(1 2 3)", LSP_REP("(list 1 (+ 1 1) (if 1 3))"));

/* let */
TEST_EQ_STR("1", LSP_REP("(let ((a 1) (b 2)) (- b a))"));
TEST_EQ_STR("1", LSP_REP("(let ((a (+ 2 1)) (b (+ 1 1))) (- a b))"));
TEST_EQ_STR("1", LSP_REP("(let ((a 1) (b (let ((a 2)) a))) (- b a))"));

/* GC */

for (int j = 0; j < 10000; j++) {
    TEST_EQ_STR("1", LSP_REP("(let ((a 1) (b 0)) (if a (+ a b) 0))"));
}

/* defun */
TEST_EQ_STR("add", LSP_REP("(defun add (a b) (+ a b))"));

/* cons */
TEST_EQ_STR("(1)", LSP_REP("(cons 1 nil)"));
TEST_EQ_STR("(1 2)", LSP_REP("(cons 1 (cons 2 nil))"));
TEST_EQ_STR("(1 2 . 3)", LSP_REP("(cons 1 (cons 2 3))"));

/* car, cdr */
TEST_EQ_STR("1", LSP_REP("(car '(1 2 3))"));
TEST_EQ_STR("(2 3)", LSP_REP("(cdr '(1 2 3))"));
TEST_EQ_STR("nil", LSP_REP("(car '())"));
TEST_EQ_STR("nil", LSP_REP("(cdr '())"));

/* progn */
TEST_EQ_STR("3", LSP_REP("(progn 1 2 (+ 2 1))"));

/* equal */
TEST_EQ_STR("t", LSP_REP("(equal \"a\" \"a\")"));
TEST_EQ_STR("nil", LSP_REP("(equal \"a\" \"b\")"));

TEST_EQ_STR("t", LSP_REP("(equal 1 1)"));
TEST_EQ_STR("nil", LSP_REP("(equal 1 2)"));

TEST_END(lsp);
