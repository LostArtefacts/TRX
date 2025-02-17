#include "game/objects/general/sphere_of_doom.h"

#include "game/items.h"
#include "game/output.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

#define SPHERE_OF_DOOM_RADIUS (STEP_L * 5 / 2) // = 640

void SphereOfDoom_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (Room_Get(lara_item->room_num)->flags & RF_UNDERWATER) {
        return;
    }

    const ITEM *const item = Item_Get(item_num);
    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
    const int32_t radius = (SPHERE_OF_DOOM_RADIUS * item->timer) >> 8;

    if (SQUARE(dx) + SQUARE(dz) >= SQUARE(radius)) {
        return;
    }

    const int16_t angle = Math_Atan(dz, dx);
    const int16_t diff = lara_item->rot.y - angle;
    if (ABS(diff) < DEG_90) {
        lara_item->speed = 150;
        lara_item->rot.y = angle;
    } else {
        lara_item->speed = -150;
        lara_item->rot.y = angle + DEG_180;
    }

    lara_item->gravity = 1;
    lara_item->fall_speed = -50;
    lara_item->pos.x =
        item->pos.x + (((radius + 50) * Math_Sin(angle)) >> W2V_SHIFT);
    lara_item->pos.z =
        item->pos.z + (((radius + 50) * Math_Cos(angle)) >> W2V_SHIFT);
    lara_item->rot.x = 0;
    lara_item->rot.z = 0;
    Item_SwitchToAnim(lara_item, LA_FALL_START, 0);
    lara_item->current_anim_state = LS_FORWARD_JUMP;
    lara_item->goal_anim_state = LS_FORWARD_JUMP;
}

void SphereOfDoom_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->timer += 64;
    item->rot.y +=
        item->object_id == O_SPHERE_OF_DOOM_2 ? DEG_1 * 10 : -DEG_1 * 10;
    const int32_t dx = item->pos.x - g_LaraItem->pos.x;
    const int32_t dy = item->pos.y - g_LaraItem->pos.y;
    const int32_t dz = item->pos.z - g_LaraItem->pos.z;
    const int32_t radius = (SPHERE_OF_DOOM_RADIUS * item->timer) >> 8;
    const int32_t dist = Math_Sqrt(SQUARE(dx) + SQUARE(dy) + SQUARE(dz));
    XYZ_32 pos = g_LaraItem->pos;
    pos.x += ((dist - radius) * dx) / radius;
    pos.y += ((dist - radius) * dy) / radius;
    pos.z += ((dist - radius) * dz) / radius;
    Sound_Effect(SFX_MARCO_BARTOLLI_TRANSFORM, &pos, SPM_NORMAL);
    if (item->timer > 60 * 64) {
        Item_Kill(item_num);
    }
}

void SphereOfDoom_Draw(const ITEM *const item)
{
    Matrix_Push();
    Matrix_TranslateAbs32(item->pos);
    Matrix_RotY(item->rot.y);

    MATRIX *const mptr = g_MatrixPtr;
    mptr->_00 = (mptr->_00 * item->timer) >> 8;
    mptr->_01 = (mptr->_01 * item->timer) >> 8;
    mptr->_02 = (mptr->_02 * item->timer) >> 8;
    mptr->_10 = (mptr->_10 * item->timer) >> 8;
    mptr->_11 = (mptr->_11 * item->timer) >> 8;
    mptr->_12 = (mptr->_12 * item->timer) >> 8;
    mptr->_20 = (mptr->_20 * item->timer) >> 8;
    mptr->_21 = (mptr->_21 * item->timer) >> 8;
    mptr->_22 = (mptr->_22 * item->timer) >> 8;

    const ANIM_FRAME *const frame_ptr = Item_GetAnim(item)->frame_ptr;
    const int32_t clip = Output_GetObjectBounds(&frame_ptr->bounds);
    if (clip) {
        Output_CalculateObjectLighting(item, &frame_ptr->bounds);
        Object_DrawMesh(Object_Get(item->object_id)->mesh_idx, clip, false);
    }

    Matrix_Pop();
}

void SphereOfDoom_Setup(OBJECT *const obj, const bool transparent)
{
    obj->collision_func = SphereOfDoom_Collision;
    obj->control_func = SphereOfDoom_Control;
    obj->draw_func = SphereOfDoom_Draw;
    obj->save_position = 1;
    obj->save_flags = 1;
    if (transparent) {
        obj->semi_transparent = 1;
    }
}
