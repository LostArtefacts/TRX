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
#include <trx/game/music.h>
#include <trx/game/objects.h>
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

#define M_MAX_PICKUP_DURATION_DISPLAY (LOGIC_FPS * 2)
#define M_MAX_PICKUP_DURATION_EASE_IN (LOGIC_FPS / 2)
#define M_MAX_PICKUP_DURATION_EASE_OUT LOGIC_FPS

typedef enum {
    DPP_EASE_IN,
    DPP_DISPLAY,
    DPP_EASE_OUT,
    DPP_DEAD,
} DISPLAY_PICKUP_PHASE;

typedef struct {
    DISPLAY_PICKUP_PHASE phase;
    OBJECT_ID object_id;
    OUTPUT_UI_PICKUP display;
    int16_t start_rot;
    int32_t elapsed;
    int32_t total_elapsed;
} DISPLAY_PICKUP;

// Last requested overlay state for the current frame.
typedef struct {
    OVERLAY_TEXT top_text;
    OVERLAY_TEXT bottom_text;
    bool arrows[OVERLAY_ARROW_NUMBER_OF];
    bool show_version;
    bool force_health_bar;
} M_STATE;

static DISPLAY_PICKUP m_Pickups[OUTPUT_UI_MAX_PICKUPS] = {};
static bool m_PickupsActive;

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

static bool M_IsSprite(const DISPLAY_PICKUP *const pickup)
{
    return !g_Config.visuals.enable_3d_pickups
        || pickup->display.object == nullptr;
}

static float M_Ease(float current, const float start, const float goal)
{
    if (start == goal) {
        return start;
    } else if (start > goal) {
        return 1.0f - M_Ease(current, goal, start);
    } else {
        CLAMP(current, start, goal);
        const float ratio = (current - start) / (goal - start);
        if (ratio < 0.5f) {
            return 2.0f * SQUARE(ratio);
        }
        const float new_ratio = ratio - 1.0f;
        return 1.0f - 2.0f * SQUARE(new_ratio);
    }
}

static void M_DrawPickup2D(const DISPLAY_PICKUP *const pickup)
{
    const VIEWPORT_RECT pickup_rect =
        OutputSource_UI_GetPickupRect(&pickup->display);
    const int16_t sprite_num = Object_Get(pickup->object_id)->mesh_idx;
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_num);
    const float sprite_w = ABS(sprite->x1 - sprite->x0);
    const float sprite_h = ABS(sprite->y1 - sprite->y0);
    const float scale = MIN(pickup_rect.h / sprite_h, pickup_rect.w / sprite_w);
    const float scaled_sprite_w = sprite_w * scale;
    const float scaled_sprite_h = sprite_h * scale;
    const float x = pickup_rect.x + (pickup_rect.w - scaled_sprite_w) / 2;
    const float y = pickup_rect.y + (pickup_rect.h - scaled_sprite_h) / 2;
    OutputSource_UI_StageSprite((OUTPUT_UI_SPRITE) {
        .sprite_idx = sprite_num,
        .x0 = x,
        .y0 = y,
        .x1 = x + (sprite->x1 - sprite->x0) * scale,
        .y1 = y + (sprite->y1 - sprite->y0) * scale,
        .z = Output_GetNearZ_UI(),
        .shade = SHADE_NEUTRAL,
        .color = {
            m_WhiteTextColor[0],
            m_WhiteTextColor[1],
            m_WhiteTextColor[2],
            m_WhiteTextColor[3],
        },
    });
}

static void M_DrawPickup3D(const DISPLAY_PICKUP *const pickup)
{
    OutputSource_UI_StagePickup(pickup->display);
}

static void M_DrawPickups(void)
{
    for (int32_t i = 0; i < OUTPUT_UI_MAX_PICKUPS; i++) {
        DISPLAY_PICKUP *const pickup = &m_Pickups[i];

        int32_t duration = 0;
        float slide_start = 0.0f;
        float slide_goal = 0.0f;
        switch (pickup->phase) {
        case DPP_DEAD:
            continue;
        case DPP_EASE_IN:
            duration = M_MAX_PICKUP_DURATION_EASE_IN;
            slide_start = 0.0f;
            slide_goal = 1.0f;
            break;
        case DPP_DISPLAY:
            duration = M_MAX_PICKUP_DURATION_DISPLAY;
            slide_start = 1.0f;
            slide_goal = 1.0f;
            break;
        case DPP_EASE_OUT:
            duration = M_MAX_PICKUP_DURATION_EASE_OUT;
            slide_start = 1.0f;
            slide_goal = 0.0f;
            break;
        }

        if (M_IsSprite(pickup)) {
            pickup->display.ease = 1.0f;
        } else {
            const float rate = Interpolation_GetRate();
            pickup->display.rot_y = pickup->start_rot
                + (4 * DEG_1 * (pickup->total_elapsed + rate));
            pickup->display.ease = M_Ease(
                (pickup->elapsed + rate) / (float)duration, slide_start,
                slide_goal);
        }

        if (M_IsSprite(pickup)) {
            M_DrawPickup2D(pickup);
        } else {
            M_DrawPickup3D(pickup);
        }
    }
}

static void M_AnimatePickups(const int32_t frames)
{
    m_PickupsActive = false;
    for (int32_t i = 0; i < OUTPUT_UI_MAX_PICKUPS; i++) {
        DISPLAY_PICKUP *const pickup = &m_Pickups[i];
        pickup->elapsed += frames;
        pickup->total_elapsed += frames;
        switch (pickup->phase) {
        case DPP_EASE_IN:
            if (pickup->elapsed >= M_MAX_PICKUP_DURATION_EASE_IN) {
                pickup->elapsed = 0;
                pickup->phase = DPP_DISPLAY;
            }
            m_PickupsActive = true;
            break;
        case DPP_DISPLAY:
            if (pickup->elapsed >= M_MAX_PICKUP_DURATION_DISPLAY) {
                pickup->elapsed = 0;
                pickup->phase = DPP_EASE_OUT;
            }
            m_PickupsActive = true;
            break;
        case DPP_EASE_OUT:
            if (pickup->elapsed >= M_MAX_PICKUP_DURATION_EASE_OUT) {
                pickup->elapsed = 0;
                pickup->phase = DPP_DEAD;
            } else {
                m_PickupsActive = true;
            }
            break;
        case DPP_DEAD:
            continue;
        }
    }
}

static void M_Init(void)
{
    UI_Flash_Init(&m_FlashState, 20);
}

void Overlay_Reset(void)
{
    for (int32_t i = 0; i < OUTPUT_UI_MAX_PICKUPS; i++) {
        m_Pickups[i].phase = DPP_DEAD;
    }
    m_State = (M_STATE) {};
}

void Overlay_Control(void)
{
    Overlay_ForceHealthBar(false);
    UI_Flash_Control(&m_FlashState);
}

void Overlay_Animate(int32_t frames)
{
    if (Game_IsPlaying()) {
        M_AnimatePickups(frames);
    }
}

void Overlay_DrawGameInfo(void)
{
    if (!Game_IsPlaying()) {
        return;
    }

    if (Camera_Binoculars_IsActive()) {
        OutputSource_UI_StageBinocularMask();
    }

    if (g_Config.ui.show_pickups_overlay && m_PickupsActive) {
        SceneCompositor_Flush();
        const int32_t old_fog_start = Output_GetFogStart();
        const int32_t old_fog_end = Output_GetFogEnd();
        Output_SetFogStart(20 * WALL_L);
        Output_SetFogEnd(100 * WALL_L);
        M_DrawPickups();
        SceneCompositor_Flush();
        Output_SetFogStart(old_fog_start);
        Output_SetFogEnd(old_fog_end);
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
    if (Object_IsType(obj_id, g_SecretObjects)) {
        const MUSIC_PLAY_MODE mode =
            g_Config.audio.fix_secrets_killing_music ? MPM_OVERLAY : MPM_ONCE;
        Music_Play(MX_SECRET, mode);
    }

    int32_t grid_x = -1;
    int32_t grid_y = -1;
    for (int32_t i = 0; i < OUTPUT_UI_MAX_PICKUPS; i++) {
        const int32_t x = i % OUTPUT_UI_MAX_PICKUP_COLUMNS;
        const int32_t y = i / OUTPUT_UI_MAX_PICKUP_COLUMNS;
        bool is_occupied = false;
        for (int32_t j = 0; j < OUTPUT_UI_MAX_PICKUPS; j++) {
            DISPLAY_PICKUP *const pickup = &m_Pickups[j];
            const bool is_dead_or_dying = pickup->phase == DPP_DEAD
                || (!M_IsSprite(pickup) && pickup->phase == DPP_EASE_OUT);
            if (pickup->display.grid_x == x && pickup->display.grid_y == y
                && !is_dead_or_dying) {
                is_occupied = true;
                break;
            }
        }
        if (!is_occupied) {
            grid_x = x;
            grid_y = y;
            break;
        }
    }

    for (int32_t i = 0; i < OUTPUT_UI_MAX_PICKUPS; i++) {
        DISPLAY_PICKUP *const pickup = &m_Pickups[i];
        if (pickup->phase != DPP_DEAD) {
            continue;
        }
        const OBJECT_ID inv_object_id = Inv_GetItemOption(obj_id);
        const INVENTORY_ITEM *const inv_item = InvRing_GetInvItem(obj_id);
        pickup->phase = DPP_EASE_IN;
        pickup->object_id = obj_id;
        pickup->display.object = nullptr;
        if (inv_object_id != NO_OBJECT) {
            const OBJECT *const obj = Object_Get(inv_object_id);
            if (obj->loaded && obj->anim_idx != NO_ANIM) {
                pickup->display.object = obj;
            }
        }
        pickup->display.grid_x = grid_x;
        pickup->display.grid_y = grid_y;
        pickup->start_rot = inv_item != nullptr ? inv_item->y_rot_sel : 0;
        pickup->elapsed = 0;
        pickup->total_elapsed = 0;
        return;
    }
}

REGISTER_SUBSYSTEM(.init = M_Init)
