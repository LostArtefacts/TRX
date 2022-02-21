#include "game/objects/savegame_crystal.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/objects/pickup.h"
#include "game/savegame.h"
#include "game/settings.h"
#include "game/sound.h"
#include "global/vars.h"

void SetupSavegameCrystal(OBJECT_INFO *obj)
{
    obj->initialise = InitialiseSavegameItem;
    if (g_GameFlow.enable_save_crystals) {
        obj->control = ControlSavegameItem;
        obj->collision = PickUpSavegameCollision;
        obj->save_flags = 1;
    }
}

void InitialiseSavegameItem(int16_t item_num)
{
    if (g_GameFlow.enable_save_crystals) {
        AddActiveItem(item_num);
    } else {
        g_Items[item_num].status = IS_INVISIBLE;
    }
}

void ControlSavegameItem(int16_t item_num)
{
    if (g_GameFlow.enable_save_crystals) {
        AnimateItem(&g_Items[item_num]);
    }
}

void PickUpSavegameCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!g_Input.action || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity_status) {
        return;
    }

    if (lara_item->current_anim_state != AS_STOP) {
        return;
    }

    item->pos.y_rot = lara_item->pos.y_rot;
    item->pos.z_rot = 0;
    item->pos.x_rot = 0;
    if (!TestLaraPosition(g_PickUpBounds, item, lara_item)) {
        return;
    }

    int32_t return_val = Display_Inventory(INV_SAVE_CRYSTAL_MODE);
    if (return_val != GF_NOP) {
        item->status = IS_INVISIBLE;
        RemoveDrawnItem(item_num);
        Savegame_Save(g_InvExtraData[1], &g_GameInfo);
        Settings_Write();
        Sound_Effect(SFX_LARA_OBJECT, NULL, SPM_ALWAYS);
    } else {
        item->status = IS_ACTIVE;
    }
}
