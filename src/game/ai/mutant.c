#include "game/ai/mutant.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/effects/blood.h"
#include "game/effects/body_part.h"
#include "game/effects/missile.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/people.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

BITE_INFO WarriorBite = { -27, 98, 0, 10 };
BITE_INFO WarriorRocket = { 51, 213, 0, 14 };
BITE_INFO WarriorShard = { -35, 269, 0, 9 };

void SetupWarrior(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = FlyerControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 3;
    obj->hit_points = FLYER_HITPOINTS;
    obj->pivot_length = 150;
    obj->radius = FLYER_RADIUS;
    obj->smartness = FLYER_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    AnimBones[obj->bone_index] |= BEB_ROT_Y;
    AnimBones[obj->bone_index + 8] |= BEB_ROT_Y;
}

void SetupWarrior2(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    *obj = Objects[O_WARRIOR1];
    obj->initialise = InitialiseWarrior2;
    obj->smartness = WARRIOR2_SMARTNESS;
}

void SetupWarrior3(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    *obj = Objects[O_WARRIOR1];
    obj->initialise = InitialiseWarrior2;
}

void FlyerControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *flyer = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (ExplodingDeath(item_num, -1, FLYER_PART_DAMAGE)) {
            Sound_Effect(SFX_ATLANTEAN_DEATH, &item->pos, SPM_NORMAL);
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
                        || Random_GetControl() < FLYER_UNPOSE_CHANCE) {
                        item->goal_anim_state = FLYER_WALK;
                    }
                } else {
                    item->goal_anim_state = FLYER_STOP;
                }
            } else if (
                flyer->mood == MOOD_BORED
                && Random_GetControl() < FLYER_UNPOSE_CHANCE) {
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
                if (Random_GetControl() < FLYER_POSE_CHANCE) {
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

void InitialiseWarrior2(int16_t item_num)
{
    InitialiseCreature(item_num);
    Items[item_num].mesh_bits = 0xFFE07FFF;
}
