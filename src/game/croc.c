#include "game/box.h"
#include "game/control.h"
#include "game/croc.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/types.h"
#include "game/vars.h"
#include "util.h"

#define CROC_BITE_DAMAGE 100
#define CROC_BITE_RANGE SQUARE(435) // = 189225
#define CROC_DIE_ANIM 11
#define CROC_FASTTURN_ANGLE 0x4000
#define CROC_FASTTURN_RANGE SQUARE(WALL_L * 3) // = 9437184
#define CROC_FASTTURN_TURN (6 * PHD_DEGREE) // = 1092
#define CROC_TOUCH 0x3FC
#define CROC_TURN (3 * PHD_DEGREE) // = 546

#define GATOR_BITE_DAMAGE 100
#define GATOR_DIE_ANIM 4
#define GATOR_FLOAT_SPEED (WALL_L / 32) // = 32
#define GATOR_TURN (3 * PHD_DEGREE) // = 546

typedef enum {
    CROC_EMPTY = 0,
    CROC_STOP = 1,
    CROC_RUN = 2,
    CROC_WALK = 3,
    CROC_FASTTURN = 4,
    CROC_ATTACK1 = 5,
    CROC_ATTACK2 = 6,
    CROC_DEATH = 7,
} CROC_ANIMS;

typedef enum {
    GATOR_EMPTY = 0,
    GATOR_SWIM = 1,
    GATOR_ATTACK = 2,
    GATOR_DEATH = 3,
} GATOR_ANIMS;

BITE_INFO CrocBite = { 5, -21, 467, 9 };

void AlligatorControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* gator = item->data;
    FLOOR_INFO* floor;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t room_num;
    int32_t wh;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != GATOR_DEATH) {
            item->current_anim_state = GATOR_DEATH;
            item->anim_number =
                Objects[O_ALLIGATOR].anim_index + GATOR_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
            item->hit_points = DONT_TARGET;
        }

        wh = GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_number);
        if (wh == NO_HEIGHT) {
            item->object_number = O_CROCODILE;
            item->current_anim_state = CROC_DEATH;
            item->goal_anim_state = CROC_DEATH;
            item->anim_number = Objects[O_CROCODILE].anim_index + CROC_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
            room_num = item->room_number;
            floor = GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
            item->pos.y =
                GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
            item->pos.x_rot = 0;
        } else if (item->pos.y > wh + GATOR_FLOAT_SPEED) {
            item->pos.y -= GATOR_FLOAT_SPEED;
        } else if (item->pos.y < wh) {
            item->pos.y = wh;
            if (gator) {
                DisableBaddieAI(item_num);
            }
        }

        AnimateItem(item);

        room_num = item->room_number;
        floor = GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        if (room_num != item->room_number) {
            ItemNewRoom(item_num, room_num);
        }
        return;
    }

    AI_INFO info;
    CreatureAIInfo(item, &info);

    if (info.ahead) {
        head = info.angle;
    }

    CreatureMood(item, &info, 1);
    CreatureTurn(item, GATOR_TURN);

    switch (item->current_anim_state) {
    case GATOR_SWIM:
        if (info.bite && item->touch_bits) {
            item->goal_anim_state = GATOR_ATTACK;
        }
        break;

    case GATOR_ATTACK:
        if (item->frame_number == Anims[item->anim_number].frame_base) {
            item->required_anim_state = GATOR_EMPTY;
        }

        if (info.bite && item->touch_bits) {
            if (item->required_anim_state == GATOR_EMPTY) {
                CreatureEffect(item, &CrocBite, DoBloodSplat);
                LaraItem->hit_points -= GATOR_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = GATOR_SWIM;
            }
        } else {
            item->goal_anim_state = GATOR_SWIM;
        }
        break;
    }

    CreatureHead(item, head);

    wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh == NO_HEIGHT) {
        item->object_number = O_CROCODILE;
        item->current_anim_state = Anims[item->anim_number].current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        item->anim_number = Objects[O_CROCODILE].anim_index;
        item->frame_number = Anims[item->anim_number].frame_base;
        item->pos.y = item->floor;
        item->pos.x_rot = 0;
        gator->LOT.step = STEP_L;
        gator->LOT.drop = -STEP_L;
        gator->LOT.fly = 0;
    } else if (item->pos.y < wh + STEP_L) {
        item->pos.y = wh + STEP_L;
    }

    CreatureAnimation(item_num, angle, 0);
}

void CrocControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* croc = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != CROC_DEATH) {
            item->current_anim_state = CROC_DEATH;
            item->anim_number = Objects[O_CROCODILE].anim_index + CROC_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        if (item->current_anim_state == CROC_FASTTURN) {
            item->pos.y_rot += CROC_FASTTURN_TURN;
        } else {
            angle = CreatureTurn(item, CROC_TURN);
        }

        switch (item->current_anim_state) {
        case CROC_STOP:
            if (info.bite && info.distance < CROC_BITE_RANGE) {
                item->goal_anim_state = CROC_ATTACK1;
            } else if (croc->mood == MOOD_ESCAPE) {
                item->goal_anim_state = CROC_RUN;
            } else if (croc->mood == MOOD_ATTACK) {
                if ((info.angle < -CROC_FASTTURN_ANGLE
                     || info.angle > CROC_FASTTURN_ANGLE)
                    && info.distance > CROC_FASTTURN_RANGE) {
                    item->goal_anim_state = CROC_FASTTURN;
                } else {
                    item->goal_anim_state = CROC_RUN;
                }
            } else if (croc->mood == MOOD_STALK) {
                item->goal_anim_state = CROC_WALK;
            }
            break;

        case CROC_WALK:
            if (info.ahead && (item->touch_bits & CROC_TOUCH)) {
                item->goal_anim_state = CROC_STOP;
            } else if (croc->mood == MOOD_ATTACK || croc->mood == MOOD_ESCAPE) {
                item->goal_anim_state = CROC_RUN;
            } else if (croc->mood == MOOD_BORED) {
                item->goal_anim_state = CROC_STOP;
            }
            break;

        case CROC_FASTTURN:
            if (info.angle > -CROC_FASTTURN_ANGLE
                && info.angle < CROC_FASTTURN_ANGLE) {
                item->goal_anim_state = CROC_WALK;
            }
            break;

        case CROC_RUN:
            if (info.ahead && (item->touch_bits & CROC_TOUCH)) {
                item->goal_anim_state = CROC_STOP;
            } else if (croc->mood == MOOD_STALK) {
                item->goal_anim_state = CROC_WALK;
            } else if (croc->mood == MOOD_BORED) {
                item->goal_anim_state = CROC_STOP;
            } else if (
                croc->mood == MOOD_ATTACK && info.distance > CROC_FASTTURN_RANGE
                && (info.angle < -CROC_FASTTURN_ANGLE
                    || info.angle > CROC_FASTTURN_ANGLE)) {
                item->goal_anim_state = CROC_STOP;
            }
            break;

        case CROC_ATTACK1:
            if (item->required_anim_state == CROC_EMPTY) {
                CreatureEffect(item, &CrocBite, DoBloodSplat);
                LaraItem->hit_points -= CROC_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = CROC_STOP;
            }
            break;
        }
    }

    if (croc) {
        CreatureHead(item, head);
    }

    if (RoomInfo[item->room_number].flags & RF_UNDERWATER) {
        item->object_number = O_ALLIGATOR;
        item->current_anim_state = Anims[item->anim_number].current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        item->anim_number = Objects[O_ALLIGATOR].anim_index;
        item->frame_number = Anims[item->anim_number].frame_base;
        if (croc) {
            croc->LOT.step = WALL_L * 20;
            croc->LOT.drop = -WALL_L * 20;
            croc->LOT.fly = STEP_L / 16;
        }
    }

    if (croc) {
        CreatureAnimation(item_num, angle, 0);
    } else {
        AnimateItem(item);
    }
}

void T1MInjectGameCroc()
{
    INJECT(0x00415520, AlligatorControl);
    INJECT(0x00415850, CrocControl);
}
