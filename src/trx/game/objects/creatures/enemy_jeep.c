#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/waypoint.h>

// clang-format off
#define M_BRAKE_ON_BITS    0b11111111'11111110'01111111'11111111
#define M_BRAKE_OFF_BITS   0b11111111'11111101'10111111'11111111
#define M_BRAKE_GLOW       ((RGB_888) { 64, 0, 0 })
#define M_WAYPOINT_UNSET   (-3)
#define M_HIT_POINTS       40
#define M_PIVOT            500
#define M_RADIUS           (WALL_L / 2) // = 512
#define M_SAMPLE_DIST      ((WALL_L - 1) * 4 / 3) // = 1364
#define M_MAX_HEIGHT_DIFF  (STEP_L * 3) // = 768
#define M_TURN_RATE        (DEG_1 * 2) // = 364
#define M_TILT_RATE        (DEG_45 / 32) // = 256
#define M_DECEL_RATE       (STEP_L / 2) // = 128
#define M_SLOW_ACCEL_RATE  18
#define M_FAST_ACCEL_RATE  37
#define M_MAX_SPEED        (STEP_L * 34) // = 8704
#define M_BOUNCE_SPEED     (STEP_L * 37 / 8) // = 1184
#define M_GRENADE_COOLDOWN (LOGIC_FPS * 5) // = 150
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_BRAKE  = 1,
    M_ANIM_BOUNCE = 8,
    M_ANIM_STOP   = 14,
    // clang-format on
} M_ANIM;

typedef enum {
    M_STATE_NULL,
    M_STATE_STOP,
    M_STATE_BRAKE,
    M_STATE_TURN_LEFT,
    M_STATE_TURN_RIGHT,
    M_STATE_BOUNCE,
} M_STATE;

typedef struct {
    int32_t front_height;
    int32_t back_height;
    XZ_16 tilt;
} M_TERRAIN;

typedef struct {
    int16_t velocity;
    int16_t jump_velocity;
    int16_t grenade_cooldown;
    int16_t waypoint;
} M_PRIV;

static const BITE m_BrakeLight = {
    .pos = { .x = 0, .y = -144, .z = -WALL_L },
    .mesh_num = 11,
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ(io, "velocity", &p->velocity));
    MUST(JSON_READ(io, "jump_velocity", &p->jump_velocity));
    MUST(JSON_READ(io, "grenade_cooldown", &p->grenade_cooldown));
    MUST(JSON_READ(io, "waypoint", &p->waypoint));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "velocity", p->velocity);
    JSONW_WRITE(io, "jump_velocity", p->jump_velocity);
    JSONW_WRITE(io, "grenade_cooldown", p->grenade_cooldown);
    JSONW_WRITE(io, "waypoint", p->waypoint);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const int16_t yaw = item->rot.y;
    Creature_Initialise(item_num);
    item->rot.y = yaw;

    Item_SetVisible(item, true);
    Item_SwitchToAnim(item, M_ANIM_STOP, 0);
    item->mesh_bits = M_BRAKE_OFF_BITS;

    M_PRIV *const p = item->priv;
    p->waypoint = M_WAYPOINT_UNSET;
}

static bool M_IsTargetable(const ITEM *const item)
{
    return false;
}

static ITEM_HIT_EFFECT M_GetHitEffect(const ITEM *const item)
{
    return ITEM_HIT_SMOKE;
}

static int32_t M_GetHeight(const XYZ_32 pos, int16_t room_num)
{
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    return Room_GetHeight(sector, pos);
}

static M_TERRAIN M_DetectTerrain(ITEM *const item)
{
    M_TERRAIN terrain = {};

    const XYZ_32 offset =
        XYZ_32_OffsetYaw((XYZ_32) {}, item->rot.y, M_SAMPLE_DIST / 2);

    XYZ_32 sample_pos = {
        .x = item->pos.x - offset.z,
        .y = item->pos.y,
        .z = item->pos.z + offset.x,
    };
    int32_t height_1 = M_GetHeight(sample_pos, item->room_num);
    if (ABS(sample_pos.y - height_1) > M_MAX_HEIGHT_DIFF) {
        item->pos.x += offset.z >> 6;
        item->pos.z -= offset.x >> 6;
        item->rot.y += M_TURN_RATE;
        height_1 = sample_pos.y;
    }

    sample_pos.x = item->pos.x + offset.z;
    sample_pos.z = item->pos.z - offset.x;
    int32_t height_2 = M_GetHeight(sample_pos, item->room_num);
    if (ABS(sample_pos.y - height_2) > M_MAX_HEIGHT_DIFF) {
        item->pos.x -= offset.z >> 6;
        item->pos.z += offset.x >> 6;
        item->rot.y -= M_TURN_RATE;
        height_2 = sample_pos.y;
    }

    terrain.tilt.z = Math_Atan(M_SAMPLE_DIST, height_2 - height_1);

    sample_pos.x = item->pos.x + offset.x;
    sample_pos.z = item->pos.z + offset.z;
    height_1 = M_GetHeight(sample_pos, item->room_num);
    terrain.front_height = height_1;
    if (ABS(sample_pos.y - height_1) > M_MAX_HEIGHT_DIFF) {
        height_1 = sample_pos.y;
    }

    sample_pos.x = item->pos.x - offset.x;
    sample_pos.z = item->pos.z - offset.z;
    height_2 = M_GetHeight(sample_pos, item->room_num);
    terrain.back_height = height_2;
    if (ABS(sample_pos.y - height_2) > M_MAX_HEIGHT_DIFF) {
        height_2 = sample_pos.y;
    }

    terrain.tilt.x = Math_Atan(M_SAMPLE_DIST, height_2 - height_1);

    return terrain;
}

static void M_FireGrenade(const ITEM *const item)
{
    const int16_t grenade_item_num = Item_Spawn(item, O_GRENADE);
    if (grenade_item_num == NO_ITEM) {
        return;
    }

    ITEM *const grenade_item = Item_Get(grenade_item_num);
    grenade_item->shade.value_1 = -0x3DF0;
    grenade_item->rot.z = 0;
    grenade_item->rot.y += DEG_180;
    grenade_item->pos =
        XYZ_32_OffsetYaw(grenade_item->pos, grenade_item->rot.y, WALL_L);
    grenade_item->pos.y -= STEP_L * 3;

    for (int32_t i = 0; i < 5; i++) {
        const GAME_VECTOR pos = {
            .pos = item->pos,
            .room_num = item->room_num,
        };
        Sparks_TriggerGunSmoke(pos, true, LGT_GRENADE, 32);
    }

    if ((Random_GetControl() & 3) == 0) {
        // TODO: the grenade should be of "super" type
    }

    grenade_item->speed = STEP_L / 8;
    grenade_item->fall_speed =
        (-grenade_item->speed * Math_Sin(grenade_item->rot.x)) >> W2V_SHIFT;
    grenade_item->current_anim_state = grenade_item->rot.x;
    grenade_item->goal_anim_state = grenade_item->rot.y;
    grenade_item->hit_points = 120;

    Item_AddSimulated(grenade_item_num);
}

static void M_Tilt(int16_t *const rot, const int16_t angle)
{
    if (ABS(angle - *rot) < M_TILT_RATE) {
        *rot = angle;
    } else if (angle > *rot) {
        *rot += M_TILT_RATE;
    } else if (angle < *rot) {
        *rot -= M_TILT_RATE;
    }
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;
    M_PRIV *const p = item->priv;
    if (p->waypoint == M_WAYPOINT_UNSET) {
        p->waypoint = item->ai_ocb;
    }

    M_TERRAIN terrain = M_DetectTerrain(item);

    AI_INFO info = {};
    Creature_AIInfo(item, &info);
    Creature_FindAITargetObject(creature, O_AI_FOLLOW, p->waypoint);

    ITEM *const lara_item = Lara_GetItem();
    AI_INFO lara_info = {};
    if (creature->enemy == lara_item) {
        lara_info.angle = info.angle;
        lara_info.distance = info.distance;
    } else {
        const XYZ_32 delta = XYZ_32_Subtract(lara_item->pos, item->pos);
        lara_info.angle = Math_Atan(delta.z, delta.x) - item->rot.y;
        lara_info.distance = (ABS(delta.x) > 32000 || ABS(delta.z) > 32000)
            ? INT32_MAX
            : (SQUARE(delta.x) + SQUARE(delta.z));
    }

    switch (item->current_anim_state) {
    case M_STATE_NULL:
    case M_STATE_BRAKE:
        p->velocity -= M_DECEL_RATE;
        CLAMPL(p->velocity, 0);

        item->mesh_bits = M_BRAKE_ON_BITS;
        XYZ_32 pos = m_BrakeLight.pos;
        Collide_GetJointAbsPosition(item, &pos, m_BrakeLight.mesh_num);
        Output_AddDynamicLightRGB(pos, 10, M_BRAKE_GLOW);

        if (item->required_anim_state != M_STATE_NULL) {
            item->goal_anim_state = item->required_anim_state;
        } else if (info.distance > 0x100000 || Waypoint_Get() >= p->waypoint) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_STOP:
        creature->maximum_turn = p->velocity >> 4;
        p->velocity += M_FAST_ACCEL_RATE;
        CLAMPG(p->velocity, M_MAX_SPEED);

        item->mesh_bits = M_BRAKE_OFF_BITS;

        if (info.angle > M_TILT_RATE) {
            item->goal_anim_state = M_STATE_TURN_RIGHT;
        } else if (info.angle < -M_TILT_RATE) {
            item->goal_anim_state = M_STATE_TURN_LEFT;
        }
        break;

    case M_STATE_TURN_LEFT:
    case M_STATE_TURN_RIGHT:
        p->velocity += M_SLOW_ACCEL_RATE;
        CLAMPG(p->velocity, M_MAX_SPEED);
        item->goal_anim_state = M_STATE_STOP;
        break;

    case M_STATE_BOUNCE:
        CLAMPL(p->velocity, M_BOUNCE_SPEED);
        break;
    }

    if (terrain.front_height > item->floor + (STEP_L * 2)) {
        creature->lot.is_jumping = true;

        if (p->jump_velocity > 0) {
            terrain.tilt.x = p->jump_velocity;
            p->jump_velocity -= 8;

            if (p->jump_velocity < 0) {
                creature->lot.is_jumping = false;
            }

            item->pos.y += p->jump_velocity >> 6;
        } else {
            p->jump_velocity = terrain.tilt.x << 1;
            creature->lot.is_jumping = true;
        }

        if (creature->lot.is_jumping) {
            creature->maximum_turn = 0;
            item->goal_anim_state = M_STATE_STOP;
        }
    } else if (
        terrain.back_height > item->floor + (STEP_L * 2)
        && item->current_anim_state != M_STATE_BOUNCE) {
        p->jump_velocity = 0;
        Item_SwitchToAnim(item, M_ANIM_BOUNCE, 0);
        item->current_anim_state = M_STATE_BOUNCE;
        item->goal_anim_state = M_STATE_STOP;
    }

    if (info.distance < 0x240000 || p->waypoint == -2) {
        creature->reached_goal = true;
    }

    if (creature->reached_goal) {
        Room_TestTriggers(creature->enemy);

        if (Waypoint_Get() < p->waypoint
            && item->current_anim_state != M_STATE_BRAKE
            && item->goal_anim_state != M_STATE_BRAKE) {
            Item_SwitchToAnim(item, M_ANIM_BRAKE, 0);
            item->current_anim_state = M_STATE_BRAKE;
            item->goal_anim_state = M_STATE_BRAKE;

            if ((Creature_GetAIObjectFlags(creature->enemy) & 4) != 0) {
                item->pos = creature->enemy->pos;
                item->rot = creature->enemy->rot;
                Item_UpdateRoom(item_num, creature->enemy->room_num);
            }
        }

        if (lara_info.distance > 0x400000 && lara_info.distance < 0x6400000
            && p->grenade_cooldown == 0 && ABS(lara_info.angle) > 20480) {
            M_FireGrenade(item);
            p->grenade_cooldown = M_GRENADE_COOLDOWN;
        }

        if (Creature_GetAIObjectFlags(creature->enemy) == 62) {
            Item_SetVisible(item, false);
            Item_RemoveSimulated(item_num);
            LOT_DisableBaddieAI(item_num);
        }

        if (Waypoint_Get() >= p->waypoint
            || (Creature_GetAIObjectFlags(creature->enemy) & 4) == 0) {
            creature->reached_goal = false;
            p->waypoint++;
            creature->enemy = Creature_FindAIObjectByOCB(p->waypoint);
        }
    }

    p->grenade_cooldown--;
    CLAMPL(p->grenade_cooldown, 0);

    M_Tilt(&item->rot.x, terrain.tilt.x);
    M_Tilt(&item->rot.z, terrain.tilt.z);

    p->velocity -= terrain.tilt.x >> 9;
    p->velocity -= 2;
    CLAMPL(p->velocity, 0);

    const XYZ_32 shift =
        XYZ_32_OffsetYaw((XYZ_32) {}, item->rot.y, p->velocity);
    item->pos.x += shift.x >> 6;
    item->pos.z += shift.z >> 6;

    for (int32_t i = 0; i < 4; i++) {
        creature->joint_rotation[i] -= p->velocity;
    }

    if (!creature->reached_goal) {
        Creature_TurnTo(item, info.angle, p->velocity >> 4);
    }

    creature->maximum_turn = 0;
    Item_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    item->floor = Room_GetHeight(sector, item->pos);
    Item_UpdateRoom(item_num, room_num);

    if (item->pos.y < item->floor) {
        item->gravity = true;
    } else {
        item->fall_speed = 0;
        item->pos.y = item->floor;
        item->gravity = false;
    }

    Sound_Effect(
        SFX_JEEP_MOVE, &item->pos,
        (p->velocity << 10) + (SPM_PITCH | 0x1000000));
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = Creature_Collision;
    obj->is_targetable_func = M_IsTargetable;
    obj->get_hit_effect_func = M_GetHitEffect;
    obj->control_func = M_Control;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = M_PIVOT;
    obj->radius = M_RADIUS;
    obj->lot_setup = LOT_Setup(LOT_SETUP_ACROBAT);

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->intelligent = true;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    Object_GetBone(obj, 8)->rot.x = true;
    Object_GetBone(obj, 9)->rot.x = true;
    Object_GetBone(obj, 11)->rot.x = true;
    Object_GetBone(obj, 12)->rot.x = true;

    OBJECT_PROPERTIES(obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS));
}

REGISTER_OBJECT(O_ENEMY_JEEP, M_Setup)
