#include <trx/game/phase/phase_photo_mode.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/game/cutscene.h>
#include <trx/game/game.h>
#include <trx/game/input.h>
#include <trx/game/lua/events/ui.h>
#include <trx/game/phase/executor.h>
#include <trx/game/photo_mode.h>
#include <trx/game/screenshot.h>
#include <trx/game/shell.h>
#include <trx/game/sound.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/viewport.h>

typedef struct {
    bool in_cutscene;
    bool taking_screenshot;
} M_PRIV;

static PHASE_CONTROL M_Start(PHASE *phase)
{
    M_PRIV *const p = phase->priv;
    p->in_cutscene = GF_GetCurrentLevel()->type == GFL_CUTSCENE;
    PhotoMode_Start();
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    PhotoMode_End();
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();

    // XXX: normally we'd be using menu_back alone to let the player go back
    // and exit the photo mode UI, BUT for controller players, the default
    // menu_back button conflicts with the roll input, as both are bound to the
    // B button. This causes neither to work as expected, when the player
    // presses B. This is a hacky solution since technically the player might
    // remap the roll input to some other button, making the roll check below
    // redundant, but this is the most straightforward approach.
    if (g_InputDB.toggle_photo_mode
        || (g_InputDB.menu_back && !g_InputDB.roll)) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    } else if (g_InputDB.action) {
        p->taking_screenshot = true;
        Screenshot_Make(g_Config.rendering.screenshot_format);
        Sound_Effect(SFX_MENU_LARA_HOME, nullptr, SPM_ALWAYS);
    }
    return PhotoMode_Control();
}

// Frames the screen in red, so the player sees photo mode is on.
static void M_DrawFrame(const int32_t t)
{
    if (t <= 0) {
        return;
    }
    const VIEWPORT_RECT rect = Viewport_GetRect(VIEWPORT_UI);
    const RGBA_8888 color = { 255, 0, 0, 96 };
    const int32_t x0 = rect.x;
    const int32_t y0 = rect.y;
    const int32_t x1 = rect.x + rect.w;
    const int32_t y1 = rect.y + rect.h;
    UI_BeginLayer(UI_LAYER_INTERFACE);
    UI_ScheduleDrawScreenFlatQuad(x0, y0, 0, rect.w, t, color);
    UI_ScheduleDrawScreenFlatQuad(x0, y1 - t, 0, rect.w, t, color);
    UI_ScheduleDrawScreenFlatQuad(x0, y0 + t, 0, t, rect.h - 2 * t, color);
    UI_ScheduleDrawScreenFlatQuad(x1 - t, y0 + t, 0, t, rect.h - 2 * t, color);
    UI_EndLayer(UI_LAYER_INTERFACE);
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->in_cutscene) {
        Cutscene_Draw();
    } else {
        Game_Draw(false);
    }

    // A screenshot is captured from the frame after the one the player asks on,
    // so the picture holds the level alone: neither the red border nor the
    // script interface is drawn into it.
    if (p->taking_screenshot) {
        p->taking_screenshot = false;
        LUA_UI_HideNextFrame();
    } else {
        M_DrawFrame(
            (int32_t)(UI_Scaler_Calc(4.0f, UI_SCALER_TARGET_TEXT) + 0.5f));
    }
}

PHASE *Phase_PhotoMode_Create(void)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    phase->priv = Memory_Alloc(sizeof(M_PRIV));
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    return phase;
}

void Phase_PhotoMode_Destroy(PHASE *phase)
{
    Memory_Free(phase->priv);
    Memory_Free(phase);
}
