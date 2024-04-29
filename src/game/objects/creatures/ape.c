#include "game/objects/creatures/ape.h"

#include "game/creature.h"
#include "game/effects/blood.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <stdbool.h>

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
#define APE_HITPOINTS 22
#define APE_RADIUS (WALL_L / 3) // = 341
#define APE_SMARTNESS 0x7FFF

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

static BITE_INFO m_ApeBite = { 0, -19, 75, 15 };

static bool Ape_Vault(int16_t item_num, int16_t angle);

static bool Ape_Vault(int16_t item_num, int16_t angle)
{
    ITEM_INFO *item = &g_Items[item_num];
    CREATURE_INFO *ape = item->data;
    int32_t x = item->pos.x >> WALL_SHIFT;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z >> WALL_SHIFT;
    int16_t room_num = item->room_number;

    if (ape->flags & APE_TURN_L_FLAG) {
        item->rot.y -= PHD_90;
        ape->flags &= ~APE_TURN_L_FLAG;
    } else if (ape->flags & APE_TURN_R_FLAG) {
        item->rot.y += PHD_90;
        ape->flags &= ~APE_TURN_R_FLAG;
    }

    Creature_Animate(item_num, angle, 0);

    if (item->pos.y > y - STEP_L * 3 / 2) {
        return false;
    }

    int32_t x_floor = item->pos.x >> WALL_SHIFT;
    int32_t z_floor = item->pos.z >> WALL_SHIFT;

    if (z == z_floor) {
        if (x == x_floor) {
            return false;
        }

        if (x >= x_floor) {
            item->rot.y = -PHD_90;
            item->pos.x = (x << WALL_SHIFT) + APE_SHIFT;
        } else {
            item->rot.y = PHD_90;
            item->pos.x = (x_floor << WALL_SHIFT) - APE_SHIFT;
        }
    } else if (x == x_floor) {
        if (z < z_floor) {
            item->rot.y = 0;
            item->pos.z = (z_floor << WALL_SHIFT) - APE_SHIFT;
        } else {
            item->rot.y = -PHD_180;
            item->pos.z = (z << WALL_SHIFT) + APE_SHIFT;
        }
    }

    item->floor = y;
    item->pos.y = y;

    if (item->room_number != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    return true;
}

void Ape_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Creature_Initialise;
    obj->control = Ape_Control;
    obj->collision = Creature_Collision;
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

void Ape_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
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
            Item_SwitchToAnim(
                item, APE_DIE_ANIM + (int16_t)(Random_GetControl() / 0x4000),
                0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, ape->maximum_turn);

        if (item->hit_status || info.distance < APE_PANIC_RANGE) {
            ape->flags |= APE_ATTACK_FLAG;
        }

        switch (item->current_anim_state) {
        case APE_STOP:
            if (ape->flags & APE_TURN_L_FLAG) {
                item->rot.y -= PHD_90;
                ape->flags &= ~APE_TURN_L_FLAG;
            } else if (ape->flags & APE_TURN_R_FLAG) {
                item->rot.y += PHD_90;
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
                item->rot.y -= PHD_90;
                ape->flags |= APE_TURN_R_FLAG;
            }
            item->goal_anim_state = APE_STOP;
            break;

        case APE_RUN_RIGHT:
            if (!(ape->flags & APE_TURN_L_FLAG)) {
                item->rot.y += PHD_90;
                ape->flags |= APE_TURN_L_FLAG;
            }
            item->goal_anim_state = APE_STOP;
            break;

        case APE_ATTACK1:
            if (!item->required_anim_state && (item->touch_bits & APE_TOUCH)) {
                Creature_Effect(item, &m_ApeBite, Effect_Blood);
                Lara_TakeDamage(APE_ATTACK_DAMAGE, true);
                item->required_anim_state = APE_STOP;
            }
            break;
        }
    }

    Creature_Head(item, head);

    if (item->current_anim_state == APE_VAULT) {
        Creature_Animate(item_num, angle, 0);
    } else if (Ape_Vault(item_num, angle)) {
        ape->maximum_turn = 0;
        item->current_anim_state = APE_VAULT;
        Item_SwitchToAnim(item, APE_VAULT_ANIM, 0);
    }
}
