#include <stdio.h>


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

#define TEST_EQ(a, b)\
    if ((a) == (b)) {\
        test_success();\
    } else {\
        test_error();\
    }

#define TEST_BEGIN(name) printf("test = %s\n", name)
#define TEST_END printf("\ntests: %d, errors: %d\n",\
                        test_count, test_error_count)

int main() {
    TEST_BEGIN("scheme");
    TEST_EQ(1, 1);
    TEST_EQ(1, 0);
    TEST_EQ(0, 0);
    TEST_END;
    
    return 0;
}
