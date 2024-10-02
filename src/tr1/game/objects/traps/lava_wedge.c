#include "game/objects/traps/lava_wedge.h"

#include "game/items.h"
#include "game/lara/misc.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/vars.h"

#define LAVA_WEDGE_SPEED 25

void LavaWedge_Setup(OBJECT *obj)
{
    obj->control = LavaWedge_Control;
    obj->collision = Object_Collision;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void LavaWedge_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }

    if (item->status != IS_DEACTIVATED) {
        int32_t x = item->pos.x;
        int32_t z = item->pos.z;

        switch (item->rot.y) {
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

        const SECTOR *const sector =
            Room_GetSector(x, item->pos.y, z, &room_num);
        if (Room_GetHeight(sector, x, item->pos.y, z) != item->pos.y) {
            item->status = IS_DEACTIVATED;
        }
    }

    if (g_Lara.water_status == LWS_CHEAT) {
        item->touch_bits = 0;
    }

    if (item->touch_bits) {
        if (g_LaraItem->hit_points > 0) {
            Lara_CatchFire();
        }

        g_Camera.item = item;
        g_Camera.flags = CHASE_OBJECT;
        g_Camera.type = CAM_FIXED;
        g_Camera.target_angle = -PHD_180;
        g_Camera.target_distance = WALL_L * 3;
    }
}
