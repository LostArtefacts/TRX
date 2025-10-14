#include "game/overlay.h"

#include "config.h"
#include "game/const.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/gym.h"
#include "game/interpolation.h"
#include "game/inventory.h"
#include "game/inventory_ring.h"
#include "game/music.h"
#include "game/objects.h"
#include "game/output.h"
#include "game/output/sources/ui.h"
#include "game/savegame.h"
#include "game/scaler.h"
#include "game/ui.h"

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

static UI_OVERLAY_STATE *m_UI = nullptr;
static DISPLAY_PICKUP m_Pickups[OUTPUT_UI_MAX_PICKUPS] = {};

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

#if TR_VERSION == 2
static void M_DrawAssaultTimer(void)
{
    if (!Game_IsInGym() || !Gym_IsAssaultTimerDisplay()) {
        return;
    }

    char buffer[32];
    const RESUME_INFO *const resume =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    const int32_t total_sec = resume->stats.timer / LOGIC_FPS;
    const int32_t frame = resume->stats.timer % LOGIC_FPS;
    sprintf(
        buffer, "%d:%02d.%d", total_sec / 60, total_sec % 60,
        frame * 10 / LOGIC_FPS);

    const int32_t scale_h = Scaler_Calc(PHD_ONE, SCALER_TARGET_ASSAULT_DIGITS);
    const int32_t scale_v = Scaler_Calc(PHD_ONE, SCALER_TARGET_ASSAULT_DIGITS);

    typedef enum {
        ASSAULT_GLYPH_DECIMAL_POINT,
        ASSAULT_GLYPH_COLON,
        ASSAULT_GLYPH_DIGIT,
    } ASSAULT_GLYPH_TYPE;

    struct {
        int32_t offset;
        int32_t width;
    } glyph_info[] = {
        [ASSAULT_GLYPH_DECIMAL_POINT] = { .offset = -6, .width = 14 },
        [ASSAULT_GLYPH_COLON] = { .offset = -6, .width = 14 },
        [ASSAULT_GLYPH_DIGIT] = { .offset = 0, .width = 20 },
    };

    const int32_t y = Scaler_Calc(36, SCALER_TARGET_ASSAULT_DIGITS);
    int32_t x = Viewport_GetCenterX(VIEWPORT_UI)
        - Scaler_Calc(50, SCALER_TARGET_ASSAULT_DIGITS);

    for (char *c = buffer; *c != '\0'; c++) {
        ASSAULT_GLYPH_TYPE glyph_type;
        int32_t mesh_num;
        if (*c == ':') {
            glyph_type = ASSAULT_GLYPH_COLON;
            mesh_num = 10;
        } else if (*c == '.') {
            glyph_type = ASSAULT_GLYPH_DECIMAL_POINT;
            mesh_num = 11;
        } else {
            glyph_type = ASSAULT_GLYPH_DIGIT;
            mesh_num = *c - '0';
        }

        x += Scaler_Calc(
            glyph_info[glyph_type].offset, SCALER_TARGET_ASSAULT_DIGITS);
        UI_ScheduleDrawScreenSprite(
            x, y, 0, scale_h, scale_v,
            Object_Get(O_ASSAULT_DIGITS)->mesh_idx + mesh_num, SHADE_NEUTRAL);
        x += Scaler_Calc(
            glyph_info[glyph_type].width, SCALER_TARGET_ASSAULT_DIGITS);
    }
}
#endif

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
        .color = { 255, 255, 255, 255 },
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
            break;
        case DPP_DISPLAY:
            if (pickup->elapsed >= M_MAX_PICKUP_DURATION_DISPLAY) {
                pickup->elapsed = 0;
                pickup->phase = DPP_EASE_OUT;
            }
            break;
        case DPP_EASE_OUT:
            if (pickup->elapsed >= M_MAX_PICKUP_DURATION_EASE_OUT) {
                pickup->elapsed = 0;
                pickup->phase = DPP_DEAD;
            }
            break;
        case DPP_DEAD:
            continue;
        }
    }
}

void Overlay_Init(void)
{
    if (m_UI == nullptr) {
        m_UI = UI_Overlay_Init();
    }
}

void Overlay_Shutdown(void)
{
    if (m_UI != nullptr) {
        UI_Overlay_Free(m_UI);
        m_UI = nullptr;
    }
}

void Overlay_Reset(void)
{
    for (int32_t i = 0; i < OUTPUT_UI_MAX_PICKUPS; i++) {
        m_Pickups[i].phase = DPP_DEAD;
    }
}

void Overlay_Control(void)
{
    if (m_UI != nullptr) {
        UI_Overlay_Control(m_UI);
    }
}

void Overlay_Animate(int32_t frames)
{
    if (Game_IsPlaying()) {
        M_AnimatePickups(frames);
    }
}

void Overlay_Draw(void)
{
    if (m_UI != nullptr) {
        UI_Overlay(m_UI);
    }
}

void Overlay_DrawGameInfo(void)
{
    if (Game_IsPlaying()) {
        M_DrawPickups();
#if TR_VERSION == 2
        M_DrawAssaultTimer();
#endif
    }
}

void Overlay_ForceHealthBar(const bool show)
{
    UI_Overlay_ForceHealthBar(m_UI, show);
}

void Overlay_SetHealthBarTimer(const int16_t timer)
{
    UI_LaraHealthBar_SetTimer(timer);
}

void Overlay_ShowArrow(const UI_OVERLAY_ARROW arrow, const bool show)
{
    if (m_UI != nullptr) {
        UI_Overlay_ShowArrow(m_UI, arrow, show);
    }
}

void Overlay_ShowVersion(const bool show)
{
    if (m_UI != nullptr) {
        UI_Overlay_ShowVersion(m_UI, show);
    }
}

void Overlay_SetTopText(const char *const text, const bool flash)
{
    if (m_UI != nullptr) {
        UI_Overlay_SetTopText(m_UI, text, flash);
    }
}

void Overlay_SetBottomText(const char *const text, const bool flash)
{
    if (m_UI != nullptr) {
        UI_Overlay_SetBottomText(m_UI, text, flash);
    }
}

void Overlay_SetTopTextPtr(const char *const *const ptr, const bool flash)
{
    if (m_UI != nullptr) {
        UI_Overlay_SetTopTextPtr(m_UI, ptr, flash);
    }
}

void Overlay_SetBottomTextPtr(const char *const *const ptr, const bool flash)
{
    if (m_UI != nullptr) {
        UI_Overlay_SetBottomTextPtr(m_UI, ptr, flash);
    }
}

void Overlay_AddDisplayPickup(const OBJECT_ID obj_id)
{
#if TR_VERSION == 2
    if (Object_IsType(obj_id, g_SecretObjects)) {
        Music_Play_Direct(g_GameFlow.secret_track, MPM_ALWAYS);
    }
#endif

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
        pickup->display.object =
            inv_object_id != NO_OBJECT ? Object_Get(inv_object_id) : nullptr;
        pickup->display.grid_x = grid_x;
        pickup->display.grid_y = grid_y;
        pickup->start_rot = inv_item != nullptr ? inv_item->y_rot_sel : 0;
        pickup->elapsed = 0;
        pickup->total_elapsed = 0;
        return;
    }
}
