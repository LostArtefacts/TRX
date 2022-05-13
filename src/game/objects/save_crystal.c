#include "game/objects/save_crystal.h"

#include "game/control.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/pickup.h"
#include "game/savegame.h"
#include "game/settings.h"
#include "game/sound.h"
#include "global/vars.h"

static int16_t m_CrystalBounds[12] = {
    -256, +256, -100, +100, -256, +256, -10 * PHD_DEGREE, +10 * PHD_DEGREE,
    0,    0,    0,    0,
};

void SaveCrystal_Setup(OBJECT_INFO *obj)
{
    obj->initialise = SaveCrystal_Initialise;
    if (g_GameFlow.enable_save_crystals) {
        obj->control = SaveCrystal_Control;
        obj->collision = SaveCrystal_Collision;
        obj->save_flags = 1;
    }
}

void SaveCrystal_Initialise(int16_t item_num)
{
    if (g_GameFlow.enable_save_crystals) {
        Item_AddActive(item_num);
    } else {
        g_Items[item_num].status = IS_INVISIBLE;
    }
}

void SaveCrystal_Control(int16_t item_num)
{
    if (g_GameFlow.enable_save_crystals) {
        Item_Animate(&g_Items[item_num]);
    }
}

void SaveCrystal_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!g_Input.action || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity_status) {
        return;
    }

    if (lara_item->current_anim_state != LS_STOP) {
        return;
    }

    item->pos.y_rot = lara_item->pos.y_rot;
    item->pos.z_rot = 0;
    item->pos.x_rot = 0;
    if (!Lara_TestPosition(item, m_CrystalBounds)) {
        return;
    }

    int32_t return_val = Display_Inventory(INV_SAVE_CRYSTAL_MODE);
    if (return_val != GF_NOP) {
        item->status = IS_INVISIBLE;
        Item_RemoveDrawn(item_num);
        Savegame_Save(g_GameInfo.current_save_slot, &g_GameInfo);
        Settings_Write();
        Sound_Effect(SFX_LARA_OBJECT, NULL, SPM_ALWAYS);
    } else {
        item->status = IS_ACTIVE;
    }
}
