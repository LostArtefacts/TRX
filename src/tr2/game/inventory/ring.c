#include "game/inventory/ring.h"

#include "game/math_misc.h"
#include "game/output.h"
#include "global/funcs.h"
#include "global/vars.h"

#define RING_OPEN_FRAMES 32
#define RING_OPEN_ROTATION PHD_180
#define RING_ROTATE_DURATION 24
#define RING_RADIUS 688
#define RING_CAMERA_START_HEIGHT (-1536)
#define RING_CAMERA_HEIGHT (-256)
#define RING_CAMERA_Y_OFFSET (-96)

void __cdecl Inv_Ring_Init(
    RING_INFO *const ring, const RING_TYPE type, INVENTORY_ITEM **const list,
    const int16_t qty, const int16_t current, IMOTION_INFO *const imo)
{
    ring->type = type;
    ring->list = list;
    ring->number_of_objects = qty;
    ring->current_object = current;
    ring->radius = 0;
    ring->angle_adder = 0x10000 / qty;

    if (g_Inv_Mode == INV_TITLE_MODE) {
        ring->camera_pitch = 1024;
    } else {
        ring->camera_pitch = 0;
    }

    ring->rotating = 0;
    ring->rot_count = 0;
    ring->target_object = 0;
    ring->rot_adder = 0;
    ring->rot_adder_l = 0;
    ring->rot_adder_r = 0;

    ring->imo = imo;

    ring->camera.pos.x = 0;
    ring->camera.pos.y = RING_CAMERA_START_HEIGHT;
    ring->camera.pos.z = 896;
    ring->camera.rot.x = 0;
    ring->camera.rot.y = 0;
    ring->camera.rot.z = 0;

    Inv_Ring_MotionInit(ring, RING_OPEN_FRAMES, RNG_OPENING, RNG_OPEN);
    Inv_Ring_MotionRadius(ring, RING_RADIUS);
    Inv_Ring_MotionCameraPos(ring, RING_CAMERA_HEIGHT);
    Inv_Ring_MotionRotation(
        ring, RING_OPEN_ROTATION,
        -PHD_90 - ring->current_object * ring->angle_adder);

    ring->ring_pos.pos.x = 0;
    ring->ring_pos.pos.y = 0;
    ring->ring_pos.pos.z = 0;
    ring->ring_pos.rot.x = 0;
    ring->ring_pos.rot.y = imo->rotate_target + RING_OPEN_ROTATION;
    ring->ring_pos.rot.z = 0;

    ring->light.x = -1536;
    ring->light.y = 256;
    ring->light.z = 1024;
}

void __cdecl Inv_Ring_GetView(
    const RING_INFO *const ring, PHD_3DPOS *const view)
{
    int16_t angles[2];

    Math_GetVectorAngles(
        -ring->camera.pos.x, RING_CAMERA_Y_OFFSET - ring->camera.pos.y,
        ring->radius - ring->camera.pos.z, angles);
    view->pos.x = ring->camera.pos.x;
    view->pos.y = ring->camera.pos.y;
    view->pos.z = ring->camera.pos.z;
    view->rot.x = angles[1] + ring->camera_pitch;
    view->rot.y = angles[0];
    view->rot.z = 0;
}

void __cdecl Inv_Ring_Light(const RING_INFO *const ring)
{
    g_LsDivider = 0x6000;
    int16_t angles[2];
    Math_GetVectorAngles(ring->light.x, ring->light.y, ring->light.z, angles);
    Output_RotateLight(angles[1], angles[0]);
}

void __cdecl Inv_Ring_CalcAdders(
    RING_INFO *const ring, const int16_t rotation_duration)
{
    ring->angle_adder = PHD_360 / ring->number_of_objects;
    ring->rot_adder_l = ring->angle_adder / rotation_duration;
    ring->rot_adder_r = -ring->angle_adder / rotation_duration;
}

void __cdecl Inv_Ring_DoMotions(RING_INFO *const ring)
{
    IMOTION_INFO *const imo = ring->imo;

    if (imo->count != 0) {
        ring->radius += imo->radius_rate;
        ring->camera.pos.y += imo->camera_y_rate;
        ring->ring_pos.rot.y += imo->rotate_rate;
        ring->camera_pitch += imo->camera_pitch_rate;

        INVENTORY_ITEM *const inv_item = ring->list[ring->current_object];
        inv_item->x_rot_pt += imo->item_pt_x_rot_rate;
        inv_item->x_rot += imo->item_x_rot_rate;
        inv_item->y_trans += imo->item_y_trans_rate;
        inv_item->z_trans += imo->item_z_trans_rate;

        imo->count--;
        if (imo->count == 0) {
            imo->status = imo->status_target;

            if (imo->radius_rate != 0) {
                imo->radius_rate = 0;
                ring->radius = imo->radius_target;
            }
            if (imo->camera_y_rate != 0) {
                imo->camera_y_rate = 0;
                ring->camera.pos.y = imo->camera_y_target;
            }
            if (imo->rotate_rate != 0) {
                imo->rotate_rate = 0;
                ring->ring_pos.rot.y = imo->rotate_target;
            }
            if (imo->item_pt_x_rot_rate != 0) {
                imo->item_pt_x_rot_rate = 0;
                inv_item->x_rot_pt = imo->item_pt_x_rot_target;
            }
            if (imo->item_x_rot_rate != 0) {
                imo->item_x_rot_rate = 0;
                inv_item->x_rot = imo->item_x_rot_target;
            }
            if (imo->item_y_trans_rate != 0) {
                imo->item_y_trans_rate = 0;
                inv_item->y_trans = imo->item_y_trans_target;
            }
            if (imo->item_z_trans_rate != 0) {
                imo->item_z_trans_rate = 0;
                inv_item->z_trans = imo->item_z_trans_target;
            }
            if (imo->camera_pitch_rate != 0) {
                imo->camera_pitch_rate = 0;
                ring->camera_pitch = imo->camera_pitch_target;
            }
        }
    }

    if (ring->rotating) {
        ring->ring_pos.rot.y += ring->rot_adder;
        ring->rot_count--;

        if (ring->rot_count == 0) {
            ring->current_object = ring->target_object;
            ring->ring_pos.rot.y =
                -PHD_90 - ring->target_object * ring->angle_adder;
            ring->rotating = 0;
        }
    }
}

void __cdecl Inv_Ring_RotateLeft(RING_INFO *const ring)
{
    ring->rotating = 1;
    if (ring->current_object == 0) {
        ring->target_object = ring->number_of_objects - 1;
    } else {
        ring->target_object = ring->current_object - 1;
    }
    ring->rot_count = RING_ROTATE_DURATION;
    ring->rot_adder = ring->rot_adder_l;
}

void __cdecl Inv_Ring_RotateRight(RING_INFO *const ring)
{
    ring->rotating = 1;
    if (ring->current_object + 1 >= ring->number_of_objects) {
        ring->target_object = 0;
    } else {
        ring->target_object = ring->current_object + 1;
    }
    ring->rot_count = RING_ROTATE_DURATION;
    ring->rot_adder = ring->rot_adder_r;
}

void __cdecl Inv_Ring_MotionInit(
    RING_INFO *const ring, const int16_t frames, const RING_STATUS status,
    const RING_STATUS status_target)
{
    IMOTION_INFO *const imo = ring->imo;
    imo->count = frames;
    imo->status = status;
    imo->status_target = status_target;

    imo->radius_target = 0;
    imo->radius_rate = 0;
    imo->camera_y_target = 0;
    imo->camera_y_rate = 0;
    imo->camera_pitch_target = 0;
    imo->camera_pitch_rate = 0;
    imo->rotate_target = 0;
    imo->rotate_rate = 0;
    imo->item_pt_x_rot_target = 0;
    imo->item_pt_x_rot_rate = 0;
    imo->item_x_rot_target = 0;
    imo->item_x_rot_rate = 0;
    imo->item_y_trans_target = 0;
    imo->item_y_trans_rate = 0;
    imo->item_z_trans_target = 0;
    imo->item_z_trans_rate = 0;

    imo->misc = 0;
}

void __cdecl Inv_Ring_MotionSetup(
    RING_INFO *const ring, const RING_STATUS status,
    const RING_STATUS status_target, const int16_t frames)
{
    IMOTION_INFO *const imo = ring->imo;
    imo->count = frames;
    imo->status = status;
    imo->status_target = status_target;
    imo->radius_rate = 0;
    imo->camera_y_rate = 0;
}

void __cdecl Inv_Ring_MotionRadius(RING_INFO *const ring, const int16_t target)
{
    IMOTION_INFO *const imo = ring->imo;
    imo->radius_target = target;
    imo->radius_rate = (target - ring->radius) / imo->count;
}

void __cdecl Inv_Ring_MotionRotation(
    RING_INFO *const ring, const int16_t rotation, const int16_t target)
{
    IMOTION_INFO *const imo = ring->imo;
    imo->rotate_target = target;
    imo->rotate_rate = rotation / imo->count;
}

void __cdecl Inv_Ring_MotionCameraPos(
    RING_INFO *const ring, const int16_t target)
{
    IMOTION_INFO *const imo = ring->imo;
    imo->camera_y_target = target;
    imo->camera_y_rate = (target - ring->camera.pos.y) / imo->count;
}

void __cdecl Inv_Ring_MotionCameraPitch(
    RING_INFO *const ring, const int16_t target)
{
    IMOTION_INFO *const imo = ring->imo;
    imo->camera_pitch_target = target;
    imo->camera_pitch_rate = target / imo->count;
}

void __cdecl Inv_Ring_MotionItemSelect(
    RING_INFO *const ring, const INVENTORY_ITEM *const inv_item)
{
    IMOTION_INFO *const imo = ring->imo;
    imo->item_pt_x_rot_target = inv_item->x_rot_pt_sel;
    imo->item_pt_x_rot_rate = inv_item->x_rot_pt_sel / imo->count;
    imo->item_x_rot_target = inv_item->x_rot_sel;
    imo->item_x_rot_rate =
        (inv_item->x_rot_sel - inv_item->x_rot_nosel) / imo->count;
    imo->item_y_trans_target = inv_item->y_trans_sel;
    imo->item_y_trans_rate = inv_item->y_trans_sel / imo->count;
    imo->item_z_trans_target = inv_item->z_trans_sel;
    imo->item_z_trans_rate = inv_item->z_trans_sel / imo->count;
}

void __cdecl Inv_Ring_MotionItemDeselect(
    RING_INFO *const ring, const INVENTORY_ITEM *const inv_item)
{
    IMOTION_INFO *const imo = ring->imo;
    imo->item_pt_x_rot_target = 0;
    imo->item_pt_x_rot_rate = -(inv_item->x_rot_pt_sel / imo->count);
    imo->item_x_rot_target = inv_item->x_rot_nosel;
    imo->item_x_rot_rate =
        (inv_item->x_rot_nosel - inv_item->x_rot_sel) / imo->count;
    imo->item_y_trans_target = 0;
    imo->item_y_trans_rate = -(inv_item->y_trans_sel / imo->count);
    imo->item_z_trans_target = 0;
    imo->item_z_trans_rate = -(inv_item->z_trans_sel / imo->count);
}
