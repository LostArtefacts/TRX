#include <trx/game/overlay.h>

#include <trx/config.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/game/camera.h>
#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/interpolation.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/objects/names.h>
#include <trx/game/output.h>
#include <trx/game/output/sources/ui.h>
#include <trx/game/ui.h>
#include <trx/game/ui/elements/flash.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/ui/elements/resize.h>
#include <trx/game/ui/elements/row_arrows.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/scaler.h>
#include <trx/version.h>

// Last requested overlay state for the current frame.
typedef struct {
    OVERLAY_TEXT top_text;
    OVERLAY_TEXT bottom_text;
    bool arrows[OVERLAY_ARROW_NUMBER_OF];
    bool show_version;
    bool force_health_bar;
} M_STATE;

static M_STATE m_State;
static UI_FLASH_STATE m_FlashState;

static const char *const m_ArrowLabels[OVERLAY_ARROW_NUMBER_OF] = {
    [OVERLAY_ARROW_TL] = "\\{arrow up}",
    [OVERLAY_ARROW_TR] = "\\{arrow up}",
    [OVERLAY_ARROW_BL] = "\\{arrow down}",
    [OVERLAY_ARROW_BR] = "\\{arrow down}",
};

static const UI_REGION m_ArrowRegions[] = {
    [OVERLAY_ARROW_TL] = UI_REGION_TOP_LEFT,
    [OVERLAY_ARROW_TR] = UI_REGION_TOP_RIGHT,
    [OVERLAY_ARROW_BL] = UI_REGION_BOTTOM_LEFT,
    [OVERLAY_ARROW_BR] = UI_REGION_BOTTOM_RIGHT,
};

static const RGBA_F m_WhiteTextColor[4] = {
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
};

static const char *M_ResolveTextRaw(const OVERLAY_TEXT *const text)
{
    switch (text->kind) {
    case OVERLAY_TEXT_NONE:
        return nullptr;
    case OVERLAY_TEXT_LITERAL:
        return text->literal;
    case OVERLAY_TEXT_GS_KEY:
        return GameString_Get(text->gs_key);
    case OVERLAY_TEXT_OBJECT_NAME:
        return Object_GetName(text->object_id);
    }
    return nullptr;
}

// Resolves deferred overlay text in the current language.
static const char *M_ResolveText(const OVERLAY_TEXT *const text)
{
    const char *const raw = M_ResolveTextRaw(text);
    if (raw == nullptr || text->fmt_gs_key == nullptr) {
        return raw;
    }
    return String_FormatStatic(GameString_Get(text->fmt_gs_key), raw);
}

static void M_Init(void)
{
    UI_Flash_Init(&m_FlashState, 20);
}

void Overlay_Reset(void)
{
    m_State = (M_STATE) {};
}

void Overlay_Control(void)
{
    Overlay_ForceHealthBar(false);
    UI_Flash_Control(&m_FlashState);
}

void Overlay_DrawGameInfo(void)
{
    if (!Game_IsPlaying()) {
        return;
    }

    if (Camera_Binoculars_IsActive()) {
        OutputSource_UI_StageBinocularMask();
    }
}

void Overlay_DrawUI(void)
{
    const char *const top = M_ResolveText(&m_State.top_text);
    if (top != nullptr) {
        UI_BeginRegion(UI_REGION_TOP_CENTER);
        if (m_State.top_text.flash_enabled) {
            UI_BeginFlash(&m_FlashState);
        }
        UI_Label(top);
        if (m_State.top_text.flash_enabled) {
            UI_EndFlash();
        }
        UI_EndRegion();
    }

    const char *const bottom = M_ResolveText(&m_State.bottom_text);
    if (bottom != nullptr) {
        UI_BeginRegion(UI_REGION_BOTTOM_CENTER);
        if (m_State.bottom_text.flash_enabled) {
            UI_BeginFlash(&m_FlashState);
        }
        UI_BeginRowArrows(
            m_State.arrows[OVERLAY_ARROW_BCL],
            m_State.arrows[OVERLAY_ARROW_BCR], UI_ROW_ARROWS_WIDE);
        UI_Label(bottom);
        UI_EndRowArrows();
        if (m_State.bottom_text.flash_enabled) {
            UI_EndFlash();
        }
        UI_EndRegion();
    }

    if (m_State.show_version && g_Config.ui.show_title_version) {
        UI_BeginRegion(UI_REGION_BOTTOM_RIGHT);
        UI_LabelEx(g_TRXVersion, (UI_LABEL_SETTINGS) { .scale = 0.5f });
        UI_EndRegion();
    }

    // Draw corner arrows only in otherwise empty regions.
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_ArrowRegions); i++) {
        const UI_REGION region = m_ArrowRegions[i];
        if (!m_State.arrows[i] || !UI_Region_IsEmpty(region)) {
            continue;
        }
        UI_BeginRegion(region);
        // Match bar height so corner regions reserve the same space.
        UI_BeginResize(
            -1.0,
            UI_BAR_HEIGHT * UI_Scaler_GetScale(UI_SCALER_TARGET_BAR)
                / UI_Scaler_GetScale(UI_SCALER_TARGET_TEXT));
        UI_Label(m_ArrowLabels[i]);
        UI_EndResize();
        UI_EndRegion();
    }
}

void Overlay_ForceHealthBar(const bool show)
{
    m_State.force_health_bar = show;
}

bool Overlay_IsHealthBarForced(void)
{
    return m_State.force_health_bar;
}

void Overlay_ShowArrow(const OVERLAY_ARROW arrow, const bool show)
{
    m_State.arrows[arrow] = show;
}

void Overlay_ShowVersion(const bool show)
{
    m_State.show_version = show;
}

void Overlay_SetTopText(const OVERLAY_TEXT text)
{
    m_State.top_text = text;
}

void Overlay_SetBottomText(const OVERLAY_TEXT text)
{
    m_State.bottom_text = text;
}

void Overlay_AddDisplayPickup(const OBJECT_ID obj_id)
{
    if (ObjectFamily_Has(obj_id, OBJ_FAMILY_SECRET)) {
        const MUSIC_PLAY_MODE mode =
            g_Config.audio.fix_secrets_killing_music ? MPM_OVERLAY : MPM_ONCE;
        Music_Play(MX_SECRET, mode);
    }
    LUA_FireEventInt32(LUA_EVENT_SHOW_PICKUP, obj_id);
}

REGISTER_SUBSYSTEM(.init = M_Init)
