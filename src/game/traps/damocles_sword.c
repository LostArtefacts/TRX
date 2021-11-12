#include "game/traps/damocles_sword.h"

#include "game/collide.h"
#include "game/effects/blood.h"
#include "game/game.h"
#include "game/items.h"
#include "game/sound.h"
#include "global/vars.h"

#define DAMOCLES_SWORD_ACTIVATE_DIST ((WALL_L * 3) / 2)
#define DAMOCLES_SWORD_DAMAGE 100

void SetupDamoclesSword(OBJECT_INFO *obj)
{
    obj->initialise = InitialiseDamoclesSword;
    obj->control = DamoclesSwordControl;
    obj->collision = DamoclesSwordCollision;
    obj->shadow_size = UNIT_SHADOW;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void InitialiseDamoclesSword(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    item->pos.y_rot = GetRandomControl();
    item->required_anim_state = (GetRandomControl() - 0x4000) / 16;
    item->fall_speed = 50;
}

void DamoclesSwordControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
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
            RemoveActiveItem(item_num);
        }
    } else if (item->pos.y != item->floor) {
        item->pos.y_rot += item->required_anim_state;
        int32_t x = LaraItem->pos.x - item->pos.x;
        int32_t y = LaraItem->pos.y - item->pos.y;
        int32_t z = LaraItem->pos.z - item->pos.z;
        if (ABS(x) <= DAMOCLES_SWORD_ACTIVATE_DIST
            && ABS(z) <= DAMOCLES_SWORD_ACTIVATE_DIST && y > 0
            && y < WALL_L * 3) {
            item->current_anim_state = x / 32;
            item->goal_anim_state = z / 32;
            item->gravity_status = 1;
        }
    }
}

void DamoclesSwordCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];
    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push) {
        ItemPushLara(item, lara_item, coll, 0, 1);
    }
    if (item->gravity_status) {
        lara_item->hit_points -= DAMOCLES_SWORD_DAMAGE;
        int32_t x = lara_item->pos.x + (GetRandomControl() - 0x4000) / 256;
        int32_t z = lara_item->pos.z + (GetRandomControl() - 0x4000) / 256;
        int32_t y = lara_item->pos.y - GetRandomControl() / 44;
        int32_t d = lara_item->pos.y_rot + (GetRandomControl() - 0x4000) / 8;
        DoBloodSplat(x, y, z, lara_item->speed, d, lara_item->room_number);
    }
}
