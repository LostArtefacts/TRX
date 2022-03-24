#include "game/ai/ape.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/effects/blood.h"
#include "game/lot.h"
#include "game/random.h"
#include "global/vars.h"

BITE_INFO g_ApeBite = { 0, -19, 75, 15 };

void SetupApe(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = ApeControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = APE_HITPOINTS;
    obj->pivot_length = 250;
    obj->radius = APE_RADIUS;
    obj->smartness = APE_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_index + 52] |= BEB_ROT_Y;
}

void ApeVault(int16_t item_num, int16_t angle)
{
    ITEM_INFO *item = &g_Items[item_num];
    CREATURE_INFO *ape = item->data;

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
    item->anim_number = g_Objects[O_APE].anim_index + APE_VAULT_ANIM;
    item->frame_number = g_Anims[item->anim_number].frame_base;
}

void ApeControl(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *ape = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != APE_DEATH) {
            item->current_anim_state = APE_DEATH;
            item->anim_number = g_Objects[O_APE].anim_index + APE_DIE_ANIM
                + (int16_t)(Random_GetControl() / 0x4000);
            item->frame_number = g_Anims[item->anim_number].frame_base;
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
                int16_t random = Random_GetControl() >> 5;
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
                int16_t random = Random_GetControl();
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
                CreatureEffect(item, &g_ApeBite, Blood_Spawn);
                g_LaraItem->hit_points -= APE_ATTACK_DAMAGE;
                g_LaraItem->hit_status = 1;
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
