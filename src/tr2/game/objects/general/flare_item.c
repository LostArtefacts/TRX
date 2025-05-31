#include "decomp/flares.h"
#include "game/objects/general/pickup.h"

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = Pickup_Collision;
    obj->bounds_func = Pickup_Bounds;
    obj->control_func = M_Control;
    obj->draw_func = Flare_DrawInAir;
    obj->save_position = true;
    obj->save_flags = true;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->fall_speed) {
        item->rot.x += DEG_1 * 3;
        item->rot.z += DEG_1 * 5;
    } else {
        item->rot.x = 0;
        item->rot.z = 0;
    }

    const int32_t x = item->pos.x;
    const int32_t y = item->pos.y;
    const int32_t z = item->pos.z;
    item->pos.z += (item->speed * Math_Cos(item->rot.y)) >> W2V_SHIFT;
    item->pos.x += (item->speed * Math_Sin(item->rot.y)) >> W2V_SHIFT;

    if (Room_Get(item->room_num)->flags & RF_UNDERWATER) {
        item->fall_speed += (5 - item->fall_speed) / 2;
        item->speed = item->speed + (5 - item->speed) / 2;
    } else {
        item->fall_speed += GRAVITY;
    }
    item->pos.y += item->fall_speed;

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    if (item->pos.y < height) {
        const int32_t ceiling =
            Room_GetCeiling(sector, item->pos.x, item->pos.y, item->pos.z);
        if (item->pos.y < ceiling) {
            item->fall_speed = -item->fall_speed;
            item->pos.y = ceiling;
        }
    } else {
        if (y > height) {
            item->pos.x = x;
            item->pos.y = y;
            item->pos.z = z;
            item->rot.y += DEG_180;
            item->speed /= 2;
            room_num = item->room_num;
        } else {
            if (item->fall_speed > 40) {
                item->fall_speed = 40 - item->fall_speed;
                CLAMPL(item->fall_speed, -100);
            } else {
                item->fall_speed = 0;
                item->speed -= 3;
                CLAMPL(item->speed, 0);
            }
            item->pos.y = height;
        }
    }

    Item_UpdateRoom(item_num, room_num);

    int32_t flare_age = ((int32_t)(intptr_t)item->data) & 0x7FFF;
    if (flare_age < Flare_GetMaxAge()) {
        flare_age++;
    } else if (item->fall_speed == 0 && item->speed == 0) {
        Item_Kill(item_num);
        return;
    }

    if (Flare_GenerateLight(item->pos, flare_age)) {
        flare_age |= 0x8000u;
        Flare_GenerateEffects(item->pos, item->pos, item->room_num);
    }

    item->data = (void *)(intptr_t)flare_age;
}

REGISTER_OBJECT(O_FLARE_ITEM, M_Setup)
