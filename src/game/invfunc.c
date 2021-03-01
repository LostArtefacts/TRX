#include "game/health.h"
#include "game/inv.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/shed.h"
#include "util.h"

#define IT_NAME 0
#define IT_QTY 1

void InitColours()
{
    InvColours[IC_BLACK] = S_Colour(0, 0, 0);
    InvColours[IC_GREY] = S_Colour(64, 64, 64);
    InvColours[IC_WHITE] = S_Colour(255, 255, 255);
    InvColours[IC_RED] = S_Colour(255, 0, 0);
    InvColours[IC_ORANGE] = S_Colour(255, 128, 0);
    InvColours[IC_YELLOW] = S_Colour(255, 255, 0);
    InvColours[IC_DARKGREEN] = S_Colour(0, 128, 0);
    InvColours[IC_GREEN] = S_Colour(0, 255, 0);
    InvColours[IC_CYAN] = S_Colour(0, 255, 255);
    InvColours[IC_BLUE] = S_Colour(0, 0, 255);
    InvColours[IC_MAGENTA] = S_Colour(255, 0, 255);
}

void RingIsOpen(RING_INFO* ring)
{
    if (InventoryMode == INV_TITLE_MODE) {
        return;
    }

    if (!InvRingText) {
        switch (ring->type) {
        case RM_MAIN:
            InvRingText = T_Print(0, 26, 0, "INVENTORY");
            break;

        case RM_OPTION:
            if (InventoryMode == INV_DEATH_MODE) {
                InvRingText = T_Print(0, 26, 0, "GAME OVER");
            } else {
                InvRingText = T_Print(0, 26, 0, "OPTION");
            }
            break;

        case RM_KEYS:
            InvRingText = T_Print(0, 26, 0, "ITEMS");
            break;
        }

        T_CentreH(InvRingText, 1);
    }

    if (InventoryMode == INV_KEYS_MODE || InventoryMode == INV_DEATH_MODE) {
        return;
    }

    if (!InvUpArrow1) {
        if (ring->type == RM_OPTION
            || (ring->type == RM_MAIN && InvKeysObjects)) {
            InvUpArrow1 = T_Print(20, 28, 0, "[");
            InvUpArrow2 = T_Print(-20, 28, 0, "[");
            T_RightAlign(InvUpArrow2, 1);
        }
    }

    if (!InvDownArrow1) {
        if (ring->type == RM_MAIN || ring->type == RM_KEYS) {
            InvDownArrow1 = T_Print(20, -15, 0, "]");
            InvDownArrow2 = T_Print(-20, -15, 0, "]");
            T_BottomAlign(InvDownArrow1, 1);
            T_BottomAlign(InvDownArrow2, 1);
            T_RightAlign(InvDownArrow2, 1);
        }
    }
}

void RingIsNotOpen(RING_INFO* ring)
{
    if (!InvRingText) {
        return;
    }

    T_RemovePrint(InvRingText);
    InvRingText = NULL;

    if (InvUpArrow1) {
        T_RemovePrint(InvUpArrow1);
        T_RemovePrint(InvUpArrow2);
        InvUpArrow1 = NULL;
        InvUpArrow2 = NULL;
    }
    if (InvDownArrow1) {
        T_RemovePrint(InvDownArrow1);
        T_RemovePrint(InvDownArrow2);
        InvDownArrow1 = NULL;
        InvDownArrow2 = NULL;
    }
}

void RingNotActive(INVENTORY_ITEM* inv_item)
{
    if (!InvItemText[IT_NAME]) {
        switch (inv_item->object_number) {
        case O_PUZZLE_OPTION1:
            InvItemText[IT_NAME] =
                T_Print(0, -16, 0, Puzzle1Strings[CurrentLevel]);
            break;

        case O_PUZZLE_OPTION2:
            InvItemText[IT_NAME] =
                T_Print(0, -16, 0, Puzzle2Strings[CurrentLevel]);
            break;

        case O_PUZZLE_OPTION3:
            InvItemText[IT_NAME] =
                T_Print(0, -16, 0, Puzzle3Strings[CurrentLevel]);
            break;

        case O_PUZZLE_OPTION4:
            InvItemText[IT_NAME] =
                T_Print(0, -16, 0, Puzzle4Strings[CurrentLevel]);
            break;

        case O_KEY_OPTION1:
            InvItemText[IT_NAME] =
                T_Print(0, -16, 0, Key1Strings[CurrentLevel]);
            break;

        case O_KEY_OPTION2:
            InvItemText[IT_NAME] =
                T_Print(0, -16, 0, Key2Strings[CurrentLevel]);
            break;

        case O_KEY_OPTION3:
            InvItemText[IT_NAME] =
                T_Print(0, -16, 0, Key3Strings[CurrentLevel]);
            break;

        case O_KEY_OPTION4:
            InvItemText[IT_NAME] =
                T_Print(0, -16, 0, Key4Strings[CurrentLevel]);
            break;

        case O_PICKUP_OPTION1:
            InvItemText[IT_NAME] =
                T_Print(0, -16, 0, Pickup1Strings[CurrentLevel]);
            break;

        case O_PICKUP_OPTION2:
            InvItemText[IT_NAME] =
                T_Print(0, -16, 0, Pickup2Strings[CurrentLevel]);
            break;

        case O_PASSPORT_OPTION:
            break;

        default:
            // XXX: terrible hack
            InvItemText[IT_NAME] = T_Print(0, -16, 0, (char*)inv_item->item_id);
            break;
        }

        if (InvItemText[IT_NAME]) {
            T_BottomAlign(InvItemText[IT_NAME], 1);
            T_CentreH(InvItemText[IT_NAME], 1);
        }
    }

    char temp_text[64];
    int32_t qty = Inv_RequestItem(inv_item->object_number);

    switch (inv_item->object_number) {
    case O_SHOTGUN_OPTION:
        if (!InvItemText[IT_QTY] && !SaveGame[0].bonus_flag) {
            sprintf(temp_text, "%5d A", Lara.shotgun.ammo / SHOTGUN_AMMO_CLIP);
            MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = T_Print(64, -56, 0, temp_text);
            T_BottomAlign(InvItemText[IT_QTY], 1);
            T_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_MAGNUM_OPTION:
        if (!InvItemText[IT_QTY] && !SaveGame[0].bonus_flag) {
            sprintf(temp_text, "%5d B", Lara.magnums.ammo);
            MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = T_Print(64, -56, 0, temp_text);
            T_BottomAlign(InvItemText[IT_QTY], 1);
            T_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_UZI_OPTION:
        if (!InvItemText[IT_QTY] && !SaveGame[0].bonus_flag) {
            sprintf(temp_text, "%5d C", Lara.uzis.ammo);
            MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = T_Print(64, -56, 0, temp_text);
            T_BottomAlign(InvItemText[IT_QTY], 1);
            T_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_SG_AMMO_OPTION:
        if (!InvItemText[IT_QTY]) {
            sprintf(temp_text, "%d", qty * NUM_SG_SHELLS);
            MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = T_Print(64, -56, 0, temp_text);
            T_BottomAlign(InvItemText[IT_QTY], 1);
            T_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_MAG_AMMO_OPTION:
        if (!InvItemText[IT_QTY]) {
            sprintf(temp_text, "%d", Inv_RequestItem(O_MAG_AMMO_OPTION) * 2);
            MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = T_Print(64, -56, 0, temp_text);
            T_BottomAlign(InvItemText[IT_QTY], 1);
            T_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_UZI_AMMO_OPTION:
        if (!InvItemText[IT_QTY]) {
            sprintf(temp_text, "%d", Inv_RequestItem(O_UZI_AMMO_OPTION) * 2);
            MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = T_Print(64, -56, 0, temp_text);
            T_BottomAlign(InvItemText[IT_QTY], 1);
            T_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_MEDI_OPTION:
        HealthBarTimer = 40;
        DrawHealthBar();
        if (!InvItemText[IT_QTY] && qty > 1) {
            sprintf(temp_text, "%d", qty);
            MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = T_Print(64, -56, 0, temp_text);
            T_BottomAlign(InvItemText[IT_QTY], 1);
            T_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_BIGMEDI_OPTION:
        HealthBarTimer = 40;
        DrawHealthBar();
        if (!InvItemText[IT_QTY] && qty > 1) {
            sprintf(temp_text, "%d", qty);
            MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = T_Print(64, -56, 0, temp_text);
            T_BottomAlign(InvItemText[IT_QTY], 1);
            T_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_KEY_OPTION1:
    case O_KEY_OPTION2:
    case O_KEY_OPTION3:
    case O_KEY_OPTION4:
    case O_LEADBAR_OPTION:
    case O_PICKUP_OPTION1:
    case O_PICKUP_OPTION2:
    case O_PUZZLE_OPTION1:
    case O_PUZZLE_OPTION2:
    case O_PUZZLE_OPTION3:
    case O_PUZZLE_OPTION4:
    case O_SCION_OPTION:
        if (!InvItemText[IT_QTY] && qty > 1) {
            sprintf(temp_text, "%d", qty);
            MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = T_Print(64, -56, 0, temp_text);
            T_BottomAlign(InvItemText[IT_QTY], 1);
            T_CentreH(InvItemText[IT_QTY], 1);
        }
        break;
    }
}

void RingActive()
{
    if (InvItemText[IT_NAME]) {
        T_RemovePrint(InvItemText[IT_NAME]);
        InvItemText[IT_NAME] = NULL;
    }
    if (InvItemText[IT_QTY]) {
        T_RemovePrint(InvItemText[IT_QTY]);
        InvItemText[IT_QTY] = NULL;
    }
}

void T1MInjectGameInvFunc()
{
    INJECT(0x0041FEF0, InitColours);
    INJECT(0x00420000, RingIsOpen);
    INJECT(0x00420150, RingIsNotOpen);
    INJECT(0x004201D0, RingNotActive);
    INJECT(0x00420980, RingActive);
}
