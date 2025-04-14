#include "game/console/common.h"

#include "./internal.h"
#include "debug.h"
#include "game/console/registry.h"
#include "game/game_string.h"
#include "game/ui2.h"
#include "log.h"
#include "memory.h"
#include "strings.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool m_IsOpened = false;
static UI2_CONSOLE_STATE m_UIState = {};

void Console_Init(void)
{
    UI2_Console_Init(&m_UIState);

    Console_History_Init();
}

void Console_Shutdown(void)
{
    UI2_Console_Free(&m_UIState);

    Console_History_Shutdown();
    Console_Registry_Shutdown();

    m_IsOpened = false;
}

void Console_Open(void)
{
    if (m_IsOpened) {
        return;
    }
    m_IsOpened = true;
    UI2_FireEvent(
        (EVENT) { .name = "console_open", .sender = nullptr, .data = nullptr });
}

void Console_Close(void)
{
    if (!m_IsOpened) {
        return;
    }
    m_IsOpened = false;
    UI2_FireEvent((EVENT) {
        .name = "console_close", .sender = nullptr, .data = nullptr });
}

bool Console_IsOpened(void)
{
    return m_IsOpened;
}

void Console_Log(const char *fmt, ...)
{
    ASSERT(fmt != nullptr);

    va_list va;

    va_start(va, fmt);
    const size_t text_length = vsnprintf(nullptr, 0, fmt, va);
    char *text = Memory_Alloc(text_length + 1);
    va_end(va);

    va_start(va, fmt);
    vsnprintf(text, text_length + 1, fmt, va);
    va_end(va);

    LOG_INFO("%s", text);

    UI2_FireEvent((EVENT) {
        .name = "console_log",
        .sender = nullptr,
        .data = text,
    });
    Memory_FreePointer(&text);
}

COMMAND_RESULT Console_Eval(const char *const cmdline)
{
    LOG_INFO("executing command: %s", cmdline);

    const CONSOLE_COMMAND *const matching_cmd = Console_Registry_Get(cmdline);
    if (matching_cmd == nullptr) {
        Console_Log(GS(OSD_UNKNOWN_COMMAND), cmdline);
        return CR_BAD_INVOCATION;
    }

    char *prefix = Memory_DupStr(cmdline);
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
        Console_Log(GS(OSD_COMMAND_BAD_INVOCATION), cmdline);
        break;

    case CR_UNAVAILABLE:
        Console_Log(GS(OSD_COMMAND_UNAVAILABLE));
        break;

    case CR_SUCCESS:
    case CR_FAILURE:
        // The commands themselves are responsible for handling logging in
        // these scenarios.
        break;
    }
    return result;
}

void Console_Control(void)
{
    UI2_Console_Control(&m_UIState);
}

void Console_Draw(void)
{
    UI2_Console(&m_UIState);
}
