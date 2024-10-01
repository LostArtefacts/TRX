#include "game/objects/traps/rolling_ball.h"

#include "game/collide.h"
#include "game/effects/blood.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

#include <libtrx/utils.h>

#define ROLLINGBALL_DAMAGE_AIR 100

void RollingBall_Setup(OBJECT *obj)
{
    obj->initialise = RollingBall_Initialise;
    obj->control = RollingBall_Control;
    obj->collision = RollingBall_Collision;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void RollingBall_Initialise(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];
    GAME_VECTOR *data = GameBuf_Alloc(sizeof(GAME_VECTOR), GBUF_TRAP_DATA);
    item->data = data;
    data->x = item->pos.x;
    data->y = item->pos.y;
    data->z = item->pos.z;
    data->room_num = item->room_num;
}

void RollingBall_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];
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
            ROOM *r = &g_RoomInfo[data->room_num];
            item->next_item = r->item_num;
            r->item_num = item_num;
            item->room_num = data->room_num;
        }
        item->current_anim_state = TRAP_SET;
        item->goal_anim_state = TRAP_SET;
        Item_SwitchToAnim(item, 0, 0);
        item->current_anim_state = g_Anims[item->anim_num].current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        item->required_anim_state = TRAP_SET;
        Item_RemoveActive(item_num);
    }
}

void RollingBall_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *item = &g_Items[item_num];

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
            Lara_Push(item, coll, coll->enable_spaz, true);
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
        Effect_Blood(x, y, z, item->speed, item->rot.y, item->room_num);
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

            g_Camera.flags = FOLLOW_CENTRE;
            g_Camera.target_angle = 170 * PHD_DEGREE;
            g_Camera.target_elevation = -25 * PHD_DEGREE;
            for (int i = 0; i < 15; i++) {
                x = lara_item->pos.x + (Random_GetControl() - 0x4000) / 256;
                z = lara_item->pos.z + (Random_GetControl() - 0x4000) / 256;
                y = lara_item->pos.y - Random_GetControl() / 64;
                d = item->rot.y + (Random_GetControl() - 0x4000) / 8;
                Effect_Blood(x, y, z, item->speed * 2, d, item->room_num);
            }
        }
    }
}
