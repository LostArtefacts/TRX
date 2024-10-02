#include "game/objects/creatures/crocodile.h"

#include "config.h"
#include "game/carrier.h"
#include "game/creature.h"
#include "game/effects/blood.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lot.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define CROCODILE_BITE_DAMAGE 100
#define CROCODILE_BITE_RANGE SQUARE(435) // = 189225
#define CROCODILE_DIE_ANIM 11
#define CROCODILE_FASTTURN_ANGLE 0x4000
#define CROCODILE_FASTTURN_RANGE SQUARE(WALL_L * 3) // = 9437184
#define CROCODILE_FASTTURN_TURN (6 * PHD_DEGREE) // = 1092
#define CROCODILE_TOUCH 0x3FC
#define CROCODILE_TURN (3 * PHD_DEGREE) // = 546
#define CROCODILE_HITPOINTS 20
#define CROCODILE_RADIUS (WALL_L / 3) // = 341
#define CROCODILE_SMARTNESS 0x2000

#define ALLIGATOR_BITE_DAMAGE 100
#define ALLIGATOR_DIE_ANIM 4
#define ALLIGATOR_FLOAT_SPEED (WALL_L / 32) // = 32
#define ALLIGATOR_TURN (3 * PHD_DEGREE) // = 546
#define ALLIGATOR_HITPOINTS 20
#define ALLIGATOR_RADIUS (WALL_L / 3) // = 341
#define ALLIGATOR_SMARTNESS 0x400
#define ALLIGATOR_BITE_AF 42

typedef enum {
    CROCODILE_EMPTY = 0,
    CROCODILE_STOP = 1,
    CROCODILE_RUN = 2,
    CROCODILE_WALK = 3,
    CROCODILE_FASTTURN = 4,
    CROCODILE_ATTACK1 = 5,
    CROCODILE_ATTACK2 = 6,
    CROCODILE_DEATH = 7,
} CROCODILE_ANIM;

typedef enum {
    ALLIGATOR_EMPTY = 0,
    ALLIGATOR_SWIM = 1,
    ALLIGATOR_ATTACK = 2,
    ALLIGATOR_DEATH = 3,
} ALLIGATOR_ANIM;

static BITE m_CrocodileBite = { 5, -21, 467, 9 };

static const HYBRID_INFO m_CrocodileInfo = {
    .land.id = O_CROCODILE,
    .land.active_anim = CROCODILE_EMPTY,
    .land.death_anim = CROCODILE_DIE_ANIM,
    .land.death_state = CROCODILE_DEATH,
    .water.id = O_ALLIGATOR,
    .water.active_anim = ALLIGATOR_EMPTY,
};

void Croc_Setup(OBJECT *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Creature_Initialise;
    obj->control = Croc_Control;
    obj->collision = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 3;
    obj->hit_points = CROCODILE_HITPOINTS;
    obj->pivot_length = 600;
    obj->radius = CROCODILE_RADIUS;
    obj->smartness = CROCODILE_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_idx + 28] |= BEB_ROT_Y;
}

void Croc_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *croc = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != CROCODILE_DEATH) {
            item->current_anim_state = CROCODILE_DEATH;
            Item_SwitchToAnim(item, CROCODILE_DIE_ANIM, 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        if (item->current_anim_state == CROCODILE_FASTTURN) {
            item->rot.y += CROCODILE_FASTTURN_TURN;
        } else {
            angle = Creature_Turn(item, CROCODILE_TURN);
        }

        switch (item->current_anim_state) {
        case CROCODILE_STOP:
            if (info.bite && info.distance < CROCODILE_BITE_RANGE) {
                item->goal_anim_state = CROCODILE_ATTACK1;
            } else if (croc->mood == MOOD_ESCAPE) {
                item->goal_anim_state = CROCODILE_RUN;
            } else if (croc->mood == MOOD_ATTACK) {
                if ((info.angle < -CROCODILE_FASTTURN_ANGLE
                     || info.angle > CROCODILE_FASTTURN_ANGLE)
                    && info.distance > CROCODILE_FASTTURN_RANGE) {
                    item->goal_anim_state = CROCODILE_FASTTURN;
                } else {
                    item->goal_anim_state = CROCODILE_RUN;
                }
            } else if (croc->mood == MOOD_STALK) {
                item->goal_anim_state = CROCODILE_WALK;
            }
            break;

        case CROCODILE_WALK:
            if (info.ahead && (item->touch_bits & CROCODILE_TOUCH)) {
                item->goal_anim_state = CROCODILE_STOP;
            } else if (croc->mood == MOOD_ATTACK || croc->mood == MOOD_ESCAPE) {
                item->goal_anim_state = CROCODILE_RUN;
            } else if (croc->mood == MOOD_BORED) {
                item->goal_anim_state = CROCODILE_STOP;
            }
            break;

        case CROCODILE_FASTTURN:
            if (info.angle > -CROCODILE_FASTTURN_ANGLE
                && info.angle < CROCODILE_FASTTURN_ANGLE) {
                item->goal_anim_state = CROCODILE_WALK;
            }
            break;

        case CROCODILE_RUN:
            if (info.ahead && (item->touch_bits & CROCODILE_TOUCH)) {
                item->goal_anim_state = CROCODILE_STOP;
            } else if (croc->mood == MOOD_STALK) {
                item->goal_anim_state = CROCODILE_WALK;
            } else if (croc->mood == MOOD_BORED) {
                item->goal_anim_state = CROCODILE_STOP;
            } else if (
                croc->mood == MOOD_ATTACK
                && info.distance > CROCODILE_FASTTURN_RANGE
                && (info.angle < -CROCODILE_FASTTURN_ANGLE
                    || info.angle > CROCODILE_FASTTURN_ANGLE)) {
                item->goal_anim_state = CROCODILE_STOP;
            }
            break;

        case CROCODILE_ATTACK1:
            if (item->required_anim_state == CROCODILE_EMPTY) {
                Creature_Effect(item, &m_CrocodileBite, Effect_Blood);
                Lara_TakeDamage(CROCODILE_BITE_DAMAGE, true);
                item->required_anim_state = CROCODILE_STOP;
            }
            break;
        }
    }

    if (croc) {
        Creature_Head(item, head);
    }

    // Test conversion to alligator and set relevant pathfinding values.
    int32_t wh;
    if (Creature_EnsureHabitat(item_num, &wh, &m_CrocodileInfo) && croc) {
        croc->lot.step = WALL_L * 20;
        croc->lot.drop = -WALL_L * 20;
        croc->lot.fly = STEP_L / 16;
    }

    if (croc) {
        Creature_Animate(item_num, angle, 0);
    } else {
        Item_Animate(item);
    }
}

void Alligator_Setup(OBJECT *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Creature_Initialise;
    obj->control = Alligator_Control;
    obj->collision = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 3;
    obj->hit_points = ALLIGATOR_HITPOINTS;
    obj->pivot_length = 600;
    obj->radius = ALLIGATOR_RADIUS;
    obj->smartness = ALLIGATOR_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_idx + 28] |= BEB_ROT_Y;
}

void Alligator_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *gator = item->data;
    const SECTOR *sector;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t room_num;
    int32_t wh;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != ALLIGATOR_DEATH) {
            item->current_anim_state = ALLIGATOR_DEATH;
            Item_SwitchToAnim(item, ALLIGATOR_DIE_ANIM, 0);
            item->hit_points = DONT_TARGET;
            Carrier_TestItemDrops(item_num);
        }

        // Test if we should convert to a crocodile. If not, control the death
        // pose of the alligator in the water.
        if (!Creature_EnsureHabitat(item_num, &wh, &m_CrocodileInfo)) {
            if (item->pos.y > wh + ALLIGATOR_FLOAT_SPEED) {
                item->pos.y -= ALLIGATOR_FLOAT_SPEED;
            } else if (item->pos.y < wh) {
                item->pos.y = wh;
                if (gator) {
                    LOT_DisableBaddieAI(item_num);
                }
            }
        }

        Item_Animate(item);

        room_num = item->room_num;
        sector =
            Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
        item->floor =
            Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
        if (room_num != item->room_num) {
            Item_NewRoom(item_num, room_num);
        }
        return;
    }

    AI_INFO info;
    Creature_AIInfo(item, &info);

    if (info.ahead) {
        head = info.angle;
    }

    Creature_Mood(item, &info, true);
    Creature_Turn(item, ALLIGATOR_TURN);

    switch (item->current_anim_state) {
    case ALLIGATOR_SWIM:
        if (info.bite && item->touch_bits) {
            item->goal_anim_state = ALLIGATOR_ATTACK;
            if (g_Config.fix_alligator_ai) {
                item->required_anim_state = ALLIGATOR_SWIM;
            }
        }
        break;

    case ALLIGATOR_ATTACK:
        if (item->frame_num
            == (g_Config.fix_alligator_ai
                    ? ALLIGATOR_BITE_AF
                    : g_Anims[item->anim_num].frame_base)) {
            item->required_anim_state = ALLIGATOR_EMPTY;
        }

        if (info.bite && item->touch_bits) {
            if (item->required_anim_state == ALLIGATOR_EMPTY) {
                Creature_Effect(item, &m_CrocodileBite, Effect_Blood);
                Lara_TakeDamage(ALLIGATOR_BITE_DAMAGE, true);
                item->required_anim_state = ALLIGATOR_SWIM;
            }
            if (g_Config.fix_alligator_ai) {
                item->goal_anim_state = ALLIGATOR_SWIM;
            }
        } else {
            item->goal_anim_state = ALLIGATOR_SWIM;
        }
        break;
    }

    Creature_Head(item, head);

    // Test alive conversion to crocodile and set relevant pathfinding values.
    if (Creature_EnsureHabitat(item_num, &wh, &m_CrocodileInfo)) {
        gator->lot.step = STEP_L;
        gator->lot.drop = -STEP_L;
        gator->lot.fly = 0;
    } else if (item->pos.y < wh + STEP_L) {
        item->pos.y = wh + STEP_L;
    }

    Creature_Animate(item_num, angle, 0);
}
