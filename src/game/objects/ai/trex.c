#include "game/objects/ai/trex.h"

#include "config.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/random.h"
#include "global/vars.h"

#define TREX_ATTACK_RANGE SQUARE(WALL_L * 4) // = 16777216
#define TREX_BITE_DAMAGE 10000
#define TREX_BITE_RANGE SQUARE(1500) // = 2250000
#define TREX_ROAR_CHANCE 512
#define TREX_RUN_RANGE SQUARE(WALL_L * 5) // = 26214400
#define TREX_RUN_TURN (4 * PHD_DEGREE) // = 728
#define TREX_TOUCH 0x3000
#define TREX_TOUCH_DAMAGE 1
#define TREX_TRAMPLE_DAMAGE 10
#define TREX_WALK_TURN (2 * PHD_DEGREE) // = 364
#define TREX_HITPOINTS 100
#define TREX_RADIUS (WALL_L / 3) // = 341
#define TREX_SMARTNESS 0x7FFF

typedef enum {
    TREX_EMPTY = 0,
    TREX_STOP = 1,
    TREX_WALK = 2,
    TREX_RUN = 3,
    TREX_ATTACK1 = 4,
    TREX_DEATH = 5,
    TREX_ROAR = 6,
    TREX_ATTACK2 = 7,
    TREX_KILL = 8,
} TREX_ANIM;

void TRex_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = TRex_Control;
    obj->draw_routine = DrawUnclippedItem;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = TREX_HITPOINTS;
    obj->pivot_length = 2000;
    obj->radius = TREX_RADIUS;
    obj->smartness = TREX_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_index + 40] |= BEB_ROT_Y;
    g_AnimBones[obj->bone_index + 44] |= BEB_ROT_Y;
}

void TRex_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *dino = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (g_Config.disable_trex_collision) {
            OBJECT_INFO *obj = &g_Objects[item->object_number];
            obj->collision = NULL;
        }

        if (item->current_anim_state == TREX_STOP) {
            item->goal_anim_state = TREX_DEATH;
        } else {
            item->goal_anim_state = TREX_STOP;
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
            if (item->current_anim_state == TREX_RUN) {
                g_LaraItem->hit_points -= TREX_TRAMPLE_DAMAGE;
            } else {
                g_LaraItem->hit_points -= TREX_TOUCH_DAMAGE;
            }
        }

        dino->flags = dino->mood != MOOD_ESCAPE && !info.ahead
            && info.enemy_facing > -FRONT_ARC && info.enemy_facing < FRONT_ARC;

        if (!dino->flags && info.distance > TREX_BITE_RANGE
            && info.distance < TREX_ATTACK_RANGE && info.bite) {
            dino->flags = 1;
        }

        switch (item->current_anim_state) {
        case TREX_STOP:
            if (item->required_anim_state != TREX_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.distance < TREX_BITE_RANGE && info.bite) {
                item->goal_anim_state = TREX_ATTACK2;
            } else if (dino->mood == MOOD_BORED || dino->flags) {
                item->goal_anim_state = TREX_WALK;
            } else {
                item->goal_anim_state = TREX_RUN;
            }
            break;

        case TREX_WALK:
            dino->maximum_turn = TREX_WALK_TURN;
            if (dino->mood != MOOD_BORED || !dino->flags) {
                item->goal_anim_state = TREX_STOP;
            } else if (info.ahead && Random_GetControl() < TREX_ROAR_CHANCE) {
                item->required_anim_state = TREX_ROAR;
                item->goal_anim_state = TREX_STOP;
            }
            break;

        case TREX_RUN:
            dino->maximum_turn = TREX_RUN_TURN;
            if (info.distance < TREX_RUN_RANGE && info.bite) {
                item->goal_anim_state = TREX_STOP;
            } else if (dino->flags) {
                item->goal_anim_state = TREX_STOP;
            } else if (
                dino->mood != MOOD_ESCAPE && info.ahead
                && Random_GetControl() < TREX_ROAR_CHANCE) {
                item->required_anim_state = TREX_ROAR;
                item->goal_anim_state = TREX_STOP;
            } else if (dino->mood == MOOD_BORED) {
                item->goal_anim_state = TREX_STOP;
            }
            break;

        case TREX_ATTACK2:
            if (item->touch_bits & TREX_TOUCH) {
                g_LaraItem->hit_points -= TREX_BITE_DAMAGE;
                g_LaraItem->hit_status = 1;
                item->goal_anim_state = TREX_KILL;
                TRex_LaraDeath(item);
            }
            item->required_anim_state = TREX_WALK;
            break;
        }
    }

    CreatureHead(item, head >> 1);
    dino->neck_rotation = dino->head_rotation;
    CreatureAnimation(item_num, angle, 0);
    item->collidable = 1;
}

void TRex_LaraDeath(ITEM_INFO *item)
{
    item->goal_anim_state = TREX_KILL;
    if (g_LaraItem->room_number != item->room_number) {
        ItemNewRoom(g_Lara.item_number, item->room_number);
    }

    g_LaraItem->pos.x = item->pos.x;
    g_LaraItem->pos.y = item->pos.y;
    g_LaraItem->pos.z = item->pos.z;
    g_LaraItem->pos.x_rot = 0;
    g_LaraItem->pos.y_rot = item->pos.y_rot;
    g_LaraItem->pos.z_rot = 0;
    g_LaraItem->gravity_status = 0;
    g_LaraItem->current_anim_state = LS_SPECIAL;
    g_LaraItem->goal_anim_state = LS_SPECIAL;
    g_LaraItem->anim_number = g_Objects[O_LARA_EXTRA].anim_index + 1;
    g_LaraItem->frame_number = g_Anims[g_LaraItem->anim_number].frame_base;
    Lara_SwapMeshExtra();

    g_LaraItem->hit_points = -1;
    g_Lara.air = -1;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    g_Lara.gun_type = LGT_UNARMED;

    g_Camera.flags = FOLLOW_CENTRE;
    g_Camera.target_angle = 170 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
}
