#include "inv.h"

#include "global/vars.h"

int16_t InvKeysCurrent;
int16_t InvKeysObjects;
int16_t InvKeysQtys[24] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

INVENTORY_ITEM *InvKeysList[23] = {
    &InvItemLeadBar,
    &InvItemPuzzle1,
    &InvItemPuzzle2,
    &InvItemPuzzle3,
    &InvItemPuzzle4,
    &InvItemKey1,
    &InvItemKey2,
    &InvItemKey3,
    &InvItemKey4,
    &InvItemPickup1,
    &InvItemPickup2,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

int16_t InvMainCurrent;
int16_t InvMainObjects = 1;
int16_t InvMainQtys[24] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

INVENTORY_ITEM *InvMainList[23] = {
    &InvItemCompass,
    &InvItemPistols,
    &InvItemShotgun,
    &InvItemMagnum,
    &InvItemUzi,
    &InvItemGrenade,
    &InvItemBigMedi,
    &InvItemMedi,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

int16_t InvOptionCurrent;
int16_t InvOptionObjects = 5;
INVENTORY_ITEM *InvOptionList[5] = {
    &InvItemGame,    &InvItemControls,  &InvItemSound,
    &InvItemDetails, &InvItemLarasHome,
};

INVENTORY_ITEM InvItemCompass = {
    .string = "Compass",
    .object_number = O_MAP_OPTION,
    .frames_total = 25,
    .current_frame = 0,
    .goal_frame = 0,
    .open_frame = 10,
    .anim_direction = 1,
    .anim_speed = 1,
    .anim_count = 0,
    .pt_xrot_sel = 4352,
    .pt_xrot = 0,
    .x_rot_sel = -8192,
    .x_rot = 0,
    .y_rot_sel = 0,
    .y_rot = 0,
    .ytrans_sel = 0,
    .ytrans = 0,
    .ztrans_sel = 456,
    .ztrans = 0,
    .which_meshes = 0x5,
    .drawn_meshes = 0x5,
    .inv_pos = 0,
    .sprlist = NULL,
};
