#include "game/box.h"
#include "game/dino.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/misc.h"
#include "game/types.h"
#include "game/vars.h"
#include "util.h"

#define RAPTOR_ATTACK_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAPTOR_BITE_DAMAGE 100
#define RAPTOR_CHARGE_DAMAGE 100
#define RAPTOR_CLOSE_RANGE SQUARE(680) // = 462400
#define RAPTOR_DIE_ANIM 9
#define RAPTOR_LUNGE_DAMAGE 100
#define RAPTOR_LUNGE_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAPTOR_ROAR_CHANCE 256
#define RAPTOR_RUN_TURN (4 * PHD_DEGREE) // = 728
#define RAPTOR_TOUCH 0xFF7C00
#define RAPTOR_WALK_TURN (1 * PHD_DEGREE) // = 182

#define DINO_ATTACK_RANGE SQUARE(WALL_L * 4) // = 16777216
#define DINO_BITE_DAMAGE 10000
#define DINO_BITE_RANGE SQUARE(1500) // = 2250000
#define DINO_ROAR_CHANCE 512
#define DINO_RUN_RANGE SQUARE(WALL_L * 5) // = 26214400
#define DINO_RUN_TURN (4 * PHD_DEGREE) // = 728
#define DINO_TOUCH 0x3000
#define DINO_TOUCH_DAMAGE 1
#define DINO_TRAMPLE_DAMAGE 10
#define DINO_WALK_TURN (2 * PHD_DEGREE) // = 364

typedef enum {
    RAPTOR_EMPTY = 0,
    RAPTOR_STOP = 1,
    RAPTOR_WALK = 2,
    RAPTOR_RUN = 3,
    RAPTOR_ATTACK1 = 4,
    RAPTOR_DEATH = 5,
    RAPTOR_WARNING = 6,
    RAPTOR_ATTACK2 = 7,
    RAPTOR_ATTACK3 = 8,
} RAPTOR_ANIMS;

typedef enum {
    DINO_EMPTY = 0,
    DINO_STOP = 1,
    DINO_WALK = 2,
    DINO_RUN = 3,
    DINO_ATTACK1 = 4,
    DINO_DEATH = 5,
    DINO_ROAR = 6,
    DINO_ATTACK2 = 7,
    DINO_KILL = 8,
} DINO_ANIMS;

BITE_INFO RaptorBite = { 0, 66, 318, 22 };

void RaptorControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* raptor = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != RAPTOR_DEATH) {
            item->current_anim_state = RAPTOR_DEATH;
            item->anim_number = Objects[O_RAPTOR].anim_index + RAPTOR_DIE_ANIM
                + (GetRandomControl() / 16200);
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, raptor->maximum_turn);

        switch (item->current_anim_state) {
        case RAPTOR_STOP:
            if (item->required_anim_state != RAPTOR_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            } else if (item->touch_bits & RAPTOR_TOUCH) {
                item->goal_anim_state = RAPTOR_ATTACK3;
            } else if (info.bite && info.distance < RAPTOR_CLOSE_RANGE) {
                item->goal_anim_state = RAPTOR_ATTACK3;
            } else if (info.bite && info.distance < RAPTOR_LUNGE_RANGE) {
                item->goal_anim_state = RAPTOR_ATTACK1;
            } else if (raptor->mood == MOOD_BORED) {
                item->goal_anim_state = RAPTOR_WALK;
            } else {
                item->goal_anim_state = RAPTOR_RUN;
            }
            break;

        case RAPTOR_WALK:
            raptor->maximum_turn = RAPTOR_WALK_TURN;
            if (raptor->mood != MOOD_BORED) {
                item->goal_anim_state = RAPTOR_STOP;
            } else if (info.ahead && GetRandomControl() < RAPTOR_ROAR_CHANCE) {
                item->required_anim_state = RAPTOR_WARNING;
                item->goal_anim_state = RAPTOR_STOP;
            }
            break;

        case RAPTOR_RUN:
            tilt = angle;
            raptor->maximum_turn = RAPTOR_RUN_TURN;
            if (item->touch_bits & RAPTOR_TOUCH) {
                item->goal_anim_state = RAPTOR_STOP;
            } else if (info.bite && info.distance < RAPTOR_ATTACK_RANGE) {
                if (item->goal_anim_state == RAPTOR_RUN) {
                    if (GetRandomControl() < 0x2000) {
                        item->goal_anim_state = RAPTOR_STOP;
                    } else {
                        item->goal_anim_state = RAPTOR_ATTACK2;
                    }
                }
            } else if (
                info.ahead && raptor->mood != MOOD_ESCAPE
                && GetRandomControl() < RAPTOR_ROAR_CHANCE) {
                item->required_anim_state = RAPTOR_WARNING;
                item->goal_anim_state = RAPTOR_STOP;
            } else if (raptor->mood == MOOD_BORED) {
                item->goal_anim_state = RAPTOR_STOP;
            }
            break;

        case RAPTOR_ATTACK1:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_EMPTY && info.ahead
                && (item->touch_bits & RAPTOR_TOUCH)) {
                CreatureEffect(item, &RaptorBite, DoBloodSplat);
                LaraItem->hit_points -= RAPTOR_LUNGE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = RAPTOR_STOP;
            }
            break;

        case RAPTOR_ATTACK2:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_EMPTY && info.ahead
                && (item->touch_bits & RAPTOR_TOUCH)) {
                CreatureEffect(item, &RaptorBite, DoBloodSplat);
                LaraItem->hit_points -= RAPTOR_CHARGE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = RAPTOR_RUN;
            }
            break;

        case RAPTOR_ATTACK3:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_EMPTY
                && (item->touch_bits & RAPTOR_TOUCH)) {
                CreatureEffect(item, &RaptorBite, DoBloodSplat);
                LaraItem->hit_points -= RAPTOR_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = RAPTOR_STOP;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);
    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, tilt);
}

void DinoControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* dino = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state == DINO_STOP) {
            item->goal_anim_state = DINO_DEATH;
        } else {
            item->goal_anim_state = DINO_STOP;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, dino->maximum_turn);

        if (item->touch_bits) {
            if (item->current_anim_state == DINO_RUN) {
                LaraItem->hit_points -= DINO_TRAMPLE_DAMAGE;
            } else {
                LaraItem->hit_points -= DINO_TOUCH_DAMAGE;
            }
        }

        dino->flags = dino->mood != MOOD_ESCAPE && !info.ahead
            && info.enemy_facing > -FRONT_ARC && info.enemy_facing < FRONT_ARC;

        if (!dino->flags && info.distance > DINO_BITE_RANGE
            && info.distance < DINO_ATTACK_RANGE && info.bite) {
            dino->flags = 1;
        }

        switch (item->current_anim_state) {
        case DINO_STOP:
            if (item->required_anim_state != DINO_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.distance < DINO_BITE_RANGE && info.bite) {
                item->goal_anim_state = DINO_ATTACK2;
            } else if (dino->mood == MOOD_BORED || dino->flags) {
                item->goal_anim_state = DINO_WALK;
            } else {
                item->goal_anim_state = DINO_RUN;
            }
            break;

        case DINO_WALK:
            dino->maximum_turn = DINO_WALK_TURN;
            if (dino->mood != MOOD_BORED || !dino->flags) {
                item->goal_anim_state = DINO_STOP;
            } else if (info.ahead && GetRandomControl() < DINO_ROAR_CHANCE) {
                item->required_anim_state = DINO_ROAR;
                item->goal_anim_state = DINO_STOP;
            }
            break;

        case DINO_RUN:
            dino->maximum_turn = DINO_RUN_TURN;
            if (info.distance < DINO_RUN_RANGE && info.bite) {
                item->goal_anim_state = DINO_STOP;
            } else if (dino->flags) {
                item->goal_anim_state = DINO_STOP;
            } else if (
                dino->mood != MOOD_ESCAPE && info.ahead
                && GetRandomControl() < DINO_ROAR_CHANCE) {
                item->required_anim_state = DINO_ROAR;
                item->goal_anim_state = DINO_STOP;
            } else if (dino->mood == MOOD_BORED) {
                item->goal_anim_state = DINO_STOP;
            }
            break;

        case DINO_ATTACK2:
            if (item->touch_bits & DINO_TOUCH) {
                LaraItem->hit_points -= DINO_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->goal_anim_state = DINO_KILL;
                LaraDinoDeath(item);
            }
            item->required_anim_state = DINO_WALK;
            break;
        }
    }

    CreatureHead(item, head >> 1);
    dino->neck_rotation = dino->head_rotation;
    CreatureAnimation(item_num, angle, 0);
    item->collidable = 1;
}

void LaraDinoDeath(ITEM_INFO* item)
{
    item->goal_anim_state = DINO_KILL;
    if (LaraItem->room_number != item->room_number) {
        ItemNewRoom(Lara.item_number, item->room_number);
    }

    LaraItem->pos.x = item->pos.x;
    LaraItem->pos.y = item->pos.y;
    LaraItem->pos.z = item->pos.z;
    LaraItem->pos.x_rot = 0;
    LaraItem->pos.y_rot = item->pos.y_rot;
    LaraItem->pos.z_rot = 0;
    LaraItem->gravity_status = 0;
    LaraItem->current_anim_state = AS_SPECIAL;
    LaraItem->goal_anim_state = AS_SPECIAL;
    LaraItem->anim_number = Objects[O_LARA_EXTRA].anim_index + 1;
    LaraItem->frame_number = Anims[LaraItem->anim_number].frame_base;
    LaraSwapMeshExtra();

    LaraItem->hit_points = -1;
    Lara.air = -1;
    Lara.gun_status = LGS_HANDSBUSY;
    Lara.gun_type = LGT_UNARMED;

    Camera.flags = FOLLOW_CENTRE;
    Camera.target_angle = 170 * PHD_DEGREE;
    Camera.target_elevation = -25 * PHD_DEGREE;
}

void T1MInjectGameDino()
{
    INJECT(0x00415DA0, RaptorControl);
    INJECT(0x004160F0, DinoControl);
    INJECT(0x004163A0, LaraDinoDeath);
}
