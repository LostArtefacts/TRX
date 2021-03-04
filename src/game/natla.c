#include "3dsystem/phd_math.h"
#include "game/box.h"
#include "game/control.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/natla.h"
#include "game/people.h"
#include "game/vars.h"
#include "game/warrior.h"
#include "specific/sndpc.h"
#include "util.h"

#define NATLA_SHOT_DAMAGE 100
#define NATLA_NEAR_DEATH 200
#define NATLA_FLY_MODE 0x8000
#define NATLA_TIMER 0x7FFF
#define NATLA_FIRE_ARC (PHD_DEGREE * 30) // = 5460
#define NATLA_FLY_TURN (PHD_DEGREE * 5) // = 910
#define NATLA_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define NATLA_LAND_CHANCE 256
#define NATLA_DIE_TIME 30 * 16 // = 480
#define NATLA_GUN_SPEED 400

#define ABORTION_DIE_ANIM 13
#define ABORTION_PART_DAMAGE 250
#define ABORTION_ATTACK_DAMAGE 500
#define ABORTION_TOUCH_DAMAGE 5
#define ABORTION_NEED_TURN (PHD_DEGREE * 45) // = 8190
#define ABORTION_TURN (PHD_DEGREE * 3) // = 546
#define ABORTION_ATTACK_RANGE SQUARE(2600) // = 6760000
#define ABORTION_CLOSE_RANGE SQUARE(2250) // = 5062500
#define ABORTION_TLEFT 0x7FF0
#define ABORTION_TRIGHT 0x3FF8000
#define ABORTION_TOUCH (ABORTION_TLEFT | ABORTION_TRIGHT)

enum ABORTION_ANIMS {
    ABORTION_EMPTY = 0,
    ABORTION_STOP = 1,
    ABORTION_TURN_L = 2,
    ABORTION_TURN_R = 3,
    ABORTION_ATTACK1 = 4,
    ABORTION_ATTACK2 = 5,
    ABORTION_ATTACK3 = 6,
    ABORTION_FORWARD = 7,
    ABORTION_SET = 8,
    ABORTION_FALL = 9,
    ABORTION_DEATH = 10,
    ABORTION_KILL = 11,
};

typedef enum {
    NATLA_EMPTY,
    NATLA_STOP,
    NATLA_FLY,
    NATLA_RUN,
    NATLA_AIM,
    NATLA_SEMIDEATH,
    NATLA_SHOOT,
    NATLA_FALL,
    NATLA_STAND,
    NATLA_DEATH
} NATLA_ANIMS;

BITE_INFO NatlaGun = { 5, 220, 7, 4 };

void AbortionControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* abortion = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != ABORTION_DEATH) {
            item->current_anim_state = ABORTION_DEATH;
            item->anim_number =
                Objects[O_ABORTION].anim_index + ABORTION_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = phd_atan(
                    abortion->target.z - item->pos.z,
                    abortion->target.x - item->pos.x)
            - item->pos.y_rot;

        if (item->touch_bits) {
            LaraItem->hit_points -= ABORTION_TOUCH_DAMAGE;
            LaraItem->hit_status = 1;
        }

        switch (item->current_anim_state) {
        case ABORTION_SET:
            item->goal_anim_state = ABORTION_FALL;
            item->gravity_status = 1;
            break;

        case ABORTION_STOP:
            if (LaraItem->hit_points <= 0) {
                break;
            }

            abortion->flags = 0;
            if (angle > ABORTION_NEED_TURN) {
                item->goal_anim_state = ABORTION_TURN_R;
            } else if (angle < -ABORTION_NEED_TURN) {
                item->goal_anim_state = ABORTION_TURN_L;
            } else if (info.distance >= ABORTION_ATTACK_RANGE) {
                item->goal_anim_state = ABORTION_FORWARD;
            } else if (LaraItem->hit_points > ABORTION_ATTACK_DAMAGE) {
                if (GetRandomControl() < 0x4000) {
                    item->goal_anim_state = ABORTION_ATTACK1;
                } else {
                    item->goal_anim_state = ABORTION_ATTACK2;
                }
            } else if (info.distance < ABORTION_CLOSE_RANGE) {
                item->goal_anim_state = ABORTION_ATTACK3;
            } else {
                item->goal_anim_state = ABORTION_FORWARD;
            }
            break;

        case ABORTION_FORWARD:
            if (angle < -ABORTION_TURN) {
                item->goal_anim_state -= ABORTION_TURN;
            } else if (angle > ABORTION_TURN) {
                item->goal_anim_state += ABORTION_TURN;
            } else {
                item->goal_anim_state += angle;
            }

            if (angle > ABORTION_NEED_TURN || angle < -ABORTION_NEED_TURN) {
                item->goal_anim_state = ABORTION_STOP;
            } else if (info.distance < ABORTION_ATTACK_RANGE) {
                item->goal_anim_state = ABORTION_STOP;
            }
            break;

        case ABORTION_TURN_L:
            if (!abortion->flags) {
                abortion->flags = item->frame_number;
            } else if (
                item->frame_number - abortion->flags > 13
                && item->frame_number - abortion->flags < 23) {
                item->pos.y_rot -= PHD_DEGREE * 9;
            }

            if (angle > -ABORTION_NEED_TURN) {
                item->goal_anim_state = ABORTION_STOP;
            }
            break;

        case ABORTION_TURN_R:
            if (!abortion->flags) {
                abortion->flags = item->frame_number;
            } else if (
                item->frame_number - abortion->flags > 16
                && item->frame_number - abortion->flags < 23) {
                item->pos.y_rot += PHD_DEGREE * 14;
            }

            if (angle < ABORTION_NEED_TURN) {
                item->goal_anim_state = ABORTION_STOP;
            }
            break;

        case ABORTION_ATTACK1:
            if (!abortion->flags && (item->touch_bits & ABORTION_TRIGHT)) {
                LaraItem->hit_points -= ABORTION_ATTACK_DAMAGE;
                LaraItem->hit_status = 1;
                abortion->flags = 1;
            }
            break;

        case ABORTION_ATTACK2:
            if (!abortion->flags && (item->touch_bits & ABORTION_TOUCH)) {
                LaraItem->hit_points -= ABORTION_ATTACK_DAMAGE;
                LaraItem->hit_status = 1;
                abortion->flags = 1;
            }
            break;

        case ABORTION_ATTACK3:
            if ((item->touch_bits & ABORTION_TRIGHT)
                || LaraItem->hit_points <= 0) {
                item->goal_anim_state = ABORTION_KILL;

                LaraItem->anim_number = Objects[O_LARA_EXTRA].anim_index;
                LaraItem->frame_number =
                    Anims[LaraItem->anim_number].frame_base;
                LaraItem->current_anim_state = AS_SPECIAL;
                LaraItem->goal_anim_state = AS_SPECIAL;
                LaraItem->room_number = item->room_number;
                LaraItem->pos.x = item->pos.x;
                LaraItem->pos.y = item->pos.y;
                LaraItem->pos.z = item->pos.z;
                LaraItem->pos.x_rot = 0;
                LaraItem->pos.y_rot = item->pos.y_rot;
                LaraItem->pos.z_rot = 0;
                LaraItem->gravity_status = 0;
                LaraItem->hit_points = -1;
                Lara.air = -1;
                Lara.gun_status = LGS_HANDSBUSY;
                Lara.gun_type = LGT_UNARMED;

                Camera.target_distance = WALL_L * 2;
                Camera.flags = FOLLOW_CENTRE;
            }
            break;

        case ABORTION_KILL:
            Camera.target_distance = WALL_L * 2;
            Camera.flags = FOLLOW_CENTRE;
            break;
        }
    }

    CreatureHead(item, head);

    if (item->current_anim_state == ABORTION_FALL) {
        AnimateItem(item);

        if (item->pos.y > item->floor) {
            item->goal_anim_state = ABORTION_STOP;
            item->gravity_status = 0;
            item->pos.y = item->floor;
            Camera.bounce = 500;
        }
    } else {
        CreatureAnimation(item_num, 0, 0);
    }

    if (item->status == IS_DEACTIVATED) {
        SoundEffect(171, &item->pos, 0);
        ExplodingDeath(item_num, -1, ABORTION_PART_DAMAGE);
        FLOOR_INFO* floor =
            GetFloor(item->pos.x, item->pos.y, item->pos.z, &item->room_number);
        GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        TestTriggers(TriggerIndex, 1);

        KillItem(item_num);
        item->status = IS_DEACTIVATED;
    }
}

void NatlaControl(int16_t item_num)
{
    static int16_t facing = 0;
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* natla = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;
    int16_t gun = natla->head_rotation * 7 / 8;
    int16_t timer = natla->flags & NATLA_TIMER;

    if (item->hit_points <= 0 && item->hit_points > DONT_TARGET) {
        item->goal_anim_state = NATLA_DEATH;
    } else if (item->hit_points <= NATLA_NEAR_DEATH) {
        natla->LOT.step = STEP_L;
        natla->LOT.drop = -STEP_L;
        natla->LOT.fly = 0;

        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, NATLA_RUN_TURN);

        int8_t shoot = info.angle > -NATLA_FIRE_ARC
            && info.angle < NATLA_FIRE_ARC && Targetable(item, &info);

        if (facing) {
            item->pos.y_rot += facing;
            facing = 0;
        }

        switch (item->current_anim_state) {
        case NATLA_FALL:
            if (item->pos.y < item->floor) {
                item->gravity_status = 1;
                item->speed = 0;
            } else {
                item->gravity_status = 0;
                item->goal_anim_state = NATLA_SEMIDEATH;
                item->pos.y = item->floor;
                timer = 0;
            }
            break;

        case NATLA_STAND:
            if (!shoot) {
                item->goal_anim_state = NATLA_RUN;
            }
            if (timer >= 20) {
                int16_t fx_num = CreatureEffect(item, &NatlaGun, ShardGun);
                if (fx_num != NO_ITEM) {
                    FX_INFO* fx = &Effects[fx_num];
                    gun = fx->pos.x_rot;
                    SoundEffect(123, &fx->pos, 0);
                }
                timer = 0;
            }
            break;

        case NATLA_RUN:
            tilt = angle;
            if (timer >= 20) {
                int16_t fx_num = CreatureEffect(item, &NatlaGun, ShardGun);
                if (fx_num != NO_ITEM) {
                    FX_INFO* fx = &Effects[fx_num];
                    gun = fx->pos.x_rot;
                    SoundEffect(123, &fx->pos, 0);
                }
                timer = 0;
            }
            if (shoot) {
                item->goal_anim_state = NATLA_STAND;
            }
            break;

        case NATLA_SEMIDEATH:
            if (timer == NATLA_DIE_TIME) {
                item->goal_anim_state = NATLA_STAND;
                natla->flags = 0;
                timer = 0;
                item->hit_points = NATLA_NEAR_DEATH;
                S_CDPlay(54);
            } else {
                item->hit_points = DONT_TARGET;
            }
            break;

        case NATLA_FLY:
            item->goal_anim_state = NATLA_FALL;
            timer = 0;
            break;

        case NATLA_STOP:
        case NATLA_AIM:
        case NATLA_SHOOT:
            item->goal_anim_state = NATLA_SEMIDEATH;
            item->flags = 0;
            timer = 0;
            break;
        }
    } else {
        natla->LOT.step = STEP_L;
        natla->LOT.drop = -STEP_L;
        natla->LOT.fly = 0;

        AI_INFO info;
        CreatureAIInfo(item, &info);

        int8_t shoot = info.angle > -NATLA_FIRE_ARC
            && info.angle < NATLA_FIRE_ARC && Targetable(item, &info);
        if (item->current_anim_state == NATLA_FLY
            && (natla->flags & NATLA_FLY_MODE)) {
            if (shoot && GetRandomControl() < NATLA_LAND_CHANCE) {
                natla->flags &= ~NATLA_FLY_MODE;
            }
            if (!(natla->flags & NATLA_FLY_MODE)) {
                CreatureMood(item, &info, 1);
            }
            natla->LOT.step = WALL_L * 20;
            natla->LOT.drop = -WALL_L * 20;
            natla->LOT.fly = STEP_L / 8;
            CreatureAIInfo(item, &info);
        } else if (!shoot) {
            natla->flags |= NATLA_FLY_MODE;
        }

        if (info.ahead) {
            head = info.angle;
        }

        if (item->current_anim_state != NATLA_FLY
            || (natla->flags & NATLA_FLY_MODE)) {
            CreatureMood(item, &info, 0);
        }

        item->pos.y_rot -= facing;
        angle = CreatureTurn(item, NATLA_FLY_TURN);

        if (item->current_anim_state == NATLA_FLY) {
            if (info.angle > NATLA_FLY_TURN) {
                facing += NATLA_FLY_TURN;
            } else if (info.angle < -NATLA_FLY_TURN) {
                facing -= NATLA_FLY_TURN;
            } else {
                facing += info.angle;
            }
            item->pos.y_rot += facing;
        } else {
            item->pos.y_rot += facing - angle;
            facing = 0;
        }

        switch (item->current_anim_state) {
        case NATLA_STOP:
            timer = 0;
            if (natla->flags & NATLA_FLY_MODE) {
                item->goal_anim_state = NATLA_FLY;
            } else {
                item->goal_anim_state = NATLA_AIM;
            }
            break;

        case NATLA_FLY:
            if (!(natla->flags & NATLA_FLY_MODE)
                && item->pos.y == item->floor) {
                item->goal_anim_state = NATLA_STOP;
            }
            if (timer >= 30) {
                int16_t fx_num = CreatureEffect(item, &NatlaGun, RocketGun);
                if (fx_num != NO_ITEM) {
                    FX_INFO* fx = &Effects[fx_num];
                    gun = fx->pos.x_rot;
                    SoundEffect(123, &fx->pos, 0);
                }
                timer = 0;
            }
            break;

        case NATLA_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (shoot) {
                item->goal_anim_state = NATLA_SHOOT;
            } else {
                item->goal_anim_state = NATLA_STOP;
            }
            break;

        case NATLA_SHOOT:
            if (!item->required_anim_state) {
                int16_t fx_num = CreatureEffect(item, &NatlaGun, RocketGun);
                if (fx_num != NO_ITEM) {
                    FX_INFO* fx = &Effects[fx_num];
                    gun = fx->pos.x_rot;
                }
                fx_num = CreatureEffect(item, &NatlaGun, RocketGun);
                if (fx_num != NO_ITEM) {
                    FX_INFO* fx = &Effects[fx_num];
                    fx->pos.y_rot += (GetRandomControl() - 0x4000) / 4;
                }
                fx_num = CreatureEffect(item, &NatlaGun, RocketGun);
                if (fx_num != NO_ITEM) {
                    FX_INFO* fx = &Effects[fx_num];
                    fx->pos.y_rot += (GetRandomControl() - 0x4000) / 4;
                }
                item->required_anim_state = NATLA_STOP;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);

    natla->neck_rotation = -head;
    if (gun) {
        natla->head_rotation = gun;
    }

    timer++;
    natla->flags &= ~NATLA_TIMER;
    natla->flags |= timer & NATLA_TIMER;

    item->pos.y_rot -= facing;
    CreatureAnimation(item_num, angle, 0);
    item->pos.y_rot += facing;
}

void T1MInjectGameNatla()
{
    INJECT(0x0042BE60, AbortionControl);
    INJECT(0x0042C330, NatlaControl);
}
