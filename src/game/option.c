#include "game/option.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/output.h"
#include "mod.h"
#include "util.h"

void S_ShowControls()
{
#ifdef TOMB1M_FEAT_UI
    int16_t centre = GetRenderWidthDownscaled() / 2;
#else
    int16_t centre = PhdWinWidth / 2;
#endif
    int16_t hpos;
    int16_t vpos;

    switch (HiRes) {
#ifndef TOMB1M_FEAT_UI
    case 0:
        ControlText[1] = T_Print(0, -55, 0, " ");
        break;
    case 1:
        ControlText[1] = T_Print(0, -58, 0, " ");
        break;
    case 3:
        ControlText[1] = T_Print(0, -62, 0, " ");
        break;
#endif
    default:
        ControlText[1] = T_Print(0, -60, 0, " ");
        break;
    }
    T_CentreH(ControlText[1], 1);
    T_CentreV(ControlText[1], 1);

    switch (HiRes) {
#ifndef TOMB1M_FEAT_UI
    case 0:
        for (int i = 0; i < 13; i++) {
            T_SetScale(CtrlTextA[i], PHD_ONE / 2, PHD_ONE);
            T_SetScale(CtrlTextB[i], PHD_ONE / 2, PHD_ONE);
        }
        hpos = 300;
        vpos = 140;
        break;

    case 1:
        hpos = 360;
        vpos = 145;
        break;

    case 3:
        hpos = 440;
        vpos = 155;
        break;

#endif
    default:
        hpos = 420;
        vpos = 150;
        break;
    }
    T_AddBackground(ControlText[1], hpos, vpos, 0, 0, 48, 0, 0, 0);

    if (!CtrlTextB[0]) {
        int16_t* layout = Layout[IConfig];

        switch (HiRes) {
#ifndef TOMB1M_FEAT_UI
        case 0:
            hpos = centre - 140;
            break;

        case 1:
            hpos = centre - 170;
            break;

        case 3:
            hpos = centre - 230;
            break;
#endif

        default:
            hpos = centre - 200;
            break;
        }

        CtrlTextB[0] = T_Print(hpos, -25, 0, ScanCodeNames[layout[0]]);
        CtrlTextB[1] = T_Print(hpos, -10, 0, ScanCodeNames[layout[1]]);
        CtrlTextB[2] = T_Print(hpos, 5, 0, ScanCodeNames[layout[2]]);
        CtrlTextB[3] = T_Print(hpos, 20, 0, ScanCodeNames[layout[3]]);
        CtrlTextB[4] = T_Print(hpos, 35, 0, ScanCodeNames[layout[4]]);
        CtrlTextB[5] = T_Print(hpos, 50, 0, ScanCodeNames[layout[5]]);
        CtrlTextB[6] = T_Print(hpos, 65, 0, ScanCodeNames[layout[6]]);
        CtrlTextB[7] = T_Print(centre + 20, -25, 0, ScanCodeNames[layout[7]]);
        CtrlTextB[8] = T_Print(centre + 20, -10, 0, ScanCodeNames[layout[8]]);
        CtrlTextB[9] = T_Print(centre + 20, 5, 0, ScanCodeNames[layout[9]]);
        CtrlTextB[10] = T_Print(centre + 20, 20, 0, ScanCodeNames[layout[10]]);
        CtrlTextB[11] = T_Print(centre + 20, 35, 0, ScanCodeNames[layout[11]]);
        CtrlTextB[12] = T_Print(centre + 20, 65, 0, ScanCodeNames[layout[12]]);

        for (int i = 0; i < 13; i++) {
            T_CentreV(CtrlTextB[i], 1);
        }
        KeyChange = 0;
    }

    if (!CtrlTextA[0]) {
        switch (HiRes) {
#ifndef TOMB1M_FEAT_UI
        case 0:
            hpos = centre - 70;
            break;

        case 1:
            hpos = centre - 100;
            break;

        case 3:
            hpos = centre - 150;
            break;
#endif

        default:
            hpos = centre - 130;
            break;
        }

        CtrlTextA[0] = T_Print(hpos, -25, 0, "Run");
        CtrlTextA[1] = T_Print(hpos, -10, 0, "Back");
        CtrlTextA[2] = T_Print(hpos, 5, 0, "Left");
        CtrlTextA[3] = T_Print(hpos, 20, 0, "Right");
        CtrlTextA[4] = T_Print(hpos, 35, 0, "Step Left");
        CtrlTextA[5] = T_Print(hpos, 50, 0, "Step Right");
        CtrlTextA[6] = T_Print(hpos, 65, 0, "Walk");
        CtrlTextA[7] = T_Print(centre + 90, -25, 0, "Jump");
        CtrlTextA[8] = T_Print(centre + 90, -10, 0, "Action");
        CtrlTextA[9] = T_Print(centre + 90, 5, 0, "Draw Weapon");
        CtrlTextA[10] = T_Print(centre + 90, 20, 0, "Look");
        CtrlTextA[11] = T_Print(centre + 90, 35, 0, "Roll");
        CtrlTextA[12] = T_Print(centre + 90, 65, 0, "Inventory");

        for (int i = 0; i < 13; i++) {
            T_CentreV(CtrlTextA[i], 1);
        }
    }
}

void Tomb1MInjectGameOption()
{
    INJECT(0x0042F230, S_ShowControls);
}
