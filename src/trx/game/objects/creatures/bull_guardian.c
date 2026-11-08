#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/fx/debris.h>
#include <trx/game/lara.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_TOUCH_BITS    0b01000000
#define M_RADIUS        (WALL_L / 2) // = 512
#define M_PIVOT         500
#define M_HIT_POINTS    1000
#define M_DAMAGE        200
#define M_PROBE_DIST    (WALL_L * 3 / 5) // = 614
#define M_SHATTER_FACES (-64)
#define M_ALERT_DIST    SQUARE(WALL_L) // = 1048576
#define M_RUN_DIST      SQUARE(WALL_L * 2) // = 4194304
#define M_COLLIDE_DIST  50
#define M_STALK_TURN    (DEG_1 * 3) // = 546
#define M_RUN_TURN      (DEG_1 / 3) // = 60
#define M_STOP_ANGLE    (DEG_45 / 16) // = 512
// clang-format on

typedef enum {
    M_STATE_NULL,
    M_STATE_REST,
    M_STATE_STIR,
    M_STATE_GET_UP,
    M_STATE_STALK,
    M_STATE_RUN,
    M_STATE_BACK_UP,
    M_STATE_COLLIDE,
    M_STATE_WARN,
    M_STATE_STOP,
} M_STATE;

typedef enum {
    M_ANIM_REST = 1,
} M_ANIM;

typedef struct {
    bool always_aggressive;
    int32_t damage;
    XZ_32 old_pos;
} M_PRIV;

static const BITE m_Bite = {
    .pos = {},
    .mesh_num = 6,
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_PUSH(io, "old_pos"));
    MUST(JSON_READ_OPT(io, "x", &p->old_pos.x));
    MUST(JSON_READ_OPT(io, "z", &p->old_pos.z));
    MUST(JSON_POP(io));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "x", p->old_pos.x);
    JSONW_WRITE(io, "z", p->old_pos.z);
    JSONW_POP_AND_SET(io, "old_pos");
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_REST, 0);
    item->goal_anim_state = M_STATE_REST;
    item->current_anim_state = M_STATE_REST;
}

static bool M_CanTakeDamage(const ITEM *const item)
{
    return false;
}

static ITEM_HIT_EFFECT M_GetHitEffect(const ITEM *const item)
{
    return ITEM_HIT_RICOCHET;
}

static void M_TestShatters(const ITEM *const item)
{
    if (item->current_anim_state != M_STATE_RUN) {
        return;
    }

    const XYZ_32 probe_pos =
        XYZ_32_OffsetYaw(item->pos, item->rot.y, M_PROBE_DIST);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(probe_pos, &room_num);
    if (!sector->stopper) {
        return;
    }

    const XZ_32 sector_pos = {
        .x = ROUND_TO_SECTOR(probe_pos.x),
        .z = ROUND_TO_SECTOR(probe_pos.z),
    };
    const ROOM *const room = Room_Get(item->room_num);
    for (int32_t i = 0; i < room->num_static_meshes; i++) {
        STATIC_MESH *const mesh = &room->static_meshes[i];
        if (Room_IsStaticShattered(mesh)) {
            continue;
        }

        const STATIC_OBJECT_3D *const obj =
            Object_Get3DStatic(mesh->static_num);
        if (!obj->shatter.by_impact) {
            continue;
        }

        if (ROUND_TO_SECTOR(mesh->pos.x) != sector_pos.x
            || ROUND_TO_SECTOR(mesh->pos.z) != sector_pos.z) {
            continue;
        }

        FX_Debris_ShatterStatic(mesh, M_SHATTER_FACES, item->room_num, 0);
        Sound_Effect(SFX_HIT_ROCK, &item->pos, SPM_NORMAL);
        Room_ShatterStatic(mesh);
    }
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    M_TestShatters(item);

    CREATURE *const creature = item->creature_data;
    ITEM *const lara_item = Lara_GetItem();
    if (item->ai_bits != 0) {
        Creature_GetAITarget(creature);
    } else {
        creature->enemy = lara_item;
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);
    Creature_Mood(item, &info, true);
    const int16_t angle = Creature_Turn(item, creature->maximum_turn);

    const XYZ_32 rear_pos =
        XYZ_32_OffsetYaw(item->pos, item->rot.y, -M_PROBE_DIST);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(rear_pos, &room_num);
    const int32_t rear_height = Room_GetHeight(sector, rear_pos);

    switch (item->current_anim_state) {
    case M_STATE_REST:
        creature->maximum_turn = 0;

        if (info.distance < M_ALERT_DIST || p->always_aggressive) {
            item->goal_anim_state = M_STATE_GET_UP;
        } else if (Random_GetControl() == 0) {
            item->goal_anim_state = M_STATE_STIR;
        }
        break;

    case M_STATE_STIR:
        creature->maximum_turn = 0;

        if (info.distance < M_ALERT_DIST || p->always_aggressive) {
            item->goal_anim_state = M_STATE_GET_UP;
        } else if (Random_GetControl() == 0) {
            item->goal_anim_state = M_STATE_REST;
        }
        break;

    case M_STATE_STALK:
        creature->maximum_turn = M_STALK_TURN;

        if ((info.distance > M_RUN_DIST && ABS(info.angle) <= M_STOP_ANGLE)
            || item->required_anim_state == M_STATE_RUN) {
            item->goal_anim_state = M_STATE_RUN;
        } else if (
            info.distance < M_RUN_DIST && item->goal_anim_state != M_STATE_RUN
            && rear_height <= item->pos.y + STEP_L
            && rear_height >= item->pos.y - STEP_L) {
            item->goal_anim_state = M_STATE_STOP;
            item->required_anim_state = M_STATE_BACK_UP;
        }
        break;

    case M_STATE_RUN:
        creature->maximum_turn = M_RUN_TURN;

        if (creature->flags == 0 && (item->touch_bits & M_TOUCH_BITS) != 0) {
            Creature_EffectEx(item, &m_Bite, 20, -1, Spawn_Blood);
            Lara_TakeDamage(p->damage, false);
            creature->flags = 1;
        }

        const XZ_32 delta = {
            .x = ABS(p->old_pos.x - item->pos.x),
            .z = ABS(p->old_pos.z - item->pos.z),
        };
        if (delta.x < M_COLLIDE_DIST && delta.z < M_COLLIDE_DIST
            && Item_TestAnimEqual(item, 0)) {
            item->goal_anim_state = M_STATE_COLLIDE;
            item->required_anim_state = M_STATE_BACK_UP;
            creature->maximum_turn = 0;
        } else if (
            info.distance > M_RUN_DIST && ABS(info.angle) > M_STOP_ANGLE) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_BACK_UP:
        creature->maximum_turn = M_STALK_TURN;

        if (info.distance > M_RUN_DIST || rear_height > item->pos.y + STEP_L
            || rear_height < item->pos.y - STEP_L) {
            item->goal_anim_state = M_STATE_STOP;
            item->required_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_COLLIDE:
        if (Item_TestFrameEqual(item, 0)) {
            Room_TestTriggers(item);

            if ((item->touch_bits & M_TOUCH_BITS) != 0) {
                Creature_EffectEx(item, &m_Bite, 50, -1, Spawn_Blood);
                Lara_TakeDamage(lara_item->hit_points, false);
            }
        }
        break;

    case M_STATE_STOP:
        creature->flags = 0;

        if (item->required_anim_state == M_STATE_BACK_UP) {
            item->goal_anim_state = M_STATE_BACK_UP;
        } else {
            item->goal_anim_state = M_STATE_STALK;
        }
        break;
    }

    p->old_pos.x = item->pos.x;
    p->old_pos.z = item->pos.z;
    Creature_Animate(item_num, angle, 0);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = Creature_Collision;
    obj->can_take_damage_func = M_CanTakeDamage;
    obj->get_hit_effect_func = M_GetHitEffect;
    obj->control_func = M_Control;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = M_PIVOT;
    obj->radius = M_RADIUS;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->intelligent = true;
    obj->save_anim = true;
    obj->save_flags = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, always_aggressive, true,
            "Whether or not the guardian always targets Lara; if disabled, it "
            "must first be shot."),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by the run attack."));
}

REGISTER_OBJECT(O_BULL_GUARDIAN, M_Setup)
