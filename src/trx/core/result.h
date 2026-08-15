#pragma once

// Warns when a RESULT is discarded without being handled. Attribute support
// is still a little uneven across C23 toolchains, especially Apple clang.
#if defined(__has_c_attribute)
    #if __has_c_attribute(nodiscard)
        #define NODISCARD [[nodiscard]]
    #endif
#endif
#ifndef NODISCARD
    #define NODISCARD
#endif

// The result of an operation that can fail. Failures may carry a message,
// which is owned by the RESULT until it is consumed.
//
// FAIL creates a failure and MUST passes one up the call stack, adding
// context along the way. Every result eventually reaches a consumer:
// EXIT_ON_FAIL reports it and exits, SHOULD logs it and carries on, and
// IGNORE quietly frees it.
typedef struct NODISCARD {
    bool ok;
    char *msg;
} RESULT;

#define OK ((RESULT) { .ok = true })

// A failure with no explanation. FAIL is the better choice in most cases;
// ERR fits only where the caller already knows what went wrong.
#define ERR ((RESULT) { .ok = false })

#define IS_OK(r_) ((r_).ok)

// Creates a failure with a formatted message.
//
// Example:
// return FAIL("%s: bad '%s' at line %d", path, key, line);
RESULT Result_Fail(const char *fmt, ...);
#define FAIL(...) Result_Fail(__VA_ARGS__)

// Returns a failure from the enclosing function when the condition is true.
//
// Example:
// FAIL_IF(obj == nullptr, "%s: missing '%s'", path, key);
#define FAIL_IF(cond_, ...)                                                    \
    do {                                                                       \
        if (cond_) {                                                           \
            return FAIL(__VA_ARGS__);                                          \
        }                                                                      \
    } while (0)

// Adds context to a failure message. Successful results pass through
// unchanged.
//
// This fits where the current function knows something the original failure
// site did not, such as which file was being loaded.
RESULT Result_Prefix(RESULT result, const char *fmt, ...);

// Returns from the enclosing function immediately if the result failed.
// Optional arguments add context before the failure passes upward.
//
// Resources that need cleanup must be owned by AUTO_FREE locals, because
// nothing else runs between the failure and the return.
//
// Example:
// MUST(M_ReadLevels(io), "%s", path);
#define MUST(r_, ...)                                                          \
    do {                                                                       \
        RESULT must_result_ = (r_);                                            \
        if (!IS_OK(must_result_)) {                                            \
            __VA_OPT__(                                                        \
                must_result_ = Result_Prefix(must_result_, __VA_ARGS__);)      \
            return must_result_;                                               \
        }                                                                      \
    } while (0)

// Combines two results. The result is OK only if both are OK; if both
// failed, their messages are joined.
//
// This fits collecting all errors instead of stopping at the first one.
RESULT Result_Merge(RESULT a, RESULT b);

// Reports a failed result and ends the run. The supplied message is the
// headline and the RESULT's message provides the detail.
//
// This fits the boundary of a complete operation, such as loading a level or
// script.
void Result_ExitOnFail(RESULT result, const char *fmt, ...);
#define EXIT_ON_FAIL(r_, ...) Result_ExitOnFail((r_), __VA_ARGS__)

// Logs a failed result as a warning, frees its message, and carries on.
// Returns true on success and false on failure, so it reads directly as a
// condition.
//
// The optional error text adds context when the RESULT itself is not enough.
// The warning names the call site rather than this module.
bool Result_Should(
    RESULT result, const char *err, const char *file, int line,
    const char *func);

#define M_SHOULD_2(r_, err_)                                                   \
    Result_Should((r_), (err_), __FILE__, __LINE__, __func__)
#define M_SHOULD_1(r_)                                                         \
    Result_Should((r_), nullptr, __FILE__, __LINE__, __func__)
#define M_SHOULD_PICK(_1, _2, NAME, ...) NAME
#define SHOULD(...)                                                            \
    M_SHOULD_PICK(__VA_ARGS__, M_SHOULD_2, M_SHOULD_1)(__VA_ARGS__)

// Consumes a result without logging it. Frees any failure message and
// returns true on success and false on failure.
//
// This fits where failure is expected or uninteresting and the caller has
// nothing useful to report; SHOULD is the better choice when the reason
// deserves a line in the log.
bool Result_Absorb(RESULT result);

#define IGNORE(r_) Result_Absorb((r_))
