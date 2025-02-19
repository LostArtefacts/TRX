#include "game/camera.h"
#include "game/collide.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#define ROLLING_BALL_DAMAGE_AIR 100
#define ROLL_SHAKE_RANGE (WALL_L * 10) // = 10240

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
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    GAME_VECTOR *const data =
        GameBuf_Alloc(sizeof(GAME_VECTOR), GBUF_ITEM_DATA);
    data->pos = item->pos;
    data->room_num = item->room_num;
    item->data = data;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_ACTIVE) {
        if (item->goal_anim_state == TRAP_WORKING) {
            Item_Animate(item);
            return;
        }

        if (item->pos.y < item->floor) {
            if (!item->gravity) {
                item->gravity = 1;
                item->fall_speed = -10;
            }
        } else if (item->current_anim_state == TRAP_SET) {
            item->goal_anim_state = TRAP_ACTIVATE;
        }

        const XYZ_32 old_pos = item->pos;
        Item_Animate(item);

        int16_t room_num = item->room_num;
        const SECTOR *const sector =
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
            if (item->object_id == O_ROLLING_BALL_2) {
                Sound_Effect(SFX_SNOWBALL_ROLL, &item->pos, SPM_NORMAL);
            } else if (item->object_id == O_ROLLING_BALL_3) {
                Sound_Effect(SFX_ROLLING_2, &item->pos, SPM_NORMAL);
            } else {
                Sound_Effect(SFX_ROLLING_BALL, &item->pos, SPM_NORMAL);
            }
            const int32_t dist = Math_Sqrt(
                (g_Camera.mic_pos.z - item->pos.z)
                    * (g_Camera.mic_pos.z - item->pos.z)
                + (g_Camera.mic_pos.x - item->pos.x)
                    * (g_Camera.mic_pos.x - item->pos.x));
            if (dist < ROLL_SHAKE_RANGE) {
                g_Camera.bounce =
                    40 * (dist - ROLL_SHAKE_RANGE) / ROLL_SHAKE_RANGE;
            }
        }

        {
            const int32_t dist =
                item->object_id == O_ROLLING_BALL_1 ? STEP_L * 3 / 2 : WALL_L;
            const int32_t x =
                item->pos.x + ((dist * Math_Sin(item->rot.y)) >> W2V_SHIFT);
            const int32_t z =
                item->pos.z + ((dist * Math_Cos(item->rot.y)) >> W2V_SHIFT);
            const SECTOR *const sector =
                Room_GetSector(x, item->pos.y, z, &room_num);
            if (Room_GetHeight(sector, x, item->pos.y, z) < item->pos.y) {
                if (item->object_id == O_ROLLING_BALL_2) {
                    Sound_Effect(SFX_SNOWBALL_STOP, &item->pos, SPM_NORMAL);
                    item->goal_anim_state = TRAP_WORKING;
                } else if (item->object_id == O_ROLLING_BALL_3) {
                    Sound_Effect(SFX_ROLLING_2_HIT, &item->pos, SPM_NORMAL);
                    item->goal_anim_state = TRAP_WORKING;
                } else {
                    item->status = IS_DEACTIVATED;
                }
                item->pos.x = old_pos.x;
                item->pos.y = item->floor;
                item->pos.z = old_pos.z;
                item->speed = 0;
                item->fall_speed = 0;
                item->touch_bits = 0;
            }
        }
    } else if (item->status == IS_DEACTIVATED && !Item_IsTriggerActive(item)) {
        item->status = IS_INACTIVE;
        const GAME_VECTOR *const data = item->data;
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
        item->goal_anim_state = TRAP_SET;
        item->current_anim_state = TRAP_SET;
        Item_SwitchToAnim(item, 0, 0);
        item->goal_anim_state = Item_GetAnim(item)->current_anim_state;
        item->current_anim_state = item->goal_anim_state;
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

    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (lara_item->gravity) {
        if (coll->enable_baddie_push) {
            Lara_Push(item, lara_item, coll, coll->enable_hit, true);
        }
        lara_item->hit_points -= ROLLING_BALL_DAMAGE_AIR;

        // TODO: handle overflows
        const int32_t dx = lara_item->pos.x - item->pos.x;
        const int32_t dy =
            (lara_item->pos.y - 350) - (item->pos.y - WALL_L / 2);
        const int32_t dz = lara_item->pos.z - item->pos.z;
        int32_t dist = Math_Sqrt(SQUARE(dx) + SQUARE(dy) + SQUARE(dz));
        CLAMPL(dist, WALL_L / 2);

        Spawn_Blood(
            item->pos.x + (dx * WALL_L / 2) / dist,
            item->pos.y + (dy * WALL_L / 2) / dist - WALL_L / 2,
            item->pos.z + (dz * WALL_L / 2) / dist, item->speed, item->rot.y,
            item->room_num);
    } else {
        lara_item->hit_status = 1;
        if (lara_item->hit_points > 0) {
            lara_item->hit_points = -1;

            lara_item->rot.x = 0;
            lara_item->rot.y = item->rot.y;
            lara_item->rot.z = 0;

            Item_SwitchToAnim(lara_item, LA_BOULDER_DEATH, 0);
            lara_item->current_anim_state = LA_REACH_TO_FREEFALL;
            lara_item->goal_anim_state = LA_REACH_TO_FREEFALL;

            g_Camera.flags = CF_FOLLOW_CENTRE;
            g_Camera.target_angle = 170 * DEG_1;
            g_Camera.target_elevation = -25 * DEG_1;

            for (int32_t i = 0; i < 15; i++) {
                Spawn_Blood(
                    lara_item->pos.x + (Random_GetControl() - 0x4000) / 256,
                    lara_item->pos.z + (Random_GetControl() - 0x4000) / 256,
                    lara_item->pos.y - Random_GetControl() / 64,
                    2 * item->speed,
                    item->rot.y + (Random_GetControl() - 0x4000) / 8,
                    item->room_num);
            }
        }
    }
}

REGISTER_OBJECT(O_ROLLING_BALL_1, M_Setup)
REGISTER_OBJECT(O_ROLLING_BALL_2, M_Setup)
REGISTER_OBJECT(O_ROLLING_BALL_3, M_Setup)
