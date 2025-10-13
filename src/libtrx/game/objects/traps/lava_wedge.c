#include "config.h"
#include "game/camera.h"
#include "game/lara.h"
#include "game/objects.h"
#include "game/rooms.h"

#define LAVA_WEDGE_SPEED 25

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    Item_UpdateRoom(item_num, room_num);

    if (item->status != IS_DEACTIVATED) {
        int32_t x = item->pos.x;
        int32_t z = item->pos.z;

        switch (item->rot.y) {
        case 0:
            item->pos.z += LAVA_WEDGE_SPEED;
            z += 2 * WALL_L;
            break;
        case -DEG_180:
            item->pos.z -= LAVA_WEDGE_SPEED;
            z -= 2 * WALL_L;
            break;
        case DEG_90:
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

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status == LWS_CHEAT) {
        item->touch_bits = 0;
    }

    if (item->touch_bits) {
        const ITEM *const lara_item = Lara_GetItem();
        if (lara_item->hit_points > 0) {
            Lara_TouchLava();
        }

        if (g_Config.debug.enable_invulnerability) {
            return;
        }
        g_Camera.item = item;
        g_Camera.flags = CF_CHASE_OBJECT;
        g_Camera.type = CAM_FIXED;
        g_Camera.target_angle = -DEG_180;
        g_Camera.target_distance = WALL_L * 3;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->save_position = true;
    obj->save_anim = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_LAVA_WEDGE, M_Setup)
