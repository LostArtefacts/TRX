#include "game/objects/savegame_crystal.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/game.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/objects/pickup.h"
#include "game/savegame.h"
#include "game/settings.h"
#include "game/sound.h"
#include "game/vars.h"

void SetupSaveGameCrystal(OBJECT_INFO *obj)
{
    obj->initialise = InitialiseSaveGameItem;
    if (GF.enable_save_crystals) {
        obj->control = ControlSaveGameItem;
        obj->collision = PickUpSaveGameCollision;
        obj->save_flags = 1;
    }
}

void InitialiseSaveGameItem(int16_t item_num)
{
    if (GF.enable_save_crystals) {
        AddActiveItem(item_num);
    } else {
        Items[item_num].status = IS_INVISIBLE;
    }
}

void ControlSaveGameItem(int16_t item_num)
{
    if (GF.enable_save_crystals) {
        AnimateItem(&Items[item_num]);
    }
}

void PickUpSaveGameCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];

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

    int32_t return_val = Display_Inventory(INV_SAVE_CRYSTAL_MODE);
    if (return_val != GF_NOP) {
        item->status = IS_INVISIBLE;
        RemoveDrawnItem(item_num);
        CreateSaveGameInfo();
        S_SaveGame(&SaveGame, InvExtraData[1]);
        S_WriteUserSettings();
        SoundEffect(SFX_LARA_OBJECT, NULL, SPM_ALWAYS);
    } else {
        item->status = IS_ACTIVE;
    }
}
