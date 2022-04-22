#include "game/objects/ai/statue.h"

#include "game/collide.h"
#include "game/effects/exploding_death.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/vars.h"

#define STATUE_EXPLODE_DIST (WALL_L * 7 / 2) // = 3584

void Statue_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Statue_Initialise;
    obj->control = Statue_Control;
    obj->collision = ObjectCollision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void Statue_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    int16_t centaur_item_num = Item_Create();
    if (centaur_item_num == NO_ITEM) {
        Shell_ExitSystem("FATAL: Cannot create CENTAUR in STATUE");
        return;
    }

    ITEM_INFO *centaur = &g_Items[centaur_item_num];
    centaur->object_number = O_CENTAUR;
    centaur->room_number = item->room_number;
    centaur->pos.x = item->pos.x;
    centaur->pos.y = item->pos.y;
    centaur->pos.z = item->pos.z;
    centaur->flags = IF_NOT_VISIBLE;
    centaur->shade = -1;

    Item_Initialise(centaur_item_num);

    centaur->anim_number = g_Objects[O_CENTAUR].anim_index + 7;
    centaur->frame_number = g_Anims[centaur->anim_number].frame_base + 36;
    centaur->current_anim_state =
        g_Anims[centaur->anim_number].current_anim_state;
    centaur->goal_anim_state = centaur->current_anim_state;
    centaur->pos.y_rot = item->pos.y_rot;

    item->data = GameBuf_Alloc(sizeof(int16_t), GBUF_CREATURE_DATA);
    *(int16_t *)item->data = centaur_item_num;

    g_LevelItemCount++;
}

void Statue_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    int32_t x = g_LaraItem->pos.x - item->pos.x;
    int32_t y = g_LaraItem->pos.y - item->pos.y;
    int32_t z = g_LaraItem->pos.z - item->pos.z;

    if (y > -WALL_L && y < WALL_L
        && SQUARE(x) + SQUARE(z) < SQUARE(STATUE_EXPLODE_DIST)) {
        Effect_ExplodingDeath(item_num, -1, 0);
        Item_Kill(item_num);
        item->status = IS_DEACTIVATED;

        int16_t centaur_item_num = *(int16_t *)item->data;
        ITEM_INFO *centaur = &g_Items[centaur_item_num];
        centaur->touch_bits = 0;
        AddActiveItem(centaur_item_num);
        EnableBaddieAI(centaur_item_num, 1);
        centaur->status = IS_ACTIVE;

        Sound_Effect(SFX_ATLANTEAN_EXPLODE, &centaur->pos, SPM_NORMAL);
    }
}
