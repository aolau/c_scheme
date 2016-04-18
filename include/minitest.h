#ifndef _MINITEST_H_
#define _MINITEST_H_

#include <string.h>

#include "trace.h"

static int test_count = 0, test_error_count = 0;

static void test_progress(const char *c) {
    test_count++;
    TRACE_SIMPLE("%s", c);
}

static void test_success() {
    test_progress(".");
}

typedef struct test_info {
    char *file;
    int line;
    char *a_expr;
    char *b_expr;
} test_info;

static void test_error(test_info *info) {
    test_error_count++;
    test_progress("e");
    TRACE_NL;
    TRACE("Assert failed in %s on line %d:", info->file, info->line);
    TRACE("%s", info->a_expr);
    TRACE("%s", info->b_expr);
}

static void test_assert(int cond, test_info *info) {
    if (cond) {
        test_success();
    } else {
        test_error(info);
    }
}

static void test_begin(const char *name, void (*setup_func)()) {
    TRACE_NL;
    TRACE("running setup");
    (*setup_func)();
    TRACE("running tests");
    TRACE_PROMPT;
}

static void test_end(const char *name, void (*teardown_func)()) {
    TRACE_NL;
    TRACE("running teardown");
    (*teardown_func)();
    TRACE("tests: %d, errors: %d", test_count, test_error_count);
}

#define TEST_EQ(a, b) \
    do {\
        test_info i = {__FILE__, __LINE__, #a, #b};\
        test_assert((a) == (b), &i);\
} while(0)

#define TEST_EQ_STR(a, b) do {\
        test_info i = {__FILE__, __LINE__, #a, #b}; \
        test_assert( strcmp((a), (b)) == 0, &i);\
} while (0)

#define TEST_BEGIN(name)                        \
    int main() {                                \
        TRACE_INIT(test_##name);                \
        test_begin(#name, name##_setup);
            
#define TEST_END(name)\
    test_end(#name, name##_teardown);\
    return test_error_count == 0 ? 0 : 1;\
    }

#define TEST_SETUP(name)\
    void name##_setup()

#define TEST_TEARDOWN(name)\
    void name##_teardown()

#endif /* _MINITEST_H_ */
