#include "game/camera.h"
#include "game/collide.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#define ROLLINGBALL_DAMAGE_AIR 100

static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    GAME_VECTOR *data = GameBuf_Alloc(sizeof(GAME_VECTOR), GBUF_ITEM_DATA);
    item->data = data;
    data->x = item->pos.x;
    data->y = item->pos.y;
    data->z = item->pos.z;
    data->room_num = item->room_num;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status == IS_ACTIVE) {
        if (item->pos.y < item->floor) {
            if (!item->gravity) {
                item->gravity = 1;
                item->fall_speed = -10;
            }
        } else if (item->current_anim_state == TRAP_SET) {
            item->goal_anim_state = TRAP_ACTIVATE;
        }

        int32_t oldx = item->pos.x;
        int32_t oldz = item->pos.z;
        Item_Animate(item);

        int16_t room_num = item->room_num;
        const SECTOR *sector =
            Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
        if (item->room_num != room_num) {
            Item_NewRoom(item_num, room_num);
        }

        item->floor =
            Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

        Room_TestTriggers(item);

        if (item->pos.y >= item->floor - STEP_L) {
            item->gravity = 0;
            item->fall_speed = 0;
            item->pos.y = item->floor;
        }

        int32_t x =
            item->pos.x + (((WALL_L / 2) * Math_Sin(item->rot.y)) >> W2V_SHIFT);
        int32_t z =
            item->pos.z + (((WALL_L / 2) * Math_Cos(item->rot.y)) >> W2V_SHIFT);
        sector = Room_GetSector(x, item->pos.y, z, &room_num);
        if (Room_GetHeight(sector, x, item->pos.y, z) < item->pos.y) {
            item->status = IS_DEACTIVATED;
            item->pos.x = oldx;
            item->pos.y = item->floor;
            item->pos.z = oldz;
            item->speed = 0;
            item->fall_speed = 0;
            item->touch_bits = 0;
        }
    } else if (item->status == IS_DEACTIVATED && !Item_IsTriggerActive(item)) {
        item->status = IS_INACTIVE;
        GAME_VECTOR *data = item->data;
        item->pos.x = data->x;
        item->pos.y = data->y;
        item->pos.z = data->z;
        if (item->room_num != data->room_num) {
            Item_RemoveDrawn(item_num);
            ROOM *const room = Room_Get(data->room_num);
            item->next_item = room->item_num;
            room->item_num = item_num;
            item->room_num = data->room_num;
        }
        item->current_anim_state = TRAP_SET;
        item->goal_anim_state = TRAP_SET;
        Item_SwitchToAnim(item, 0, 0);
        item->current_anim_state = Item_GetAnim(item)->current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        item->required_anim_state = TRAP_SET;
        Item_RemoveActive(item_num);
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status != IS_ACTIVE) {
        if (item->status != IS_INVISIBLE) {
            Object_Collision(item_num, lara_item, coll);
        }
        return;
    }

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    int32_t x, y, z, d;
    if (lara_item->gravity) {
        if (coll->enable_baddie_push) {
            Lara_Push(item, coll, coll->enable_hit, true);
        }
        lara_item->hit_points -= ROLLINGBALL_DAMAGE_AIR;
        x = lara_item->pos.x - item->pos.x;
        z = lara_item->pos.z - item->pos.z;
        y = (lara_item->pos.y - 350) - (item->pos.y - WALL_L / 2);
        d = Math_Sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z));
        if (d < WALL_L / 2) {
            d = WALL_L / 2;
        }
        x = item->pos.x + (x << WALL_SHIFT) / 2 / d;
        z = item->pos.z + (z << WALL_SHIFT) / 2 / d;
        y = item->pos.y - WALL_L / 2 + (y << WALL_SHIFT) / 2 / d;
        Spawn_Blood(x, y, z, item->speed, item->rot.y, item->room_num);
    } else {
        lara_item->hit_status = 1;
        if (lara_item->hit_points > 0) {
            lara_item->hit_points = -1;
            if (lara_item->room_num != item->room_num) {
                Item_NewRoom(g_Lara.item_num, item->room_num);
            }

            lara_item->rot.x = 0;
            lara_item->rot.z = 0;
            lara_item->rot.y = item->rot.y;

            lara_item->current_anim_state = LS_SPECIAL;
            lara_item->goal_anim_state = LS_SPECIAL;
            Item_SwitchToAnim(lara_item, LA_ROLLING_BALL_DEATH, 0);

            g_Camera.flags = CF_FOLLOW_CENTRE;
            g_Camera.target_angle = 170 * DEG_1;
            g_Camera.target_elevation = -25 * DEG_1;
            for (int i = 0; i < 15; i++) {
                x = lara_item->pos.x + (Random_GetControl() - 0x4000) / 256;
                z = lara_item->pos.z + (Random_GetControl() - 0x4000) / 256;
                y = lara_item->pos.y - Random_GetControl() / 64;
                d = item->rot.y + (Random_GetControl() - 0x4000) / 8;
                Spawn_Blood(x, y, z, item->speed * 2, d, item->room_num);
            }
        }
    }
}

REGISTER_OBJECT(O_ROLLING_BALL, M_Setup)
