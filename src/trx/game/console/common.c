#include <trx/game/console/common.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/debug.h>
#include <trx/game/console/internal.h>
#include <trx/game/console/registry.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/lua/events.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool m_IsOpened = false;

// Controls whether console commands emit log events to the UI console
static bool m_Verbose = true;

// Collected console text while capture is active.
static char *m_Capture = nullptr;
static bool m_IsCapturing = false;

static void M_Shutdown(void)
{
    Console_History_Shutdown();

    m_IsOpened = false;
}

static RESULT M_Load(void)
{
    Console_History_Init();
    return OK;
}

static void M_Capture(const char *const text)
{
    char *const merged = m_Capture == nullptr
        ? Memory_DupStr(text)
        : String_Format("%s\n%s", m_Capture, text);
    Memory_FreePointer(&m_Capture);
    m_Capture = merged;
}

static void M_Emit(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const bool show, const char *const fmt, va_list va)
{
    va_list va_copy;
    va_copy(va_copy, va);

    const size_t text_length = vsnprintf(nullptr, 0, fmt, va);
    char *text = Memory_Alloc(text_length + 1);

    vsnprintf(text, text_length + 1, fmt, va_copy);
    va_end(va_copy);

    Log_Message(level, file, line, func, "%s", text);

    if (m_IsCapturing) {
        M_Capture(text);
    }

    if (show) {
        const LUA_EVENT_ARG args[] = {
            { .type = LUA_EVENT_ARG_STRING, .value.str = text },
        };
        LUA_FireEventEx(LUA_EVENT_CONSOLE_LOG, args, 1);
    }

    Memory_FreePointer(&text);
}

void Console_Open(void)
{
    if (m_IsOpened) {
        return;
    }
    m_IsOpened = true;
    LUA_FireEvent(LUA_EVENT_CONSOLE_OPEN);
}

void Console_Close(void)
{
    if (!m_IsOpened) {
        return;
    }
    m_IsOpened = false;
    LUA_FireEvent(LUA_EVENT_CONSOLE_CLOSE);
}

bool Console_IsOpened(void)
{
    return m_IsOpened;
}

void Console_LogImpl(
    const LOG_LEVEL level, const char *file, int line, const char *func,
    const char *const fmt, ...)
{
    ASSERT(fmt != nullptr);

    va_list va;
    va_start(va, fmt);
    M_Emit(level, file, line, func, m_Verbose, fmt, va);
    va_end(va);
}

void Console_ShowImpl(
    const LOG_LEVEL level, const char *file, int line, const char *func,
    const char *const fmt, ...)
{
    ASSERT(fmt != nullptr);

    va_list va;
    va_start(va, fmt);
    M_Emit(level, file, line, func, true, fmt, va);
    va_end(va);
}

void Console_SetVerbose(const bool verbose)
{
    m_Verbose = verbose;
}

bool Console_IsVerbose(void)
{
    return m_Verbose;
}

void Console_BeginCapture(void)
{
    ASSERT(!m_IsCapturing);
    Memory_FreePointer(&m_Capture);
    m_IsCapturing = true;
}

char *Console_EndCapture(void)
{
    m_IsCapturing = false;
    char *const text = m_Capture != nullptr ? m_Capture : Memory_DupStr("");
    m_Capture = nullptr;
    return text;
}

void Console_Clear(void)
{
    LUA_FireEvent(LUA_EVENT_CONSOLE_CLEAR);
}

COMMAND_RESULT Console_Eval(const char *const cmdline)
{
    LOG_INFO("executing command: %s", cmdline);

    // A completed line can carry leading indentation; the command word and its
    // arguments both begin after it.
    const char *line = cmdline;
    while (*line == ' ') {
        line++;
    }

    const CONSOLE_COMMAND *const matching_cmd = Console_Registry_Get(line);
    if (matching_cmd == nullptr) {
        Console_Error(GS("general/osd/unknown_command"), line);
        return CR_BAD_INVOCATION;
    }

    char *prefix = Memory_DupStr(line);
    char *args = "";
    char *space = strchr(prefix, ' ');
    if (space != nullptr) {
        *space = '\0';
        args = space + 1;
    }

    const COMMAND_CONTEXT ctx = {
        .cmd = matching_cmd,
        .prefix = prefix,
        .args = args,
    };
    ASSERT(matching_cmd->proc != nullptr);
    const COMMAND_RESULT result = matching_cmd->proc(&ctx);
    Memory_FreePointer(&prefix);

    switch (result) {
    case CR_BAD_INVOCATION:
        Console_Error(GS("general/osd/command_bad_invocation"), cmdline);
        break;

    case CR_UNAVAILABLE:
        Console_Error(GS("general/osd/command_unavailable"));
        break;

    case CR_SUCCESS:
    case CR_FAILURE:
        // The commands themselves are responsible for handling logging in
        // these scenarios.
        break;
    }
    return result;
}

REGISTER_SUBSYSTEM(.load = M_Load, .shutdown = M_Shutdown)
