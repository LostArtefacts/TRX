// The regions the interface lays out, the draw calls a scene schedules, and
// the font the text measures with, at 640x480 and with the shipped fonts.

#include <fakes/settings.h>
#include <fakes/ui.h>
#include <fakes/ui_draw.h>

#include <trx/game/ui/regions.h>
#include <harness/harness.h>

#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/game/ui.h>
#include <trx/game/ui/scaler.h>

static void M_SetUp(const int32_t tr_version, const float text_scale)
{
    FakeUI_SetGame(tr_version);
    FakeUI_SetViewport(640, 480);
    Config_RegisterBuiltInOptions();
    CONFIG_SET(g_Config.ui.text_scale, text_scale);
    UI_LoadText();
}

// Slots of one region stack down it in the order they were reserved, a gap
// apart, and stand against the side of the screen that the region does.
TEST(ui_slots_of_a_region_stack_in_order)
{
    Subsystem_InitAll();
    M_SetUp(1, 1.0f);

    UI_BeginScene();
    const int32_t first = UI_Region_Reserve(UI_REGION_BOTTOM_RIGHT, 40, 10);
    const int32_t second = UI_Region_Reserve(UI_REGION_BOTTOM_RIGHT, 20, 10);
    UI_Region_Layout();

    float x1 = 0.0f;
    float y1 = 0.0f;
    float w1 = 0.0f;
    float h1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
    float w2 = 0.0f;
    float h2 = 0.0f;
    CHECK(UI_Region_GetSlotBox(first, &x1, &y1, &w1, &h1));
    CHECK(UI_Region_GetSlotBox(second, &x2, &y2, &w2, &h2));
    CHECK_EQ_INT(w1, 40);
    CHECK_EQ_INT(w2, 20);
    CHECK(y2 > y1 + h1);
    CHECK_EQ_INT(x1 + w1, x2 + w2);
    CHECK(x1 + w1 < UI_GetCanvasWidth());

    Subsystem_ShutdownAll();
}

// A slot from an earlier scene is forgotten as the next one begins.
TEST(ui_a_new_scene_forgets_the_slots)
{
    Subsystem_InitAll();
    M_SetUp(1, 1.0f);

    UI_BeginScene();
    const int32_t slot = UI_Region_Reserve(UI_REGION_TOP_LEFT, 10, 10);
    UI_Region_Layout();
    UI_BeginScene();
    UI_Region_Layout();

    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    CHECK(!UI_Region_GetSlotBox(slot, &x, &y, &w, &h));

    Subsystem_ShutdownAll();
}

// Edge regions reduce the middle box available to dialogs.
TEST(ui_the_middle_region_gets_what_the_bands_leave)
{
    Subsystem_InitAll();
    M_SetUp(1, 1.0f);

    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    // Matching top and bottom slots leave a centered middle box.
    UI_BeginScene();
    UI_Region_Reserve(UI_REGION_TOP_CENTER, 60, 15);
    UI_Region_Reserve(UI_REGION_BOTTOM_CENTER, 80, 15);
    UI_Region_Layout();

    UI_Region_GetCenterBox(&x, &y, &w, &h);
    CHECK(y > 0);
    CHECK(h < UI_GetCanvasHeight());
    CHECK_EQ_INT(y + h, UI_GetCanvasHeight() - y);

    Subsystem_ShutdownAll();
}

// A corner takes room only from a box wide enough to reach it.
TEST(ui_a_corner_narrows_only_a_box_that_reaches_it)
{
    Subsystem_InitAll();
    M_SetUp(1, 1.0f);

    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    UI_BeginScene();
    UI_Region_Reserve(UI_REGION_TOP_LEFT, 50, 15);
    UI_Region_Layout();

    float full_y = 0.0f;
    UI_Region_GetCenterBox(&x, &full_y, &w, &h);
    CHECK(full_y > 0);

    // A narrow box in the middle does not reach the corner.
    UI_Region_GetCenterBoxFor(100.0f, &x, &y, &w, &h);
    CHECK(y < full_y);
    CHECK(UI_GetSafeCanvasTopFor(100.0f) < UI_GetSafeCanvasTop());

    // A box as wide as the screen does.
    UI_Region_GetCenterBoxFor(UI_GetCanvasWidth(), &x, &y, &w, &h);
    CHECK_EQ_INT(y, full_y);
    UI_Region_GetCenterBoxFor(-1.0f, &x, &y, &w, &h);
    CHECK_EQ_INT(y, full_y);

    Subsystem_ShutdownAll();
}

// Recorded draw calls describe the scene without rendering it.
TEST(ui_draw_describes_what_a_scene_scheduled)
{
    Subsystem_InitAll();
    M_SetUp(1, 1.0f);

    UI_BeginScene();
    UI_Region_Layout();
    char *empty = FakeUIDraw_Describe();
    CHECK_EQ_STR(empty, "");
    Memory_FreePointer(&empty);

    UI_BeginScene();
    UI_BeginLayer(UI_LAYER_INTERFACE);
    UI_ScheduleDrawScreenFlatQuad(
        10, 20, 3, 30, 40, (RGBA_8888) { 0x11, 0x22, 0x33, 0x44 });
    UI_ScheduleDrawScreenCircle(
        5, 6, 1, 2, 7, (RGBA_8888) { 0xAA, 0xBB, 0xCC, 0xDD });
    UI_EndLayer(UI_LAYER_INTERFACE);
    char *described = FakeUIDraw_Describe();
    CHECK_EQ_STR(
        described,
        "quad x=10 y=20 z=3 w=30 h=40 color=11223344\n"
        "circle cx=5 cy=6 r_inner=1 r_outer=2 z=7 color=aabbccdd\n");
    Memory_FreePointer(&described);
    UI_Region_Layout();

    // A new scene starts with no recorded draw calls.
    UI_BeginScene();
    char *fresh = FakeUIDraw_Describe();
    CHECK_EQ_STR(fresh, "");
    Memory_FreePointer(&fresh);
    UI_Region_Layout();

    Subsystem_ShutdownAll();
}

// Text measures the way it does in the game only with the font loaded, and a
// font that failed to load would measure everything as nothing. These widths
// are the sum of the glyph advances in each game's own font.
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
        M_SetUp(expected[i].tr_version, 1.0f);
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
