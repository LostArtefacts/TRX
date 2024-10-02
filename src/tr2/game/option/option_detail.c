#include "game/input.h"
#include "game/option/option.h"
#include "game/text.h"
#include "global/funcs.h"
#include "global/vars.h"

static void M_InitText(void);
static void M_ShutdownText(void);

static void M_InitText(void)
{
    g_DetailText[0] =
        Text_Create(0, 50, 0, g_GF_GameStrings[GF_S_GAME_DETAIL_LOW]);
    g_DetailText[1] =
        Text_Create(0, 25, 0, g_GF_GameStrings[GF_S_GAME_DETAIL_MEDIUM]);
    g_DetailText[2] =
        Text_Create(0, 0, 0, g_GF_GameStrings[GF_S_GAME_DETAIL_HIGH]);
    g_DetailText[3] = Text_Create(0, -32, 0, " ");
    g_DetailText[4] = Text_Create(
        0, -30, 0, g_GF_GameStrings[GF_S_GAME_DETAIL_SELECT_DETAIL]);
    Text_AddBackground(g_DetailText[4], 156, 0, 0, 0, 8, 0, 0, 0);
    Text_AddOutline(g_DetailText[4], 1, 4, 0, 0);
    Text_AddBackground(g_DetailText[g_DetailLevel], 148, 0, 0, 0, 8, 0, 0, 0);
    Text_AddOutline(g_DetailText[g_DetailLevel], 1, 4, 0, 0);
    Text_AddBackground(g_DetailText[3], 160, 107, 0, 0, 16, 0, 0, 0);
    Text_AddOutline(g_DetailText[3], 1, 15, 0, 0);

    for (int32_t i = 0; i < 5; i++) {
        Text_CentreH(g_DetailText[i], 1);
        Text_CentreV(g_DetailText[i], 1);
    }
}

static void M_ShutdownText(void)
{
    for (int32_t i = 0; i < 5; i++) {
        Text_Remove(g_DetailText[i]);
        g_DetailText[i] = NULL;
    }
}

void __cdecl Option_Detail(INVENTORY_ITEM *const item)
{
    if (g_DetailText[0] == NULL) {
        M_InitText();
    }

    if ((g_InputDB & IN_BACK) && g_DetailLevel > 0) {
        Text_RemoveOutline(g_DetailText[g_DetailLevel]);
        Text_RemoveBackground(g_DetailText[g_DetailLevel]);
        g_DetailLevel--;
        Text_AddOutline(g_DetailText[g_DetailLevel], 1, 4, 0, 0);
        Text_AddBackground(
            g_DetailText[g_DetailLevel], 148, 0, 0, 0, 8, 0, 0, 0);
    }
    if ((g_InputDB & IN_FORWARD) && g_DetailLevel < 2) {
        Text_RemoveOutline(g_DetailText[g_DetailLevel]);
        Text_RemoveBackground(g_DetailText[g_DetailLevel]);
        g_DetailLevel++;
        Text_AddOutline(g_DetailText[g_DetailLevel], 1, 4, 0, 0);
        Text_AddBackground(
            g_DetailText[g_DetailLevel], 148, 0, 0, 0, 8, 0, 0, 0);
    }

    switch (g_DetailLevel) {
    case 0:
        g_PerspectiveDistance = 0;
        break;
    case 1:
        g_PerspectiveDistance = 0x3000000;
        break;
    case 2:
        g_PerspectiveDistance = 0x14000000;
        break;
    }

    if (g_InputDB & (IN_SELECT | IN_DESELECT)) {
        M_ShutdownText();
    }
}

void Option_Detail_Shutdown(void)
{
    M_ShutdownText();
}
