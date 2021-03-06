#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/box.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/people.h"
#include "game/vars.h"
#include "game/warrior.h"
#include "specific/init.h"
#include "util.h"

#define CENTAUR_PART_DAMAGE 100
#define CENTAUR_REAR_DAMAGE 200
#define CENTAUR_TOUCH 0x30199
#define CENTAUR_DIE_ANIM 8
#define CENTAUR_TURN (PHD_DEGREE * 4) // = 728
#define CENTAUR_REAR_CHANCE 96
#define CENTAUR_REAR_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296

#define FLYER_CHARGE_DAMAGE 100
#define FLYER_LUNGE_DAMAGE 150
#define FLYER_PUNCH_DAMAGE 200
#define FLYER_PART_DAMAGE 100
#define FLYER_WALK_TURN (PHD_DEGREE * 2) // = 364
#define FLYER_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define FLYER_POSE_CHANCE 80
#define FLYER_UNPOSE_CHANCE 256
#define FLYER_WALK_RANGE SQUARE(WALL_L * 9 / 2) // = 21233664
#define FLYER_ATTACK1_RANGE SQUARE(600) // = 360000
#define FLYER_ATTACK2_RANGE SQUARE(WALL_L * 5 / 2) // = 6553600
#define FLYER_ATTACK3_RANGE SQUARE(300) // = 90000
#define FLYER_ATTACK_RANGE SQUARE(WALL_L * 15 / 4) // = 14745600
#define FLYER_TOUCH 0x678
#define FLYER_BULLET1 1
#define FLYER_BULLET2 2
#define FLYER_FLYMODE 4
#define FLYER_TWIST 8

#define SHARD_DAMAGE 30
#define SHARD_SPEED 250
#define ROCKET_DAMAGE 100
#define ROCKET_RANGE SQUARE(WALL_L) // = 1048576
#define ROCKET_SPEED 220

typedef enum {
    CENTAUR_EMPTY = 0,
    CENTAUR_STOP = 1,
    CENTAUR_SHOOT = 2,
    CENTAUR_RUN = 3,
    CENTAUR_AIM = 4,
    CENTAUR_DEATH = 5,
    CENTAUR_WARNING = 6,
} CENTAUR_ANIM;

enum FLYER_ANIM {
    FLYER_EMPTY = 0,
    FLYER_STOP = 1,
    FLYER_WALK = 2,
    FLYER_RUN = 3,
    FLYER_ATTACK1 = 4,
    FLYER_DEATH = 5,
    FLYER_POSE = 6,
    FLYER_ATTACK2 = 7,
    FLYER_ATTACK3 = 8,
    FLYER_AIM1 = 9,
    FLYER_AIM2 = 10,
    FLYER_SHOOT = 11,
    FLYER_MUMMY = 12,
    FLYER_FLY = 13,
};

enum MUMMY_ANIM {
    MUMMY_EMPTY = 0,
    MUMMY_STOP = 1,
    MUMMY_DEATH = 2,
};

static BITE_INFO CentaurRocket = { 11, 415, 41, 13 };
static BITE_INFO CentaurRear = { 50, 30, 0, 5 };
static BITE_INFO WarriorBite = { -27, 98, 0, 10 };
static BITE_INFO WarriorRocket = { 51, 213, 0, 14 };
static BITE_INFO WarriorShard = { -35, 269, 0, 9 };

void CentaurControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* centaur = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != CENTAUR_DEATH) {
            item->current_anim_state = CENTAUR_DEATH;
            item->anim_number =
                Objects[O_CENTAUR].anim_index + CENTAUR_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, CENTAUR_TURN);

        switch (item->current_anim_state) {
        case CENTAUR_STOP:
            centaur->neck_rotation = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.bite && info.distance < CENTAUR_REAR_RANGE) {
                item->goal_anim_state = CENTAUR_RUN;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = CENTAUR_AIM;
            } else {
                item->goal_anim_state = CENTAUR_RUN;
            }
            break;

        case CENTAUR_RUN:
            if (info.bite && info.distance < CENTAUR_REAR_RANGE) {
                item->required_anim_state = CENTAUR_WARNING;
                item->goal_anim_state = CENTAUR_STOP;
            } else if (Targetable(item, &info)) {
                item->required_anim_state = CENTAUR_AIM;
                item->goal_anim_state = CENTAUR_STOP;
            } else if (GetRandomControl() < CENTAUR_REAR_CHANCE) {
                item->required_anim_state = CENTAUR_WARNING;
                item->goal_anim_state = CENTAUR_STOP;
            }
            break;

        case CENTAUR_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = CENTAUR_SHOOT;
            } else {
                item->goal_anim_state = CENTAUR_STOP;
            }
            break;

        case CENTAUR_SHOOT:
            if (item->required_anim_state == CENTAUR_EMPTY) {
                item->required_anim_state = CENTAUR_AIM;
                int16_t fx_num =
                    CreatureEffect(item, &CentaurRocket, RocketGun);
                if (fx_num != NO_ITEM) {
                    centaur->neck_rotation = Effects[fx_num].pos.x_rot;
                }
            }
            break;

        case CENTAUR_WARNING:
            if (item->required_anim_state == CENTAUR_EMPTY
                && (item->touch_bits & CENTAUR_TOUCH)) {
                CreatureEffect(item, &CentaurRear, DoBloodSplat);
                LaraItem->hit_points -= CENTAUR_REAR_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = CENTAUR_STOP;
            }
            break;
        }
    }

    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, 0);

    if (item->status == IS_DEACTIVATED) {
        SoundEffect(171, &item->pos, 0);
        ExplodingDeath(item_num, -1, CENTAUR_PART_DAMAGE);
        KillItem(item_num);
        item->status = IS_DEACTIVATED;
    }
}

void InitialiseWarrior2(int16_t item_num)
{
    InitialiseCreature(item_num);
    Items[item_num].mesh_bits = 0xFFE07FFF;
}

void FlyerControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* flyer = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (ExplodingDeath(item_num, -1, FLYER_PART_DAMAGE)) {
            SoundEffect(171, &item->pos, 0);
            DisableBaddieAI(item_num);
            KillItem(item_num);
            item->status = IS_DEACTIVATED;
            return;
        }
    } else {
        flyer->LOT.step = STEP_L;
        flyer->LOT.drop = -STEP_L;
        flyer->LOT.fly = 0;

        AI_INFO info;
        CreatureAIInfo(item, &info);

        int32_t shoot1 = 0;
        int32_t shoot2 = 0;
        if (item->object_number != O_WARRIOR3 && Targetable(item, &info)
            && (info.zone_number != info.enemy_zone
                || info.distance > FLYER_ATTACK_RANGE)) {
            if (info.angle > 0 && info.angle < PHD_45) {
                shoot1 = 1;
            } else if (info.angle < 0 && info.angle > -PHD_45) {
                shoot2 = 1;
            }
        }

        if (item->object_number == O_WARRIOR1) {
            if (item->current_anim_state == FLYER_FLY) {
                if ((flyer->flags & FLYER_FLYMODE) && flyer->mood != MOOD_ESCAPE
                    && info.zone_number == info.enemy_zone) {
                    flyer->flags &= ~FLYER_FLYMODE;
                }

                if (!(flyer->flags & FLYER_FLYMODE)) {
                    CreatureMood(item, &info, 1);
                }

                flyer->LOT.step = WALL_L * 30;
                flyer->LOT.drop = -WALL_L * 30;
                flyer->LOT.fly = STEP_L / 8;
                CreatureAIInfo(item, &info);
            } else if (
                (info.zone_number != info.enemy_zone && !shoot1 && !shoot2
                 && (!info.ahead || flyer->mood == MOOD_BORED))
                || flyer->mood == MOOD_ESCAPE) {
                flyer->flags |= FLYER_FLYMODE;
            }
        }

        if (info.ahead) {
            head = info.angle;
        }

        if (item->current_anim_state != FLYER_FLY) {
            CreatureMood(item, &info, 0);
        } else if (flyer->flags & FLYER_FLYMODE) {
            CreatureMood(item, &info, 1);
        }

        angle = CreatureTurn(item, flyer->maximum_turn);

        switch (item->current_anim_state) {
        case FLYER_MUMMY:
            item->goal_anim_state = FLYER_STOP;
            break;

        case FLYER_STOP:
            flyer->flags &= ~(FLYER_BULLET1 | FLYER_BULLET2 | FLYER_TWIST);
            if (flyer->flags & FLYER_FLYMODE) {
                item->goal_anim_state = FLYER_FLY;
            } else if (item->touch_bits & FLYER_TOUCH) {
                item->goal_anim_state = FLYER_ATTACK3;
            } else if (info.bite && info.distance < FLYER_ATTACK3_RANGE) {
                item->goal_anim_state = FLYER_ATTACK3;
            } else if (info.bite && info.distance < FLYER_ATTACK1_RANGE) {
                item->goal_anim_state = FLYER_ATTACK1;
            } else if (shoot1) {
                item->goal_anim_state = FLYER_AIM1;
            } else if (shoot2) {
                item->goal_anim_state = FLYER_AIM2;
            } else if (
                flyer->mood == MOOD_BORED
                || (flyer->mood == MOOD_STALK
                    && info.distance < FLYER_WALK_RANGE)) {
                item->goal_anim_state = FLYER_POSE;
            } else {
                item->goal_anim_state = FLYER_RUN;
            }
            break;

        case FLYER_POSE:
            head = 0;
            if (shoot1 || shoot2 || (flyer->flags & FLYER_FLYMODE)) {
                item->goal_anim_state = FLYER_STOP;
            } else if (flyer->mood == MOOD_STALK) {
                if (info.distance < FLYER_WALK_RANGE) {
                    if (info.zone_number == info.enemy_zone
                        || GetRandomControl() < FLYER_UNPOSE_CHANCE) {
                        item->goal_anim_state = FLYER_WALK;
                    }
                } else {
                    item->goal_anim_state = FLYER_STOP;
                }
            } else if (
                flyer->mood == MOOD_BORED
                && GetRandomControl() < FLYER_UNPOSE_CHANCE) {
                item->goal_anim_state = FLYER_WALK;
            } else if (
                flyer->mood == MOOD_ATTACK || flyer->mood == MOOD_ESCAPE) {
                item->goal_anim_state = FLYER_STOP;
            }
            break;

        case FLYER_WALK:
            flyer->maximum_turn = FLYER_WALK_TURN;
            if (shoot1 || shoot2 || (flyer->flags & FLYER_FLYMODE)) {
                item->goal_anim_state = FLYER_STOP;
            } else if (
                flyer->mood == MOOD_ATTACK || flyer->mood == MOOD_ESCAPE) {
                item->goal_anim_state = FLYER_STOP;
            } else if (
                flyer->mood == MOOD_BORED
                || (flyer->mood == MOOD_STALK
                    && info.zone_number != info.enemy_zone)) {
                if (GetRandomControl() < FLYER_POSE_CHANCE) {
                    item->goal_anim_state = FLYER_POSE;
                }
            } else if (
                flyer->mood == MOOD_STALK && info.distance > FLYER_WALK_RANGE) {
                item->goal_anim_state = FLYER_STOP;
            }
            break;

        case FLYER_RUN:
            flyer->maximum_turn = FLYER_RUN_TURN;
            if (flyer->flags & FLYER_FLYMODE) {
                item->goal_anim_state = FLYER_STOP;
            } else if (item->touch_bits & FLYER_TOUCH) {
                item->goal_anim_state = FLYER_STOP;
            } else if (info.bite && info.distance < FLYER_ATTACK1_RANGE) {
                item->goal_anim_state = FLYER_STOP;
            } else if (info.ahead && info.distance < FLYER_ATTACK2_RANGE) {
                item->goal_anim_state = FLYER_ATTACK2;
            } else if (shoot1 || shoot2) {
                item->goal_anim_state = FLYER_STOP;
            } else if (
                flyer->mood == MOOD_BORED
                || (flyer->mood == MOOD_STALK
                    && info.distance < FLYER_WALK_RANGE)) {
                item->goal_anim_state = FLYER_STOP;
            }
            break;

        case FLYER_ATTACK1:
            if (item->required_anim_state == FLYER_EMPTY
                && (item->touch_bits & FLYER_TOUCH)) {
                CreatureEffect(item, &WarriorBite, DoBloodSplat);
                LaraItem->hit_points -= FLYER_LUNGE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = FLYER_STOP;
            }
            break;

        case FLYER_ATTACK2:
            if (item->required_anim_state == FLYER_EMPTY
                && (item->touch_bits & FLYER_TOUCH)) {
                CreatureEffect(item, &WarriorBite, DoBloodSplat);
                LaraItem->hit_points -= FLYER_CHARGE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = FLYER_RUN;
            }
            break;

        case FLYER_ATTACK3:
            if (item->required_anim_state == FLYER_EMPTY
                && (item->touch_bits & FLYER_TOUCH)) {
                CreatureEffect(item, &WarriorBite, DoBloodSplat);
                LaraItem->hit_points -= FLYER_PUNCH_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = FLYER_STOP;
            }
            break;

        case FLYER_AIM1:
            flyer->flags |= FLYER_TWIST;
            flyer->flags |= FLYER_BULLET1;
            if (shoot1) {
                item->goal_anim_state = FLYER_SHOOT;
            } else {
                item->goal_anim_state = FLYER_STOP;
            }
            break;

        case FLYER_AIM2:
            flyer->flags |= FLYER_BULLET2;
            if (shoot2) {
                item->goal_anim_state = FLYER_SHOOT;
            } else {
                item->goal_anim_state = FLYER_STOP;
            }
            break;

        case FLYER_SHOOT:
            if (flyer->flags & FLYER_BULLET1) {
                flyer->flags &= ~FLYER_BULLET1;
                CreatureEffect(item, &WarriorShard, ShardGun);
            } else if (flyer->flags & FLYER_BULLET2) {
                flyer->flags &= ~FLYER_BULLET2;
                CreatureEffect(item, &WarriorRocket, RocketGun);
            }
            break;

        case FLYER_FLY:
            if (!(flyer->flags & FLYER_FLYMODE) && item->pos.y == item->floor) {
                item->goal_anim_state = FLYER_STOP;
            }
            break;
        }
    }

    if (!(flyer->flags & FLYER_TWIST)) {
        flyer->head_rotation = flyer->neck_rotation;
    }

    CreatureHead(item, head);

    if (!(flyer->flags & FLYER_TWIST)) {
        flyer->neck_rotation = flyer->head_rotation;
        flyer->head_rotation = 0;
    } else {
        flyer->neck_rotation = 0;
    }

    CreatureAnimation(item_num, angle, 0);
}

void ControlMissile(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];

    int32_t speed = (fx->speed * phd_cos(fx->pos.x_rot)) >> W2V_SHIFT;
    fx->pos.y += (fx->speed * phd_sin(-fx->pos.x_rot)) >> W2V_SHIFT;
    fx->pos.z += (speed * phd_cos(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->pos.x += (speed * phd_sin(fx->pos.y_rot)) >> W2V_SHIFT;

    int16_t room_num = fx->room_number;
    FLOOR_INFO* floor = GetFloor(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);
    int32_t height = GetHeight(floor, fx->pos.x, fx->pos.y, fx->pos.z);
    int32_t ceiling = GetCeiling(floor, fx->pos.x, fx->pos.y, fx->pos.z);

    if (fx->pos.y >= height || fx->pos.y <= ceiling) {
        if (fx->object_number == O_MISSILE2) {
            fx->object_number = O_RICOCHET1;
            fx->frame_number = -GetRandomControl() / 11000;
            fx->speed = 0;
            fx->counter = 6;
            SoundEffect(10, &fx->pos, 0);
        } else {
            fx->object_number = O_EXPLOSION1;
            fx->frame_number = 0;
            fx->speed = 0;
            fx->counter = 0;
            SoundEffect(104, &fx->pos, 0);

            int32_t x = fx->pos.x - LaraItem->pos.x;
            int32_t y = fx->pos.y - LaraItem->pos.y;
            int32_t z = fx->pos.z - LaraItem->pos.z;
            int32_t range = SQUARE(x) + SQUARE(y) + SQUARE(z);
            if (range < ROCKET_RANGE) {
                LaraItem->hit_points -= (int16_t)(
                    ROCKET_DAMAGE * (ROCKET_RANGE - range) / ROCKET_RANGE);
                LaraItem->hit_status = 1;
            }
        }
        return;
    }

    if (room_num != fx->room_number) {
        EffectNewRoom(fx_num, room_num);
    }

    if (!ItemNearLara(&fx->pos, 200)) {
        return;
    }

    if (fx->object_number == O_MISSILE2) {
        LaraItem->hit_points -= SHARD_DAMAGE;
        fx->object_number = O_BLOOD1;
        SoundEffect(50, &fx->pos, 0);
    } else {
        LaraItem->hit_points -= ROCKET_DAMAGE;
        fx->object_number = O_EXPLOSION1;
        if (LaraItem->hit_points > 0) {
            SoundEffect(31, &LaraItem->pos, 0);
            Lara.spaz_effect = fx;
            Lara.spaz_effect_count = 5;
        }
        SoundEffect(104, &fx->pos, 0);
    }
    LaraItem->hit_status = 1;

    fx->frame_number = 0;
    fx->pos.y_rot = LaraItem->pos.y_rot;
    fx->speed = LaraItem->speed;
    fx->counter = 0;
}

void ShootAtLara(FX_INFO* fx)
{
    int32_t x = LaraItem->pos.x - fx->pos.x;
    int32_t y = LaraItem->pos.y - fx->pos.y;
    int32_t z = LaraItem->pos.z - fx->pos.z;

    int16_t* bounds = GetBoundsAccurate(LaraItem);
    y += bounds[FRAME_BOUND_MAX_Y]
        + (bounds[FRAME_BOUND_MIN_Y] - bounds[FRAME_BOUND_MAX_Y]) * 3 / 4;

    int32_t dist = phd_sqrt(SQUARE(x) + SQUARE(z));
    fx->pos.x_rot = -(PHD_ANGLE)phd_atan(dist, y);
    fx->pos.y_rot = phd_atan(z, x);
    fx->pos.x_rot += (GetRandomControl() - 0x4000) / 0x40;
    fx->pos.y_rot += (GetRandomControl() - 0x4000) / 0x40;
}

int16_t ShardGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    int16_t fx_num = CreateEffect(room_num);
    if (fx_num != NO_ITEM) {
        FX_INFO* fx = &Effects[fx_num];
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->room_number = room_num;
        fx->pos.x_rot = 0;
        fx->pos.y_rot = y_rot;
        fx->pos.z_rot = 0;
        fx->object_number = O_MISSILE2;
        fx->frame_number = 0;
        fx->speed = SHARD_SPEED;
        fx->shade = 3584;
        ShootAtLara(fx);
    }
    return fx_num;
}

int16_t RocketGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num)
{
    int16_t fx_num = CreateEffect(room_num);
    if (fx_num != NO_ITEM) {
        FX_INFO* fx = &Effects[fx_num];
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->room_number = room_num;
        fx->pos.x_rot = 0;
        fx->pos.y_rot = y_rot;
        fx->pos.z_rot = 0;
        fx->object_number = O_MISSILE3;
        fx->frame_number = 0;
        fx->speed = ROCKET_SPEED;
        fx->shade = 4096;
        ShootAtLara(fx);
    }
    return fx_num;
}

void InitialiseMummy(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    item->touch_bits = 0;
    item->mesh_bits = 0xFFFF87FF;
    item->data = game_malloc(sizeof(int16_t), GBUF_MUMMY_HEAD_TURN);
    *(int16_t*)item->data = 0;
}

void MummyControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    int16_t head = 0;

    if (item->current_anim_state == MUMMY_STOP) {
        head = phd_atan(
                   LaraItem->pos.z - item->pos.z, LaraItem->pos.x - item->pos.x)
            - item->pos.y_rot;
        CLAMP(head, -FRONT_ARC, FRONT_ARC);

        if (item->hit_points <= 0 || item->touch_bits) {
            item->goal_anim_state = MUMMY_DEATH;
        }
    }

    CreatureHead(item, head);
    AnimateItem(item);

    if (item->status == IS_DEACTIVATED) {
        RemoveActiveItem(item_num);
        item->hit_points = DONT_TARGET;
    }
}

int32_t ExplodingDeath(int16_t item_num, int32_t mesh_bits, int16_t damage)
{
    ITEM_INFO* item = &Items[item_num];
    OBJECT_INFO* object = &Objects[item->object_number];
    int32_t abortion = item->object_number == O_ABORTION;

    int16_t* frame = GetBestFrame(item);

    phd_PushUnitMatrix();
    PhdMatrixPtr->_03 = 0;
    PhdMatrixPtr->_13 = 0;
    PhdMatrixPtr->_23 = 0;

    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
    phd_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);

    int32_t* packed_rotation = (int32_t*)(frame + FRAME_ROT);
    phd_RotYXZpack(*packed_rotation++);

    int32_t* bone = &AnimBones[object->bone_index];
#if 0
    // NOTE: present in OG, removed by GLrage on the grounds that it sometimes
    // crashes.
    int16_t *extra_rotation = (int16_t*)item->data;
#endif

    int32_t bit = 1;
    if ((bit & mesh_bits) && (bit & item->mesh_bits)) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO* fx = &Effects[fx_num];
            fx->pos.x = (PhdMatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
            fx->pos.y = (PhdMatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
            fx->pos.z = (PhdMatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
            fx->room_number = item->room_number;
            fx->pos.y_rot = (GetRandomControl() - 0x4000) * 2;
            if (abortion) {
                fx->speed = GetRandomControl() >> 7;
                fx->fall_speed = -GetRandomControl() >> 7;
            } else {
                fx->speed = GetRandomControl() >> 8;
                fx->fall_speed = -GetRandomControl() >> 8;
            }
            fx->counter = damage;
            fx->frame_number = object->mesh_index;
            fx->object_number = O_BODY_PART;
        }
        item->mesh_bits -= bit;
    }

    for (int i = 1; i < object->nmeshes; i++) {
        int32_t bone_extra_flags = *bone++;
        if (bone_extra_flags & BEB_POP) {
            phd_PopMatrix();
        }
        if (bone_extra_flags & BEB_PUSH) {
            phd_PushMatrix();
        }

        phd_TranslateRel(bone[0], bone[1], bone[2]);
        phd_RotYXZpack(*packed_rotation++);

#if 0
    if (extra_rotation) {
        if (bone_extra_flags & (BEB_ROT_X | BEB_ROT_Y | BEB_ROT_Z)) {
            if (bone_extra_flags & BEB_ROT_Y) {
                phd_RotY(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_X) {
                phd_RotX(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_Z) {
                phd_RotZ(*extra_rotation++);
            }
        }
    }
#endif

        bit <<= 1;
        if ((bit & mesh_bits) && (bit & item->mesh_bits)) {
            int16_t fx_num = CreateEffect(item->room_number);
            if (fx_num != NO_ITEM) {
                FX_INFO* fx = &Effects[fx_num];
                fx->pos.x = (PhdMatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
                fx->pos.y = (PhdMatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
                fx->pos.z = (PhdMatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
                fx->room_number = item->room_number;
                fx->pos.y_rot = (GetRandomControl() - 0x4000) * 2;
                if (abortion) {
                    fx->speed = GetRandomControl() >> 7;
                    fx->fall_speed = -GetRandomControl() >> 7;
                } else {
                    fx->speed = GetRandomControl() >> 8;
                    fx->fall_speed = -GetRandomControl() >> 8;
                }
                fx->counter = damage;
                fx->object_number = O_BODY_PART;
                fx->frame_number = object->mesh_index + i;
            }
            item->mesh_bits -= bit;
        }

        bone += 3;
    }

    phd_PopMatrix();

    return !(item->mesh_bits & (0x7FFFFFFF >> (31 - object->nmeshes)));
}

void T1MInjectGameWarrior()
{
    INJECT(0x0043B850, CentaurControl);
    INJECT(0x0043BB30, InitialiseWarrior2);
    INJECT(0x0043BB60, FlyerControl);
    INJECT(0x0043C1C0, ControlMissile);
    INJECT(0x0043C430, ShardGun);
    INJECT(0x0043C540, RocketGun);
    INJECT(0x0043C650, InitialiseMummy);
    INJECT(0x0043C690, MummyControl);
    INJECT(0x0043C730, ExplodingDeath);
}
