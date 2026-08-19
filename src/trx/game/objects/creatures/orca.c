#include <trx/game/creature.h>
#include <trx/game/fx.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>

#define M_FAST_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define M_ATTACK_1_RANGE SQUARE(WALL_L * 3 / 4) // = 589824
#define M_FAST_TURN (DEG_1 * 2) // = 364
#define M_SLOW_TURN (DEG_1 * 2) // = 364
#define M_RADIUS (WALL_L / 3) // = 341

typedef enum {
    M_STATE_SLOW,
    M_STATE_FAST,
    M_STATE_JUMP,
    M_STATE_SPLASH,
    M_STATE_SLOW_BUTT,
    M_STATE_FAST_BUTT,
    M_STATE_BREACH,
    M_STATE_ROLL_180
} M_STATE;

static bool M_IsTargetable(const ITEM *const item)
{
    return false;
}

static bool M_CanBeProjectileTarget(const ITEM *const item)
{
    return false;
}

static void M_Control(int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;
    const ITEM *const lara_item = Lara_GetItem();

    AI_INFO info = {};
    Creature_AIInfo(item, &info);
    Creature_UpdateMood(item, &info, true);

    if (!(Room_Get(lara_item->room_num)->flags.underwater)
        && Lara_Vehicle_GetItem() == nullptr) {
        creature->mood = MOOD_BORED;
    }

    Creature_ApplyMood(item, &info, true);
    const int16_t angle = Creature_Turn(item, creature->maximum_turn);

    switch (item->current_anim_state) {
    case M_STATE_SLOW:
        creature->flags = 0;
        creature->maximum_turn = M_SLOW_TURN;

        if (creature->mood == MOOD_BORED) {
            if (Random_GetControl() & 0xFF) {
                item->goal_anim_state = M_STATE_SLOW;
            } else {
                item->goal_anim_state = M_STATE_JUMP;
            }
        } else if (info.ahead && info.distance < M_ATTACK_1_RANGE) {
            item->goal_anim_state = M_STATE_SLOW_BUTT;
        } else if (creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_FAST;
        } else if (info.distance > M_FAST_RANGE) {
            if (info.angle >= 0x5000 || info.angle <= -0x5000) {
                item->goal_anim_state = M_STATE_ROLL_180;
            } else if (Random_GetControl() & 0x3F) {
                item->goal_anim_state = M_STATE_FAST;
            } else {
                item->goal_anim_state = M_STATE_BREACH;
            }
        }

        break;

    case M_STATE_FAST:
        creature->flags = 0;
        creature->maximum_turn = M_FAST_TURN;

        if (creature->mood == MOOD_BORED) {
            if (Random_GetControl() & 0xFF) {
                item->goal_anim_state = M_STATE_SLOW;
            } else {
                item->goal_anim_state = M_STATE_JUMP;
            }
        } else if (creature->mood != MOOD_ESCAPE) {
            if (info.ahead && info.distance < M_FAST_RANGE
                && info.zone_num == info.enemy_zone_num) {
                item->goal_anim_state = M_STATE_SLOW;
            } else if (
                info.distance > M_FAST_RANGE && !(Random_GetControl() & 0x7F)) {
                item->goal_anim_state = M_STATE_JUMP;
            } else if (info.distance > M_FAST_RANGE && !info.ahead) {
                item->goal_anim_state = M_STATE_SLOW;
            }
        }
        break;

    case M_STATE_ROLL_180:
        creature->maximum_turn = 0;
        if (Item_GetRelativeFrame(item) == 59) {
            item->rot.x = -item->rot.x;
            item->rot.y += DEG_180;
            Interpolation_RememberItem(item);
        }
        break;
    }

    Creature_Animate(item_num, angle, 0);
    Creature_Underwater(item, WALL_L / 5);

    const int32_t time4 = Output_GetTimeInGame() * 4;
    if ((time4 & 4) != 0) {
        XYZ_32 pos = { .x = -32, .y = 16, .z = -300 };
        Collide_GetJointAbsPosition(item, &pos, 5);

        int16_t room_num = item->room_num;
        Room_GetSector(pos, &room_num);
        const int32_t water_height = Room_GetWaterHeight(pos, room_num);

        if (water_height != NO_HEIGHT && pos.y < water_height) {
            FX_WATER_RIPPLE *const ripple = FX_Water_SetupRipple(
                (XYZ_32) { pos.x, water_height, pos.z },
                2 + (Random_GetControl() & 1),
                FX_RIPPLE_SLOW | FX_RIPPLE_JITTER);
            if (ripple != nullptr) {
                ripple->init = 0;
            }
        }
    }
}

static void M_CollisionDolphin(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    Lara_Col_ItemPush(item, coll, coll->enable_hit, false);
}

static void M_SetupCommon(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->is_targetable_func = M_IsTargetable;
    obj->can_be_projectile_target_func = M_CanBeProjectileTarget;

    obj->shadow_size = 128;
    obj->pivot_length = 200;
    obj->radius = M_RADIUS;
    obj->lot_setup = LOT_Setup(LOT_SETUP_FLYER);
    obj->lot_setup.block_mask = BOX_BLOCKABLE;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_SetupOrca(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    M_SetupCommon(obj);
    obj->collision_func = Creature_Collision;
}

static void M_SetupDolphin(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    M_SetupCommon(obj);
    obj->collision_func = M_CollisionDolphin;
}

REGISTER_OBJECT(O_ORCA, M_SetupOrca)
REGISTER_OBJECT(O_DOLPHIN, M_SetupDolphin)
