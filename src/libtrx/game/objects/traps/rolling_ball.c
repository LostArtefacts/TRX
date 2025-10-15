#include "config.h"
#include "game/camera.h"
#include "game/game_buf.h"
#include "game/lara.h"
#include "game/objects.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "version.h"

#define M_DAMAGE_AIR 100
#define M_SHAKE_RANGE (WALL_L * 10) // = 10240

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    GAME_VECTOR *const data =
        GameBuf_Alloc(sizeof(GAME_VECTOR), GBUF_ITEM_DATA);
    data->pos = item->pos;
    data->room_num = item->room_num;
    item->data = data;
}

static void M_Roll(ITEM *const item)
{
    item->gravity = false;
    item->fall_speed = 0;
    item->pos.y = item->floor;

    if (g_TRVersion != 2) {
        return;
    }
    if (item->object_id == O_ROLLING_BALL_2) {
        Sound_Effect(SFX_SNOWBALL_ROLL, &item->pos, SPM_NORMAL);
    } else if (item->object_id == O_ROLLING_BALL_3) {
        Sound_Effect(SFX_ROLLING_2, &item->pos, SPM_NORMAL);
    } else {
        Sound_Effect(SFX_ROLLING_BALL, &item->pos, SPM_NORMAL);
    }

#if TR_VERSION == 2
    const int32_t dist = Math_Sqrt(
        (g_Camera.mic_pos.z - item->pos.z) * (g_Camera.mic_pos.z - item->pos.z)
        + (g_Camera.mic_pos.x - item->pos.x)
            * (g_Camera.mic_pos.x - item->pos.x));
    if (dist < M_SHAKE_RANGE) {
        g_Camera.bounce = 40 * (dist - M_SHAKE_RANGE) / M_SHAKE_RANGE;
    }
#endif
}

static bool M_TestStop(const ITEM *const item)
{
    int32_t dist;
    if (g_TRVersion == 1) {
        dist = WALL_L / 2;
    } else {
        dist = item->object_id == O_ROLLING_BALL_1 ? STEP_L * 3 / 2 : WALL_L;
    }

    int16_t room_num = item->room_num;
    const int32_t x =
        item->pos.x + ((dist * Math_Sin(item->rot.y)) >> W2V_SHIFT);
    const int32_t z =
        item->pos.z + ((dist * Math_Cos(item->rot.y)) >> W2V_SHIFT);
    const SECTOR *const sector = Room_GetSector(x, item->pos.y, z, &room_num);
    return Room_GetHeight(sector, x, item->pos.y, z) < item->pos.y;
}

static void M_Stop(ITEM *const item, const XYZ_32 old_pos)
{
    if (item->object_id == O_ROLLING_BALL_1) {
        item->status = IS_DEACTIVATED;
    }
    if (g_TRVersion == 2) {
        if (item->object_id == O_ROLLING_BALL_2) {
            Sound_Effect(SFX_SNOWBALL_STOP, &item->pos, SPM_NORMAL);
            item->goal_anim_state = TRAP_WORKING;
        } else if (item->object_id == O_ROLLING_BALL_3) {
            Sound_Effect(SFX_ROLLING_2_HIT, &item->pos, SPM_NORMAL);
            item->goal_anim_state = TRAP_WORKING;
        }
    }

    item->pos.x = old_pos.x;
    item->pos.y = item->floor;
    item->pos.z = old_pos.z;
    item->speed = 0;
    item->fall_speed = 0;
    item->touch_bits = 0;
}

static void M_Reset(ITEM *const item)
{
    const int16_t item_num = Item_GetIndex(item);
    const GAME_VECTOR *const data = item->data;

    item->status = IS_INACTIVE;
    item->pos = data->pos;
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

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_DEACTIVATED && !Item_IsTriggerActive(item)) {
        M_Reset(item);
        return;
    }

    if (item->status != IS_ACTIVE) {
        return;
    }

    if (item->goal_anim_state == TRAP_WORKING) {
        Item_Animate(item);
        return;
    }

    if (item->pos.y < item->floor) {
        if (!item->gravity) {
            item->gravity = true;
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
    Item_UpdateRoom(item_num, room_num);

    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    Room_TestTriggers(item);

    if (item->pos.y >= item->floor - STEP_L) {
        M_Roll(item);
    }

    if (M_TestStop(item)) {
        M_Stop(item, old_pos);
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const LARA_INFO *const lara = Lara_GetLaraInfo();

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

    if (lara_item->gravity || g_Config.debug.enable_invulnerability) {
        if (coll->enable_baddie_push) {
            Lara_Push(item, coll, coll->enable_hit, true);
        }
        if (!g_Config.debug.enable_invulnerability) {
            lara_item->hit_points -= M_DAMAGE_AIR;
        }

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
        lara_item->hit_status = true;
        if (lara_item->hit_points > 0) {
            lara_item->hit_points = -1;
            Item_UpdateRoom(lara->item_num, item->room_num);

            lara_item->rot.x = 0;
            lara_item->rot.y = item->rot.y;
            lara_item->rot.z = 0;

            Item_SwitchToAnim(lara_item, LA(LA_BOULDER_DEATH), 0);
            lara_item->goal_anim_state =
                Item_GetAnim(lara_item)->current_anim_state;
            lara_item->current_anim_state = lara_item->goal_anim_state;

            g_Camera.flags = CF_FOLLOW_CENTRE;
            g_Camera.target_angle = 170 * DEG_1;
            g_Camera.target_elevation = -25 * DEG_1;

            for (int32_t i = 0; i < 15; i++) {
                Spawn_Blood(
                    lara_item->pos.x + (Random_GetControl() - 0x4000) / 256,
                    lara_item->pos.y - Random_GetControl() / 64,
                    lara_item->pos.z + (Random_GetControl() - 0x4000) / 256,
                    2 * item->speed,
                    item->rot.y + (Random_GetControl() - 0x4000) / 8,
                    item->room_num);
            }
        }
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_ROLLING_BALL_1, M_Setup)
REGISTER_OBJECT(O_ROLLING_BALL_2, M_Setup)
REGISTER_OBJECT(O_ROLLING_BALL_3, M_Setup)
