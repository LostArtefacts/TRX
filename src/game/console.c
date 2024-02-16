#include "game/console.h"

#include "config.h"
#include "game/clock.h"
#include "game/console_cmd.h"
#include "game/input.h"
#include "game/output.h"
#include "game/screen.h"
#include "game/text.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "log.h"

#include <SDL2/SDL_keycode.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LOG_LINES 10
#define MAX_PROMPT_LENGTH 100
#define HOVER_DELAY_CPS 5
#define MARGIN 5
#define PADDING 3

static bool m_IsOpened = false;
static bool m_AreAnyLogsOnScreen = false;

static struct {
    char text[MAX_PROMPT_LENGTH];
    uint32_t caret;
    TEXTSTRING *prompt_ts;
    TEXTSTRING *caret_ts;
} m_Prompt = { 0 };

static struct {
    int32_t expire_at;
    TEXTSTRING *ts;
} m_Logs[MAX_LOG_LINES] = { 0 };

static const double m_PromptScale = 1.0;
static const double m_LogScale = 0.8;
static const int32_t m_TextHeight = 15;
static const char m_ValidPromptChars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.- ";

static void Console_UpdatePromptTextstring(void);
static void Console_UpdateCaretTextstring(void);

static void Console_UpdatePromptTextstring(void)
{
    Text_ChangeText(m_Prompt.prompt_ts, m_Prompt.text);
}

static void Console_UpdateCaretTextstring(void)
{
    const char old = m_Prompt.prompt_ts->string[m_Prompt.caret];
    m_Prompt.prompt_ts->string[m_Prompt.caret] = '\0';
    const int32_t width = Text_GetWidth(m_Prompt.prompt_ts);
    m_Prompt.prompt_ts->string[m_Prompt.caret] = old;
    Text_SetPos(m_Prompt.caret_ts, MARGIN + width, -MARGIN);
}

void Console_Init(void)
{
    for (int i = 0; i < MAX_LOG_LINES; i++) {
        m_Logs[i].expire_at = -1;
        m_Logs[i].ts = Text_Create(MARGIN, -MARGIN, "");
        Text_SetScale(m_Logs[i].ts, PHD_ONE * m_LogScale, PHD_ONE * m_LogScale);
        Text_AlignBottom(m_Logs[i].ts, true);
        Text_SetMultiline(m_Logs[i].ts, true);
    }
}

void Console_Shutdown(void)
{
    for (int i = 0; i < MAX_LOG_LINES; i++) {
        Text_Remove(m_Logs[i].ts);
        m_Logs[i].ts = NULL;
    }
}

void Console_Open(void)
{
    m_IsOpened = true;

    if (m_Prompt.prompt_ts) {
        return;
    }

    m_Prompt.caret = strlen(m_Prompt.text);

    m_Prompt.prompt_ts = Text_Create(MARGIN, -MARGIN, m_Prompt.text);
    Text_SetScale(
        m_Prompt.prompt_ts, PHD_ONE * m_PromptScale, PHD_ONE * m_PromptScale);
    Text_AlignBottom(m_Prompt.prompt_ts, true);

    m_Prompt.caret_ts = Text_Create(MARGIN, -MARGIN, "\x80");
    Text_SetScale(
        m_Prompt.caret_ts, PHD_ONE * m_PromptScale, PHD_ONE * m_PromptScale);
    Text_AlignBottom(m_Prompt.caret_ts, true);
    Text_Flash(m_Prompt.caret_ts, 1, 20);
}

void Console_Close(void)
{
    m_IsOpened = false;
    strcpy(m_Prompt.text, "");
    Text_Remove(m_Prompt.prompt_ts);
    Text_Remove(m_Prompt.caret_ts);
    m_Prompt.prompt_ts = NULL;
    m_Prompt.caret_ts = NULL;
}

bool Console_IsOpened(void)
{
    return m_IsOpened;
}

void Console_Confirm(void)
{
    if (strcmp(m_Prompt.text, "") != 0) {
        LOG_INFO("executing command: %s", m_Prompt.text);

        const char *args = NULL;
        const CONSOLE_COMMAND *matching_cmd = NULL;

        for (CONSOLE_COMMAND *cur_cmd = &g_ConsoleCommands[0];
             cur_cmd->proc != NULL; cur_cmd++) {
            if (strstr(m_Prompt.text, cur_cmd->prefix) != m_Prompt.text) {
                continue;
            }

            if (m_Prompt.text[strlen(cur_cmd->prefix)] == ' ') {
                args = m_Prompt.text + strlen(cur_cmd->prefix) + 1;
            } else if (m_Prompt.text[strlen(cur_cmd->prefix)] == '\0') {
                args = "";
            } else {
                continue;
            }

            matching_cmd = cur_cmd;
            break;
        }

        if (matching_cmd == NULL) {
            Console_Log("Unknown command: %s", m_Prompt.text);
        } else {
            bool success = matching_cmd->proc(args);
            if (!success) {
                Console_Log("Failed to run: %s", m_Prompt.text);
            }
        }
    }

    Console_Close();
}

void Console_HandleKeyDown(const SDL_Event event)
{
    if (!g_Config.enable_console) {
        return;
    }

    if (!m_IsOpened) {
        const INPUT_SCANCODE open_console_keysym = Input_GetAssignedScancode(
            g_Config.input.layout, INPUT_ROLE_ENTER_CONSOLE);
        if (event.key.keysym.scancode == open_console_keysym) {
            Console_Open();
        }
        return;
    }

    switch (event.key.keysym.sym) {
    case SDLK_LEFT:
        if (m_Prompt.caret > 0) {
            m_Prompt.caret--;
            Console_UpdateCaretTextstring();
        }
        break;

    case SDLK_RIGHT:
        if (m_Prompt.caret < strlen(m_Prompt.text)) {
            m_Prompt.caret++;
            Console_UpdateCaretTextstring();
        }
        break;

    case SDLK_HOME:
        m_Prompt.caret = 0;
        Console_UpdateCaretTextstring();
        break;

    case SDLK_END:
        m_Prompt.caret = strlen(m_Prompt.text);
        Console_UpdateCaretTextstring();
        break;

    case SDLK_BACKSPACE:
        if (m_Prompt.caret > 0) {
            for (int i = m_Prompt.caret; i < MAX_PROMPT_LENGTH; i++) {
                m_Prompt.text[i - 1] = m_Prompt.text[i];
            }
            m_Prompt.caret--;
            Console_UpdatePromptTextstring();
            Console_UpdateCaretTextstring();
        }
        break;
    }
}

void Console_HandleTextEdit(const SDL_Event event)
{
    if (!m_IsOpened) {
        return;
    }
    strncpy(m_Prompt.text, event.text.text, MAX_PROMPT_LENGTH);
    m_Prompt.text[MAX_PROMPT_LENGTH - 1] = '\0';
    Console_UpdatePromptTextstring();
    Console_UpdateCaretTextstring();
}

void Console_HandleTextInput(const SDL_Event event)
{
    if (!m_IsOpened) {
        return;
    }
    if (strlen(event.text.text) != 1
        || !strstr(m_ValidPromptChars, event.text.text)) {
        return;
    }

    const char *insert_string = event.text.text;
    const size_t insert_length = strlen(insert_string);
    const size_t available_space =
        MAX_PROMPT_LENGTH - strlen(m_Prompt.text) - 1;

    if (insert_length > available_space) {
        return;
    }

    for (int i = strlen(m_Prompt.text); i >= (int)m_Prompt.caret; i--) {
        m_Prompt.text[i + insert_length] = m_Prompt.text[i];
    }

    memcpy(m_Prompt.text + m_Prompt.caret, insert_string, insert_length);

    m_Prompt.caret += insert_length;
    m_Prompt.text[MAX_PROMPT_LENGTH - 1] = '\0';
    Console_UpdatePromptTextstring();
    Console_UpdateCaretTextstring();
}

void Console_Log(const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    size_t text_length = vsnprintf(NULL, 0, fmt, va);
    char *text = malloc(text_length + 1);
    va_end(va);

    va_start(va, fmt);
    vsnprintf(text, text_length + 1, fmt, va);
    va_end(va);

    LOG_INFO("%s", text);
    int32_t dst_idx = -1;
    for (int i = MAX_LOG_LINES - 1; i > 0; i--) {
        Text_ChangeText(m_Logs[i].ts, m_Logs[i - 1].ts->string);
        m_Logs[i].expire_at = m_Logs[i - 1].expire_at;
    }

    m_Logs[0].expire_at = Clock_GetMS() + 1000 * strlen(text) / HOVER_DELAY_CPS;
    Text_ChangeText(m_Logs[0].ts, text);
    int32_t y = -MARGIN - m_TextHeight * m_PromptScale;

    for (int i = 0; i < MAX_LOG_LINES; i++) {
        const int32_t text_height = Text_GetHeight(m_Logs[i].ts);
        y -= text_height;
        y -= PADDING * m_LogScale / PHD_ONE;
        Text_SetPos(m_Logs[i].ts, m_Logs[i].ts->pos.x, y);
    }

    m_AreAnyLogsOnScreen = true;
    free(text);
}

void Console_ScrollLogs(void)
{
    int i = MAX_LOG_LINES - 1;
    while (i >= 0 && !m_Logs[i].expire_at) {
        i--;
    }

    while (i >= 0 && m_Logs[i].expire_at
           && Clock_GetMS() >= m_Logs[i].expire_at) {
        m_Logs[i].expire_at = 0;
        Text_ChangeText(m_Logs[i].ts, "");
        i--;
    }

    m_AreAnyLogsOnScreen = i >= 0;
}

void Console_Draw(void)
{
    Console_ScrollLogs();

    if (m_IsOpened || m_AreAnyLogsOnScreen) {
        int32_t sx = 0;
        int32_t sw = Viewport_GetWidth();
        int32_t sh = Screen_GetRenderScale(
            // not entirely accurate, but good enough
            TEXT_HEIGHT * m_PromptScale
                + MAX_LOG_LINES * TEXT_HEIGHT * m_LogScale,
            RSR_TEXT);
        int32_t sy = Viewport_GetHeight() - sh;

        RGBA_8888 top = { 0, 0, 0, 0 };
        RGBA_8888 bottom = { 0, 0, 0, 196 };

        Output_DrawScreenGradientQuad(sx, sy, sw, sh, top, top, bottom, bottom);
    }

    if (m_Prompt.prompt_ts) {
        Text_DrawText(m_Prompt.prompt_ts);
    }
    if (m_Prompt.caret_ts) {
        Text_DrawText(m_Prompt.caret_ts);
    }
    for (int i = 0; i < MAX_LOG_LINES; i++) {
        if (m_Logs[i].ts) {
            Text_DrawText(m_Logs[i].ts);
        }
    }
}
