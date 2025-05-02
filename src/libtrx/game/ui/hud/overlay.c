#include "game/ui/hud/overlay.h"

#include "config.h"
#include "game/clock.h"
#include "game/game.h"
#include "game/scaler.h"
#include "game/ui/elements/ammo_label.h"
#include "game/ui/elements/bar.h"
#include "game/ui/elements/bar_enemy_hp.h"
#include "game/ui/elements/bar_lara_air.h"
#include "game/ui/elements/bar_lara_hp.h"
#include "game/ui/elements/flash.h"
#include "game/ui/elements/fps_counter.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/resize.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"
#include "memory.h"

typedef struct UI_OVERLAY_STATE {
    struct {
        CLOCK_TIMER timer;
        bool state;
    } blink;
    UI_FPS_COUNTER_STATE *fps;
    bool force_show_healthbar;
    bool show_arrows[4];
    struct {
        const char *text;
        bool flash_enabled;
    } bottom_text;
    UI_FLASH_STATE flash_state;
} UI_OVERLAY_STATE;

static bool M_LaraHealthBar(const UI_OVERLAY_STATE *s, BAR_LOCATION location);
static bool M_LaraAirBar(const UI_OVERLAY_STATE *s, BAR_LOCATION location);
static bool M_EnemyHealthBar(const BAR_LOCATION location);
static void M_Arrow(const UI_OVERLAY_STATE *s, UI_OVERLAY_ARROW arrow);
static void M_TopLeftRegion(const UI_OVERLAY_STATE *s);
static void M_TopCenterRegion(const UI_OVERLAY_STATE *s);
static void M_TopRightRegion(const UI_OVERLAY_STATE *s);
static void M_BottomLeftRegion(const UI_OVERLAY_STATE *s);
static void M_BottomCenterRegion(const UI_OVERLAY_STATE *s);
static void M_BottomRightRegion(const UI_OVERLAY_STATE *s);

static bool M_LaraHealthBar(
    const UI_OVERLAY_STATE *const s, const BAR_LOCATION location)
{
    if (location != g_Config.ui.lara_health_bar.location) {
        return false;
    }
    if (!Game_IsPlaying() && !s->force_show_healthbar) {
        return false;
    }
    if (!g_Config.ui.enable_game_ui) {
        return false;
    }
    return UI_LaraHealthBar(
        s->blink.state,
        s->force_show_healthbar ? BSM_ALWAYS
                                : g_Config.ui.lara_health_bar.show_mode);
}

static bool M_LaraAirBar(
    const UI_OVERLAY_STATE *const s, const BAR_LOCATION location)
{
    if (location != g_Config.ui.lara_air_bar.location) {
        return false;
    }
    if (!Game_IsPlaying()) {
        return false;
    }
    if (!g_Config.ui.enable_game_ui) {
        return false;
    }
    return UI_LaraAirBar(s->blink.state);
}

static bool M_EnemyHealthBar(const BAR_LOCATION location)
{
    if (location != g_Config.ui.enemy_health_bar.location) {
        return false;
    }
    if (!Game_IsPlaying()) {
        return false;
    }
    if (!g_Config.ui.enable_game_ui) {
        return false;
    }
    return UI_EnemyHealthBar();
}

static void M_Arrow(
    const UI_OVERLAY_STATE *const s, const UI_OVERLAY_ARROW arrow)
{
    if (s->show_arrows[arrow]) {
        // make sure the arrow has exactly the same size as the bar
        UI_BeginResize(
            -1.0,
            UI_BAR_HEIGHT * Scaler_GetScale(SCALER_TARGET_BAR)
                / Scaler_GetScale(SCALER_TARGET_TEXT));
        switch (arrow) {
        case UI_OVERLAY_ARROW_TL:
        case UI_OVERLAY_ARROW_TR:
            UI_Label("\\{arrow up}");
            break;
        case UI_OVERLAY_ARROW_BL:
        case UI_OVERLAY_ARROW_BR:
            UI_Label("\\{arrow down}");
            break;
        }
        UI_EndResize();
    }
}

static void M_TopLeftRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(0.0f, 0.0f);
    bool bar_shown = false;
    bar_shown |= M_LaraHealthBar(s, BL_TOP_LEFT);
    bar_shown |= M_LaraAirBar(s, BL_TOP_LEFT);
    bar_shown |= M_EnemyHealthBar(BL_TOP_LEFT);
    if (!bar_shown) {
        M_Arrow(s, UI_OVERLAY_ARROW_TL);
    }
    if (g_Config.ui.enable_fps_counter && g_Config.ui.enable_game_ui) {
        UI_FPSCounter(s->fps);
    }
    UI_EndOverlayRegion();
}

static void M_TopCenterRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(0.5f, 0.0f);
    M_LaraHealthBar(s, BL_TOP_CENTER);
    M_LaraAirBar(s, BL_TOP_CENTER);
    M_EnemyHealthBar(BL_TOP_CENTER);
    UI_EndOverlayRegion();
}

static void M_TopRightRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(1.0f, 0.0f);
    bool bar_shown = false;
    bar_shown |= M_LaraHealthBar(s, BL_TOP_RIGHT);
    bar_shown |= M_LaraAirBar(s, BL_TOP_RIGHT);
    bar_shown |= M_EnemyHealthBar(BL_TOP_RIGHT);
    if (!bar_shown) {
        M_Arrow(s, UI_OVERLAY_ARROW_TR);
    }
    if (Game_IsPlaying() && g_Config.ui.enable_game_ui) {
        UI_AmmoLabel();
    }
    UI_EndOverlayRegion();
}

static void M_BottomLeftRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(0.0f, 1.0f);
    bool bar_shown = false;
    bar_shown |= M_LaraHealthBar(s, BL_BOTTOM_LEFT);
    bar_shown |= M_LaraAirBar(s, BL_BOTTOM_LEFT);
    bar_shown |= M_EnemyHealthBar(BL_BOTTOM_LEFT);
    if (!bar_shown) {
        M_Arrow(s, UI_OVERLAY_ARROW_BL);
    }
    UI_EndOverlayRegion();
}

static void M_BottomCenterRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(0.5f, 1.0f);
    if (s->bottom_text.text != nullptr) {
        if (s->bottom_text.flash_enabled) {
            UI_BeginFlash(&s->flash_state);
        }
        UI_Label(s->bottom_text.text);
        if (s->bottom_text.flash_enabled) {
            UI_EndFlash();
        }
    }
    M_LaraHealthBar(s, BL_BOTTOM_CENTER);
    M_LaraAirBar(s, BL_BOTTOM_CENTER);
    M_EnemyHealthBar(BL_BOTTOM_CENTER);
    UI_EndOverlayRegion();
}

static void M_BottomRightRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(1.0f, 1.0f);
    bool bar_shown = false;
    bar_shown |= M_LaraHealthBar(s, BL_BOTTOM_RIGHT);
    bar_shown |= M_LaraAirBar(s, BL_BOTTOM_RIGHT);
    bar_shown |= M_EnemyHealthBar(BL_BOTTOM_RIGHT);
    if (!bar_shown) {
        M_Arrow(s, UI_OVERLAY_ARROW_BR);
    }
    UI_EndOverlayRegion();
}

UI_OVERLAY_STATE *UI_Overlay_Init(void)
{
    UI_OVERLAY_STATE *const s = Memory_Alloc(sizeof(UI_OVERLAY_STATE));
    s->blink.timer.type = CLOCK_TIMER_SIM;
    s->fps = UI_FPSCounter_Init();
    UI_Flash_Init(&s->flash_state, 20);
    return s;
}

void UI_Overlay_Free(UI_OVERLAY_STATE *const s)
{
    if (s == nullptr) {
        return;
    }
    if (s->fps != nullptr) {
        UI_FPSCounter_Free(s->fps);
        s->fps = nullptr;
    }
    UI_Flash_Free(&s->flash_state);
    Memory_Free(s);
}

void UI_Overlay_Control(UI_OVERLAY_STATE *const s)
{
    s->force_show_healthbar = false;
    UI_LaraHealthBar_Control();
    if (ClockTimer_CheckElapsedAndTake(
            &s->blink.timer, 10.0 / (double)LOGIC_FPS)) {
        s->blink.state = !s->blink.state;
    }
    UI_Flash_Control(&s->flash_state);
}

void UI_Overlay_ForceHealthBar(UI_OVERLAY_STATE *s, bool show)
{
    s->force_show_healthbar = show;
}

void UI_Overlay(UI_OVERLAY_STATE *const s)
{
    M_TopLeftRegion(s);
    M_TopCenterRegion(s);
    M_TopRightRegion(s);

    M_BottomLeftRegion(s);
    M_BottomCenterRegion(s);
    M_BottomRightRegion(s);
}

void UI_BeginOverlayRegion(const float x, const float y)
{
    // clang-format off
    const UI_STACK_H_ALIGN h_align =
        x > 0.55f ? UI_STACK_H_ALIGN_RIGHT :
        x < 0.45f ? UI_STACK_H_ALIGN_LEFT :
        UI_STACK_H_ALIGN_CENTER;
    // clang-format on
    UI_BeginModal(x, y);
    UI_BeginPad(20.0f, 14.0f);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = h_align },
        .spacing = { .v = 3 },
    });
}

void UI_EndOverlayRegion(void)
{
    UI_EndStack();
    UI_EndPad();
    UI_EndModal();
}

void UI_Overlay_ShowArrows(
    UI_OVERLAY_STATE *const s, const UI_OVERLAY_ARROW arrow, const bool show)
{
    s->show_arrows[arrow] = show;
}

void UI_Overlay_SetBottomText(
    UI_OVERLAY_STATE *const s, const char *const text, const bool flash)
{
    s->bottom_text.text = text;
    s->bottom_text.flash_enabled = flash;
}
