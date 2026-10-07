#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_RADIUS                           (WALL_L / 10)      // = 102
#define M_ALERT_DIST                       SQUARE(WALL_L)     // = 1048576
#define M_ALERT_HEIGHT                     (WALL_L * 2)       // = 2048
#define M_FOLLOW_DIST                      SQUARE(WALL_L * 2) // = 4194304
#define M_RUN_TURN                         (DEG_1 * 10)       // = 1820
#define M_DUCK_TURN                        DEG_1              // = 182
#define M_SHOOT_1_CHANCE                   0x2000
#define M_SHOOT_2_CHANCE                   0x4000
#define M_DUCK_END_CHANCE                  0x1F
#define M_NO_FRAME                         (-1)

#define M_RX_WORKER_1_HIT_POINTS           34
#define M_RX_WORKER_1_DAMAGE               35
#define M_RX_WORKER_1_FINAL_SHOT_DAMAGE    (M_RX_WORKER_1_DAMAGE * 3)
#define M_SECURITY_GUARD_HIT_POINTS        28
#define M_SECURITY_GUARD_DAMAGE            32
#define M_SECURITY_GUARD_FINAL_SHOT_DAMAGE (M_SECURITY_GUARD_DAMAGE * 2)
#define M_MP_2_HIT_POINTS                  28
#define M_MP_2_DAMAGE                      32
#define M_MP_2_FINAL_SHOT_DAMAGE           M_MP_2_DAMAGE
// clang-format on

typedef enum {
    M_STATE_NULL,
    M_STATE_WAIT,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_AIM_1,
    M_STATE_SHOOT_1,
    M_STATE_AIM_2,
    M_STATE_SHOOT_2,
    M_STATE_SHOOT_3A,
    M_STATE_SHOOT_3B,
    M_STATE_SHOOT_4A,
    M_STATE_AIM_3,
    M_STATE_AIM_4,
    M_STATE_DEATH,
    M_STATE_SHOOT_4B,
    M_STATE_DUCK_START,
    M_STATE_DUCKED,
    M_STATE_DUCK_AIM,
    M_STATE_DUCK_SHOOT,
    M_STATE_DUCK_WALK,
    M_STATE_DUCK_END,
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_SHOOT_1    = 1,
    M_ANIM_AIM_1      = 12,
    M_ANIM_DEATH      = 14,
    M_ANIM_WALK_STOP  = 17,
    M_ANIM_AIM_4A     = 18,
    M_ANIM_AIM_4B     = 19,
    M_ANIM_RUN_STOP_1 = 27,
    M_ANIM_RUN_STOP_2 = 28,
    // clang-format on
} M_ANIM;

typedef enum {
    M_ALERT_UNLESS_FOLLOWING,
    M_ALERT_WITHIN_HEIGHT,
    M_ALERT_ALWAYS,
} M_ALERT;

typedef struct {
    OBJECT_ID object_id;
    int32_t walk_dist;
    int16_t walk_turn;
    M_ALERT alert;
    SAMPLE_ID alert_sfx;
    struct {
        int16_t frames[2];
        SAMPLE_ID sfx;
        // Fires when the random draw masked with odds_mask matches odds_value.
        // A zero mask fires it without drawing.
        int32_t odds_mask;
        int32_t odds_value;
    } final_shot;
    bool tests_box_damage;
    bool picks_enemy;
    bool walks_before_shooting;
    bool watches_on_patrol;
    bool aim_4_miss_sets_goal;
    bool turns_on_duck_end;
    bool clamps_torso;
} M_VARIANT;

typedef struct {
    const M_VARIANT *variant;
    int32_t damage;
    int32_t final_shot_damage;
} M_PRIV;

static const M_VARIANT m_Variants[] = {
    {
        .object_id = O_RX_WORKER_1,
        .walk_dist = SQUARE(WALL_L * 2),
        .walk_turn = DEG_1 * 6,
        .alert = M_ALERT_UNLESS_FOLLOWING,
        .alert_sfx = SFX_AMERICAN_HOY,
        .final_shot = {
            .frames = { 47, M_NO_FRAME },
            .sfx = SFX_LONDON_SWAT_FIRE,
        },
        .walks_before_shooting = true,
        .watches_on_patrol = true,
        .turns_on_duck_end = true,
        .clamps_torso = true,
    },
    {
        .object_id = O_SECURITY_GUARD,
        .walk_dist = SQUARE(WALL_L * 2),
        .walk_turn = DEG_1 * 5,
        .alert = M_ALERT_WITHIN_HEIGHT,
        .alert_sfx = SFX_ENGLISH_HOY,
        .final_shot = {
            .frames = { 3, 28 },
            .sfx = SFX_SECURITY_GUARD_FIRE,
            .odds_mask = 1,
            .odds_value = 1,
        },
        .walks_before_shooting = true,
        .aim_4_miss_sets_goal = true,
    },
    {
        .object_id = O_MP_2,
        .walk_dist = SQUARE(WALL_L * 3 / 2),
        .walk_turn = DEG_1 * 6,
        .alert = M_ALERT_ALWAYS,
        .alert_sfx = SFX_AMERICAN_HOY,
        .final_shot = {
            .frames = { 1, M_NO_FRAME },
            .sfx = SFX_LONDON_SWAT_FIRE,
            .odds_mask = 3,
            .odds_value = 0,
        },
        .tests_box_damage = true,
        .picks_enemy = true,
    },
};

static const CREATURE_GUN m_Gun = {
    .muzzle = { .pos = { 0, 160, 40 }, .mesh_num = 13 },
    .tr3_enemy_flash = true,
    .tr3_flash = { .pos = { 0, 192, 40 }, .mesh_num = 13 },
    .tr3_enemy_weapon_flags = 0,
    .tr3_flash_shade = 600,
    .tr3_flash_rot_x = -DEG_90,
};

static const M_VARIANT *M_GetVariant(const OBJECT_ID object_id)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Variants); i++) {
        if (m_Variants[i].object_id == object_id) {
            return &m_Variants[i];
        }
    }
    ASSERT_FAIL();
    return nullptr;
}

static bool M_ShouldFireFinalShot(const M_VARIANT *const variant)
{
    return variant->final_shot.odds_mask == 0
        || (Random_GetControl() & variant->final_shot.odds_mask)
        == variant->final_shot.odds_value;
}

static void M_Initialise(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->variant = M_GetVariant(item->object_id);
}

static void M_FireFinalShot(
    const M_VARIANT *const variant, ITEM *const item, int16_t *const head,
    int16_t *const torso_y)
{
    const int16_t frame_idx = Item_GetRelativeFrame(item);
    if (frame_idx != variant->final_shot.frames[0]
        && frame_idx != variant->final_shot.frames[1]) {
        return;
    }

    const M_PRIV *const p = item->priv;
    Creature_FireFinalShot(
        item, &m_Gun, p->final_shot_damage, variant->final_shot.sfx, head,
        torso_y);
}

static void M_CalculateEnemy(ITEM *const item)
{
    CREATURE *const gunman = item->creature_data;
    const ITEM *const lara_item = Lara_GetItem();
    gunman->enemy = (ITEM *)lara_item;

    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
    int32_t best_distance = XYZ_32_GetLength2((XYZ_32) { dx, 0, dz });

    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const creature = LOT_GetBaddieSlot(i);
        if (creature->item_num == NO_ITEM || creature == gunman) {
            continue;
        }

        const ITEM *const candidate = Item_Get(creature->item_num);
        if (candidate != lara_item && candidate->object_id != O_PRISONER) {
            continue;
        }

        const XYZ_32 delta = {
            .x = candidate->pos.x - item->pos.x,
            .y = 0,
            .z = candidate->pos.z - item->pos.z,
        };
        const int32_t distance = XYZ_32_GetLength2(delta);
        if (distance < best_distance) {
            gunman->enemy = (ITEM *)candidate;
            best_distance = distance;
        }
    }
}

static bool M_ShouldAlert(
    const M_VARIANT *const variant, const ITEM *const item,
    const ITEM *const lara_item, const AI_INFO *const lara_info)
{
    switch (variant->alert) {
    case M_ALERT_UNLESS_FOLLOWING:
        return (lara_info->distance < M_ALERT_DIST || item->hit_status
                || Creature_CanSeeEnemy(item, lara_info))
            && (item->ai_bits & AI_FOLLOW) == 0;
    case M_ALERT_WITHIN_HEIGHT:
        return item->hit_status
            || ((lara_info->distance < M_ALERT_DIST
                 || Creature_CanSeeEnemy(item, lara_info))
                && ABS(lara_item->pos.y - item->pos.y) < M_ALERT_HEIGHT);
    case M_ALERT_ALWAYS:
        return item->hit_status || lara_info->distance < M_ALERT_DIST
            || Creature_CanSeeEnemy(item, lara_info);
    }
    return false;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    const M_VARIANT *const variant = p->variant;
    CREATURE *const creature = item->creature_data;

    int16_t angle = 0;
    int16_t head = 0;
    int16_t tilt = 0;
    int16_t torso_x = 0;
    int16_t torso_y = 0;

    if (variant->tests_box_damage) {
        Creature_TestBoxDamage(item_num);
    }

    if (item->hit_points <= 0) {
        item->hit_points = 0;
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
        } else if (M_ShouldFireFinalShot(variant)) {
            M_FireFinalShot(variant, item, &head, &torso_y);
        }

        goto finish;
    }

    ITEM *const lara_item = Lara_GetItem();
    if (item->ai_bits != 0) {
        Creature_GetAITarget(creature);
    } else if (variant->picks_enemy) {
        M_CalculateEnemy(item);
    } else {
        creature->enemy = lara_item;
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);

    AI_INFO lara_info = {};
    if (creature->enemy == lara_item) {
        lara_info.distance = info.distance;
        lara_info.angle = info.angle;
    } else {
        const int32_t dx = lara_item->pos.x - item->pos.x;
        const int32_t dz = lara_item->pos.z - item->pos.z;
        lara_info.angle = Math_Atan(dz, dx) - item->rot.y;
        lara_info.distance = XYZ_32_GetLength2((XYZ_32) { dx, 0, dz });
    }

    Creature_Mood(item, &info, creature->enemy != lara_item);
    angle = Creature_Turn(item, creature->maximum_turn);
    const bool near_cover = Creature_IsNearCover(item, &lara_info);

    ITEM *const enemy = creature->enemy;
    creature->enemy = lara_item;
    if (M_ShouldAlert(variant, item, lara_item, &lara_info)) {
        if (!creature->alerted) {
            Sound_Effect(variant->alert_sfx, &item->pos, SPM_NORMAL);
        }
        Creature_AlertAllGuards(item_num);
    }
    creature->enemy = enemy;

    const int16_t anim_idx = Item_GetRelativeAnim(item);
    const int16_t frame_idx = Item_GetRelativeFrame(item);
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    switch (item->current_anim_state) {
    case M_STATE_WAIT:
        head = lara_info.angle;
        creature->maximum_turn = 0;

        if (anim_idx == M_ANIM_WALK_STOP || anim_idx == M_ANIM_RUN_STOP_1
            || anim_idx == M_ANIM_RUN_STOP_2) {
            if (ABS(info.angle) < M_RUN_TURN) {
                item->rot.y += info.angle;
            } else if (info.angle < 0) {
                item->rot.y -= M_RUN_TURN;
            } else {
                item->rot.y += M_RUN_TURN;
            }
        }

        if ((item->ai_bits & AI_GUARD) != 0) {
            head = Creature_AIGuard(creature);
            item->goal_anim_state = M_STATE_WAIT;
        } else if ((item->ai_bits & AI_PATROL_1) != 0) {
            item->goal_anim_state = M_STATE_WALK;
            if (!variant->watches_on_patrol) {
                head = 0;
            }
        } else if (near_cover && (lara->target == item || item->hit_status)) {
            item->goal_anim_state = M_STATE_DUCK_START;
        } else if (item->required_anim_state == M_STATE_DUCK_START) {
            item->goal_anim_state = M_STATE_DUCK_START;
        } else if (creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_RUN;
        } else if (Creature_CanTargetEnemy(item, &info)) {
            if (variant->walks_before_shooting
                && info.distance > variant->walk_dist) {
                item->goal_anim_state = M_STATE_WALK;
            } else {
                const int32_t rnd = Random_GetControl();
                if (rnd < M_SHOOT_1_CHANCE) {
                    item->goal_anim_state = M_STATE_SHOOT_1;
                } else if (rnd < M_SHOOT_2_CHANCE) {
                    item->goal_anim_state = M_STATE_SHOOT_2;
                } else {
                    item->goal_anim_state = M_STATE_AIM_3;
                }
            }
        } else if (
            creature->mood == MOOD_BORED
            || ((item->ai_bits & AI_FOLLOW) != 0
                && (creature->reached_goal
                    || lara_info.distance > M_FOLLOW_DIST))) {
            if (info.ahead) {
                item->goal_anim_state = M_STATE_WAIT;
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
        } else {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_WALK:
        head = lara_info.angle;
        creature->maximum_turn = variant->walk_turn;

        if ((item->ai_bits & AI_PATROL_1) != 0) {
            item->goal_anim_state = M_STATE_WALK;
            head = 0;
        } else if (near_cover && (lara->target == item || item->hit_status)) {
            item->required_anim_state = M_STATE_DUCK_START;
            item->goal_anim_state = M_STATE_WAIT;
        } else if (creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_RUN;
        } else if (Creature_CanTargetEnemy(item, &info)) {
            if (info.distance > variant->walk_dist
                && info.zone_num == info.enemy_zone_num) {
                item->goal_anim_state = M_STATE_AIM_4;
            } else {
                item->goal_anim_state = M_STATE_WAIT;
            }
        } else if (creature->mood == MOOD_BORED) {
            if (info.ahead) {
                item->goal_anim_state = M_STATE_WALK;
            } else {
                item->goal_anim_state = M_STATE_WAIT;
            }
        } else {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_RUN:
        if (info.ahead) {
            head = info.angle;
        }
        creature->maximum_turn = M_RUN_TURN;
        tilt = angle / 2;

        if ((item->ai_bits & AI_GUARD) != 0) {
            item->goal_anim_state = M_STATE_WAIT;
        } else if (near_cover && (lara->target == item || item->hit_status)) {
            item->required_anim_state = M_STATE_DUCK_START;
            item->goal_anim_state = M_STATE_WAIT;
        } else if (creature->mood != MOOD_ESCAPE) {
            if (Creature_CanTargetEnemy(item, &info)
                || ((item->ai_bits & AI_FOLLOW) != 0
                    && (creature->reached_goal
                        || lara_info.distance > M_FOLLOW_DIST))) {
                item->goal_anim_state = M_STATE_WAIT;
            } else if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_WALK;
            }
        }
        break;

    case M_STATE_AIM_1:
        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle;
        }

        if (anim_idx == M_ANIM_AIM_1
            || (anim_idx == M_ANIM_SHOOT_1 && frame_idx == 10)) {
            if (!Creature_Shoot(item, &info, &m_Gun, torso_y, p->damage)) {
                item->required_anim_state = M_STATE_WAIT;
            }
        } else if (Creature_ShouldDuck(item, near_cover)) {
            item->required_anim_state = M_STATE_DUCK_START;
            item->goal_anim_state = M_STATE_WAIT;
        }
        break;

    case M_STATE_SHOOT_1:
        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle;
        }

        if (item->required_anim_state == M_STATE_WAIT) {
            item->goal_anim_state = M_STATE_WAIT;
        }
        break;

    case M_STATE_SHOOT_2:
        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle;
        }

        if (frame_idx == 0) {
            if (!Creature_Shoot(item, &info, &m_Gun, torso_y, p->damage)) {
                item->goal_anim_state = M_STATE_WAIT;
            }
        } else if (Creature_ShouldDuck(item, near_cover)) {
            item->required_anim_state = M_STATE_DUCK_START;
            item->goal_anim_state = M_STATE_WAIT;
        }
        break;

    case M_STATE_SHOOT_3A:
    case M_STATE_SHOOT_3B:
        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle;
        }

        if (frame_idx == 0 || frame_idx == 11) {
            if (!Creature_Shoot(item, &info, &m_Gun, torso_y, p->damage)) {
                item->goal_anim_state = M_STATE_WAIT;
            }
        } else if (Creature_ShouldDuck(item, near_cover)) {
            item->required_anim_state = M_STATE_DUCK_START;
            item->goal_anim_state = M_STATE_WAIT;
        }
        break;

    case M_STATE_AIM_4:
        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle;
        }

        if ((anim_idx == M_ANIM_AIM_4A && frame_idx == 16)
            || (anim_idx == M_ANIM_AIM_4B && frame_idx == 6)) {
            if (!Creature_Shoot(item, &info, &m_Gun, torso_y, p->damage)) {
                if (variant->aim_4_miss_sets_goal) {
                    item->goal_anim_state = M_STATE_WALK;
                } else {
                    item->required_anim_state = M_STATE_WALK;
                }
            }
        } else if (Creature_ShouldDuck(item, near_cover)) {
            item->required_anim_state = M_STATE_DUCK_START;
            item->goal_anim_state = M_STATE_WAIT;
        }

        if (info.distance < variant->walk_dist) {
            item->required_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_SHOOT_4A:
    case M_STATE_SHOOT_4B:
        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle;
        }

        if (item->required_anim_state == M_STATE_WALK) {
            item->goal_anim_state = M_STATE_WALK;
        }

        if (frame_idx == 16
            && !Creature_Shoot(item, &info, &m_Gun, torso_y, p->damage)) {
            item->goal_anim_state = M_STATE_WALK;
        }

        if (info.distance < variant->walk_dist) {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_DUCKED:
        if (info.ahead) {
            head = info.angle;
        }
        creature->maximum_turn = 0;

        if (Creature_CanTargetEnemy(item, &info)) {
            item->goal_anim_state = M_STATE_DUCK_AIM;
        } else if (
            item->hit_status || !near_cover
            || (info.ahead && (Random_GetControl() & M_DUCK_END_CHANCE) == 0)) {
            item->goal_anim_state = M_STATE_DUCK_END;
        } else {
            item->goal_anim_state = M_STATE_DUCK_WALK;
        }
        break;

    case M_STATE_DUCK_AIM:
        if (info.ahead) {
            torso_y = info.angle;
        }
        creature->maximum_turn = M_DUCK_TURN;

        if (Creature_CanTargetEnemy(item, &info)) {
            item->goal_anim_state = M_STATE_DUCK_SHOOT;
        } else {
            item->goal_anim_state = M_STATE_DUCKED;
        }
        break;

    case M_STATE_DUCK_SHOOT:
        if (info.ahead) {
            torso_y = info.angle;
        }

        if (frame_idx == 0
            && (!Creature_Shoot(item, &info, &m_Gun, torso_y, p->damage)
                || (Random_GetControl() & 7) == 0)) {
            item->goal_anim_state = M_STATE_DUCKED;
        }
        break;

    case M_STATE_DUCK_WALK:
        if (info.ahead) {
            head = info.angle;
        }
        creature->maximum_turn = variant->walk_turn;

        if (Creature_CanTargetEnemy(item, &info) || item->hit_status
            || !near_cover
            || (info.ahead && (Random_GetControl() & M_DUCK_END_CHANCE) == 0)) {
            item->goal_anim_state = M_STATE_DUCKED;
        }
        break;

    case M_STATE_DUCK_END:
        if (!variant->turns_on_duck_end) {
            break;
        }
        if (ABS(info.angle) < variant->walk_turn) {
            item->rot.y += info.angle;
        } else if (info.angle < 0) {
            item->rot.y -= variant->walk_turn;
        } else {
            item->rot.y += variant->walk_turn;
        }
        break;

    default:
        break;
    }

finish:
    if (variant->clamps_torso) {
        CLAMP(torso_y, -DEG_45, DEG_45);
    }
    Creature_Tilt(item, tilt);
    Creature_Joint(item, 0, torso_y);
    Creature_Joint(item, 1, torso_x);
    Creature_Joint(item, 2, head);
    Creature_Animate(item_num, angle, 0);
}

static void M_SetupCommon(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PRIV);
    obj->initialise_func = M_Initialise;
    obj->collision_func = Creature_Collision;
    obj->control_func = M_Control;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->radius = M_RADIUS;

    obj->intelligent = true;
    obj->save_anim = true;
    obj->save_flags = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    Object_GetBone(obj, 6)->rot.x = true;
    Object_GetBone(obj, 6)->rot.y = true;
    Object_GetBone(obj, 13)->rot.y = true;
}

static void M_SetupRXWorker1(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    M_SetupCommon(obj);
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_RX_WORKER_1_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_RX_WORKER_1_DAMAGE, "Damage dealt by shots."),
        OBJECT_PROPERTY(
            M_PRIV, final_shot_damage, M_RX_WORKER_1_FINAL_SHOT_DAMAGE,
            "Damage dealt by the death-state final shot."));
}

static void M_SetupSecurityGuard(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    M_SetupCommon(obj);
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_SECURITY_GUARD_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_SECURITY_GUARD_DAMAGE, "Damage dealt by shots."),
        OBJECT_PROPERTY(
            M_PRIV, final_shot_damage, M_SECURITY_GUARD_FINAL_SHOT_DAMAGE,
            "Damage dealt by the death-state final shot."));
}

static void M_SetupMP2(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    M_SetupCommon(obj);
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_MP_2_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_MP_2_DAMAGE, "Damage dealt by gun shots."),
        OBJECT_PROPERTY(
            M_PRIV, final_shot_damage, M_MP_2_FINAL_SHOT_DAMAGE,
            "Damage dealt by the death-state final shot."));
}

REGISTER_OBJECT(O_RX_WORKER_1, M_SetupRXWorker1)
REGISTER_OBJECT(O_SECURITY_GUARD, M_SetupSecurityGuard)
REGISTER_OBJECT(O_MP_2, M_SetupMP2)
