#include <trx/game/inventory_ring/draw.h>

#include <trx/config.h>
#include <trx/game/game.h>
#include <trx/game/input.h>
#include <trx/game/interpolation.h>
#include <trx/game/inventory_ring.h>
#include <trx/game/inventory_ring/priv.h>
#include <trx/game/inventory_ring/vars.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/option.h>
#include <trx/game/option/stats.h>
#include <trx/game/output.h>
#include <trx/game/savegame.h>
#include <trx/game/shell.h>
#include <trx/game/viewport.h>
#include <trx/utils.h>
#include <trx/version.h>

#include <math.h>

#define M_CAMERA_2_RING 598

static bool M_IsEnterTransition(const INV_RING *const ring)
{
    return ring->motion.status == RNG_OPENING;
}

static bool M_IsExitTransition(const INV_RING *const ring)
{
    return ring->motion.status == RNG_EXITING_INVENTORY
        || ring->motion.status == RNG_FADING_OUT
        || ring->motion.status == RNG_DONE
        || (ring->motion.status == RNG_CLOSING
            && (ring->motion.status_target == RNG_FADING_OUT
                || ring->motion.status_target == RNG_DONE));
}

static float M_GetMotionProgress(
    const int16_t frames_remaining, const int16_t total_frames)
{
    if (total_frames == 0) {
        return 1.0f;
    }
    float result = frames_remaining / (float)total_frames;
    CLAMP(result, 0.0f, 1.0f);
    return result;
}

static int32_t M_GetFrames(
    const INV_RING *const ring, const INVENTORY_ITEM *const inv_item,
    ANIM_FRAME **const out_frame1, ANIM_FRAME **const out_frame2,
    int32_t *const out_rate)
{
    const OBJECT *const obj = Object_Get(inv_item->object_id);
    const INVENTORY_ITEM *const cur_inv_item = ring->list[ring->current_object];

    if (inv_item != cur_inv_item || inv_item->current_frame == 0
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
    return (Interpolation_GetWorldRate() - 0.5) * 10.0;

    // OG
fallback:
    *out_frame1 = &obj->frame_base[inv_item->current_frame];
    *out_frame2 = *out_frame1;
    *out_rate = 1;
    return 0;
}

static void M_DrawItem(
    const INV_RING *const ring, const INVENTORY_ITEM *const inv_item)
{
    if (ring->motion.status != RNG_FADING_OUT && ring->motion.status != RNG_DONE
        && inv_item == ring->list[ring->current_object] && !ring->rotating) {
        Output_SetLightAdder(SHADE_NEUTRAL);
    } else {
        Output_SetLightAdder(SHADE_LOW);
    }

    Matrix_TranslateRel(0, inv_item->y_trans, inv_item->z_trans);
    Matrix_RotY(inv_item->y_rot);
    Matrix_RotX(inv_item->x_rot);

    const OBJECT *const obj = Object_Get(inv_item->object_id);
    if (!obj->loaded || obj->mesh_count < 0) {
        return;
    }

    int32_t rate;
    ANIM_FRAME *frame1;
    ANIM_FRAME *frame2;
    const int32_t frac = M_GetFrames(ring, inv_item, &frame1, &frame2, &rate);
    if (inv_item->object_id == O_COMPASS_OPTION) {
        const int16_t extra_rotation[1] = {
            Option_Stats_GetCompassNeedleAngle()
        };
        Object_GetBone(obj, 0)->rot.y = true;
        Object_DrawInterpolatedObject(
            obj, inv_item->meshes_drawn, extra_rotation, frame1, frame2, frac,
            rate);
    } else if (inv_item->object_id == O_STOPWATCH_OPTION) {
        const RESUME_INFO *const current_info =
            Savegame_GetCurrentInfo(Game_GetCurrentLevel());
        const int32_t total_seconds = current_info->stats.timer / LOGIC_FPS;
        const int32_t hours = (total_seconds % 43200) * DEG_1 * -360 / 43200;
        const int32_t minutes = (total_seconds % 3600) * DEG_1 * -360 / 3600;
        const int32_t seconds = (total_seconds % 60) * DEG_1 * -360 / 60;

        const int16_t extra_rotation[3] = { hours, minutes, seconds };
        Object_GetBone(obj, 3)->rot.z = true;
        Object_GetBone(obj, 4)->rot.z = true;
        Object_GetBone(obj, 5)->rot.z = true;
        Object_DrawInterpolatedObject(
            obj, inv_item->meshes_drawn, extra_rotation, frame1, frame2, frac,
            rate);
    } else {
        Object_DrawInterpolatedObject(
            obj, inv_item->meshes_drawn, nullptr, frame1, frame2, frac, rate);
    }
}

const INVENTORY_ITEM *InvRing_GetInvItem(const OBJECT_ID obj_id)
{
    for (int32_t i = 0; i < g_InvRing_Items->count; i++) {
        INVENTORY_ITEM *const item =
            *(INVENTORY_ITEM **)Vector_Get(g_InvRing_Items, i);
        if (item->object_id == obj_id) {
            return item;
        }
    }
    return nullptr;
}

void InvRing_Draw(INV_RING *const ring)
{
    InvRing_DrawUI(ring);

    const int32_t num_frames = round(
        ClockTimer_TakeElapsed(&ring->motion_timer) * LOGIC_FPS
        * INV_RING_FRAMES);

    if (ring->motion.status != RNG_OPENING && ring->motion.status != RNG_DONE
        && ring->motion.status != RNG_FADING_OUT) {
        for (int32_t i = 0; i < ring->number_of_objects; i++) {
            InvRing_UpdateInventoryItem(ring, ring->list[i], num_frames);
        }
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

    ring->camera.pos.z = ring->radius + M_CAMERA_2_RING;

    if (ring->mode == INV_TITLE_MODE) {
        if (ring->background_path != nullptr) {
            Output_Overlay_DrawImage(ring->background_path);
        }
        Interpolation_Interpolate();
    } else {
        const float opacity = g_Config.ui.inventory_fade_effects
            ? Fader_GetCurrentValue(&ring->back_fader)
            : ring->back_fader.args.target;

        switch (ring->background_style) {
        case BK_TRANSPARENT_MEDIUM:
            Output_Overlay_DrawGame();
            Output_Overlay_DrawBlackRectangle(opacity * 0.5f, false);
            break;

        case BK_TRANSPARENT_DARK:
            Output_Overlay_DrawGame();
            Output_Overlay_DrawBlackRectangle(opacity * 0.8f, false);
            break;

        case BK_MONOCHROME:
            Output_Overlay_DrawGameMono(opacity);
            break;

        case BK_PATTERN_STATIC:
        case BK_PATTERN_WAVE:
            if (opacity < 1.0f) {
                Output_Overlay_DrawGame();
            }
            Output_Overlay_DrawPatternOpacity(
                ring->background_style == BK_PATTERN_WAVE, opacity);
            break;

        case BK_IMAGE:
            if (ring->background_path != nullptr
                && Output_Overlay_LoadImage(ring->background_path)) {
                Output_Overlay_DrawImage(ring->background_path);
                Output_Overlay_DrawBlackRectangle(1.0f - opacity, false);
            } else {
                Output_Overlay_DrawBlackRectangle(1.0f, false);
            }
            break;

        default:
            Output_Overlay_DrawGame();
            break;
        }
    }
    Output_Flush();

    const int16_t old_fov = Viewport_GetSystemFOV();
    const FOV_MODE old_fov_mode = Viewport_GetFOVMode();
    Viewport_AlterFOV(FOV_VALUE_PASSPORT * DEG_1, FOV_MODE_PASSPORT);
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

    Matrix_Pop();
    SceneCompositor_Flush();
    Viewport_AlterFOV(old_fov, old_fov_mode);

    if (ring->motion.status == RNG_SELECTED) {
        INVENTORY_ITEM *const inv_item = ring->list[ring->current_object];
        if (inv_item->object_id == O_PASSPORT_CLOSED) {
            inv_item->object_id = O_PASSPORT_OPTION;
        }
        Option_Draw(inv_item);
    }

    Output_Overlay_DrawBlackRectangle(
        Fader_GetCurrentValue(&ring->top_fader), true);
}
