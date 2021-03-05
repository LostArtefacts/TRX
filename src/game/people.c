#include "game/box.h"
#include "game/control.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/people.h"
#include "game/sphere.h"
#include "game/vars.h"
#include "util.h"

#define PEOPLE_POSE_CHANCE 0x60 // = 96
#define PEOPLE_SHOOT_RANGE SQUARE(WALL_L * 7) // = 51380224
#define PEOPLE_SHOT_DAMAGE 50
#define PEOPLE_WALK_TURN (PHD_DEGREE * 3) // = 546
#define PEOPLE_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define PEOPLE_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define PEOPLE_MISS_CHANCE 0x2000

#define LARSON_DIE_ANIM 15

typedef enum {
    PEOPLE_EMPTY = 0,
    PEOPLE_STOP = 1,
    PEOPLE_WALK = 2,
    PEOPLE_RUN = 3,
    PEOPLE_AIM = 4,
    PEOPLE_DEATH = 5,
    PEOPLE_POSE = 6,
    PEOPLE_SHOOT = 7,
} PEOPLE_ANIM;

BITE_INFO LarsonGun = { -60, 170, 0, 14 };

int32_t Targetable(ITEM_INFO* item, AI_INFO* info)
{
    if (!info->ahead || info->distance >= PEOPLE_SHOOT_RANGE) {
        return 0;
    }

    GAME_VECTOR start;
    start.x = item->pos.x;
    start.y = item->pos.y - STEP_L * 3;
    start.z = item->pos.z;
    start.room_number = item->room_number;

    GAME_VECTOR target;
    target.x = LaraItem->pos.x;
    target.y = LaraItem->pos.y - STEP_L * 3;
    target.z = LaraItem->pos.z;

    return LOS(&start, &target);
}

void ControlGunShot(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->counter--;
    if (!fx->counter) {
        KillEffect(fx_num);
        return;
    }
    fx->pos.z_rot = GetRandomControl();
}

int16_t GunShot(
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
        fx->counter = 3;
        fx->frame_number = 0;
        fx->object_number = O_GUN_FLASH;
        fx->shade = 4096;
    }
    return fx_num;
}

int16_t GunHit(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    PHD_VECTOR pos;
    pos.x = 0;
    pos.y = 0;
    pos.z = 0;
    GetJointAbsPosition(LaraItem, &pos, (GetRandomControl() * 25) / 0x7FFF);
    DoBloodSplat(
        pos.x, pos.y, pos.z, LaraItem->speed, LaraItem->pos.y_rot,
        LaraItem->room_number);
    SoundEffect(50, &LaraItem->pos, 0);
    return GunShot(x, y, z, speed, y_rot, room_num);
}

int16_t GunMiss(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    GAME_VECTOR pos;
    pos.x =
        LaraItem->pos.x + ((GetRandomDraw() - 0x4000) * (WALL_L / 2)) / 0x7FFF;
    pos.y = LaraItem->floor;
    pos.z =
        LaraItem->pos.z + ((GetRandomDraw() - 0x4000) * (WALL_L / 2)) / 0x7FFF;
    pos.room_number = LaraItem->room_number;
    Ricochet(&pos);
    return GunShot(x, y, z, speed, y_rot, room_num);
}

int32_t ShotLara(
    ITEM_INFO* item, int32_t distance, BITE_INFO* gun, int16_t extra_rotation)
{
    int32_t hit;
    if (distance > PEOPLE_SHOOT_RANGE) {
        hit = 0;
    } else {
        hit = GetRandomControl()
            < ((PEOPLE_SHOOT_RANGE - distance) / (PEOPLE_SHOOT_RANGE / 0x7FFF)
               - PEOPLE_MISS_CHANCE);
    }

    int16_t fx_num;
    if (hit) {
        fx_num = CreatureEffect(item, gun, GunHit);
    } else {
        fx_num = CreatureEffect(item, gun, GunMiss);
    }

    if (fx_num != NO_ITEM) {
        Effects[fx_num].pos.y_rot += extra_rotation;
    }

    return hit;
}

void PeopleControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* person = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != PEOPLE_DEATH) {
            item->current_anim_state = PEOPLE_DEATH;
            item->anim_number = Objects[O_LARSON].anim_index + LARSON_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, person->maximum_turn);

        switch (item->current_anim_state) {
        case PEOPLE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (person->mood == MOOD_BORED) {
                item->goal_anim_state = GetRandomControl() < PEOPLE_POSE_CHANCE
                    ? PEOPLE_POSE
                    : PEOPLE_WALK;
            } else if (person->mood == MOOD_ESCAPE) {
                item->goal_anim_state = PEOPLE_RUN;
            } else {
                item->goal_anim_state = PEOPLE_WALK;
            }
            break;

        case PEOPLE_POSE:
            if (person->mood != MOOD_BORED) {
                item->goal_anim_state = PEOPLE_STOP;
            } else if (GetRandomControl() < PEOPLE_POSE_CHANCE) {
                item->required_anim_state = PEOPLE_WALK;
                item->goal_anim_state = PEOPLE_STOP;
            }
            break;

        case PEOPLE_WALK:
            person->maximum_turn = PEOPLE_WALK_TURN;
            if (person->mood == MOOD_BORED
                && GetRandomControl() < PEOPLE_POSE_CHANCE) {
                item->required_anim_state = PEOPLE_POSE;
                item->goal_anim_state = PEOPLE_STOP;
            } else if (person->mood == MOOD_ESCAPE) {
                item->required_anim_state = PEOPLE_RUN;
                item->goal_anim_state = PEOPLE_STOP;
            } else if (Targetable(item, &info)) {
                item->required_anim_state = PEOPLE_AIM;
                item->goal_anim_state = PEOPLE_STOP;
            } else if (!info.ahead || info.distance > PEOPLE_WALK_RANGE) {
                item->required_anim_state = PEOPLE_RUN;
                item->goal_anim_state = PEOPLE_STOP;
            }
            break;

        case PEOPLE_RUN:
            person->maximum_turn = PEOPLE_RUN_TURN;
            tilt = angle / 2;
            if (person->mood == MOOD_BORED
                && GetRandomControl() < PEOPLE_POSE_CHANCE) {
                item->required_anim_state = PEOPLE_POSE;
                item->goal_anim_state = PEOPLE_STOP;
            } else if (Targetable(item, &info)) {
                item->required_anim_state = PEOPLE_AIM;
                item->goal_anim_state = PEOPLE_STOP;
            } else if (info.ahead && info.distance < PEOPLE_WALK_RANGE) {
                item->required_anim_state = PEOPLE_WALK;
                item->goal_anim_state = PEOPLE_STOP;
            }
            break;

        case PEOPLE_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = PEOPLE_SHOOT;
            } else {
                item->goal_anim_state = PEOPLE_STOP;
            }
            break;

        case PEOPLE_SHOOT:
            if (!item->required_anim_state) {
                if (ShotLara(item, info.distance, &LarsonGun, head)) {
                    LaraItem->hit_points -= PEOPLE_SHOT_DAMAGE;
                    LaraItem->hit_status = 1;
                }
                item->required_anim_state = PEOPLE_AIM;
            }
            if (person->mood == MOOD_ESCAPE) {
                item->required_anim_state = PEOPLE_STOP;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);
    CreatureHead(item, head);

    CreatureAnimation(item_num, angle, 0);
}

void T1MInjectGamePeople()
{
    INJECT(0x00430D80, Targetable);
    INJECT(0x00430E00, ControlGunShot);
    INJECT(0x00430E40, GunShot);
    INJECT(0x00430EB0, GunHit);
    INJECT(0x00430FA0, GunMiss);
    INJECT(0x00431090, PeopleControl);
}
