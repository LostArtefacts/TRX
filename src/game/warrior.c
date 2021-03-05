#include "game/box.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/people.h"
#include "game/vars.h"
#include "game/warrior.h"
#include "util.h"

#define CENTAUR_PART_DAMAGE 100
#define CENTAUR_REAR_DAMAGE 200
#define CENTAUR_TOUCH 0x30199
#define CENTAUR_DIE_ANIM 8
#define CENTAUR_TURN (PHD_DEGREE * 4) // = 728
#define CENTAUR_REAR_CHANCE 96
#define CENTAUR_REAR_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296

typedef enum {
    CENTAUR_EMPTY = 0,
    CENTAUR_STOP = 1,
    CENTAUR_SHOOT = 2,
    CENTAUR_RUN = 3,
    CENTAUR_AIM = 4,
    CENTAUR_DEATH = 5,
    CENTAUR_WARNING = 6,
} CENTAUR_ANIM;

static BITE_INFO CentaurRocket = { 11, 415, 41, 13 };
static BITE_INFO CentaurRear = { 50, 30, 0, 5 };

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

void T1MInjectGameWarrior()
{
    INJECT(0x0043B850, CentaurControl);
    INJECT(0x0043BB30, InitialiseWarrior2);
}
