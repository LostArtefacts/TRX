#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_RADIUS         (WALL_L / 5)        // = 204
#define M_HIT_POINTS     24
#define M_DAMAGE         50
#define M_TOUCH_BITS     0b00010000'00000000
#define M_WAIT_TURN      DEG_1               // = 182
#define M_FLY_TURN       (DEG_1 * 3)         // = 546
#define M_LAND_SPEED     (STEP_L / 5)        // = 51
#define M_ATTACK_DIST    SQUARE(WALL_L / 2)  // = 262144
#define M_TAKEOFF_DIST   SQUARE(WALL_L * 3)  // = 9437184
#define M_TAKEOFF_CHANCE 0x80
// clang-format on

typedef enum {
    M_STATE_HOVER,
    M_STATE_LAND,
    M_STATE_WAIT,
    M_STATE_TAKEOFF,
    M_STATE_ATTACK,
    M_STATE_FALL,
    M_STATE_DEATH,
    M_STATE_MOVE,
} M_STATE;

typedef enum {
    M_ANIM_WAIT = 2,
    M_ANIM_FALL = 5,
} M_ANIM;

typedef struct {
    int32_t damage;
    int16_t light;
} M_PRIV;

static const BITE m_Sting = {
    .pos = {},
    .mesh_num = 12,
};

static void M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    JSON_OPTIONAL(JSON_READ(io, "light", &p->light));
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "light", p->light);
}

static void M_TriggerParticles(const ITEM *const item)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
    if (dx < -0x4000 || dx > 0x4000 || dz < -0x4000 || dz > 0x4000) {
        return;
    }

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.g = (Random_GetControl() & 0x3F) + 32;
    spark->src_color.b = spark->src_color.g >> 1;
    spark->src_color.r = spark->src_color.g >> 2;
    spark->dst_color.g = (Random_GetControl() & 0x1F) + 224;
    spark->dst_color.b = spark->dst_color.g >> 1;
    spark->dst_color.r = spark->dst_color.g >> 2;
    spark->life = 8;
    spark->s_life = 8;
    spark->col_fade_speed = 4;
    spark->fade_to_black = 2;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = (Random_GetControl() & 0xF) - 8;
    spark->pos.y = (Random_GetControl() & 0xF) - 8;
    spark->pos.z = (Random_GetControl() & 0x7F) - 64;
    spark->vel.x = (Random_GetControl() & 0x1F) - 16;
    spark->vel.y = (Random_GetControl() & 0x1F) - 16;
    spark->vel.z = (Random_GetControl() & 0x1F) - 16;
    spark->flags =
        SPARK_F_ATTACHED_NODE | SPARK_F_ITEM | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->friction = 34;
    spark->max_y_vel = 0;
    spark->gravity = 0;
    spark->effect_num = Item_GetIndex(item);
    spark->node_num = 1;
    spark->scalar = 3;
    spark->size.width = (Random_GetControl() & 3) + 3;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = spark->size.width >> 1;
    spark->size.height = spark->size.width;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = spark->size.height >> 1;
    Sparks_FinishSetup(spark);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_WAIT, 0);
    item->current_anim_state = M_STATE_WAIT;
    item->goal_anim_state = M_STATE_WAIT;

    M_PRIV *const p = item->priv;
    p->light = Random_GetControl() & 0x7F;
}

static void M_ControlDeath(ITEM *const item)
{
    switch (item->current_anim_state) {
    case M_STATE_FALL:
        if (item->pos.y > item->floor) {
            item->pos.y = item->floor;
            item->fall_speed = 0;
            item->gravity = false;
            item->goal_anim_state = M_STATE_DEATH;
        }
        item->rot.x = 0;
        break;

    case M_STATE_DEATH:
        item->pos.y = item->floor;
        item->rot.x = 0;
        break;

    default:
        Item_SwitchToAnim(item, M_ANIM_FALL, 0);
        item->current_anim_state = M_STATE_FALL;
        item->gravity = true;
        item->speed = 0;
        item->rot.x = 0;
        break;
    }
}

static void M_TriggerLight(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    const int32_t intensity = ABS(Math_Sin(p->light << 10) * 31) >> 14;
    const RGB_888 color = {
        .r = 0,
        .g = intensity << 3,
        .b = 0,
    };

    XYZ_32 pos = {};
    Collide_GetJointAbsPosition(item, &pos, 10);
    Output_AddDynamicLightRGB(pos, 10, color);

    p->light = (p->light + 1) & 0x3F;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    CREATURE *const creature = item->creature_data;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        M_ControlDeath(item);
        goto finish;
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);

    Creature_Mood(item, &info, true);
    angle = Creature_Turn(item, creature->maximum_turn);

    switch (item->current_anim_state) {
    case M_STATE_HOVER:
        creature->flags = 0;
        creature->maximum_turn = M_FLY_TURN;

        if (item->required_anim_state != 0) {
            item->goal_anim_state = item->required_anim_state;
        } else if (
            item->hit_status || Random_GetControl() < M_TAKEOFF_CHANCE * 3
            || item->ai_bits == AI_MODIFY) {
            item->goal_anim_state = M_STATE_MOVE;
        } else if (
            (creature->mood != MOOD_BORED
             && Random_GetControl() >= M_TAKEOFF_CHANCE)
            || item->hit_status || item->ai_bits == AI_MODIFY) {
            if (info.ahead && info.distance < M_ATTACK_DIST) {
                item->goal_anim_state = M_STATE_ATTACK;
            }
        } else {
            item->goal_anim_state = M_STATE_LAND;
        }
        break;

    case M_STATE_LAND:
        item->pos.y += M_LAND_SPEED;
        CLAMPG(item->pos.y, item->floor);
        break;

    case M_STATE_WAIT:
        item->pos.y = item->floor;
        creature->maximum_turn = M_WAIT_TURN;

        if (item->hit_status || info.distance < M_TAKEOFF_DIST
            || creature->hurt_by_lara || item->ai_bits == AI_MODIFY) {
            item->goal_anim_state = M_STATE_TAKEOFF;
        }
        break;

    case M_STATE_ATTACK:
        creature->maximum_turn = M_FLY_TURN;

        if (info.ahead && info.distance < M_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_ATTACK;
        } else if (info.distance < M_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_HOVER;
        } else {
            item->goal_anim_state = M_STATE_HOVER;
            item->required_anim_state = M_STATE_MOVE;
        }

        if (creature->flags == 0 && (item->touch_bits & M_TOUCH_BITS) != 0) {
            Lara_TakeDamage(p->damage, true);
            Creature_Effect(item, &m_Sting, Spawn_Blood);
            creature->flags = 1;
        }
        break;

    case M_STATE_MOVE:
        creature->flags = 0;
        creature->maximum_turn = M_FLY_TURN;

        if (item->required_anim_state != 0) {
            item->goal_anim_state = item->required_anim_state;
        } else if (
            (creature->mood != MOOD_BORED
             && Random_GetControl() >= M_TAKEOFF_CHANCE)
            || creature->hurt_by_lara || item->ai_bits == AI_MODIFY) {
            if (info.ahead && info.distance < M_ATTACK_DIST) {
                item->goal_anim_state = M_STATE_ATTACK;
            }
        } else {
            item->goal_anim_state = M_STATE_HOVER;
        }
        break;
    }

finish:
    M_TriggerLight(item);
    for (int32_t i = 0; i < 2; i++) {
        M_TriggerParticles(item);
    }

    Creature_Animate(item_num, angle, 0);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = Creature_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->radius = M_RADIUS;

    obj->lot_setup = LOT_Setup(LOT_SETUP_FLYER);

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by the sting attack."));
}

REGISTER_OBJECT(O_WASP_MUTANT, M_Setup)
