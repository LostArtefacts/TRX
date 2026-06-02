#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS 1
#define M_DAMAGE     2
#define M_RADIUS     (WALL_L / 10) // = 102
#define M_TURN       (20 * DEG_1) // = 3640
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_FLY,
    M_STATE_ATTACK,
    M_STATE_FALL,
    M_STATE_DEATH,
} M_STATE;

static BITE m_BatBite = {
    .pos = { 0, 16, 45 },
    .mesh_num = 4,
};

static int32_t M_GetAttackDamage(const ITEM *const item)
{
    OBJECT_PROPERTY_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, "damage", &damage)) {
        return damage.as_int;
    }

    return M_DAMAGE;
}

static void M_FixEmbeddedPosition(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status == IS_ACTIVE) {
        return;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    const int32_t ceiling = Room_GetCeiling(sector, item->pos);

    // The bats animation and frame have to be changed to the hanging
    // one to properly measure them. Save it so it can be restored
    // after.
    const int16_t old_anim = Item_GetRelativeAnim(item);
    const int16_t old_frame = Item_GetRelativeFrame(item);

    Item_SwitchToAnim(item, 0, 0);
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    Item_SwitchToAnim(item, old_anim, old_frame);

    const int16_t bat_height = ABS(bounds->min.y);

    // Only move the bat if it's above the calculated position,
    // Palace Midas has many bats that aren't intended to be at
    // ceiling level.
    if (item->pos.y < ceiling + bat_height) {
        item->pos.y = ceiling + bat_height;
    }
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const bat = item->creature_data;
    int16_t angle = 0;
    if (item->hit_points <= 0) {
        if (item->pos.y < item->floor) {
            item->gravity = true;
            item->goal_anim_state = M_STATE_FALL;
            item->speed = 0;
        } else {
            item->gravity = false;
            item->fall_speed = 0;
            item->goal_anim_state = M_STATE_DEATH;
            item->pos.y = item->floor;
        }
        Creature_Animate(item_num, 0, 0);
        return;
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, false);
        angle = Creature_Turn(item, M_TURN);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            item->goal_anim_state = M_STATE_FLY;
            break;

        case M_STATE_FLY:
            if (item->touch_bits) {
                item->goal_anim_state = M_STATE_ATTACK;
                Creature_Animate(item_num, angle, 0);
                return;
            }
            break;

        case M_STATE_ATTACK:
            if (item->touch_bits) {
                Creature_Effect(item, &m_BatBite, Spawn_Blood);
                Lara_TakeDamage(M_GetAttackDamage(item), true);
            } else {
                item->goal_anim_state = M_STATE_FLY;
                bat->mood = MOOD_BORED;
            }
            break;
        }
    }

    Creature_Animate(item_num, angle, 0);
}

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);

    // Almost all of the bats in the OG levels are embedded in the ceiling.
    // This will move all bats up to the ceiling of their rooms and down
    // by the height of their hanging animation.
    M_FixEmbeddedPosition(item_num);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->radius = M_RADIUS;
    obj->smartness = 0x400;
    obj->lot_setup = LOT_Setup(LOT_SETUP_FLYER);
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "damage", M_DAMAGE, "Damage dealt by the bat attack."));
}

REGISTER_OBJECT(O_BAT, M_Setup)
