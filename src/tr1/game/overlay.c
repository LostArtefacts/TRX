#include "game/overlay.h"

#include "game/game.h"
#include "game/inventory.h"
#include "game/output.h"
#include "game/output/sources/ui.h"
#include "game/viewport.h"

#include <libtrx/config.h>
#include <libtrx/game/inventory_ring.h>
#include <libtrx/game/ui/draw.h>

#define M_MAX_PICKUP_DURATION_DISPLAY 2.0 // seconds
#define M_MAX_PICKUP_DURATION_EASE_IN 0.5 // seconds
#define M_MAX_PICKUP_DURATION_EASE_OUT 1.0 // seconds

typedef enum {
    DPP_EASE_IN,
    DPP_DISPLAY,
    DPP_EASE_OUT,
    DPP_DEAD,
} DISPLAY_PICKUP_PHASE;

typedef struct {
    DISPLAY_PICKUP_PHASE phase;
    GAME_OBJECT_ID object_id;
    OUTPUT_UI_PICKUP display;
    float elapsed;
} DISPLAY_PICKUP;

static DISPLAY_PICKUP m_Pickups[OUTPUT_UI_MAX_PICKUPS] = {};
static CLOCK_TIMER m_PickupsTimer = { .type = CLOCK_TIMER_SIM };

static float M_Ease(float cur_frame, float max_frames);
static void M_DrawPickup2D(DISPLAY_PICKUP *pu);
static void M_DrawPickup3D(DISPLAY_PICKUP *pu);
static void M_DrawPickups(void);

static float M_Ease(const float cur_frame, const float max_frames)
{
    const float ratio = cur_frame / max_frames;
    if (ratio < 0.5f) {
        return 2.0f * ratio * ratio;
    }
    const float new_ratio = ratio - 1.0f;
    return 1.0f - 2.0f * new_ratio * new_ratio;
}

static void M_DrawPickup2D(DISPLAY_PICKUP *const pu)
{
    const VIEWPORT_RECT pickup_rect =
        OutputSource_UI_GetPickupRect(&pu->display);
    const int16_t sprite_num = Object_Get(pu->object_id)->mesh_idx;
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

static void M_DrawPickup3D(DISPLAY_PICKUP *const pu)
{
    OutputSource_UI_StagePickup(pu->display);
}

static void M_DrawPickups(void)
{
    const float elapsed = ClockTimer_TakeElapsed(&m_PickupsTimer);
    for (int32_t i = 0; i < OUTPUT_UI_MAX_PICKUPS; i++) {
        DISPLAY_PICKUP *const pu = &m_Pickups[i];

        switch (pu->phase) {
        case DPP_EASE_IN:
            pu->elapsed += elapsed;
            pu->display.ease =
                M_Ease(pu->elapsed, M_MAX_PICKUP_DURATION_EASE_IN);
            if (!g_Config.visuals.enable_3d_pickups
                || pu->elapsed >= M_MAX_PICKUP_DURATION_EASE_IN) {
                pu->phase = DPP_DISPLAY;
                pu->elapsed = 0.0;
            }
            break;

        case DPP_DISPLAY:
            pu->elapsed += elapsed;
            if (pu->elapsed >= M_MAX_PICKUP_DURATION_DISPLAY) {
                pu->phase = DPP_EASE_OUT;
                pu->elapsed = 0.0;
            }
            pu->display.ease = 1.0f;
            break;

        case DPP_EASE_OUT:
            pu->elapsed += elapsed;
            pu->display.ease = M_Ease(
                M_MAX_PICKUP_DURATION_EASE_OUT - pu->elapsed,
                M_MAX_PICKUP_DURATION_EASE_OUT);
            if (!g_Config.visuals.enable_3d_pickups
                || pu->elapsed >= M_MAX_PICKUP_DURATION_EASE_OUT) {
                pu->phase = DPP_DEAD;
                pu->elapsed = 0.0;
                continue;
            }
            break;

        case DPP_DEAD:
            continue;
        }

        pu->display.rot_y += 4 * DEG_1 * (elapsed * LOGIC_FPS);
        if (!g_Config.ui.enable_game_ui) {
            continue;
        }

        if (g_Config.visuals.enable_3d_pickups) {
            M_DrawPickup3D(pu);
        } else {
            M_DrawPickup2D(pu);
        }
    }
}

void Overlay_Reset(void)
{
    for (int32_t i = 0; i < OUTPUT_UI_MAX_PICKUPS; i++) {
        m_Pickups[i].phase = DPP_DEAD;
    }
}

void Overlay_HideGameInfo(void)
{
    ClockTimer_Sync(&m_PickupsTimer);
}

void Overlay_DrawGameInfo(void)
{
    if (Game_IsPlaying()) {
        M_DrawPickups();
    }
}

void Overlay_AddDisplayPickup(const GAME_OBJECT_ID obj_id)
{
    int32_t grid_x = -1;
    int32_t grid_y = -1;
    for (int32_t i = 0; i < OUTPUT_UI_MAX_PICKUPS; i++) {
        const int32_t x = i % OUTPUT_UI_MAX_PICKUP_COLUMNS;
        const int32_t y = i / OUTPUT_UI_MAX_PICKUP_COLUMNS;
        bool is_occupied = false;
        for (int32_t j = 0; j < OUTPUT_UI_MAX_PICKUPS; j++) {
            DISPLAY_PICKUP *const pu = &m_Pickups[j];
            const bool is_dead_or_dying =
                pu->phase == DPP_DEAD || pu->phase == DPP_EASE_OUT;
            if (pu->display.grid_x == x && pu->display.grid_y == y
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
        DISPLAY_PICKUP *const pu = &m_Pickups[i];
        if (pu->phase != DPP_DEAD) {
            continue;
        }
        const GAME_OBJECT_ID inv_object_id = Inv_GetItemOption(obj_id);
        const INVENTORY_ITEM *const inv_item = InvRing_GetInvItem(obj_id);
        pu->object_id = obj_id;
        pu->elapsed = 0.0;
        pu->display.object = Object_Get(inv_object_id);
        pu->display.grid_x = grid_x;
        pu->display.grid_y = grid_y;
        pu->display.rot_y = inv_item != nullptr ? inv_item->y_rot_sel : 0;
        pu->display.ease = 0.0f;
        pu->phase = DPP_EASE_IN;
        return;
    }
}
