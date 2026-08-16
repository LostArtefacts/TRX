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

static float M_Measure(const char *const text)
{
    float width = 0.0f;
    UI_Text_Measure(
        text, &width, nullptr, (UI_TEXT_SETTINGS) { .scale = 1.0f });
    return width;
}

// A key role stands for the keys the player bound it to, and a binding can name
// more than one: Alt+Enter spells out as two keycaps and the sign between them,
// so a placeholder has to measure as wide as all three.
TEST(ui_text_input_spells_out_a_combo)
{
    UI_InitText();
    M_SetUp();

    const char *const single = "\\{keyboard return}";
    const char *const combo =
        "\\{keyboard l_alt}\\{icon plus}\\{keyboard return}";
    const char *const placeholder = "\\{input toggle_fullscreen}";

    FakeUI_SetKeyName(single);
    CHECK(M_Measure(placeholder) == M_Measure(single));

    FakeUI_SetKeyName(combo);
    const float combo_width = M_Measure(combo);
    CHECK(M_Measure(placeholder) == combo_width);
    CHECK(combo_width > M_Measure(single));

    FakeUI_ResetKeyName();
    UI_ShutdownText();
}

TEST(ui_text_input_falls_back_to_a_question_mark)
{
    UI_InitText();
    M_SetUp();

    const char *const placeholder = "\\{input toggle_fullscreen}";
    const float unknown = M_Measure("?");

    FakeUI_SetKeyName(nullptr);
    CHECK(M_Measure(placeholder) == unknown);

    FakeUI_SetKeyName("");
    CHECK(M_Measure(placeholder) == unknown);

    FakeUI_ResetKeyName();
    UI_ShutdownText();
}
