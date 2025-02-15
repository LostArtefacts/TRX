#include "game/inventory_ring/draw.h"

#include "decomp/savegame.h"
#include "game/console/common.h"
#include "game/input.h"
#include "game/inventory_ring/control.h"
#include "game/option/option.h"
#include "game/output.h"
#include "game/overlay.h"
#include "global/vars.h"

#include <libtrx/game/inventory_ring/priv.h>
#include <libtrx/game/matrix.h>

static void M_DrawItem(const INV_RING *ring, const INVENTORY_ITEM *inv_item);

static void M_DrawItem(
    const INV_RING *const ring, const INVENTORY_ITEM *const inv_item)
{
    if (ring->motion.status != RNG_FADING_OUT && ring->motion.status != RNG_DONE
        && inv_item == ring->list[ring->current_object] && !ring->rotating) {
        Output_SetLightAdder(HIGH_LIGHT);
    } else {
        Output_SetLightAdder(LOW_LIGHT);
    }

    int32_t minutes;
    int32_t hours;
    int32_t seconds;
    if (inv_item->object_id == O_COMPASS_OPTION) {
        const int32_t total_seconds =
            g_SaveGame.current_stats.timer / FRAMES_PER_SECOND;
        hours = (total_seconds % 43200) * DEG_1 * -360 / 43200;
        minutes = (total_seconds % 3600) * DEG_1 * -360 / 3600;
        seconds = (total_seconds % 60) * DEG_1 * -360 / 60;
    } else {
        seconds = 0;
        minutes = 0;
        hours = 0;
    }

    Matrix_TranslateRel(0, inv_item->y_trans, inv_item->z_trans);
    Matrix_RotY(inv_item->y_rot);
    Matrix_RotX(inv_item->x_rot);
    const OBJECT *const obj = Object_Get(inv_item->object_id);
    if ((obj->flags & 1) == 0) {
        return;
    }

    if (obj->mesh_count < 0) {
        Output_DrawSprite(0, 0, 0, 0, obj->mesh_idx, 0, 0);
        return;
    }

    ANIM_FRAME *frame_ptr = &obj->frame_base[inv_item->current_frame];

    Matrix_Push();
    const int32_t clip = Output_GetObjectBounds(&frame_ptr->bounds);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Matrix_TranslateRel16(frame_ptr->offset);
    Matrix_Rot16(frame_ptr->mesh_rots[0]);

    for (int32_t mesh_idx = 0; mesh_idx < obj->mesh_count; mesh_idx++) {
        if (mesh_idx > 0) {
            const ANIM_BONE *const bone = Object_GetBone(obj, mesh_idx - 1);
            if (bone->matrix_pop) {
                Matrix_Pop();
            }
            if (bone->matrix_push) {
                Matrix_Push();
            }

            Matrix_TranslateRel32(bone->pos);
            Matrix_Rot16(frame_ptr->mesh_rots[mesh_idx]);

            if (inv_item->object_id == O_COMPASS_OPTION) {
                if (mesh_idx == 6) {
                    Matrix_RotZ(seconds);
#if 0
                    const int32_t tmp = inv_item->reserved[0];
                    inv_item->reserved[0] = seconds;
                    inv_item->reserved[1] = tmp;
#endif
                }
                if (mesh_idx == 5) {
                    Matrix_RotZ(minutes);
                }
                if (mesh_idx == 4) {
                    Matrix_RotZ(hours);
                }
            }
        }

        if (inv_item->meshes_drawn & (1 << mesh_idx)) {
            Object_DrawMesh(obj->mesh_idx + mesh_idx, clip, false);
        }
    }

    Matrix_Pop();
}

void InvRing_Draw(INV_RING *const ring)
{
    const int32_t num_frames = round(
        ClockTimer_TakeElapsed(&ring->motion_timer) * LOGIC_FPS
        * INV_RING_FRAMES);

    ring->camera.pos.z = ring->radius + 598;

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
        const INVENTORY_ITEM *const inv_item = ring->list[ring->current_object];
        if (inv_item != nullptr) {
            switch (inv_item->object_id) {
            case O_SMALL_MEDIPACK_OPTION:
            case O_LARGE_MEDIPACK_OPTION:
                Overlay_DrawHealthBar();
                break;

            default:
                break;
            }
        }
    }

    Matrix_Pop();
    Output_DrawPolyList();

    if (ring->motion.status == RNG_SELECTED) {
        INVENTORY_ITEM *const inv_item = ring->list[ring->current_object];
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
