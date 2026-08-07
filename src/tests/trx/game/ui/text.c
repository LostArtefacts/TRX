// Word wrapping, with the glyph widths the game measures by.
//
// A line the wrapper breaks in two still has to read as the one entry it was.
// The dialogs indent under a heading - the values a preset would change sit
// below the setting they belong to - and a continuation that starts at the
// margin reads as the next heading instead.

#include <fakes/ui.h>
#include <harness/harness.h>

#include <trx/config/vars.h>
#include <trx/core/memory.h>
#include <trx/game/ui/text.h>

#include <string.h>

// Narrow enough that every line below breaks, wide enough to hold a word.
#define M_WRAP_W 60.0f

static void M_SetUp(void)
{
    FakeUI_SetGame(1);
    FakeUI_SetViewport(640, 480);
    g_ConfigStorage.ui.text_scale = 1.0f;
    UI_LoadText();
}

static int32_t M_GetIndent(const char *const line)
{
    int32_t indent = 0;
    while (line[indent] == ' ') {
        indent++;
    }
    return indent;
}

static void M_CheckWrap(const char *const text, const int32_t expected_indent)
{
    char *const wrapped = UI_Text_WordWrap(text, 1.0f, M_WRAP_W);
    if (wrapped == nullptr) {
        TEST_FAIL("\"%s\": nothing came back", text);
        return;
    }
    if (strchr(wrapped, '\n') == nullptr) {
        TEST_FAIL(
            "\"%s\": did not wrap, so it says nothing about indent", text);
        Memory_Free(wrapped);
        return;
    }

    const char *line = wrapped;
    bool first = true;
    while (line != nullptr) {
        const char *const end = strchr(line, '\n');
        if (!first) {
            const int32_t indent = M_GetIndent(line);
            if (indent != expected_indent) {
                TEST_FAIL(
                    "\"%s\": continuation opens at %d spaces, expected %d",
                    text, indent, expected_indent);
                break;
            }
        }
        first = false;
        line = end != nullptr ? end + 1 : nullptr;
    }
    Memory_Free(wrapped);
}

TEST(ui_text_wrap_keeps_indent)
{
    UI_InitText();
    M_SetUp();
    M_CheckWrap("Bars appearance and the rest of it", 0);
    M_CheckWrap("  Saving pickups becomes healing", 2);
    M_CheckWrap("- one bullet holding several words", 2);
    M_CheckWrap("  - an indented bullet holding several words", 4);
    UI_ShutdownText();
}
