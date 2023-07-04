#include "game/objects/traps/lava.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

#define LAVA_EMBER_DAMAGE 10
#define LAVA_WEDGE_SPEED 25

bool Lava_TestFloor(ITEM_INFO *item)
{
    // OG fix: check if floor index has lava
    if (g_Lara.water_status == LWS_CHEAT) {
        return false;
    }

    if (item->hit_points < 0) {
        return false;
    }

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        Room_GetFloor(item->pos.x, 32000, item->pos.z, &room_num);

    int16_t *data = &g_FloorData[floor->index];
    int16_t type;
    do {
        type = *data++;

        switch (type & DATA_TYPE) {
        case FT_TILT: {
            data++;
            break;
        }

        case FT_ROOF:
        case FT_DOOR:
            data++;
            break;

        case FT_LAVA:
            return true;

        case FT_TRIGGER:
            data++;
            break;

        default:
            break;
        }
    } while (!(type & END_BIT));

    return false;
}

void Lava_Burn(ITEM_INFO *item)
{
    if (g_Lara.water_status == LWS_CHEAT) {
        return;
    }

    if (item->hit_points < 0) {
        return;
    }

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        Room_GetFloor(item->pos.x, 32000, item->pos.z, &room_num);
    int16_t height = Room_GetHeight(floor, item->pos.x, 32000, item->pos.z);

    if (item->floor != height) {
        return;
    }

    item->hit_points = -1;
    item->hit_status = 1;
    for (int i = 0; i < 10; i++) {
        int16_t fx_num = Effect_Create(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[fx_num];
            fx->object_number = O_FLAME;
            fx->frame_number =
                (g_Objects[O_FLAME].nmeshes * Random_GetControl()) / 0x7FFF;
            fx->counter = -1 - Random_GetControl() * 24 / 0x7FFF;
        }
    }
}

void Lava_Setup(OBJECT_INFO *obj)
{
    obj->control = Lava_Control;
}

void Lava_Control(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    fx->pos.z += (fx->speed * Math_Cos(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->pos.x += (fx->speed * Math_Sin(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->fall_speed += GRAVITY;
    fx->pos.y += fx->fall_speed;

    int16_t room_num = fx->room_number;
    FLOOR_INFO *floor =
        Room_GetFloor(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);
    if (fx->pos.y >= Room_GetHeight(floor, fx->pos.x, fx->pos.y, fx->pos.z)
        || fx->pos.y
            < Room_GetCeiling(floor, fx->pos.x, fx->pos.y, fx->pos.z)) {
        Effect_Kill(fx_num);
    } else if (Lara_IsNearItem(&fx->pos, 200)) {
        Lara_TakeDamage(LAVA_EMBER_DAMAGE, true);
        Effect_Kill(fx_num);
    } else if (room_num != fx->room_number) {
        Effect_NewRoom(fx_num, room_num);
    }
}

void LavaEmitter_Setup(OBJECT_INFO *obj)
{
    obj->control = LavaEmitter_Control;
    obj->draw_routine = Object_DrawDummyItem;
    obj->collision = Object_Collision;
    obj->save_flags = 1;
}

void LavaEmitter_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    int16_t fx_num = Effect_Create(item->room_number);
    if (fx_num != NO_ITEM) {
        FX_INFO *fx = &g_Effects[fx_num];
        fx->pos.x = item->pos.x;
        fx->pos.y = item->pos.y;
        fx->pos.z = item->pos.z;
        fx->pos.y_rot = (Random_GetControl() - 0x4000) * 2;
        fx->speed = Random_GetControl() >> 10;
        fx->fall_speed = -Random_GetControl() / 200;
        fx->frame_number = -4 * Random_GetControl() / 0x7FFF;
        fx->object_number = O_LAVA;
        Sound_Effect(SFX_LAVA_FOUNTAIN, &item->pos, SPM_NORMAL);
    }
}

void LavaWedge_Setup(OBJECT_INFO *obj)
{
    obj->control = LavaWedge_Control;
    obj->collision = Object_Collision;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void LavaWedge_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    int16_t room_num = item->room_number;
    Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_number) {
        Item_NewRoom(item_num, room_num);
    }

    if (item->status != IS_DEACTIVATED) {
        int32_t x = item->pos.x;
        int32_t z = item->pos.z;

        switch (item->pos.y_rot) {
        case 0:
            item->pos.z += LAVA_WEDGE_SPEED;
            z += 2 * WALL_L;
            break;
        case -PHD_180:
            item->pos.z -= LAVA_WEDGE_SPEED;
            z -= 2 * WALL_L;
            break;
        case PHD_90:
            item->pos.x += LAVA_WEDGE_SPEED;
            x += 2 * WALL_L;
            break;
        default:
            item->pos.x -= LAVA_WEDGE_SPEED;
            x -= 2 * WALL_L;
            break;
        }

        FLOOR_INFO *floor = Room_GetFloor(x, item->pos.y, z, &room_num);
        if (Room_GetHeight(floor, x, item->pos.y, z) != item->pos.y) {
            item->status = IS_DEACTIVATED;
        }
    }

    if (g_Lara.water_status == LWS_CHEAT) {
        item->touch_bits = 0;
    }

    if (item->touch_bits) {
        if (g_LaraItem->hit_points > 0) {
            Lava_Burn(g_LaraItem);
        }

        g_Camera.item = item;
        g_Camera.flags = CHASE_OBJECT;
        g_Camera.type = CAM_FIXED;
        g_Camera.target_angle = -PHD_180;
        g_Camera.target_distance = WALL_L * 3;
    }
}
