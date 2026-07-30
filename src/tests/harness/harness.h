#pragma once

// Minimal test harness. Tests self-register from constructors, the same pattern
// REGISTER_OBJECT and TYPE_DEFINE already use, so a test file needs no manual
// registration list. main() reports and exits non-zero on failure, which is the
// entire protocol `meson test` requires.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    void (*func)(void);
} TEST_CASE;

#define TEST_MAX_CASES 128
static TEST_CASE m_TestCases[TEST_MAX_CASES];
static int m_TestCaseCount = 0;
static int m_TestFails = 0; // failures in the case being run
static int m_TestTotalFails = 0;

static void Test_Register(const char *name, void (*func)(void))
{
    if (m_TestCaseCount < TEST_MAX_CASES) {
        m_TestCases[m_TestCaseCount++] = (TEST_CASE) { name, func };
    }
}

#define TEST(name_)                                                            \
    static void name_(void);                                                   \
    __attribute__((constructor)) static void M_Register_##name_(void)          \
    {                                                                          \
        Test_Register(#name_, name_);                                          \
    }                                                                          \
    static void name_(void)

#define TEST_FAIL(fmt_, ...)                                                   \
    do {                                                                       \
        printf("    %s:%d: " fmt_ "\n", __FILE__, __LINE__, ##__VA_ARGS__);    \
        m_TestFails++;                                                         \
    } while (0)

#define CHECK(cond_)                                                           \
    do {                                                                       \
        if (!(cond_)) {                                                        \
            TEST_FAIL("expected: %s", #cond_);                                 \
        }                                                                      \
    } while (0)

#define CHECK_EQ_INT(actual_, expected_)                                       \
    do {                                                                       \
        const long long a_ = (long long)(actual_);                             \
        const long long e_ = (long long)(expected_);                           \
        if (a_ != e_) {                                                        \
            TEST_FAIL("%s: expected %lld, got %lld", #actual_, e_, a_);        \
        }                                                                      \
    } while (0)

#define CHECK_EQ_STR(actual_, expected_)                                       \
    do {                                                                       \
        const char *a_ = (actual_);                                            \
        const char *e_ = (expected_);                                          \
        if (a_ == nullptr || strcmp(a_, e_) != 0) {                            \
            TEST_FAIL(                                                         \
                "%s: expected \"%s\", got \"%s\"", #actual_, e_,               \
                a_ != nullptr ? a_ : "(null)");                                \
        }                                                                      \
    } while (0)

#define CHECK_NULL(ptr_)                                                       \
    do {                                                                       \
        if ((ptr_) != nullptr) {                                               \
            TEST_FAIL("%s: expected null", #ptr_);                             \
        }                                                                      \
    } while (0)

#define CHECK_NOT_NULL(ptr_)                                                   \
    do {                                                                       \
        if ((ptr_) == nullptr) {                                               \
            TEST_FAIL("%s: expected non-null", #ptr_);                         \
        }                                                                      \
    } while (0)

static int Test_Main(void)
{
    int passed = 0;
    for (int i = 0; i < m_TestCaseCount; i++) {
        m_TestFails = 0;
        m_TestCases[i].func();
        if (m_TestFails == 0) {
            printf("  PASS  %s\n", m_TestCases[i].name);
            passed++;
        } else {
            printf(
                "  FAIL  %s (%d check(s))\n", m_TestCases[i].name, m_TestFails);
            m_TestTotalFails++;
        }
    }
    printf(
        "\n%d passed, %d failed, %d total\n", passed, m_TestTotalFails,
        m_TestCaseCount);
    return m_TestTotalFails == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(void)
{
    return Test_Main();
}
