#include "game/objects/traps/damocles_sword.h"

#include "game/effects/blood.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

#define DAMOCLES_SWORD_ACTIVATE_DIST ((WALL_L * 3) / 2)
#define DAMOCLES_SWORD_DAMAGE 100

void DamoclesSword_Setup(OBJECT_INFO *obj)
{
    obj->initialise = DamoclesSword_Initialise;
    obj->control = DamoclesSword_Control;
    obj->collision = DamoclesSword_Collision;
    obj->shadow_size = UNIT_SHADOW;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void DamoclesSword_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    item->pos.y_rot = Random_GetControl();
    item->required_anim_state = (Random_GetControl() - 0x4000) / 16;
    item->fall_speed = 50;
}

void DamoclesSword_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->gravity_status) {
        item->pos.y_rot += item->required_anim_state;
        item->fall_speed += item->fall_speed < FASTFALL_SPEED ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
        item->pos.x += item->current_anim_state;
        item->pos.z += item->goal_anim_state;

        if (item->pos.y > item->floor) {
            Sound_Effect(SFX_DAMOCLES_SWORD, &item->pos, SPM_NORMAL);
            item->pos.y = item->floor + 10;
            item->gravity_status = 0;
            item->status = IS_DEACTIVATED;
            Item_RemoveActive(item_num);
        }
    } else if (item->pos.y != item->floor) {
        item->pos.y_rot += item->required_anim_state;
        int32_t x = g_LaraItem->pos.x - item->pos.x;
        int32_t y = g_LaraItem->pos.y - item->pos.y;
        int32_t z = g_LaraItem->pos.z - item->pos.z;
        if (ABS(x) <= DAMOCLES_SWORD_ACTIVATE_DIST
            && ABS(z) <= DAMOCLES_SWORD_ACTIVATE_DIST && y > 0
            && y < WALL_L * 3) {
            item->current_anim_state = x / 32;
            item->goal_anim_state = z / 32;
            item->gravity_status = 1;
        }
    }
}

void DamoclesSword_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, false, true);
    }
    if (item->gravity_status) {
        lara_item->hit_points -= DAMOCLES_SWORD_DAMAGE;
        int32_t x = lara_item->pos.x + (Random_GetControl() - 0x4000) / 256;
        int32_t z = lara_item->pos.z + (Random_GetControl() - 0x4000) / 256;
        int32_t y = lara_item->pos.y - Random_GetControl() / 44;
        int32_t d = lara_item->pos.y_rot + (Random_GetControl() - 0x4000) / 8;
        Effect_Blood(x, y, z, lara_item->speed, d, lara_item->room_number);
    }
}
