// The settings dialogs size themselves from the text they hold, so a long
// translation can push one past the edge of the screen. 4:3 is where that
// bites: the canvas is 640 units wide there against 853 at 16:9, so a dialog
// that fits a widescreen window has 213 units less to work with.
//
// These tests lay the dialogs out for real - the same measure and layout passes
// the game runs - at 640x480, for every game and every shipped language, and
// check that nothing ends up outside the canvas.

#include <fakes/settings.h>
#include <fakes/ui.h>
#include <harness/harness.h>

#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/config/presets.h>
#include <trx/core/json.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/ui.h>
#include <trx/game/input.h>
#include <trx/game/ui/dialogs/config_presets.h>
#include <trx/game/ui/dialogs/gameplay_settings.h>
#include <trx/game/ui/dialogs/graphic_settings.h>
#include <trx/game/ui/dialogs/sound_settings.h>

#include <stdio.h>
#include <string.h>

// Rounding in the layout arithmetic can leave a node a hair over the edge
// without anything being visibly clipped.
#define M_SLACK 1.0f

// A few widgets - the bar preview and the colour swatch among them - measure a
// couple of units wider than the box they are given, and have done so for as
// long as they have existed. The tolerance is set above that and far below the
// hundreds of units a dialog spills when it sizes itself wrongly.
#define M_BOX_SLACK 8.0f

typedef struct {
    const char *name;
    UI_SETTINGS_DIALOG_STATE *(*init)(void);
    void (*draw)(UI_SETTINGS_DIALOG_STATE *);
    void (*free)(UI_SETTINGS_DIALOG_STATE *);
} M_DIALOG;

// How far the worst node reaches outside the screen, and outside its own box.
typedef struct {
    float screen;
    float box;
} M_OVERFLOW;

static const M_DIALOG m_Dialogs[] = {
    { "gameplay", UI_GameplaySettings_Init, UI_GameplaySettings,
      UI_GameplaySettings_Free },
    { "graphics", UI_GraphicSettings_Init, UI_GraphicSettings,
      UI_GraphicSettings_Free },
    { "sound", UI_SoundSettings_Init, UI_SoundSettings, UI_SoundSettings_Free },
};

// The languages the game ships, base_strings.json5 being the English one.
static const char *m_Languages[] = {
    nullptr, "de", "en-gb", "fr", "gd", "it", "pl", "ru",
};

static void M_DefineStrings(JSON_OBJECT *const obj, const char *const prefix)
{
    for (const JSON_OBJECT_ELEMENT *elem = obj->start; elem != nullptr;
         elem = elem->next) {
        const char *const key = prefix == nullptr
            ? elem->name->string
            : String_FormatStatic("%s/%s", prefix, elem->name->string);
        JSON_OBJECT *const child = JSON_ValueAsObject(elem->value);
        if (child != nullptr) {
            M_DefineStrings(child, key);
        } else {
            const char *const value = JSON_ValueGetString(elem->value, nullptr);
            if (value != nullptr) {
                GameString_Define(key, value);
            }
        }
    }
}

// Overlay a shipped string layer on top of the built-in English defaults, the
// way the game does when a player picks a language.
static bool M_LoadLanguage(const char *const lang)
{
    const char *const path = lang == nullptr
        ? TEST_SHIP_CFG_DIR "/base_strings.json5"
        : String_FormatStatic(
              "%s/base_strings-%s.json5", TEST_SHIP_CFG_DIR, lang);

    char *data = nullptr;
    size_t size = 0;
    if (!File_Load(path, &data, &size)) {
        return false;
    }
    JSON_VALUE *const root = JSON_ParseEx(
        data, size, JSON_PARSE_FLAGS_ALLOW_JSON5, nullptr, nullptr, nullptr);
    Memory_Free(data);
    if (root == nullptr) {
        return false;
    }
    JSON_OBJECT *const obj = JSON_ValueAsObject(root);
    if (obj != nullptr) {
        M_DefineStrings(obj, nullptr);
    }
    JSON_ValueFree(root);
    return true;
}

// Nothing in the UI clips, so a node whose ink reaches past its edge is text
// the player cannot read. The edge is the safe width, not the canvas: dialogs
// are meant to stay clear of the screen by UI_SCREEN_MARGIN.
static void M_CheckNode(
    const UI_NODE *const node, const float canvas_w, float *const worst,
    float *const worst_box)
{
    if (node == nullptr) {
        return;
    }

    // A node with no box was never laid out - a row scrolled out of its list,
    // say - and the draw pass skips it along with everything under it.
    if (node->w <= 0.0f || node->h <= 0.0f) {
        return;
    }

    // Nothing in this UI clips, so text wider than the box it was given spills
    // over what is beside it - the frame it sits in, or the row below.
    const float outside_box = node->measure_w - node->w;
    if (outside_box > *worst_box) {
        *worst_box = outside_box;
    }

    // The scene root and a modal span the screen by definition and draw nothing
    // of their own. The margin is about the dialog sitting inside them.
    const bool is_full_screen = node->w >= canvas_w - M_SLACK;
    if (!is_full_screen) {
        const float node_right = node->x + MAX(node->w, node->measure_w);
        const float over =
            MAX(UI_SCREEN_MARGIN - node->x,
                node_right - (canvas_w - UI_SCREEN_MARGIN));
        if (over > *worst) {
            *worst = over;
        }
    }

    for (const UI_NODE *child = node->first_child; child != nullptr;
         child = child->next_sibling) {
        M_CheckNode(child, canvas_w, worst, worst_box);
    }
}

static M_OVERFLOW M_MeasureScene(void)
{
    M_OVERFLOW result = {};
    M_CheckNode(
        UI_GetSceneRoot(), UI_GetCanvasWidth(), &result.screen, &result.box);
    return result;
}

static M_OVERFLOW M_MeasureDialogOverflow(const M_DIALOG *const dialog)
{
    UI_SETTINGS_DIALOG_STATE *const state = dialog->init();
    UI_BeginScene();
    dialog->draw(state);
    UI_EndScene();
    const M_OVERFLOW result = M_MeasureScene();
    dialog->free(state);
    return result;
}

static void M_SetUp(const int32_t tr_version, const char *const lang)
{
    GameString_Clear();
    GameString_Init();
    M_LoadLanguage(lang);
    FakeUI_SetGame(tr_version);
    FakeUI_SetViewport(640, 480);
    // The dialogs walk the options this game names, so they have to exist
    // before a row can be measured.
    Config_RegisterBuiltInOptions();
    CONFIG_SET(g_Config.ui.text_scale, 1.0f);
    UI_LoadText();
}

// Every check here rests on text measuring the way it does in the game, and a
// font that failed to load would measure everything as nothing and quietly
// pass. These widths are the sum of the glyph advances in each game's own font.
TEST(ui_font_metrics)
{
    static const struct {
        int32_t tr_version;
        float width;
    } expected[] = {
        { 1, 259.0f },
        { 2, 259.0f },
        { 3, 282.0f },
        { 4, 275.0f },
    };

    UI_InitText();
    for (size_t i = 0; i < ARRAY_SIZE(expected); i++) {
        M_SetUp(expected[i].tr_version, nullptr);
        float width = 0.0f;
        UI_Text_Measure(
            "Remember guns between levels", &width, nullptr,
            (UI_TEXT_SETTINGS) { .scale = 1.0f });
        if (width != expected[i].width) {
            TEST_FAIL(
                "tr%d: expected width %.1f, got %.1f", expected[i].tr_version,
                expected[i].width, width);
        }
    }
    UI_ShutdownText();
}

// The list of settings a preset would change is the longest text in any of
// these dialogs: a setting title and its before/after values, for every key the
// preset touches. It wraps rather than sizing the dialog to the widest of them.
TEST(ui_config_presets_confirm_fits_4_3)
{
    UI_InitText();
    Config_Presets_ScanFiles();

    for (int32_t tr_version = 1; tr_version <= TR_VERSION_COUNT; tr_version++) {
        for (size_t i = 0; i < ARRAY_SIZE(m_Languages); i++) {
            M_SetUp(tr_version, m_Languages[i]);

            const int32_t preset_count = Config_Presets_GetCount();
            if (preset_count == 0) {
                TEST_FAIL("no presets found to confirm");
                break;
            }

            for (int32_t preset = 0; preset < preset_count; preset++) {
                UI_CONFIG_PRESETS_STATE *const state = UI_ConfigPresets_Init();
                // Walk the list to the preset, then accept it, the way a player
                // reaches the confirmation.
                for (int32_t row = 0; row <= preset; row++) {
                    g_InputDB = (INPUT_STATE) { .menu_down = true };
                    UI_ConfigPresets_Control(state);
                }
                g_InputDB = (INPUT_STATE) { .menu_confirm = true };
                UI_ConfigPresets_Control(state);
                g_InputDB = (INPUT_STATE) {};

                UI_BeginScene();
                UI_ConfigPresetsApplyModal(state);
                UI_EndScene();

                const M_OVERFLOW over = M_MeasureScene();
                const char *const lang =
                    m_Languages[i] != nullptr ? m_Languages[i] : "en";
                if (over.screen > M_SLACK) {
                    TEST_FAIL(
                        "tr%d %s preset %d: %.0f units off screen", tr_version,
                        lang, preset, over.screen);
                }
                if (over.box > M_BOX_SLACK) {
                    TEST_FAIL(
                        "tr%d %s preset %d: %.0f units outside its box",
                        tr_version, lang, preset, over.box);
                }
                UI_ConfigPresets_Free(state);
            }
        }
    }
    UI_ShutdownText();
}

TEST(ui_settings_dialogs_fit_4_3)
{
    UI_InitText();
    for (int32_t tr_version = 1; tr_version <= TR_VERSION_COUNT; tr_version++) {
        for (size_t i = 0; i < ARRAY_SIZE(m_Languages); i++) {
            M_SetUp(tr_version, m_Languages[i]);
            for (size_t j = 0; j < ARRAY_SIZE(m_Dialogs); j++) {
                const M_OVERFLOW over = M_MeasureDialogOverflow(&m_Dialogs[j]);
                const char *const lang =
                    m_Languages[i] != nullptr ? m_Languages[i] : "en";
                if (over.screen > M_SLACK) {
                    TEST_FAIL(
                        "tr%d %s %s: %.0f units off screen", tr_version, lang,
                        m_Dialogs[j].name, over.screen);
                }
                if (over.box > M_BOX_SLACK) {
                    TEST_FAIL(
                        "tr%d %s %s: %.0f units outside its box", tr_version,
                        lang, m_Dialogs[j].name, over.box);
                }
            }
        }
    }
    UI_ShutdownText();
}
