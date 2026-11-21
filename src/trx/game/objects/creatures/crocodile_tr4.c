#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS     36
#define M_RADIUS         (WALL_L * 2 / 5) // = 409
#define M_PIVOT          300
#define M_DAMAGE         120
#define M_TURN           (DEG_1 * 3) // = 546
#define M_TOUCH          0x300
#define M_BITE_RANGE     SQUARE(WALL_L * 3 / 4) // = 589824
#define M_WALK_RANGE     SQUARE(WALL_L) // = 1048576
#define M_RUN_RANGE      SQUARE(WALL_L * 3 / 2) // = 2359296
#define M_ALERT_RANGE    SQUARE(WALL_L * 3 / 2) // = 2359296
#define M_SIGHT_RANGE    SQUARE(WALL_L * 5) // = 26214400
#define M_ROLL_PROBE     WALL_L
#define M_ROLL_STEP      STEP_L
#define M_GUARD_TURN     12
#define M_GUARD_MAX_ROT  1024
#define M_SWIM_DEPTH     STEP_L
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_STOP        = 0,
    M_ANIM_DEATH       = 11,
    M_ANIM_SWIM        = 12,
    M_ANIM_WATER_DEATH = 16,
    M_ANIM_DIVE        = 17,
    // clang-format on
} M_ANIM;

typedef enum {
    // clang-format off
    M_STATE_EMPTY        = 0,
    M_STATE_STOP         = 1,
    M_STATE_RUN          = 2,
    M_STATE_WALK         = 3,
    M_STATE_ATTACK       = 5,
    M_STATE_DEATH        = 7,
    M_STATE_SWIM         = 8,
    M_STATE_WATER_ATTACK = 9,
    M_STATE_WATER_DEATH  = 10,
    // clang-format on
} M_STATE;

typedef struct {
    int32_t damage;
    int16_t guard_rot;
    int16_t guard_turn;
} M_PRIV;

static const BITE m_Bite = {
    .pos = { 0, -100, 500 },
    .mesh_num = 9,
};

static bool M_IsUnderwater(const int16_t room_num)
{
    return Room_Get(room_num)->flags.underwater;
}

static int32_t M_GetFloor(const ITEM *const item, const XYZ_32 pos)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    return ABS(item->pos.y - height) > STEP_L * 2 ? item->pos.y : height;
}

static int16_t M_GetRoll(const ITEM *const item)
{
    const int32_t front = M_GetFloor(
        item, XYZ_32_OffsetYaw(item->pos, item->rot.y, M_ROLL_PROBE));
    const int32_t back = M_GetFloor(
        item, XYZ_32_OffsetYaw(item->pos, item->rot.y, -M_ROLL_PROBE));
    return Math_Atan(M_ROLL_PROBE * 2, back - front);
}

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    if (M_IsUnderwater(item->room_num)) {
        Item_SwitchToAnim(item, M_ANIM_SWIM, 0);
        item->current_anim_state = M_STATE_SWIM;
        item->goal_anim_state = M_STATE_SWIM;
    } else {
        Item_SwitchToAnim(item, M_ANIM_STOP, 0);
        item->current_anim_state = M_STATE_STOP;
        item->goal_anim_state = M_STATE_STOP;
    }
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "guard_rot", &p->guard_rot));
    MUST(JSON_READ_OPT(io, "guard_turn", &p->guard_turn));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "guard_rot", p->guard_rot);
    JSONW_WRITE(io, "guard_turn", p->guard_turn);
}

static int16_t M_Guard(M_PRIV *const p)
{
    const int16_t rot = p->guard_rot;
    p->guard_rot += p->guard_turn;
    if ((Random_GetControl() & 0x1F) == 0) {
        if ((Random_GetControl() & 1) != 0) {
            p->guard_turn = 0;
        } else {
            p->guard_turn =
                (Random_GetControl() & 1) != 0 ? M_GUARD_TURN : -M_GUARD_TURN;
        }
    }
    CLAMP(p->guard_rot, -M_GUARD_MAX_ROT, M_GUARD_MAX_ROT);
    return rot;
}

static void M_Bite(ITEM *const item, const M_PRIV *const p)
{
    Creature_EffectEx(item, &m_Bite, 10, -1, Spawn_Blood);
    Lara_TakeDamage(p->damage, true);
}

static void M_SetLand(CREATURE *const croc)
{
    croc->lot.setup.step = STEP_L;
    croc->lot.setup.drop = -STEP_L;
}

static void M_SetWater(CREATURE *const croc)
{
    croc->lot.setup.step = WALL_L * 20;
    croc->lot.setup.drop = -WALL_L * 20;
}

static int16_t M_Think(
    const int16_t item_num, ITEM *const item, int16_t *const angle)
{
    CREATURE *const croc = item->creature_data;
    M_PRIV *const p = item->priv;

    if (item->ai_bits != 0) {
        Creature_GetAITarget(croc);
    } else if (croc->hurt_by_lara) {
        croc->enemy = Lara_GetItem();
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);
    Creature_UpdateMood(item, &info, true);
    Creature_ApplyMood(item, &info, true);
    *angle = Creature_Turn(item, croc->maximum_turn);

    if (item->hit_status || info.distance < M_ALERT_RANGE
        || (Creature_CanSeeEnemy(item, &info)
            && info.distance < M_SIGHT_RANGE)) {
        croc->alerted = true;
        Creature_AlertAllGuards(item_num);
    }

    int16_t rot = *angle * 4;
    const bool is_biting = info.bite && (item->touch_bits & M_TOUCH) != 0;
    switch (item->current_anim_state) {
    case M_STATE_SWIM:
        croc->maximum_turn = M_TURN;
        M_SetWater(croc);
        if (item->required_anim_state != M_STATE_EMPTY) {
            item->goal_anim_state = item->required_anim_state;
        } else if (is_biting) {
            item->goal_anim_state = M_STATE_WATER_ATTACK;
        }
        break;

    case M_STATE_WATER_ATTACK:
        if (Item_TestFrameEqual(item, 0)) {
            item->required_anim_state = M_STATE_EMPTY;
        }
        if (!is_biting) {
            item->goal_anim_state = M_STATE_SWIM;
        } else if (item->required_anim_state == M_STATE_EMPTY) {
            M_Bite(item, p);
            item->required_anim_state = M_STATE_SWIM;
        }
        break;

    case M_STATE_STOP:
        croc->maximum_turn = 0;
        if ((item->ai_bits & AI_GUARD) != 0) {
            rot = M_Guard(p);
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.bite && info.distance < M_BITE_RANGE) {
            item->goal_anim_state = M_STATE_ATTACK;
        } else if (info.ahead && info.distance < M_WALK_RANGE) {
            item->goal_anim_state = M_STATE_WALK;
        } else {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_WALK:
        croc->maximum_turn = M_TURN;
        M_SetLand(croc);
        if (item->required_anim_state != M_STATE_EMPTY) {
            item->goal_anim_state = item->required_anim_state;
        } else if (info.bite && info.distance < M_BITE_RANGE) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (!info.ahead || info.distance > M_RUN_RANGE) {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_RUN:
        croc->maximum_turn = M_TURN;
        if (item->required_anim_state != M_STATE_EMPTY) {
            item->goal_anim_state = item->required_anim_state;
        } else if (info.bite && info.distance < M_BITE_RANGE) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.ahead && info.distance < M_WALK_RANGE) {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_ATTACK:
        if (Item_TestFrameEqual(item, 0)) {
            item->required_anim_state = M_STATE_EMPTY;
        }
        if (!is_biting) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (item->required_anim_state == M_STATE_EMPTY) {
            M_Bite(item, p);
            item->required_anim_state = M_STATE_STOP;
        }
        break;

    default:
        break;
    }

    return rot;
}

static void M_UpdateHabitat(ITEM *const item)
{
    CREATURE *const croc = item->creature_data;
    if (!M_IsUnderwater(item->room_num)) {
        croc->lot.setup.fly = 0;
        return;
    }

    // Out of the swim state, the original probes the x axis at half the
    // distance of the z axis.
    const int32_t reach_x =
        item->current_anim_state == M_STATE_SWIM ? WALL_L : WALL_L / 2;
    const XYZ_32 ahead = {
        .x = XYZ_32_OffsetYaw(item->pos, item->rot.y, reach_x).x,
        .y = item->pos.y,
        .z = XYZ_32_OffsetYaw(item->pos, item->rot.y, WALL_L).z,
    };
    int16_t room_num = item->room_num;
    Room_GetSector(ahead, &room_num);

    if (!M_IsUnderwater(room_num)) {
        item->goal_anim_state = M_STATE_WALK;
        item->required_anim_state = M_STATE_WALK;
        M_SetLand(croc);
        croc->lot.setup.fly = 0;
        Creature_Underwater(item, 0);
    } else if (item->current_anim_state == M_STATE_RUN) {
        item->goal_anim_state = M_STATE_WALK;
        item->required_anim_state = M_STATE_WALK;
    } else if (item->current_anim_state == M_STATE_WALK) {
        item->goal_anim_state = M_STATE_SWIM;
        item->required_anim_state = M_STATE_SWIM;
    } else if (!Item_TestAnimEqual(item, M_ANIM_DIVE)) {
        M_SetWater(croc);
        croc->lot.setup.fly = STEP_L / 16;
        Creature_Underwater(item, M_SWIM_DEPTH);
    }
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    int16_t angle = 0;
    int16_t rot = 0;
    const int16_t roll = M_GetRoll(item);

    if (item->hit_points <= 0) {
        item->hit_points = 0;
        if (item->current_anim_state != M_STATE_DEATH
            && item->current_anim_state != M_STATE_WATER_DEATH) {
            if (M_IsUnderwater(item->room_num)) {
                Item_SwitchToAnim(item, M_ANIM_WATER_DEATH, 0);
                item->current_anim_state = M_STATE_WATER_DEATH;
                item->goal_anim_state = M_STATE_WATER_DEATH;
            } else {
                Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
                item->current_anim_state = M_STATE_DEATH;
                item->goal_anim_state = M_STATE_DEATH;
            }
        }
        if (M_IsUnderwater(item->room_num)) {
            Creature_Float(item_num);
            return;
        }
    } else {
        rot = M_Think(item_num, item, &angle);
    }

    Creature_Tilt(item, 0);
    Creature_Joint(item, 0, rot);
    Creature_Joint(item, 1, rot);
    Creature_Joint(item, 2, -rot);
    Creature_Joint(item, 3, -rot);

    if (item->current_anim_state < M_STATE_SWIM) {
        if (ABS(roll - item->rot.x) < M_ROLL_STEP) {
            item->rot.x = roll;
        } else if (roll > item->rot.x) {
            item->rot.x += M_ROLL_STEP;
        } else if (roll < item->rot.x) {
            item->rot.x -= M_ROLL_STEP;
        }
    }

    Creature_Animate(item_num, angle, 0);
    if (item->creature_data != nullptr) {
        M_UpdateHabitat(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = Creature_Collision;
    obj->control_func = M_Control;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = M_PIVOT;
    obj->lot_setup = (LOT_SETUP) {
        .step = WALL_L * 20,
        .drop = -WALL_L * 20,
        .fly = STEP_L / 8,
        .block_mask = BOX_BLOCKED,
        .zone = LOT_ZONE_CROCODILE,
    };

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->intelligent = true;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    Object_GetBone(obj, 0)->rot.y = true;
    Object_GetBone(obj, 7)->rot.y = true;
    Object_GetBone(obj, 9)->rot.y = true;
    Object_GetBone(obj, 10)->rot.y = true;

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by the crocodile's bite."));
}

REGISTER_OBJECT(O_CROCODILE_TR4, M_Setup)
