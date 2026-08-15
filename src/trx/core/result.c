#include <trx/core/result.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/shell.h>
#include <trx/core/strings/common.h>

#include <stdarg.h>

RESULT Result_Fail(const RESULT_SITE *const site, const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char *const message = Memory_DupStr(String_FormatStaticV(fmt, va));
    va_end(va);
    // The message travels with the failure, and the consumer decides whether
    // to log it. A failure the caller only probed for and then absorbed is
    // never logged.
    return (RESULT) { .msg = message, .site = site };
}

RESULT Result_Prefix(RESULT result, const char *const fmt, ...)
{
    if (IS_OK(result)) {
        return result;
    }
    va_list va;
    va_start(va, fmt);
    char *context = Memory_DupStr(String_FormatStaticV(fmt, va));
    va_end(va);
    if (result.msg == nullptr) {
        result.msg = context;
        return result;
    }
    char *const prefixed = String_Format("%s: %s", context, result.msg);
    Memory_FreePointer(&context);
    Memory_FreePointer(&result.msg);
    result.msg = prefixed;
    return result;
}

RESULT Result_Merge(const RESULT a, const RESULT b)
{
    if (IS_OK(a)) {
        return b;
    }
    if (IS_OK(b) || b.msg == nullptr) {
        IGNORE(b);
        return a;
    }
    if (a.msg == nullptr) {
        return b;
    }
    RESULT joined = { .site = a.site };
    joined.msg = String_Format("%s\n%s", a.msg, b.msg);
    IGNORE(a);
    IGNORE(b);
    return joined;
}

void Result_ExitOnFail(RESULT result, const char *const fmt, ...)
{
    if (IS_OK(result)) {
        return;
    }
    va_list va;
    va_start(va, fmt);
    char *message = Memory_DupStr(String_FormatStaticV(fmt, va));
    va_end(va);
    if (result.msg != nullptr) {
        char *const joined = String_Format("%s\n%s", message, result.msg);
        Memory_FreePointer(&message);
        Memory_FreePointer(&result.msg);
        message = joined;
    }
    char *const logged = String_Format(
        "%s (at %s:%d:%s)", message, result.site->file, result.site->line,
        result.site->func);
    Shell_ExitSystemEx(logged, message);
}

bool Result_Should(RESULT result, const char *const err)
{
    if (IS_OK(result)) {
        return true;
    }
    const char *const src_file = result.site->file;
    const int src_line = result.site->line;
    const char *const src_func = result.site->func;
    if (result.msg != nullptr && err != nullptr) {
        Log_Message(
            LOG_LEVEL_WARNING, src_file, src_line, src_func, "%s: %s", err,
            result.msg);
    } else if (result.msg != nullptr) {
        Log_Message(
            LOG_LEVEL_WARNING, src_file, src_line, src_func, "%s", result.msg);
    } else if (err != nullptr) {
        Log_Message(LOG_LEVEL_WARNING, src_file, src_line, src_func, "%s", err);
    }
    Memory_FreePointer(&result.msg);
    return false;
}

bool Result_Absorb(RESULT result)
{
    const bool ok = IS_OK(result);
    Memory_FreePointer(&result.msg);
    return ok;
}
