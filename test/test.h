#ifndef T1M_TEST_H
#define T1M_TEST_H

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

extern size_t tests;
extern size_t fails;

#define TEST_RESULTS()                                                         \
    do {                                                                       \
        if (fails == 0) {                                                      \
            printf("ALL TESTS PASSED (%lu/%lu)\n", tests, tests);              \
        } else {                                                               \
            printf("SOME TESTS FAILED (%lu/%lu)\n", tests - fails, tests);     \
        }                                                                      \
    } while (0)

#define ASSERT_OK(test)                                                        \
    do {                                                                       \
        tests++;                                                               \
        if (!(test)) {                                                         \
            fails++;                                                           \
            printf("%s:%d error \n", __FILE__, __LINE__);                      \
        }                                                                      \
    } while (0)

#define ASSERT_EQUAL_BASE(equality, a, b, format)                              \
    do {                                                                       \
        tests++;                                                               \
        if (!(equality)) {                                                     \
            fails++;                                                           \
            printf(                                                            \
                "%s:%u (" format " != " format ")\n", __FILE__, __LINE__, (a), \
                (b));                                                          \
        }                                                                      \
    } while (0)

#define ASSERT_INT_EQUAL(a, b) ASSERT_EQUAL_BASE((a) == (b), a, b, "%d")
#define ASSERT_ULONG_EQUAL(a, b) ASSERT_EQUAL_BASE((a) == (b), a, b, "%lu")
#define ASSERT_STR_EQUAL(a, b) ASSERT_EQUAL_BASE(!strcmp(a, b), a, b, "%s")

#endif
