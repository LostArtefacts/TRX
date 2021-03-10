#include "game/gameflow.h"
#include "game/vars.h"
#include "specific/file.h"
#include <stdio.h>
#include <string.h>

static void SetRequesterHeading(REQUEST_INFO *req, char *text)
{
    req->heading_text = text;
}

static void
SetRequesterItemText(REQUEST_INFO *req, int8_t index, const char *text)
{
    strncpy(
        &req->item_texts[index * req->item_text_len], text, req->item_text_len);
}

static int8_t S_LoadGameFlow(const char *file_name)
{
    return 0;
}

int8_t GF_LoadScriptFile(const char *file_name)
{
    int8_t result = S_LoadGameFlow(file_name);

    InvItemMedi.string = GF_GameStringTable[GSI_INV_ITEM_MEDI],
    InvItemBigMedi.string = GF_GameStringTable[GSI_INV_ITEM_BIG_MEDI],

    InvItemPuzzle1.string = GF_GameStringTable[GSI_INV_ITEM_PUZZLE1],
    InvItemPuzzle2.string = GF_GameStringTable[GSI_INV_ITEM_PUZZLE2],
    InvItemPuzzle3.string = GF_GameStringTable[GSI_INV_ITEM_PUZZLE3],
    InvItemPuzzle4.string = GF_GameStringTable[GSI_INV_ITEM_PUZZLE4],

    InvItemKey1.string = GF_GameStringTable[GSI_INV_ITEM_KEY1],
    InvItemKey2.string = GF_GameStringTable[GSI_INV_ITEM_KEY2],
    InvItemKey3.string = GF_GameStringTable[GSI_INV_ITEM_KEY3],
    InvItemKey4.string = GF_GameStringTable[GSI_INV_ITEM_KEY4],

    InvItemPickup1.string = GF_GameStringTable[GSI_INV_ITEM_PICKUP1],
    InvItemPickup2.string = GF_GameStringTable[GSI_INV_ITEM_PICKUP2],
    InvItemLeadBar.string = GF_GameStringTable[GSI_INV_ITEM_LEADBAR],
    InvItemScion.string = GF_GameStringTable[GSI_INV_ITEM_SCION],

    InvItemPistols.string = GF_GameStringTable[GSI_INV_ITEM_PISTOLS],
    InvItemShotgun.string = GF_GameStringTable[GSI_INV_ITEM_SHOTGUN],
    InvItemMagnum.string = GF_GameStringTable[GSI_INV_ITEM_MAGNUM],
    InvItemUzi.string = GF_GameStringTable[GSI_INV_ITEM_UZI],
    InvItemGrenade.string = GF_GameStringTable[GSI_INV_ITEM_GRENADE],

    InvItemPistolAmmo.string = GF_GameStringTable[GSI_INV_ITEM_PISTOL_AMMO],
    InvItemShotgunAmmo.string = GF_GameStringTable[GSI_INV_ITEM_SHOTGUN_AMMO],
    InvItemMagnumAmmo.string = GF_GameStringTable[GSI_INV_ITEM_MAGNUM_AMMO],
    InvItemUziAmmo.string = GF_GameStringTable[GSI_INV_ITEM_UZI_AMMO],

    InvItemCompass.string = GF_GameStringTable[GSI_INV_ITEM_COMPASS],
    InvItemGame.string = GF_GameStringTable[GSI_INV_ITEM_GAME];
    InvItemDetails.string = GF_GameStringTable[GSI_INV_ITEM_DETAILS];
    InvItemSound.string = GF_GameStringTable[GSI_INV_ITEM_SOUND];
    InvItemControls.string = GF_GameStringTable[GSI_INV_ITEM_CONTROLS];
    InvItemGamma.string = GF_GameStringTable[GSI_INV_ITEM_GAMMA];
    InvItemLarasHome.string = GF_GameStringTable[GSI_INV_ITEM_LARAS_HOME];

    SetRequesterHeading(
        &LoadSaveGameRequester, GF_GameStringTable[GSI_PASSPORT_SELECT_LEVEL]);

#ifdef T1M_FEAT_GAMEPLAY
    SetRequesterHeading(
        &NewGameRequester, GF_GameStringTable[GSI_PASSPORT_SELECT_MODE]);
    SetRequesterItemText(
        &NewGameRequester, 0, GF_GameStringTable[GSI_PASSPORT_MODE_NEW_GAME]);
    SetRequesterItemText(
        &NewGameRequester, 1,
        GF_GameStringTable[GSI_PASSPORT_MODE_NEW_GAME_PLUS]);
#endif

    return result;
}
