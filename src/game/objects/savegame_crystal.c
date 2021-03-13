#include "game/objects/savegame_crystal.h"
#include "game/vars.h"

void SetupSaveGameCrystal(OBJECT_INFO *obj)
{
    obj->initialise = InitialiseSaveGameItem;
#ifdef T1M_FEAT_SAVE_CRYSTALS
    obj->control = ControlSaveGameItem;
    obj->collision = PickUpSaveGameCollision;
    obj->save_flags = 1;
#endif
}

void InitialiseSaveGameItem(int16_t item_num)
{
#ifdef T1M_FEAT_SAVE_CRYSTALS
    AddActiveItem(item_num);
#else
    Items[item_num].status = IS_INVISIBLE;
#endif
}

void ControlSaveGameItem(int16_t item_num)
{
#ifdef T1M_FEAT_SAVE_CRYSTALS
    AnimateItem(&Items[item_num]);
#endif
}

void PickUpSaveGameCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
#ifdef T1M_FEAT_SAVE_CRYSTALS
    ITEM_INFO *item = &Items[item_num];
    ObjectCollision(item_num, lara_item, coll);

    if (!CHK_ANY(Input, IN_ACTION) || Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity_status) {
        return;
    }

    if (lara_item->current_anim_state != AS_STOP) {
        return;
    }

    item->pos.y_rot = lara_item->pos.y_rot;
    item->pos.z_rot = 0;
    item->pos.x_rot = 0;
    if (!TestLaraPosition(PickUpBounds, item, lara_item)) {
        return;
    }

    item->status = IS_INVISIBLE;
    CreateSaveGameInfo();
    if (S_SaveGame(&SaveGame, -1)) {
        item->status = IS_INVISIBLE;
        RemoveDrawnItem(item_num);
    } else {
        item->status = IS_ACTIVE;
    }
#endif
}
