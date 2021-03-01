#include "game/text.h"
#include "game/vars.h"
#include "specific/shed.h"
#include "util.h"

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

void T1MInjectGameInvFunc()
{
    INJECT(0x0041FEF0, InitColours);
    INJECT(0x00420000, RingIsOpen);
    INJECT(0x00420150, RingIsNotOpen);
}
