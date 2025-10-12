#include "game/ui/hud/overlay.h"

#include "config.h"
#include "game/clock.h"
#include "game/const.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/lara.h"
#include "game/scaler.h"
#include "game/ui/elements/ammo_label.h"
#include "game/ui/elements/bar.h"
#include "game/ui/elements/bar_enemy_hp.h"
#include "game/ui/elements/bar_lara_air.h"
#include "game/ui/elements/bar_lara_exposure.h"
#include "game/ui/elements/bar_lara_hp.h"
#include "game/ui/elements/bar_lara_sprint.h"
#include "game/ui/elements/fixed.h"
#include "game/ui/elements/flash.h"
#include "game/ui/elements/fps_counter.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/resize.h"
#include "game/ui/elements/row_arrows.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"
#include "memory.h"
#include "strings.h"
#include "version.h"

typedef struct UI_OVERLAY_STATE {
    struct {
        CLOCK_TIMER timer;
        bool state;
    } blink;
    UI_FPS_COUNTER_STATE *fps;
    bool force_show_healthbar;
    bool show_arrows[6];
    bool show_version;
    struct {
        const char *one_off;
        const char *const *live_ptr;
        bool flash_enabled;
    } top_text, bottom_text;
    UI_FLASH_STATE flash_state;
} UI_OVERLAY_STATE;

static struct {
    bool resize;
    const char *label;
} m_ArrowInfo[] = {
    [UI_OVERLAY_ARROW_TR] = { true, "\\{arrow up}" },
    [UI_OVERLAY_ARROW_TL] = { true, "\\{arrow up}" },
    [UI_OVERLAY_ARROW_BL] = { true, "\\{arrow down}" },
    [UI_OVERLAY_ARROW_BR] = { true, "\\{arrow down}" },
    [UI_OVERLAY_ARROW_BCL] = { false, "\\{button left}" },
    [UI_OVERLAY_ARROW_BCR] = { false, "\\{button right}" },
};

static bool M_LaraHealthBar(
    const UI_OVERLAY_STATE *const s, const BAR_LOCATION location)
{
    if (location != g_Config.ui.lara_health_bar.location) {
        return false;
    }
    if (!Lara_IsControllable()
        || (!Game_IsPlaying() && !s->force_show_healthbar)) {
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
    if (!Lara_IsControllable() || !Game_IsPlaying()) {
        return false;
    }
    if (!g_Config.ui.enable_game_ui) {
        return false;
    }
    return UI_LaraAirBar(s->blink.state);
}

static bool M_LaraSprintBar(
    const UI_OVERLAY_STATE *const s, const BAR_LOCATION location)
{
    if (location != g_Config.ui.lara_sprint_bar.location) {
        return false;
    }
    if (!Lara_IsControllable() || !Game_IsPlaying()) {
        return false;
    }
    if (!g_Config.ui.enable_game_ui) {
        return false;
    }
    return UI_LaraSprintBar();
}

static bool M_LaraExposureBar(
    const UI_OVERLAY_STATE *const s, const BAR_LOCATION location)
{
    if (location != g_Config.ui.lara_exposure_bar.location) {
        return false;
    }
    if (!Lara_IsControllable() || !Game_IsPlaying()) {
        return false;
    }
    if (!g_Config.ui.enable_game_ui) {
        return false;
    }
    return UI_LaraExposureBar(s->blink.state);
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
        if (m_ArrowInfo[arrow].resize) {
            UI_BeginResize(
                -1.0,
                UI_BAR_HEIGHT * Scaler_GetScale(SCALER_TARGET_BAR)
                    / Scaler_GetScale(SCALER_TARGET_TEXT));
        } else {
            UI_BeginFixed(0.5f, 0.5f);
        }
        UI_Label(m_ArrowInfo[arrow].label);
        if (m_ArrowInfo[arrow].resize) {
            UI_EndResize();
        } else {
            UI_EndFixed();
        }
    }
}

static void M_DebugPosTopLeft(void)
{
    const ITEM *const lara = Lara_GetItem();
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara == nullptr) {
        return;
    }

    const OBJECT_ID obj_id = Lara_GetAnimationObject();
    const ITEM *const vehicle = Lara_Vehicle_GetItem();
    UI_BeginStack(UI_STACK_HORIZONTAL);
    UI_BeginStack(UI_STACK_VERTICAL);
    UI_Label(GS(OVERLAY_DEBUG_POSITION));
    UI_Label(GS(OVERLAY_DEBUG_ROTATION));
    UI_Label(GS(OVERLAY_DEBUG_SPEED));
    UI_Label(GS(OVERLAY_DEBUG_ANIMATION));
    UI_EndStack();
    UI_BeginStack(UI_STACK_VERTICAL);
    UI_Label(String_StylizeSmallDigitsStatic(String_FormatStatic(
        "%d, %d, %d", lara->pos.x / WALL_L, lara->pos.y / WALL_L,
        lara->pos.z / WALL_L)));
    UI_Label(String_StylizeSmallDigitsStatic(String_FormatStatic(
        "%d°, %d°, %d°", (int32_t)lara->rot.x * 360 / DEG_360,
        (int32_t)lara->rot.y * 360 / DEG_360,
        (int32_t)lara->rot.z * 360 / DEG_360)));
    UI_Label(String_StylizeSmallDigitsStatic(String_FormatStatic(
        "%d, %d", vehicle != nullptr ? vehicle->speed : lara->speed,
        vehicle != nullptr ? vehicle->fall_speed : lara->fall_speed)));
    UI_Label(String_StylizeSmallDigitsStatic(String_FormatStatic(
        "%d, %d, %d", obj_id, Item_GetRelativeObjAnim(lara, obj_id),
        Item_GetRelativeFrame(lara))));
    UI_EndStack();
    UI_EndStack();
}

static void M_DebugPosTopRight(void)
{
    const ITEM *const lara = Lara_GetItem();
    if (lara == nullptr) {
        return;
    }

    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_RIGHT },
    });
    UI_Label(String_StylizeSmallDigitsStatic(String_FormatStatic(
        "%d, %d, %d", lara->pos.x, lara->pos.y, lara->pos.z)));
    UI_Label(String_StylizeSmallDigitsStatic(String_FormatStatic(
        "%d, %d, %d", lara->rot.x, lara->rot.y, lara->rot.z)));
    if (g_Config.debug.enable_invulnerability) {
        UI_LabelEx(
            GS(OVERLAY_DEBUG_IMMUNE), (UI_LABEL_SETTINGS) { .scale = 0.8 });
    }
    UI_EndStack();
}

static bool M_RegionBars(
    const UI_OVERLAY_STATE *const s, const BAR_LOCATION location)
{
    bool bar_shown = false;
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_CENTER },
        .spacing = { .v = 10 },
    });
    bar_shown |= M_LaraHealthBar(s, location);
    bar_shown |= M_LaraAirBar(s, location);
    bar_shown |= M_LaraSprintBar(s, location);
    bar_shown |= M_LaraExposureBar(s, location);
    bar_shown |= M_EnemyHealthBar(location);
    UI_EndStack();
    return bar_shown;
}

static void M_TopLeftRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(0.0f, 0.0f);
    if (!M_RegionBars(s, BL_TOP_LEFT)) {
        M_Arrow(s, UI_OVERLAY_ARROW_TL);
    }
    if (g_Config.ui.enable_game_ui) {
        if (Game_IsPlaying() && g_Config.debug.enable_debug_pos) {
            M_DebugPosTopLeft();
        }
        if (g_Config.ui.enable_fps_counter) {
            UI_FPSCounter(s->fps);
        }
    }
    UI_EndOverlayRegion();
}

static void M_TopCenterRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(0.5f, 0.0f);
    M_RegionBars(s, BL_TOP_CENTER);
    {
        const char *const txt =
            s->top_text.live_ptr ? *s->top_text.live_ptr : s->top_text.one_off;
        if (txt != nullptr) {
            if (s->top_text.flash_enabled) {
                UI_BeginFlash(&s->flash_state);
            }
            UI_Label(txt);
            if (s->top_text.flash_enabled) {
                UI_EndFlash();
            }
        }
    }
    UI_EndOverlayRegion();
}

static void M_TopRightRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(1.0f, 0.0f);
    if (!M_RegionBars(s, BL_TOP_RIGHT)) {
        M_Arrow(s, UI_OVERLAY_ARROW_TR);
    }
    if (Game_IsPlaying() && g_Config.ui.enable_game_ui) {
        if (g_Config.debug.enable_debug_pos) {
            M_DebugPosTopRight();
        }
        UI_AmmoLabel();
    }
    UI_EndOverlayRegion();
}

static void M_BottomLeftRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(0.0f, 1.0f);
    if (!M_RegionBars(s, BL_BOTTOM_LEFT)) {
        M_Arrow(s, UI_OVERLAY_ARROW_BL);
    }
    UI_EndOverlayRegion();
}

static void M_BottomCenterRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(0.5f, 1.0f);
    {
        const char *const txt = s->bottom_text.live_ptr
            ? *s->bottom_text.live_ptr
            : s->bottom_text.one_off;
        if (txt != nullptr) {
            if (s->bottom_text.flash_enabled) {
                UI_BeginFlash(&s->flash_state);
            }
            UI_BeginRowArrows(
                s->show_arrows[UI_OVERLAY_ARROW_BCL],
                s->show_arrows[UI_OVERLAY_ARROW_BCR], UI_ROW_ARROWS_WIDE);
            UI_Label(txt);
            UI_EndRowArrows();
            if (s->bottom_text.flash_enabled) {
                UI_EndFlash();
            }
        }
    }
    M_RegionBars(s, BL_BOTTOM_CENTER);
    UI_EndOverlayRegion();
}

static void M_BottomRightRegion(const UI_OVERLAY_STATE *const s)
{
    UI_BeginOverlayRegion(1.0f, 1.0f);
    if (!M_RegionBars(s, BL_BOTTOM_RIGHT)) {
        M_Arrow(s, UI_OVERLAY_ARROW_BR);
    }
    if (s->show_version) {
        UI_LabelEx(g_TRXVersion, (UI_LABEL_SETTINGS) { .scale = 0.5f });
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

void UI_Overlay_ShowArrow(
    UI_OVERLAY_STATE *const s, const UI_OVERLAY_ARROW arrow, const bool show)
{
    s->show_arrows[arrow] = show;
}

void UI_Overlay_ShowVersion(UI_OVERLAY_STATE *const s, const bool show)
{
    s->show_version = show;
}

void UI_Overlay_SetTopText(
    UI_OVERLAY_STATE *const s, const char *const text, const bool flash)
{
    s->top_text.one_off = text;
    s->top_text.live_ptr = nullptr;
    s->top_text.flash_enabled = flash;
}

void UI_Overlay_SetBottomText(
    UI_OVERLAY_STATE *const s, const char *const text, const bool flash)
{
    s->bottom_text.one_off = text;
    s->bottom_text.live_ptr = nullptr;
    s->bottom_text.flash_enabled = flash;
}

void UI_Overlay_SetTopTextPtr(
    UI_OVERLAY_STATE *const s, const char *const *const ptr, const bool flash)
{
    s->top_text.one_off = nullptr;
    s->top_text.live_ptr = ptr;
    s->top_text.flash_enabled = flash;
}

void UI_Overlay_SetBottomTextPtr(
    UI_OVERLAY_STATE *const s, const char *const *const ptr, const bool flash)
{
    s->bottom_text.one_off = nullptr;
    s->bottom_text.live_ptr = ptr;
    s->bottom_text.flash_enabled = flash;
}
