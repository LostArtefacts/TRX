#include "game/overlay.h"

#include "game/game.h"
#include "game/inventory.h"
#include "game/output.h"
#include "game/viewport.h"

#include <libtrx/config.h>
#include <libtrx/game/ui/draw.h>

#define M_MAX_PICKUP_COLUMNS 4
#define M_MAX_PICKUP_DURATION_DISPLAY 2.0 // seconds
#define M_MAX_PICKUP_DURATION_EASE_IN 0.5 // seconds
#define M_MAX_PICKUP_DURATION_EASE_OUT 1.0 // seconds
#define M_MAX_PICKUPS 16
#define M_PICKUPS_FOV 65

typedef enum {
    DPP_EASE_IN,
    DPP_DISPLAY,
    DPP_EASE_OUT,
    DPP_DEAD,
} DISPLAY_PICKUP_PHASE;

typedef struct {
    GAME_OBJECT_ID object_id;
    double elapsed;
    int32_t grid_x;
    int32_t grid_y;
    int32_t rot_y;
    DISPLAY_PICKUP_PHASE phase;
} DISPLAY_PICKUP;

static DISPLAY_PICKUP m_Pickups[M_MAX_PICKUPS] = {};
static CLOCK_TIMER m_PickupsTimer = { .type = CLOCK_TIMER_SIM };

static float M_Ease(float cur_frame, float max_frames);
static void M_DrawPickup3D(DISPLAY_PICKUP *pu);
static void M_DrawPickups3D(void);
static void M_DrawPickupsSprites(void);
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

static void M_DrawPickup3D(DISPLAY_PICKUP *pu)
{
    if (!g_Config.ui.enable_game_ui) {
        return;
    }

    struct {
        GLint x, y, w, h;
    } old_vp, new_vp;
    glGetIntegerv(GL_VIEWPORT, &old_vp.x);

    const int32_t pickup_width = old_vp.w / 8;
    const int32_t pickup_height = old_vp.h / 8;
    const int32_t padding_x = ((old_vp.w + old_vp.h) / 2) / 8;
    const int32_t padding_y = padding_x * 3 / 4;
    const int32_t scale = 768;

    new_vp.x = old_vp.x + old_vp.w / 2 - padding_x - pu->grid_x * pickup_width;
    new_vp.y = old_vp.y - old_vp.h / 2 + padding_y - pu->grid_y * pickup_height;
    new_vp.w = old_vp.w;
    new_vp.h = old_vp.h;

    int32_t dst_x = 0;
    int32_t dst_y = 0;
    int32_t src_x = padding_x + (pu->grid_x + 1.0f) * pickup_width;
    int32_t src_y = 0;

    float ease = 1.0f;
    switch (pu->phase) {
    case DPP_EASE_IN:
        ease = M_Ease(pu->elapsed, M_MAX_PICKUP_DURATION_EASE_IN);
        break;
    case DPP_EASE_OUT:
        ease = M_Ease(
            M_MAX_PICKUP_DURATION_EASE_OUT - pu->elapsed,
            M_MAX_PICKUP_DURATION_EASE_OUT);
        break;
    case DPP_DISPLAY:
        ease = 1.0f;
        break;
    case DPP_DEAD:
        return;
    }

    // Reset the FOV and the W2V matrix in case they get changed by a cinematic
    // camera (when picking up the Scion). Move the viewport rather than
    // translating the object in order to avoid perspective distortion in the
    // screen corners.
    const int16_t old_fov = Viewport_GetSystemFOV();
    Viewport_Init(new_vp.x, new_vp.y, new_vp.w, new_vp.h);
    Viewport_AlterFOV(M_PICKUPS_FOV * DEG_1);
    glViewport(new_vp.x, new_vp.y, new_vp.w, new_vp.h);
    Output_ApplyFOV();

    Matrix_PushUnit();
    Matrix_TranslateSet(
        src_x + (dst_x - src_x) * ease, src_y + (dst_y - src_y) * ease, scale);
    Matrix_RotX(DEG_1 * 15);
    Matrix_RotY(pu->rot_y);

    Output_SetLightDivider(0x6000);
    Output_SetLightAdder(SHADE_LOW);
    Output_RotateLight(0, 0);

    const OBJECT *const obj = Object_Get(Inv_GetItemOption(pu->object_id));
    const ANIM_FRAME *const frame = Object_GetAnim(obj, 0)->frame_ptr;

    Matrix_Push();
    Matrix_TranslateRel16(frame->offset);
    Matrix_TranslateRel32((XYZ_32) {
        .x = -(frame->bounds.min.x + frame->bounds.max.x) / 2,
        .y = -(frame->bounds.min.y + frame->bounds.max.y) / 2,
        .z = -(frame->bounds.min.z + frame->bounds.max.z) / 2,
    });
    Matrix_Rot16(frame->mesh_rots[0]);

    Object_DrawMesh(obj->mesh_idx, 0, false);

    for (int i = 1; i < obj->mesh_count; i++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
        if (bone->matrix_pop) {
            Matrix_Pop();
        }

        if (bone->matrix_push) {
            Matrix_Push();
        }

        Matrix_TranslateRel32(bone->pos);
        Matrix_Rot16(frame->mesh_rots[i]);

        Object_DrawMesh(obj->mesh_idx + i, 0, false);
    }
    Matrix_Pop();

    Matrix_Pop();
    Viewport_Init(-1, -1, -1, -1);
    Viewport_AlterFOV(old_fov);
    glViewport(old_vp.x, old_vp.y, old_vp.w, old_vp.h);
    Output_ApplyFOV();
}

static void M_DrawPickups3D(void)
{
    const double elapsed = ClockTimer_TakeElapsed(&m_PickupsTimer);

    for (int i = 0; i < M_MAX_PICKUPS; i++) {
        DISPLAY_PICKUP *const pu = &m_Pickups[i];

        switch (pu->phase) {
        case DPP_DEAD:
            continue;

        case DPP_EASE_IN:
            pu->elapsed += elapsed;
            if (pu->elapsed >= M_MAX_PICKUP_DURATION_EASE_IN) {
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
            break;

        case DPP_EASE_OUT:
            pu->elapsed += elapsed;
            if (pu->elapsed >= M_MAX_PICKUP_DURATION_EASE_OUT) {
                pu->phase = DPP_DEAD;
                pu->elapsed = 0.0;
            }
            break;
        }

        pu->rot_y += 4 * DEG_1 * (elapsed * LOGIC_FPS);

        M_DrawPickup3D(pu);
    }
}

static void M_DrawPickupsSprites(void)
{
    const double elapsed = ClockTimer_TakeElapsed(&m_PickupsTimer);

    const VIEWPORT_SPACE space = VIEWPORT_UI;
    const int32_t sprite_height =
        MIN(Viewport_GetWidth(space), Viewport_GetHeight(space) * 320 / 200)
        / 10;
    const int32_t sprite_width = sprite_height * 4 / 3;

    for (int i = 0; i < M_MAX_PICKUPS; i++) {
        DISPLAY_PICKUP *const pu = &m_Pickups[i];
        if (pu->phase == DPP_DEAD) {
            continue;
        }

        pu->elapsed += elapsed;
        if (pu->elapsed >= M_MAX_PICKUP_DURATION_DISPLAY) {
            pu->phase = DPP_DEAD;
            continue;
        }

        if (!g_Config.ui.enable_game_ui) {
            return;
        }

        const int32_t x = Viewport_GetWidth(space) - sprite_height
            - sprite_width * pu->grid_x;
        const int32_t y = Viewport_GetHeight(space) - sprite_height
            - sprite_height * pu->grid_y;
        const int32_t scale = 12288 * Viewport_GetWidth(space) / 640;
        const int16_t sprite_num = Object_Get(pu->object_id)->mesh_idx;
        UI_ScheduleDrawScreenSprite(
            x, y, 0, scale, scale, sprite_num, SHADE_NEUTRAL);
    }
}

static void M_DrawPickups(void)
{
    if (g_Config.visuals.enable_3d_pickups) {
        M_DrawPickups3D();
    } else {
        M_DrawPickupsSprites();
    }
}

void Overlay_Reset(void)
{
    for (int i = 0; i < M_MAX_PICKUPS; i++) {
        m_Pickups[i].phase = DPP_DEAD;
    }
}

void Overlay_HideGameInfo(void)
{
    ClockTimer_Sync(&m_PickupsTimer);
}

void Overlay_DrawGameInfo(void)
{
    Output_ClearDepthBuffer();
    if (Game_IsPlaying()) {
        M_DrawPickups();
    }
}

void Overlay_AddDisplayPickup(const GAME_OBJECT_ID obj_id)
{
    int32_t grid_x = -1;
    int32_t grid_y = -1;
    for (int i = 0; i < M_MAX_PICKUPS; i++) {
        int x = i % M_MAX_PICKUP_COLUMNS;
        int y = i / M_MAX_PICKUP_COLUMNS;
        bool is_occupied = false;
        for (int j = 0; j < M_MAX_PICKUPS; j++) {
            bool is_dead_or_dying = m_Pickups[j].phase == DPP_DEAD
                || m_Pickups[j].phase == DPP_EASE_OUT;
            if (m_Pickups[j].grid_x == x && m_Pickups[j].grid_y == y
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

    for (int i = 0; i < M_MAX_PICKUPS; i++) {
        if (m_Pickups[i].phase == DPP_DEAD) {
            m_Pickups[i].object_id = obj_id;
            m_Pickups[i].elapsed = 0.0;
            m_Pickups[i].grid_x = grid_x;
            m_Pickups[i].grid_y = grid_y;
            m_Pickups[i].rot_y = 0;
            m_Pickups[i].phase =
                g_Config.visuals.enable_3d_pickups ? DPP_EASE_IN : DPP_DISPLAY;
            return;
        }
    }
}
