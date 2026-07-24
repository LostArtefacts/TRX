#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/game/creature.h>
#include <trx/game/effects.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS  400
#define M_FLY_MODE    0x8000
#define M_TIMER       0x7FFF
#define M_FIRE_ARC    (DEG_1 * 30) // = 5460
#define M_FLY_TURN    (DEG_1 * 5) // = 910
#define M_RUN_TURN    (DEG_1 * 6) // = 1092
#define M_LAND_CHANCE 256
#define M_DIE_TIME    (LOGIC_FPS * 16) // = 480
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_FLY,
    M_STATE_RUN,
    M_STATE_AIM,
    M_STATE_SEMIDEATH,
    M_STATE_SHOOT,
    M_STATE_FALL,
    M_STATE_STAND,
    M_STATE_DEATH,
} M_STATE;

static BITE m_NatlaGun = {
    .pos = { 5, 220, 7 },
    .mesh_num = 4,
};

static int32_t M_GetStage2HitPoints(const ITEM *const item)
{
    return item->max_hit_points / 2;
}

static bool M_GunHit(
    ITEM *const item, const GAME_VECTOR *const start,
    const GAME_VECTOR *const hit_pos, int32_t *const damage)
{
    if (item->current_anim_state == M_STATE_SEMIDEATH) {
        if (damage != nullptr) {
            *damage = 0;
        }
        return false;
    }
    return true;
}

static bool M_IsTargetable(const ITEM *const item)
{
    return item->hit_points > 0 && Item_IsInPlay(item)
        && item->current_anim_state != M_STATE_SEMIDEATH;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const natla = item->creature_data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;
    int16_t gun = natla->head_rotation * 7 / 8;
    int16_t timer = natla->flags & M_TIMER;
    int16_t facing = (int16_t)(intptr_t)item->priv;

    if (item->hit_points <= 0
        && item->current_anim_state != M_STATE_SEMIDEATH) {
        item->goal_anim_state = M_STATE_DEATH;
    } else if (item->hit_points <= M_GetStage2HitPoints(item)) {
        natla->lot.setup.step = STEP_L;
        natla->lot.setup.drop = -STEP_L;
        natla->lot.setup.fly = 0;

        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead && item->current_anim_state != M_STATE_SEMIDEATH) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, M_RUN_TURN);

        int8_t shoot = info.angle > -M_FIRE_ARC && info.angle < M_FIRE_ARC
            && Creature_CanTargetEnemy(item, &info);

        if (facing) {
            item->rot.y += facing;
            facing = 0;
        }

        switch (item->current_anim_state) {
        case M_STATE_FALL:
            if (item->pos.y < item->floor) {
                item->gravity = true;
                item->speed = 0;
            } else {
                item->gravity = false;
                item->goal_anim_state = M_STATE_SEMIDEATH;
                item->pos.y = item->floor;
                timer = 0;
            }
            break;

        case M_STATE_STAND:
            if (!shoot) {
                item->goal_anim_state = M_STATE_RUN;
            }
            if (timer >= 20) {
                int16_t effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_AtlanteanShard);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    gun = effect->rot.x;
                    Sound_Effect(
                        SFX_ATLANTEAN_NEEDLE, &effect->pos, SPM_NORMAL);
                }
                timer = 0;
            }
            break;

        case M_STATE_RUN:
            tilt = angle;
            if (timer >= 20) {
                int16_t effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_AtlanteanShard);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    gun = effect->rot.x;
                    Sound_Effect(
                        SFX_ATLANTEAN_NEEDLE, &effect->pos, SPM_NORMAL);
                }
                timer = 0;
            }
            if (shoot) {
                item->goal_anim_state = M_STATE_STAND;
            }
            break;

        case M_STATE_SEMIDEATH:
            if (timer == M_DIE_TIME) {
                item->goal_anim_state = M_STATE_STAND;
                natla->flags = 0;
                timer = 0;
                item->hit_points = M_GetStage2HitPoints(item);
                const MUSIC_PLAY_MODE mode =
                    g_Config.audio.fix_speeches_killing_music ? MPM_OVERLAY
                                                              : MPM_NO_REPEAT;
                Music_Play(MX_NATLA_SPEECH, mode);
            } else {
                if (g_Config.gameplay.target_mode == TARGET_LOCK_MODE_SEMI
                    || g_Config.gameplay.target_mode == TARGET_LOCK_MODE_NONE) {
                    LARA_INFO *const lara = Lara_GetLaraInfo();
                    lara->target = nullptr;
                }
                item->hit_points = 0;
            }
            break;

        case M_STATE_FLY:
            item->goal_anim_state = M_STATE_FALL;
            timer = 0;
            break;

        case M_STATE_STOP:
        case M_STATE_AIM:
        case M_STATE_SHOOT:
            item->goal_anim_state = M_STATE_SEMIDEATH;
            item->trigger = (ITEM_TRIGGER_STATE) { 0 };
            timer = 0;
            break;
        }
    } else {
        natla->lot.setup.step = STEP_L;
        natla->lot.setup.drop = -STEP_L;
        natla->lot.setup.fly = 0;

        AI_INFO info;
        Creature_AIInfo(item, &info);

        int8_t shoot = info.angle > -M_FIRE_ARC && info.angle < M_FIRE_ARC
            && Creature_CanTargetEnemy(item, &info);
        if (item->current_anim_state == M_STATE_FLY
            && (natla->flags & M_FLY_MODE)) {
            if (shoot && Random_GetControl() < M_LAND_CHANCE) {
                natla->flags &= ~M_FLY_MODE;
            }
            if (!(natla->flags & M_FLY_MODE)) {
                Creature_Mood(item, &info, true);
            }
            natla->lot.setup.step = WALL_L * 20;
            natla->lot.setup.drop = -WALL_L * 20;
            natla->lot.setup.fly = STEP_L / 8;
            Creature_AIInfo(item, &info);
        } else if (!shoot) {
            natla->flags |= M_FLY_MODE;
        }

        if (info.ahead) {
            head = info.angle;
        }

        if (item->current_anim_state != M_STATE_FLY
            || (natla->flags & M_FLY_MODE)) {
            Creature_Mood(item, &info, false);
        }

        item->rot.y -= facing;
        angle = Creature_Turn(item, M_FLY_TURN);

        if (item->current_anim_state == M_STATE_FLY) {
            if (info.angle > M_FLY_TURN) {
                facing += M_FLY_TURN;
            } else if (info.angle < -M_FLY_TURN) {
                facing -= M_FLY_TURN;
            } else {
                facing += info.angle;
            }
            item->rot.y += facing;
        } else {
            item->rot.y += facing - angle;
            facing = 0;
        }

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            timer = 0;
            if (natla->flags & M_FLY_MODE) {
                item->goal_anim_state = M_STATE_FLY;
            } else {
                item->goal_anim_state = M_STATE_AIM;
            }
            break;

        case M_STATE_FLY:
            if (!(natla->flags & M_FLY_MODE) && item->pos.y == item->floor) {
                item->goal_anim_state = M_STATE_STOP;
            }
            if (timer >= 30) {
                int16_t effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_AtlanteanBomb);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    gun = effect->rot.x;
                    Sound_Effect(
                        SFX_ATLANTEAN_NEEDLE, &effect->pos, SPM_NORMAL);
                }
                timer = 0;
            }
            break;

        case M_STATE_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (shoot) {
                item->goal_anim_state = M_STATE_SHOOT;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_SHOOT:
            if (!item->required_anim_state) {
                int16_t effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_AtlanteanBomb);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    gun = effect->rot.x;
                }
                effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_AtlanteanBomb);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    effect->rot.y += (Random_GetControl() - 0x4000) / 4;
                }
                effect_num =
                    Creature_Effect(item, &m_NatlaGun, Spawn_AtlanteanBomb);
                if (effect_num != NO_EFFECT) {
                    EFFECT *effect = Effect_Get(effect_num);
                    effect->rot.y += (Random_GetControl() - 0x4000) / 4;
                }
                item->required_anim_state = M_STATE_STOP;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);

    natla->neck_rotation = -head;
    if (gun) {
        natla->head_rotation = gun;
    }

    timer++;
    natla->flags &= ~M_TIMER;
    natla->flags |= timer & M_TIMER;

    item->rot.y -= facing;
    Creature_Animate(item_num, angle, 0);
    item->rot.y += facing;

    item->priv = (void *)(intptr_t)facing;
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->collision_func = Creature_Collision;
    obj->initialise_func = Creature_Initialise;
    obj->control_func = M_Control;
    obj->gun_hit_func = M_GunHit;
    obj->is_targetable_func = M_IsTargetable;

    obj->shadow_size = UNIT_SHADOW / 2;

    obj->radius = WALL_L / 5;
    obj->smartness = 0x7FFF;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    Object_GetBone(obj, 2)->rot.x = true;
    Object_GetBone(obj, 2)->rot.z = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."));
}

REGISTER_OBJECT(O_NATLA, M_Setup)
