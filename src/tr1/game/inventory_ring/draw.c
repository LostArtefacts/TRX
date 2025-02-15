#include "game/inventory_ring/draw.h"

#include "game/game.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/inventory_ring.h"
#include "game/objects/common.h"
#include "game/option.h"
#include "game/option/option_compass.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/shell.h"
#include "game/text.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/inventory_ring/priv.h>
#include <libtrx/game/matrix.h>

static int32_t M_GetFrames(
    const INV_RING *ring, const INVENTORY_ITEM *inv_item,
    ANIM_FRAME **out_frame1, ANIM_FRAME **out_frame2, int32_t *out_rate);
static void M_DrawItem(const INV_RING *ring, INVENTORY_ITEM *inv_item);

static int32_t M_GetFrames(
    const INV_RING *const ring, const INVENTORY_ITEM *const inv_item,
    ANIM_FRAME **const out_frame1, ANIM_FRAME **const out_frame2,
    int32_t *const out_rate)
{
    const OBJECT *const obj = Object_Get(inv_item->object_id);
    const INVENTORY_ITEM *const cur_inv_item = ring->list[ring->current_object];
    if (inv_item != cur_inv_item
        || (ring->motion.status != RNG_SELECTED
            && ring->motion.status != RNG_CLOSING_ITEM)) {
        // only apply to animations, eg. the states where Inv_AnimateItem is
        // being actively called
        goto fallback;
    }

    if (inv_item->current_frame == inv_item->goal_frame
        || inv_item->frames_total == 1 || g_Config.rendering.fps == 30) {
        goto fallback;
    }

    const int32_t cur_frame_num = inv_item->current_frame;
    int32_t next_frame_num = inv_item->current_frame + inv_item->anim_direction;
    if (next_frame_num < 0) {
        next_frame_num = 0;
    }
    if (next_frame_num >= inv_item->frames_total) {
        next_frame_num = 0;
    }

    *out_frame1 = &obj->frame_base[cur_frame_num];
    *out_frame2 = &obj->frame_base[next_frame_num];
    *out_rate = 10;
    return (Interpolation_GetRate() - 0.5) * 10.0;

    // OG
fallback:
    *out_frame1 = &obj->frame_base[inv_item->current_frame];
    *out_frame2 = *out_frame1;
    *out_rate = 1;
    return 0;
}

static void M_DrawItem(
    const INV_RING *const ring, INVENTORY_ITEM *const inv_item)
{
    if (ring->motion.status != RNG_FADING_OUT && ring->motion.status != RNG_DONE
        && inv_item == ring->list[ring->current_object] && !ring->rotating) {
        Output_SetLightAdder(HIGH_LIGHT);
    } else {
        Output_SetLightAdder(LOW_LIGHT);
    }

    Matrix_TranslateRel(0, inv_item->y_trans, inv_item->z_trans);
    Matrix_RotY(inv_item->y_rot);
    Matrix_RotX(inv_item->x_rot);

    OBJECT *const obj = Object_Get(inv_item->object_id);
    if (obj->mesh_count < 0) {
        Output_DrawSpriteRel(0, 0, 0, obj->mesh_idx, 4096);
        return;
    }

    int32_t rate;
    ANIM_FRAME *frame1;
    ANIM_FRAME *frame2;
    const int32_t frac = M_GetFrames(ring, inv_item, &frame1, &frame2, &rate);
    if (inv_item->object_id == O_COMPASS_OPTION) {
        const int16_t extra_rotation[1] = { Option_Compass_GetNeedleAngle() };
        Object_GetBone(obj, 0)->rot_y = true;
        Object_DrawInterpolatedObject(
            obj, inv_item->meshes_drawn, extra_rotation, frame1, frame2, frac,
            rate);
    } else {
        Object_DrawInterpolatedObject(
            obj, inv_item->meshes_drawn, nullptr, frame1, frame2, frac, rate);
    }
}

void InvRing_Draw(INV_RING *const ring)
{
    const int32_t num_frames = round(
        ClockTimer_TakeElapsed(&ring->motion_timer) * LOGIC_FPS
        * INV_RING_FRAMES);

    ring->camera.pos.z = ring->radius + CAMERA_2_RING;

    if (ring->mode == INV_TITLE_MODE) {
        Interpolation_Commit();
    } else {
        Matrix_LookAt(
            g_InvRing_OldCamera.pos.x,
            g_InvRing_OldCamera.pos.y + g_InvRing_OldCamera.shift,
            g_InvRing_OldCamera.pos.z, g_InvRing_OldCamera.target.x,
            g_InvRing_OldCamera.target.y, g_InvRing_OldCamera.target.z, 0);
        Interpolation_Disable();
        Game_Draw(false);
        Interpolation_Enable();

        Fader_Draw(&ring->back_fader);

        int32_t width = Screen_GetResWidth();
        int32_t height = Screen_GetResHeight();
        Viewport_Init(0, 0, width, height);
    }

    int16_t old_fov = Viewport_GetFOV();
    Viewport_SetFOV(PASSPORT_FOV * DEG_1);
    Output_ApplyFOV();

    XYZ_32 view_pos;
    XYZ_16 view_rot;
    InvRing_GetView(ring, &view_pos, &view_rot);
    Matrix_GenerateW2V(&view_pos, &view_rot);
    InvRing_Light(ring);

    Matrix_Push();
    Matrix_TranslateAbs32(ring->ring_pos.pos);
    Matrix_Rot16(ring->ring_pos.rot);

    if (!(ring->mode == INV_TITLE_MODE
          && (Fader_IsActive(&ring->top_fader)
              || Fader_IsActive(&ring->back_fader))
          && ring->motion.status == RNG_OPENING)) {
        int16_t angle = 0;
        for (int32_t i = 0; i < ring->number_of_objects; i++) {
            INVENTORY_ITEM *const inv_item = ring->list[i];
            Matrix_Push();
            Matrix_RotY(angle);
            Matrix_TranslateRel(ring->radius, 0, 0);
            Matrix_RotY(DEG_90);
            Matrix_RotX(inv_item->x_rot_pt);
            M_DrawItem(ring, inv_item);
            angle += ring->angle_adder;
            Matrix_Pop();
        }
    }

    if (ring->list != nullptr && !ring->rotating
        && (ring->motion.status == RNG_OPEN
            || ring->motion.status == RNG_SELECTING
            || ring->motion.status == RNG_SELECTED
            || ring->motion.status == RNG_DESELECTING
            || ring->motion.status == RNG_DESELECT
            || ring->motion.status == RNG_CLOSING_ITEM)) {
        const INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
        switch (inv_item->object_id) {
        case O_MEDI_OPTION:
        case O_BIGMEDI_OPTION:
            if (g_Config.ui.enable_game_ui) {
                Overlay_BarDrawHealth();
            }
            break;

        default:
            break;
        }
    }

    Matrix_Pop();
    Viewport_SetFOV(old_fov);

    Output_ClearDepthBuffer();
    if (ring->motion.status == RNG_SELECTED) {
        INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
        if (inv_item->object_id == O_PASSPORT_CLOSED) {
            inv_item->object_id = O_PASSPORT_OPTION;
        }
        Option_Draw(inv_item);
    }

    if (ring->motion.status != RNG_DONE
        && (ring->motion.status != RNG_OPENING
            || (ring->mode != INV_TITLE_MODE
                || (!Fader_IsActive(&ring->top_fader)
                    && !Fader_IsActive(&ring->back_fader))))) {
        for (int32_t i = 0; i < num_frames; i++) {
            InvRing_DoMotions(ring);
        }
    }

    Fader_Draw(&ring->top_fader);
}
