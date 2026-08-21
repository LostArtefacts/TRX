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
#include <trx/core/event_manager.h>
#include <trx/core/subsystem.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/ui.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/input.h>
#include <trx/game/ui/dialogs/config_presets.h>
#include <trx/game/ui/dialogs/controls_editor.h>
#include <trx/game/ui/dialogs/gameplay_settings.h>
#include <trx/game/ui/dialogs/graphic_settings.h>
#include <trx/game/ui/dialogs/settings.h>
#include <trx/game/ui/dialogs/settings.h>
#include <trx/game/ui/dialogs/sound_settings.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define M_SLACK 5.0f

// A few widgets - the bar preview and the colour swatch among them - measure a
// couple of units wider than the box they are given, and have done so for as
// long as they have existed. The tolerance is set above that and far below the
// hundreds of units a dialog spills when it sizes itself wrongly.
#define M_BOX_SLACK 10.0f
#define M_TARGET_SCALE 1.2f
// What the overlay keeps clear at the bottom of the screen for one line of
// text, in text units.
#define M_EDGE_INSET 25.0f
#define M_TEXT_BUF_SIZE 256
#define M_KEY_BUF_SIZE 256

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
    const UI_NODE *screen_node;
    const char *screen_axis;
    const UI_NODE *box_node;
    float fit_scale;
    float reached_scale;
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

static const float m_TextScales[] = { 1.0f, 2.0f };

static void M_AppendText(
    const char *const text, char *const buf, const size_t buf_size)
{
    char clean[M_TEXT_BUF_SIZE] = "";
    size_t len = 0;
    for (const char *c = text; *c != '\0' && len + 1 < sizeof(clean); c++) {
        if (c[0] == '\\' && c[1] == '{') {
            while (*c != '\0' && *c != '}') {
                c++;
            }
            continue;
        }
        clean[len++] = *c;
    }
    clean[len] = '\0';
    for (char *c = clean; *c != '\0'; c++) {
        if (*c != ' ') {
            if (buf[0] != '\0') {
                strncat(buf, " / ", buf_size - strlen(buf) - 1);
            }
            strncat(buf, c, buf_size - strlen(buf) - 1);
            return;
        }
    }
}

static void M_DescribeNode(
    const UI_NODE *const node, char *const buf, const size_t buf_size)
{
    if (node == nullptr || strlen(buf) > buf_size / 2) {
        return;
    }
    const char *const text = UI_Label_GetText(node);
    if (text != nullptr && text[0] != '\0') {
        M_AppendText(text, buf, buf_size);
    }
    for (const UI_NODE *child = node->first_child; child != nullptr;
         child = child->next_sibling) {
        M_DescribeNode(child, buf, buf_size);
    }
}

static const char *M_Describe(const UI_NODE *const node)
{
    static char buf[M_TEXT_BUF_SIZE];
    buf[0] = '\0';
    M_DescribeNode(node, buf, sizeof(buf));
    return buf;
}

static const char *M_NodeName(const UI_NODE *node)
{
    for (; node != nullptr; node = node->parent) {
        if (node->name != nullptr) {
            return node->name;
        }
    }
    return nullptr;
}

static const UI_NODE *M_FindEdgeLabel(
    const UI_NODE *const node, const bool tall)
{
    const UI_NODE *result = UI_Label_GetText(node) != nullptr ? node : nullptr;
    for (const UI_NODE *child = node->first_child; child != nullptr;
         child = child->next_sibling) {
        const UI_NODE *const found = M_FindEdgeLabel(child, tall);
        if (found == nullptr) {
            continue;
        }
        if (result == nullptr
            || (tall ? found->y + found->measure_h
                        > result->y + result->measure_h
                     : found->x + found->measure_w
                        > result->x + result->measure_w)) {
            result = found;
        }
    }
    return result;
}

static const char *M_DescribeOverflow(
    const UI_NODE *const node, const char *const axis)
{
    static char buf[M_TEXT_BUF_SIZE * 2];
    buf[0] = '\0';
    const UI_NODE *const label =
        M_FindEdgeLabel(node, strcmp(axis, "tall") == 0);
    if (label == nullptr) {
        return buf;
    }
    char text[M_TEXT_BUF_SIZE] = "";
    M_AppendText(UI_Label_GetText(label), text, sizeof(text));
    const char *const part = M_NodeName(label);
    snprintf(buf, sizeof(buf), "%s: %s", part != nullptr ? part : "?", text);
    return buf;
}

static const char *M_DescribeWidest(void)
{
    static char buf[M_TEXT_BUF_SIZE * 2];
    buf[0] = '\0';
    const UI_MEASURE_NOTE widest = UI_Measure_GetWidest();
    if (widest.text == nullptr || widest.text[0] == '\0') {
        return buf;
    }
    char text[M_TEXT_BUF_SIZE] = "";
    M_AppendText(widest.text, text, sizeof(text));
    snprintf(
        buf, sizeof(buf), "widest %.0f in %s: %s", widest.width,
        widest.part != nullptr ? widest.part : "?", text);
    return buf;
}

static void M_DefineStrings(JSON_OBJECT *const obj, const char *const prefix)
{
    for (const JSON_OBJECT_ELEMENT *elem = obj->start; elem != nullptr;
         elem = elem->next) {
        char key[M_KEY_BUF_SIZE];
        if (prefix == nullptr) {
            snprintf(key, sizeof(key), "%s", elem->name->string);
        } else {
            snprintf(key, sizeof(key), "%s/%s", prefix, elem->name->string);
        }
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
    if (!IGNORE(FS_Load(path, &data, &size))) {
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

static void M_CheckNode(
    const UI_NODE *const node, const float canvas_w, const float canvas_h,
    M_OVERFLOW *const result)
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
    const char *const node_text = UI_Label_GetText(node);

    const float outside_box = node->measure_w - node->w;
    if (node_text != nullptr && outside_box > result->box) {
        result->box = outside_box;
        result->box_node = node;
    }

    // The scene root and a modal span the screen by definition and draw nothing
    // of their own. The margin is about the dialog sitting inside them.
    const bool is_full_screen = node->w >= canvas_w - M_SLACK;
    if (!is_full_screen) {
        const float node_right = node->x + MAX(node->w, node->measure_w);
        const float over =
            MAX(UI_SCREEN_MARGIN - node->x,
                node_right - (canvas_w - UI_SCREEN_MARGIN));
        if (over > result->screen) {
            result->screen = over;
            result->screen_node = node;
            result->screen_axis = "wide";
        }
    }

    const bool is_full_height = node->h >= canvas_h - M_SLACK;
    if (!is_full_height) {
        const float node_bottom = node->y + MAX(node->h, node->measure_h);
        const float over =
            MAX(UI_GetSafeCanvasTop() - node->y,
                node_bottom - UI_GetSafeCanvasBottom());
        if (over > result->screen) {
            result->screen = over;
            result->screen_node = node;
            result->screen_axis = "tall";
        }
    }

    for (const UI_NODE *child = node->first_child; child != nullptr;
         child = child->next_sibling) {
        M_CheckNode(child, canvas_w, canvas_h, result);
    }
}

static float M_GetReachedScale(void)
{
    return UI_GetSmallestFitScale() * UI_Scaler_GetTextScale();
}

static float M_GetScaleTarget(void)
{
    return MIN(UI_Scaler_GetTextScale(), M_TARGET_SCALE);
}

static float M_GetSlack(const float slack)
{
    return slack * UI_Scaler_GetTextScale();
}

static M_OVERFLOW M_MeasureScene(void)
{
    M_OVERFLOW result = {};
    M_CheckNode(
        UI_GetSceneRoot(), UI_GetCanvasWidth(), UI_GetCanvasHeight(), &result);
    return result;
}

static M_OVERFLOW M_MeasureDialogOverflow(const M_DIALOG *const dialog)
{
    UI_ForgetSmallestFitScale();
    UI_Measure_Forget();
    UI_SETTINGS_DIALOG_STATE *const state = dialog->init();
    UI_BeginScene();
    dialog->draw(state);
    UI_EndScene();
    M_OVERFLOW result = M_MeasureScene();
    result.fit_scale = UI_GetSmallestFitScale();
    result.reached_scale = M_GetReachedScale();
    dialog->free(state);
    return result;
}

static void M_SetLanguage(const char *const lang)
{
    GameString_Reset();
    M_LoadLanguage(lang);
}

static void M_SetUp(
    const int32_t tr_version, const char *const lang, const float text_scale)
{
    FakeUI_SetGame(tr_version);
    FakeUI_SetViewport(640, 480);
    // The dialogs walk the options this game names, so they have to exist
    // before a row can be measured.
    Config_RegisterBuiltInOptions();
    CONFIG_SET(g_Config.ui.text_scale, text_scale);
    CONFIG_SET(g_Config.ui.enable_wraparound, false);
    FakeUI_SetKeyName("R");
    UI_LoadText();
}

static bool M_IsNear(const float actual, const float expected)
{
    return fabsf(actual - expected) < 0.001f;
}

TEST(ui_screen_insets_cover_the_scene_being_built)
{
    Subsystem_InitAll();
    M_SetLanguage(nullptr);
    M_SetUp(1, nullptr, 1.0f);

    UI_BeginScene();
    UI_SetScreenInset(UI_SCREEN_INSET_OVERLAY, 20.0f, 10.0f);
    CHECK_EQ_INT(UI_GetScreenInsetTop(), 20);
    CHECK_EQ_INT(UI_GetScreenInsetBottom(), 10);
    UI_EndScene();

    UI_BeginScene();
    UI_EndScene();
    CHECK_EQ_INT(UI_GetScreenInsetTop(), 20);
    CHECK_EQ_INT(UI_GetScreenInsetBottom(), 10);
    CHECK_EQ_INT(UI_GetSafeCanvasTop(), 20);
    CHECK_EQ_INT(UI_GetSafeCanvasBottom(), 470);
    CHECK_EQ_INT(UI_GetSafeCanvasHeight(), 450);

    UI_BeginScene();
    UI_EndScene();
    CHECK_EQ_INT(UI_GetScreenInsetTop(), 0);
    CHECK_EQ_INT(UI_GetSafeCanvasTop(), UI_SCREEN_MARGIN);

    Subsystem_ShutdownAll();
}

TEST(ui_fit_scale_gives_way_only_where_it_must)
{
    Subsystem_InitAll();
    M_SetLanguage(nullptr);
    M_SetUp(1, nullptr, 1.0f);
    UI_ForgetSmallestFitScale();

    const float safe_width = UI_GetSafeCanvasWidth();
    CHECK(UI_GetFitScale(safe_width - 1.0f, -1.0f) == 1.0f);
    CHECK(UI_GetFitScale(-1.0f, -1.0f) == 1.0f);
    CHECK(M_IsNear(UI_GetFitScale(safe_width / 0.9f, -1.0f), 0.9f));
    CHECK(M_IsNear(UI_GetFitScale(safe_width * 100.0f, -1.0f), 0.01f));

    CHECK(
        M_IsNear(UI_GetFitScale(-1.0f, UI_GetSafeCanvasHeight() / 0.9f), 0.9f));

    M_SetUp(1, nullptr, 2.0f);
    CHECK(M_IsNear(UI_GetFitScale(safe_width / 1.8f, -1.0f), 0.9f));

    Subsystem_ShutdownAll();
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

    Subsystem_InitAll();
    for (size_t i = 0; i < ARRAY_SIZE(expected); i++) {
        M_SetLanguage(nullptr);
        M_SetUp(expected[i].tr_version, nullptr, 1.0f);
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
    Subsystem_ShutdownAll();
}

// The list of settings a preset would change is the longest text in any of
// these dialogs: a setting title and its before/after values, for every key the
// preset touches. It wraps rather than sizing the dialog to the widest of them.
TEST(ui_config_presets_confirm_fits_4_3)
{
    Subsystem_InitAll();
    Config_Presets_ScanFiles();

    for (size_t i = 0; i < ARRAY_SIZE(m_Languages); i++) {
        M_SetLanguage(m_Languages[i]);
        for (int32_t tr_version = 1; tr_version <= TR_VERSION_COUNT;
             tr_version++) {
            M_SetUp(tr_version, m_Languages[i], 1.0f);

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
                if (over.screen > M_GetSlack(M_SLACK)) {
                    TEST_FAIL(
                        "%s tr%d preset %d: %.0f units off screen; %s", lang,
                        tr_version, preset, over.screen, M_DescribeWidest());
                }
                if (over.box > M_GetSlack(M_BOX_SLACK)) {
                    TEST_FAIL(
                        "%s tr%d preset %d: %.0f units outside its box: %s",
                        lang, tr_version, preset, over.box,
                        M_Describe(over.box_node));
                }
                UI_ConfigPresets_Free(state);
            }
        }
    }
    Subsystem_ShutdownAll();
}

TEST(ui_controls_editor_fits_4_3)
{
    Subsystem_InitAll();
    for (size_t i = 0; i < ARRAY_SIZE(m_Languages); i++) {
        M_SetLanguage(m_Languages[i]);
        for (int32_t tr_version = 1; tr_version <= TR_VERSION_COUNT;
             tr_version++) {
            for (size_t k = 0; k < ARRAY_SIZE(m_TextScales); k++) {
                M_SetUp(tr_version, m_Languages[i], m_TextScales[k]);

                UI_CONTROLS_EDITOR_STATE state = {};
                EVENT_MANAGER *const events = EventManager_Create();
                UI_ForgetSmallestFitScale();
                UI_Measure_Forget();
                UI_ControlsEditor_Init(
                    &state, INPUT_BACKEND_KEYBOARD, INPUT_LAYOUT_CUSTOM_1,
                    events);

                UI_BeginScene();
                UI_ControlsEditor(&state);
                UI_EndScene();
                M_OVERFLOW over = M_MeasureScene();
                over.fit_scale = UI_GetSmallestFitScale();
                over.reached_scale = M_GetReachedScale();

                const char *const lang =
                    m_Languages[i] != nullptr ? m_Languages[i] : "en";
                if (over.reached_scale < M_GetScaleTarget()) {
                    TEST_FAIL(
                        "%s tr%d controls at %.0f%%: text reaches %.1f%%, "
                        "short of %.0f%%; %s",
                        lang, tr_version, m_TextScales[k] * 100.0f,
                        over.reached_scale * 100.0f,
                        M_GetScaleTarget() * 100.0f, M_DescribeWidest());
                } else if (over.screen > M_GetSlack(M_SLACK)) {
                    TEST_FAIL(
                        "%s tr%d controls at %.0f%%: %.0f units too %s in %s: "
                        "%s",
                        lang, tr_version, m_TextScales[k] * 100.0f, over.screen,
                        over.screen_axis,
                        M_DescribeOverflow(over.screen_node, over.screen_axis),
                        M_DescribeWidest());
                }
                if (over.box > M_GetSlack(M_BOX_SLACK)) {
                    TEST_FAIL(
                        "%s tr%d controls at %.0f%%: %.0f units outside its "
                        "box: %s",
                        lang, tr_version, m_TextScales[k] * 100.0f, over.box,
                        M_Describe(over.box_node));
                }

                UI_ControlsEditor_Free(&state);
                EventManager_Free(events);
            }
        }
    }
    Subsystem_ShutdownAll();
}

TEST(ui_settings_dialogs_fit_4_3)
{
    Subsystem_InitAll();
    for (size_t i = 0; i < ARRAY_SIZE(m_Languages); i++) {
        M_SetLanguage(m_Languages[i]);
        for (int32_t tr_version = 1; tr_version <= TR_VERSION_COUNT;
             tr_version++) {
            for (size_t k = 0; k < ARRAY_SIZE(m_TextScales); k++) {
                M_SetUp(tr_version, m_Languages[i], m_TextScales[k]);
                for (size_t j = 0; j < ARRAY_SIZE(m_Dialogs); j++) {
                    const M_OVERFLOW over =
                        M_MeasureDialogOverflow(&m_Dialogs[j]);
                    const char *const lang =
                        m_Languages[i] != nullptr ? m_Languages[i] : "en";
                    if (over.reached_scale < M_GetScaleTarget()) {
                        TEST_FAIL(
                            "%s tr%d %s at %.0f%%: text reaches %.1f%%, short "
                            "of %.0f%%; %s",
                            lang, tr_version, m_Dialogs[j].name,
                            m_TextScales[k] * 100.0f,
                            over.reached_scale * 100.0f,
                            M_GetScaleTarget() * 100.0f, M_DescribeWidest());
                    } else if (over.screen > M_GetSlack(M_SLACK)) {
                        TEST_FAIL(
                            "%s tr%d %s at %.0f%%: %.0f units too %s in %s; "
                            "%s",
                            lang, tr_version, m_Dialogs[j].name,
                            m_TextScales[k] * 100.0f, over.screen,
                            over.screen_axis,
                            M_DescribeOverflow(
                                over.screen_node, over.screen_axis),
                            M_DescribeWidest());
                    } else if (over.box > M_GetSlack(M_BOX_SLACK)) {
                        TEST_FAIL(
                            "%s tr%d %s at %.0f%%: %.0f units outside its box "
                            "in %s; %s",
                            lang, tr_version, m_Dialogs[j].name,
                            m_TextScales[k] * 100.0f, over.box,
                            M_NodeName(over.box_node) != nullptr
                                ? M_NodeName(over.box_node)
                                : "?",
                            M_Describe(over.box_node));
                    }
                }
            }
        }
    }
    Subsystem_ShutdownAll();
}

// The inventory ring names the item under the dialog, and the overlay draws
// that text at the bottom of the screen. A dialog that reaches past what those
// keep clear lands on top of them.
TEST(ui_settings_dialogs_clear_the_screen_edges)
{
    Subsystem_InitAll();
    M_SetLanguage(nullptr);
    for (int32_t tr_version = 1; tr_version <= TR_VERSION_COUNT; tr_version++) {
        for (size_t k = 0; k < ARRAY_SIZE(m_TextScales); k++) {
            M_SetUp(tr_version, nullptr, m_TextScales[k]);
            for (size_t j = 0; j < ARRAY_SIZE(m_Dialogs); j++) {
                UI_ForgetSmallestFitScale();
                UI_Measure_Forget();
                UI_SETTINGS_DIALOG_STATE *const state = m_Dialogs[j].init();

                // An inset reaches the dialog in the scene after the one that
                // states it, and the dialog sizes itself in its control pass,
                // so it takes a few frames to settle the way it does in game.
                for (int32_t frame = 0; frame < 3; frame++) {
                    g_InputDB = (INPUT_STATE) { .menu_down = frame == 0 };
                    UI_SettingsDialog_Control(state);
                    g_InputDB = (INPUT_STATE) {};
                    UI_BeginScene();
                    UI_SetScreenInset(
                        UI_SCREEN_INSET_OVERLAY, M_EDGE_INSET, M_EDGE_INSET);
                    m_Dialogs[j].draw(state);
                    UI_EndScene();
                }

                const M_OVERFLOW over = M_MeasureScene();
                if (over.screen > M_GetSlack(M_SLACK)) {
                    TEST_FAIL(
                        "tr%d %s at %.0f%%: %.0f units too %s in %s",
                        tr_version, m_Dialogs[j].name, m_TextScales[k] * 100.0f,
                        over.screen, over.screen_axis,
                        M_DescribeOverflow(over.screen_node, over.screen_axis));
                }
                m_Dialogs[j].free(state);
            }
        }
    }
    Subsystem_ShutdownAll();
}
