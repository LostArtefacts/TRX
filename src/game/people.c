#include "game/box.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/people.h"
#include "game/sphere.h"
#include "game/vars.h"
#include "specific/sndpc.h"
#include "util.h"

#define PEOPLE_POSE_CHANCE 0x60 // = 96
#define PEOPLE_SHOOT_RANGE SQUARE(WALL_L * 7) // = 51380224
#define PEOPLE_SHOT_DAMAGE 50
#define PEOPLE_WALK_TURN (PHD_DEGREE * 3) // = 546
#define PEOPLE_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define PEOPLE_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define PEOPLE_MISS_CHANCE 0x2000

#define LARSON_DIE_ANIM 15

#define PIERRE_DIE_ANIM 12
#define PIERRE_WIMP_CHANCE 0x2000
#define PIERRE_RUN_HITPOINTS 40
#define PIERRE_DISAPPEAR 10

#define APE_ATTACK_DAMAGE 200
#define APE_TOUCH 0xFF00
#define APE_DIE_ANIM 7
#define APE_RUN_TURN (PHD_DEGREE * 5) // = 910
#define APE_DISPLAY_ANGLE (PHD_DEGREE * 45) // = 8190
#define APE_ATTACK_RANGE SQUARE(430) // = 184900
#define APE_PANIC_RANGE SQUARE(WALL_L * 2) // = 4194304
#define APE_JUMP_CHANCE 160
#define APE_WARN1_CHANCE (APE_JUMP_CHANCE + 160) // = 320
#define APE_WARN2_CHANCE (APE_WARN1_CHANCE + 160) // = 480
#define APE_RUN_LEFT_CHANCE (APE_WARN2_CHANCE + 272) // = 752
#define APE_ATTACK_FLAG 1
#define APE_VAULT_ANIM 19
#define APE_TURN_L_FLAG 2
#define APE_TURN_R_FLAG 4
#define APE_SHIFT 75

#define KID_STOP_SHOT_DAMAGE 50
#define KID_SKATE_SHOT_DAMAGE 40
#define KID_STOP_RANGE SQUARE(WALL_L * 4) // = 16777216
#define KID_DONT_STOP_RANGE SQUARE(WALL_L * 5 / 2) // = 6553600
#define KID_TOO_CLOSE SQUARE(WALL_L) // = 1048576
#define KID_SKATE_TURN (PHD_DEGREE * 4) // = 728
#define KID_PUSH_CHANCE 0x200
#define KID_SKATE_CHANCE 0x400
#define KID_DIE_ANIM 13

#define COWBOY_SHOT_DAMAGE 70
#define COWBOY_WALK_TURN (PHD_DEGREE * 3) // = 546
#define COWBOY_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define COWBOY_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define COWBOY_DIE_ANIM 7

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

typedef enum {
    APE_EMPTY = 0,
    APE_STOP = 1,
    APE_WALK = 2,
    APE_RUN = 3,
    APE_ATTACK1 = 4,
    APE_DEATH = 5,
    APE_WARNING = 6,
    APE_WARNING2 = 7,
    APE_RUN_LEFT = 8,
    APE_RUN_RIGHT = 9,
    APE_JUMP = 10,
    APE_VAULT = 11,
} APE_ANIM;

typedef enum {
    KID_STOP = 0,
    KID_SHOOT = 1,
    KID_SKATE = 2,
    KID_PUSH = 3,
    KID_SHOOT2 = 4,
    KID_DEATH = 5,
} KID_ANIM;

typedef enum {
    COWBOY_EMPTY = 0,
    COWBOY_STOP = 1,
    COWBOY_WALK = 2,
    COWBOY_RUN = 3,
    COWBOY_AIM = 4,
    COWBOY_DEATH = 5,
    COWBOY_SHOOT = 6,
} COWBOY_ANIM;

static BITE_INFO LarsonGun = { -60, 170, 0, 14 };
static BITE_INFO PierreGun1 = { 60, 200, 0, 11 };
static BITE_INFO PierreGun2 = { -57, 200, 0, 14 };
static BITE_INFO ApeBite = { 0, -19, 75, 15 };
static BITE_INFO KidGun1 = { 0, 150, 34, 7 };
static BITE_INFO KidGun2 = { 0, 150, 37, 4 };
static BITE_INFO CowboyGun1 = { 1, 200, 41, 5 };
static BITE_INFO CowboyGun2 = { -2, 200, 40, 8 };

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

void PierreControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (PierreItem == NO_ITEM) {
        PierreItem = item_num;
    } else if (PierreItem != item_num) {
        if (item->flags & IF_ONESHOT) {
            KillItem(PierreItem);
        } else {
            KillItem(item_num);
        }
    }

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* pierre = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= PIERRE_RUN_HITPOINTS
        && !(item->flags & IF_ONESHOT)) {
        item->hit_points = PIERRE_RUN_HITPOINTS;
        pierre->flags++;
    }

    if (item->hit_points <= 0) {
        if (item->current_anim_state != PEOPLE_DEATH) {
            item->current_anim_state = PEOPLE_DEATH;
            item->anim_number = Objects[O_PIERRE].anim_index + PIERRE_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
            SpawnItem(item, O_MAGNUM_ITEM);
            SpawnItem(item, O_SCION_ITEM2);
            SpawnItem(item, O_KEY_ITEM1);
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        if (pierre->flags) {
            info.enemy_zone = -1;
            item->hit_status = 1;
        }
        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, pierre->maximum_turn);

        switch (item->current_anim_state) {
        case PEOPLE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (pierre->mood == MOOD_BORED) {
                item->goal_anim_state = GetRandomControl() < PEOPLE_POSE_CHANCE
                    ? PEOPLE_POSE
                    : PEOPLE_WALK;
            } else if (pierre->mood == MOOD_ESCAPE) {
                item->goal_anim_state = PEOPLE_RUN;
            } else {
                item->goal_anim_state = PEOPLE_WALK;
            }
            break;

        case PEOPLE_POSE:
            if (pierre->mood != MOOD_BORED) {
                item->goal_anim_state = PEOPLE_STOP;
            } else if (GetRandomControl() < PEOPLE_POSE_CHANCE) {
                item->required_anim_state = PEOPLE_WALK;
                item->goal_anim_state = PEOPLE_STOP;
            }
            break;

        case PEOPLE_WALK:
            pierre->maximum_turn = PEOPLE_WALK_TURN;
            if (pierre->mood == MOOD_BORED
                && GetRandomControl() < PEOPLE_POSE_CHANCE) {
                item->required_anim_state = PEOPLE_POSE;
                item->goal_anim_state = PEOPLE_STOP;
            } else if (pierre->mood == MOOD_ESCAPE) {
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
            pierre->maximum_turn = PEOPLE_RUN_TURN;
            tilt = angle / 2;
            if (pierre->mood == MOOD_BORED
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
                if (ShotLara(item, info.distance, &PierreGun1, head)) {
                    LaraItem->hit_points -= PEOPLE_SHOT_DAMAGE / 2;
                    LaraItem->hit_status = 1;
                }
                if (ShotLara(item, info.distance, &PierreGun2, head)) {
                    LaraItem->hit_points -= PEOPLE_SHOT_DAMAGE / 2;
                    LaraItem->hit_status = 1;
                }
                item->required_anim_state = PEOPLE_AIM;
            }
            if (pierre->mood == MOOD_ESCAPE
                && GetRandomControl() > PIERRE_WIMP_CHANCE) {
                item->required_anim_state = PEOPLE_STOP;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);
    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, 0);

    if (pierre->flags) {
        GAME_VECTOR target;
        target.x = item->pos.x;
        target.y = item->pos.y - WALL_L;
        target.z = item->pos.z;

        GAME_VECTOR start;
        start.x = Camera.pos.x;
        start.y = Camera.pos.y;
        start.z = Camera.pos.z;
        start.room_number = Camera.pos.room_number;

        if (LOS(&start, &target)) {
            pierre->flags = 1;
        } else if (pierre->flags > PIERRE_DISAPPEAR) {
            item->hit_points = DONT_TARGET;
            DisableBaddieAI(item_num);
            KillItem(item_num);
            PierreItem = NO_ITEM;
        }
    }

    int16_t wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh != NO_HEIGHT) {
        item->hit_points = DONT_TARGET;
        DisableBaddieAI(item_num);
        KillItem(item_num);
        PierreItem = NO_ITEM;
    }
}

void ApeVault(int16_t item_num, int16_t angle)
{
    ITEM_INFO* item = &Items[item_num];
    CREATURE_INFO* ape = item->data;

    if (ape->flags & APE_TURN_L_FLAG) {
        item->pos.y_rot -= PHD_90;
        ape->flags &= ~APE_TURN_L_FLAG;
    } else if (item->flags & APE_TURN_R_FLAG) {
        item->pos.y_rot += PHD_90;
        ape->flags &= ~APE_TURN_R_FLAG;
    }

    int32_t xx = item->pos.z >> WALL_SHIFT;
    int32_t yy = item->pos.x >> WALL_SHIFT;
    int32_t y = item->pos.y;

    CreatureAnimation(item_num, angle, 0);

    if (item->pos.y > y - STEP_L * 3 / 2) {
        return;
    }

    int32_t x_floor = item->pos.z >> WALL_SHIFT;
    int32_t y_floor = item->pos.x >> WALL_SHIFT;
    if (xx == x_floor) {
        if (yy == y_floor) {
            return;
        }

        if (yy < y_floor) {
            item->pos.x = (y_floor << WALL_SHIFT) - APE_SHIFT;
            item->pos.y_rot = PHD_90;
        } else {
            item->pos.x = (yy << WALL_SHIFT) + APE_SHIFT;
            item->pos.y_rot = -PHD_90;
        }
    } else if (yy == y_floor) {
        if (xx < x_floor) {
            item->pos.z = (x_floor << WALL_SHIFT) - APE_SHIFT;
            item->pos.y_rot = 0;
        } else {
            item->pos.z = (xx << WALL_SHIFT) + APE_SHIFT;
            item->pos.y_rot = -PHD_180;
        }
    }

    item->pos.y = y;
    item->current_anim_state = APE_VAULT;
    item->anim_number = Objects[O_APE].anim_index + APE_VAULT_ANIM;
    item->frame_number = Anims[item->anim_number].frame_base;
}

void ApeControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* ape = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != APE_DEATH) {
            item->current_anim_state = APE_DEATH;
            item->anim_number = Objects[O_APE].anim_index + APE_DIE_ANIM
                + (int16_t)(GetRandomControl() / 0x4000);
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, ape->maximum_turn);

        if (item->hit_status || info.distance < APE_PANIC_RANGE) {
            ape->flags |= APE_ATTACK_FLAG;
        }

        switch (item->current_anim_state) {
        case APE_STOP:
            if (ape->flags & APE_TURN_L_FLAG) {
                item->pos.y_rot -= PHD_90;
                ape->flags &= ~APE_TURN_L_FLAG;
            } else if (item->flags & APE_TURN_R_FLAG) {
                item->pos.y_rot += PHD_90;
                ape->flags &= ~APE_TURN_R_FLAG;
            }

            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.bite && info.distance < APE_ATTACK_RANGE) {
                item->goal_anim_state = APE_ATTACK1;
            } else if (
                !(ape->flags & APE_ATTACK_FLAG)
                && info.zone_number == info.enemy_zone && info.ahead) {
                int16_t random = GetRandomControl() >> 5;
                if (random < APE_JUMP_CHANCE) {
                    item->goal_anim_state = APE_JUMP;
                } else if (random < APE_WARN1_CHANCE) {
                    item->goal_anim_state = APE_WARNING;
                } else if (random < APE_WARN2_CHANCE) {
                    item->goal_anim_state = APE_WARNING2;
                } else if (random < APE_RUN_LEFT_CHANCE) {
                    item->goal_anim_state = APE_RUN_LEFT;
                    ape->maximum_turn = 0;
                } else {
                    item->goal_anim_state = APE_RUN_RIGHT;
                    ape->maximum_turn = 0;
                }
            } else {
                item->goal_anim_state = APE_RUN;
            }
            break;

        case APE_RUN:
            ape->maximum_turn = APE_RUN_TURN;
            if (!ape->flags && info.angle > -APE_DISPLAY_ANGLE
                && info.angle < APE_DISPLAY_ANGLE) {
                item->goal_anim_state = APE_STOP;
            } else if (info.ahead && (item->touch_bits & APE_TOUCH)) {
                item->required_anim_state = APE_ATTACK1;
                item->goal_anim_state = APE_STOP;
            } else if (ape->mood != MOOD_ESCAPE) {
                int16_t random = GetRandomControl();
                if (random < APE_JUMP_CHANCE) {
                    item->required_anim_state = APE_JUMP;
                    item->goal_anim_state = APE_STOP;
                } else if (random < APE_WARN1_CHANCE) {
                    item->required_anim_state = APE_WARNING;
                    item->goal_anim_state = APE_STOP;
                } else if (random < APE_WARN2_CHANCE) {
                    item->required_anim_state = APE_WARNING2;
                    item->goal_anim_state = APE_STOP;
                }
            }
            break;

        case APE_RUN_LEFT:
            if (!(ape->flags & APE_TURN_R_FLAG)) {
                item->pos.y_rot -= PHD_90;
                ape->flags |= APE_TURN_R_FLAG;
            }
            item->goal_anim_state = APE_STOP;
            break;

        case APE_RUN_RIGHT:
            if (!(ape->flags & APE_TURN_L_FLAG)) {
                item->pos.y_rot += PHD_90;
                ape->flags |= APE_TURN_L_FLAG;
            }
            item->goal_anim_state = APE_STOP;
            break;

        case APE_ATTACK1:
            if (!item->required_anim_state && (item->touch_bits & APE_TOUCH)) {
                CreatureEffect(item, &ApeBite, DoBloodSplat);
                LaraItem->hit_points -= APE_ATTACK_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = APE_STOP;
            }
            break;
        }
    }

    CreatureHead(item, head);

    if (item->current_anim_state == APE_VAULT) {
        CreatureAnimation(item_num, angle, 0);
    } else {
        ApeVault(item_num, angle);
    }
}

void InitialiseSkateKid(int16_t item_num)
{
    InitialiseCreature(item_num);
    Items[item_num].current_anim_state = KID_SKATE;
}

void SkateKidControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    CREATURE_INFO* kid = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != KID_DEATH) {
            item->current_anim_state = KID_DEATH;
            item->anim_number = Objects[O_MERCENARY1].anim_index + KID_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
            SpawnItem(item, O_UZI_ITEM);
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, KID_SKATE_TURN);

        if (item->hit_points < 120 && CDTrack != 56) {
            S_CDPlay(56);
        }

        switch (item->current_anim_state) {
        case KID_STOP:
            kid->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = KID_SHOOT;
            } else {
                item->goal_anim_state = KID_SKATE;
            }
            break;

        case KID_SKATE:
            kid->flags = 0;
            if (GetRandomControl() < KID_PUSH_CHANCE) {
                item->goal_anim_state = KID_PUSH;
            } else if (Targetable(item, &info)) {
                if (info.distance > KID_DONT_STOP_RANGE
                    && info.distance < KID_STOP_RANGE
                    && kid->mood != MOOD_ESCAPE) {
                    item->goal_anim_state = KID_STOP;
                } else {
                    item->goal_anim_state = KID_SHOOT2;
                }
            }
            break;

        case KID_PUSH:
            if (GetRandomControl() < KID_SKATE_CHANCE) {
                item->goal_anim_state = KID_SKATE;
            }
            break;

        case KID_SHOOT:
        case KID_SHOOT2:
            if (!kid->flags && Targetable(item, &info)) {
                if (ShotLara(item, info.distance, &KidGun1, head)) {
                    LaraItem->hit_points -=
                        item->current_anim_state == KID_SHOOT
                        ? KID_STOP_SHOT_DAMAGE
                        : KID_SKATE_SHOT_DAMAGE;
                    LaraItem->hit_status = 1;
                }

                if (ShotLara(item, info.distance, &KidGun2, head)) {
                    LaraItem->hit_points -=
                        item->current_anim_state == KID_SHOOT
                        ? KID_STOP_SHOT_DAMAGE
                        : KID_SKATE_SHOT_DAMAGE;
                    LaraItem->hit_status = 1;
                }

                kid->flags = 1;
            }
            if (kid->mood == MOOD_ESCAPE || info.distance < KID_TOO_CLOSE) {
                item->required_anim_state = KID_SKATE;
            }
            break;
        }
    }

    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, 0);
}

void DrawSkateKid(ITEM_INFO* item)
{
    DrawAnimatingItem(item);
    int16_t anim = item->anim_number;
    int16_t frame = item->frame_number;
    item->object_number = O_SKATEBOARD;
    item->anim_number = anim + Objects[O_SKATEBOARD].anim_index
        - Objects[O_MERCENARY1].anim_index;
    DrawAnimatingItem(item);
    item->anim_number = anim;
    item->frame_number = frame;
    item->object_number = O_MERCENARY1;
}

void CowboyControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* cowboy = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != COWBOY_DEATH) {
            item->current_anim_state = COWBOY_DEATH;
            item->anim_number =
                Objects[O_MERCENARY2].anim_index + COWBOY_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
            SpawnItem(item, O_MAGNUM_ITEM);
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, cowboy->maximum_turn);

        switch (item->current_anim_state) {
        case COWBOY_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = COWBOY_AIM;
            } else if (cowboy->mood == MOOD_BORED) {
                item->goal_anim_state = COWBOY_WALK;
            } else {
                item->goal_anim_state = COWBOY_RUN;
            }
            break;

        case COWBOY_WALK:
            cowboy->maximum_turn = COWBOY_WALK_TURN;
            if (cowboy->mood == MOOD_ESCAPE || !info.ahead) {
                item->required_anim_state = COWBOY_RUN;
                item->goal_anim_state = COWBOY_STOP;
            } else if (Targetable(item, &info)) {
                item->required_anim_state = COWBOY_AIM;
                item->goal_anim_state = COWBOY_STOP;
            } else if (info.distance > COWBOY_WALK_RANGE) {
                item->required_anim_state = COWBOY_RUN;
                item->goal_anim_state = COWBOY_STOP;
            }
            break;

        case COWBOY_RUN:
            cowboy->maximum_turn = COWBOY_RUN_TURN;
            tilt = angle / 2;
            if (cowboy->mood != MOOD_ESCAPE || info.ahead) {
                if (Targetable(item, &info)) {
                    item->required_anim_state = COWBOY_AIM;
                    item->goal_anim_state = COWBOY_STOP;
                } else if (info.ahead && info.distance < COWBOY_WALK_RANGE) {
                    item->required_anim_state = COWBOY_WALK;
                    item->goal_anim_state = COWBOY_STOP;
                }
            }
            break;

        case COWBOY_AIM:
            cowboy->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = COWBOY_STOP;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = COWBOY_SHOOT;
            } else {
                item->goal_anim_state = COWBOY_STOP;
            }
            break;

        case COWBOY_SHOOT:
            if (!cowboy->flags) {
                if (ShotLara(item, info.distance, &CowboyGun1, head)) {
                    LaraItem->hit_points -= COWBOY_SHOT_DAMAGE;
                    LaraItem->hit_status = 1;
                }
            } else if (cowboy->flags == 6) {
                if (Targetable(item, &info)) {
                    if (ShotLara(item, info.distance, &CowboyGun2, head)) {
                        LaraItem->hit_points -= COWBOY_SHOT_DAMAGE;
                        LaraItem->hit_status = 1;
                    }
                } else {
                    int16_t fx_num = CreatureEffect(item, &CowboyGun2, GunShot);
                    if (fx_num != NO_ITEM) {
                        Effects[fx_num].pos.y_rot += head;
                    }
                }
            }
            cowboy->flags++;

            if (cowboy->mood == MOOD_ESCAPE) {
                item->required_anim_state = COWBOY_RUN;
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
    INJECT(0x00431550, PierreControl);
    INJECT(0x00431C30, ApeVault);
    INJECT(0x00431D40, ApeControl);
    INJECT(0x004320B0, InitialiseSkateKid);
    INJECT(0x004320E0, SkateKidControl);
    INJECT(0x00432550, DrawSkateKid);
    INJECT(0x004325A0, CowboyControl);
}
