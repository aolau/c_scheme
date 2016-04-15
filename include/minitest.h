#ifndef _MINITEST_H_
#define _MINITEST_H_

#include <stdio.h>
#include <string.h>

static int test_count = 0, test_error_count = 0;

static void test_progress(const char *c) {
    test_count++;
    printf("%s", c);
}

static void test_success() {
    test_progress(".");
}

static void test_error(const char *file, int line,
                       const char *a_str, const char *b_str) {
    test_error_count++;
    test_progress("e");
    printf("\nAssert failed in %s on line %d:\n", file, line);
    printf("%s\n", a_str);
    printf("%s\n", b_str);
}

static void test_assert(int cond, const char *file, int line,
                        const char *a_str, const char *b_str) {
    if (cond) {
        test_success();
    } else {
        test_error(file, line, a_str, b_str);
    }
}

static void test_begin(const char *name, void (*setup_func)()) {
    printf("test = %s\n", name);
    printf("> running setup\n");
    (*setup_func)();
    printf("> running tests\n");
}

static void test_end(const char *name, void (*teardown_func)()) {
    printf("> running teardown\n");
    (*teardown_func)();
    printf("\n> finished test: %s\n", name);
    printf("> tests: %d, errors: %d\n", test_count, test_error_count);
}

#define TEST_EQ(a, b) test_assert((a) == (b), __FILE__, __LINE__, #a, #b)
#define TEST_EQ_STR(a, b) test_assert( strcmp((a), (b)) == 0,\
                                       __FILE__, __LINE__, #a, #b)
    
#define TEST_BEGIN(name)\
    int main() {\
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
