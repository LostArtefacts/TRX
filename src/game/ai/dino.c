#include "game/ai/dino.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/vars.h"

void SetupDino(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = DinoControl;
    obj->draw_routine = DrawUnclippedItem;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = DINO_HITPOINTS;
    obj->pivot_length = 2000;
    obj->radius = DINO_RADIUS;
    obj->smartness = DINO_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    AnimBones[obj->bone_index + 40] |= BEB_ROT_Y;
    AnimBones[obj->bone_index + 44] |= BEB_ROT_Y;
}

void DinoControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

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

void LaraDinoDeath(ITEM_INFO *item)
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
