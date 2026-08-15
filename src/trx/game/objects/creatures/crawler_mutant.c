#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/creature.h>
#include <trx/game/effects.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_HIT_POINTS      50
#define M_RADIUS          (WALL_L / 5)       // = 204
#define M_MAX_POISON      256
#define M_MAX_BURN_TIME   80
#define M_START_BURN_MESH 9
#define M_ALERT_DIST      SQUARE(WALL_L)     // = 1048576
#define M_ATTACK_DIST     SQUARE(WALL_L * 2) // = 4194304
#define M_WALK_TURN       (DEG_1 * 3)        // = 546
// clang-format on

typedef enum {
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_BURP,
    M_STATE_DEATH,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 5,
} M_ANIM;

typedef struct {
    int16_t burn_timer;
} M_PRIV;

static const BITE m_Gas = {
    .pos = { 0, 48, 140 },
    .mesh_num = 10,
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "burn_timer", &p->burn_timer));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "burn_timer", p->burn_timer);
}

static void M_TriggerGas(
    const XYZ_32 pos, const XYZ_32 vel, const int16_t effect_num)
{
    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = (Random_GetControl() & 0x3F) + 128;
    spark->src_color.g = (Random_GetControl() & 0x3F) + 128;
    spark->src_color.b = 32;
    spark->dst_color.r = (Random_GetControl() & 0xF) + 32;
    spark->dst_color.g = (Random_GetControl() & 0xF) + 32;
    spark->dst_color.b = 0;

    if (vel.x != 0 || vel.y != 0 || vel.z != 0) {
        spark->col_fade_speed = 6;
        spark->fade_to_black = 2;
        spark->life = (Random_GetControl() & 1) + 16;
    } else {
        spark->col_fade_speed = 8;
        spark->fade_to_black = 16;
        spark->life = (Random_GetControl() & 3) + 28;
    }

    spark->s_life = spark->life;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
    spark->vel.x = vel.x + (Random_GetControl() & 0xF) - 16;
    spark->vel.y = vel.y;
    spark->vel.z = vel.z + (Random_GetControl() & 0xF) - 16;
    spark->friction = 0;

    if ((Random_GetControl() & 1) != 0) {
        if (effect_num < 0) {
            spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
                | SPARK_F_SCALE;
        } else {
            spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_FX | SPARK_F_ROTATE
                | SPARK_F_SPRITE | SPARK_F_SCALE;
        }

        spark->rot_angle = Random_GetControl() & 0xFFF;

        if ((Random_GetControl() & 1) != 0) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = +16 + (Random_GetControl() & 0xF);
        }
    } else if (effect_num < 0) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    } else {
        spark->flags =
            SPARK_F_ALT_SPRITE | SPARK_F_FX | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->max_y_vel = 0;
    spark->effect_num = effect_num;
    spark->gravity = 0;

    const int32_t size = (Random_GetControl() & 0x1F) + 48;
    if (vel.x != 0 || vel.y != 0 || vel.z != 0) {
        spark->size.width = size >> 5;
        spark->src_size.width = spark->size.width;
        spark->dst_size.width = size >> 1;
        spark->size.height = spark->size.width;
        spark->src_size.height = spark->size.height;
        spark->dst_size.height = spark->dst_size.width;

        if (effect_num == -2) {
            spark->scalar = 2;
        } else {
            spark->scalar = 3;
        }
    } else {
        spark->scalar = 4;
        spark->size.width = size >> 4;
        spark->src_size.width = spark->size.width;
        spark->dst_size.width = size >> 1;
        spark->size.height = spark->size.width;
        spark->src_size.height = spark->size.height;
        spark->dst_size.height = spark->dst_size.width;
    }
    Sparks_FinishSetup(spark);
}

static void M_TriggerGasThrower(
    const ITEM *const item, const BITE *const bite, const int16_t speed)
{
    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num == NO_ITEM) {
        return;
    }

    XYZ_32 pos_1 = bite->pos;
    Collide_GetJointAbsPosition(item, &pos_1, bite->mesh_num);

    XYZ_32 pos_2 = {
        .x = bite->pos.x,
        .y = bite->pos.y << 1,
        .z = bite->pos.z << 3,
    };
    Collide_GetJointAbsPosition(item, &pos_2, bite->mesh_num);

    int16_t angles[2];
    Math_GetVectorAngles(
        pos_2.x - pos_1.x, pos_2.y - pos_1.y, pos_2.z - pos_1.z, angles);

    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos = pos_1;
    effect->rot.x = angles[1];
    effect->rot.y = angles[0];
    effect->speed = speed << 2;
    effect->object_id = O_MISSILE_POISON;
    effect->counter = 20;
    effect->flag1 = 1;

    M_TriggerGas((XYZ_32) {}, (XYZ_32) {}, effect_num);

    for (int32_t i = 0; i < 2; i++) {
        const int32_t s = Random_GetControl() % (speed << 2) + 32;
        const XYZ_32 vel = XYZ_32_FromYawPitch(effect->rot.y, effect->rot.x, s);
        M_TriggerGas(
            effect->pos, (XYZ_32) { vel.x << 5, vel.y << 5, vel.z << 5 }, -1);
    }

    {
        const XYZ_32 vel =
            XYZ_32_FromYawPitch(effect->rot.y, effect->rot.x, speed << 1);
        M_TriggerGas(
            effect->pos, (XYZ_32) { vel.x << 5, vel.y << 5, vel.z << 5 }, -2);
    }
}

static void M_BurnDeath(ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    for (int32_t mesh = M_START_BURN_MESH; mesh < obj->mesh_count; mesh++) {
        XYZ_32 pos = {};
        Collide_GetJointAbsPosition(item, &pos, mesh);
        Sparks_TriggerFireFlame(pos, -1, 255);
    }

    int32_t scale = 0;
    const int16_t frame_idx = Item_GetRelativeFrame(item);
    if (frame_idx > 16) {
        const ANIM *const anim = Item_GetAnim(item);
        const int16_t remaining_frames = anim->frame_end - item->frame_num;
        scale = MIN(remaining_frames, 16);
    } else {
        scale = frame_idx;
    }

    const int32_t rnd = Random_GetControl();
    const RGB_888 color = {
        .r = (scale * (255 - ((rnd >> 4) & 0x1F))) >> 4,
        .g = (scale * (192 - ((rnd >> 6) & 0x3F))) >> 4,
        .b = (scale * (rnd & 0x3F)) >> 4,
    };
    Output_AddDynamicLightRGB(item->pos, 12, color);
}

static void M_GasDeath(ITEM *const item)
{
    const ANIM *const anim = Item_GetAnim(item);
    const int16_t frame_idx = Item_GetRelativeFrame(item);
    const int16_t end_frame_idx = anim->frame_end - anim->frame_base - 8;

    if (frame_idx >= 1 && frame_idx <= end_frame_idx) {
        int16_t speed = frame_idx + 1;
        if (speed > 24) {
            const int16_t remaining_frames = end_frame_idx - frame_idx;
            if (remaining_frames <= 0) {
                speed = 1;
            } else if (remaining_frames > 24) {
                speed = (Random_GetControl() & 0xF) + 8;
            } else {
                speed = remaining_frames;
            }
        }

        CLAMPL(speed, 1);
        M_TriggerGasThrower(item, &m_Gas, speed);
    }
}

static void M_CalculateEnemy(ITEM *const item)
{
    CREATURE *const mutant = item->creature_data;
    ITEM *const lara_item = Lara_GetItem();
    mutant->enemy = lara_item;

    int32_t dx = lara_item->pos.x - item->pos.x;
    int32_t dz = lara_item->pos.z - item->pos.z;
    int32_t best_distance = XYZ_32_GetLength2((XYZ_32) { dx, 0, dz });

    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const creature = LOT_GetBaddieSlot(i);
        if (creature->item_num == NO_ITEM || creature == mutant) {
            continue;
        }

        const ITEM *const candidate = Item_Get(creature->item_num);
        if (candidate->object_id != O_LARA
            && candidate->object_id != O_RX_WORKER_3) {
            continue;
        }

        dx = candidate->pos.x - item->pos.x;
        dz = candidate->pos.z - item->pos.z;
        const int32_t distance = XYZ_32_GetLength2((XYZ_32) { dx, 0, dz });
        if (distance < best_distance) {
            mutant->enemy = (ITEM *)candidate;
            best_distance = distance;
        }
    }
}

static void M_ControlCrawler(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;
    int16_t tilt = 0;
    int16_t angle = 0;
    int16_t head = 0;
    int16_t torso_x = 0;
    int16_t torso_y = 0;

    M_PRIV *const p = item->priv;
    if (p->burn_timer > M_MAX_BURN_TIME) {
        Item_TakeFatalDamage(item, creature->enemy);
    }

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
            creature->flags = 0;
        } else if (p->burn_timer > M_MAX_BURN_TIME) {
            M_BurnDeath(item);
        } else {
            M_GasDeath(item);
        }
        goto finish;
    }

    if (item->ai_bits != 0) {
        Creature_GetAITarget(creature);
    } else {
        M_CalculateEnemy(item);
    }

    ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    AI_INFO info = {};
    Creature_AIInfo(item, &info);

    AI_INFO lara_info = {};
    if (creature->enemy == lara_item) {
        lara_info.angle = info.angle;
        lara_info.distance = info.distance;
    } else {
        const int32_t dx = lara_item->pos.x - item->pos.x;
        const int32_t dz = lara_item->pos.z - item->pos.z;
        lara_info.angle = Math_Atan(dz, dx) - item->rot.y;
        lara_info.distance = XYZ_32_GetLength2((XYZ_32) { dx, 0, dz });
    }

    const bool violent = info.zone_num == info.enemy_zone_num;
    Creature_UpdateMood(item, &info, violent);
    if (creature->enemy == lara_item && lara->poison.value >= M_MAX_POISON) {
        creature->mood = MOOD_ESCAPE;
    }
    Creature_ApplyMood(item, &info, violent);

    angle = Creature_Turn(item, creature->maximum_turn);

    ITEM *const enemy = creature->enemy;
    creature->enemy = lara_item;
    if ((lara_info.distance < M_ALERT_DIST || item->hit_status
         || Creature_CanSeeEnemy(item, &lara_info))
        && (item->ai_bits & AI_FOLLOW) == 0) {
        Creature_AlertAllGuards(item_num);
    }
    creature->enemy = enemy;

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        creature->maximum_turn = 0;
        creature->flags = 0;
        head = info.angle;

        if ((item->ai_bits & AI_GUARD) != 0) {
            head = Creature_AIGuard(creature);
            item->goal_anim_state = M_STATE_STOP;
        } else if ((item->ai_bits & AI_PATROL_1) != 0) {
            head = 0;
            item->goal_anim_state = M_STATE_WALK;
        } else if (creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (
            Creature_CanTargetEnemy(item, &info)
            && info.distance < M_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_BURP;
        } else if (item->required_anim_state != 0) {
            item->goal_anim_state = item->required_anim_state;
        } else {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_WALK:
        if (info.ahead) {
            head = info.angle;
        }
        creature->maximum_turn = M_WALK_TURN;

        if ((item->ai_bits & AI_PATROL_1) != 0) {
            head = 0;
            item->goal_anim_state = M_STATE_WALK;
        } else if (
            Creature_CanTargetEnemy(item, &info)
            && info.distance < M_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_BURP:
        if (ABS(info.angle) < M_WALK_TURN) {
            item->rot.y += info.angle;
        } else if (info.angle < 0) {
            item->rot.y -= M_WALK_TURN;
        } else {
            item->rot.y += M_WALK_TURN;
        }

        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle / 2;
        }

        if (Item_TestFrameRange(item, 35, 58)) {
            if (creature->flags < 24) {
                creature->flags += 3;
            }

            const int16_t speed = creature->flags < 24
                ? creature->flags
                : ((Random_GetControl() & 0xF) + 8);
            M_TriggerGasThrower(item, &m_Gas, speed);
            if (creature->enemy != nullptr && creature->enemy != lara_item) {
                creature->enemy->hit_status = true;
            }
        }
        break;
    }

finish:
    Creature_Tilt(item, 0);
    Creature_Joint(item, 0, torso_x);
    Creature_Joint(item, 1, torso_y);
    Creature_Joint(item, 2, head);
    Creature_Animate(item_num, angle, 0);
}

static void M_HandleEvent(
    ITEM *const item, const OBJECT_EVENT event, const void *const data)
{
    if (event != OBJECT_EVENT_BURNT) {
        return;
    }

    M_PRIV *const p = item->priv;
    p->burn_timer++;
}

static void M_ControlDying(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsInPlay(item)) {
        return;
    }

    M_GasDeath(item);
    Item_Animate(item);
}

static void M_SetupCrawler(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = Creature_Collision;
    obj->control_func = M_ControlCrawler;
    obj->event_func = M_HandleEvent;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->radius = M_RADIUS;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 8)->rot.x = true;
    Object_GetBone(obj, 8)->rot.z = true;
    Object_GetBone(obj, 9)->rot.y = true;
    OBJECT_PROPERTIES(obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS));
}

static void M_SetupDying(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_ControlDying;

    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_CRAWLER_MUTANT, M_SetupCrawler)
REGISTER_OBJECT(O_DYING_MUTANT, M_SetupDying)
