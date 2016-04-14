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

static void test_error() {
    test_error_count++;
    test_progress("e");
}

static void test(int cond) {
    if (cond) {
        test_success();
    } else {
        test_error();
    }
}
    
#define TEST_EQ(a, b) test((a) == (b))
#define TEST_EQ_STR(a, b) test( strcmp((a), (b)) == 0)

    
#define TEST_BEGIN(name)\
    int main() {\
        printf("test = %s\n", name)
            
#define TEST_END\
    printf("\ntests: %d, errors: %d\n", test_count, test_error_count);\
    return test_error_count == 0 ? 0 : 1;\
    }

TEST_BEGIN("scheme");
TEST_EQ(1, 1);
TEST_EQ(1, 0);
TEST_EQ(0, 0);
TEST_EQ_STR("hej", "hej");
TEST_EQ_STR("hej", "nej");
TEST_END;

