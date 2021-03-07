#include "game/effects.h"
#include "game/game.h"
#include "game/option.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/output.h"
#include "config.h"
#include "util.h"

#define GAMMA_MODIFIER 8
#define MIN_GAMMA_LEVEL -127
#define MAX_GAMMA_LEVEL 127

// original name: do_inventory_options
void DoInventoryOptions(INVENTORY_ITEM* inv_item)
{
    switch (inv_item->object_number) {
    case O_PASSPORT_OPTION:
        DoPassportOption(inv_item);
        break;

    case O_MAP_OPTION:
        DoCompassOption(inv_item);
        break;

    case O_DETAIL_OPTION:
        DoDetailOption(inv_item);
        break;

    case O_SOUND_OPTION:
        DoSoundOption(inv_item);
        break;

    case O_CONTROL_OPTION:
        DoControlOption(inv_item);
        break;

    case O_GAMMA_OPTION:
        DoGammaOption(inv_item);
        break;

    case O_GUN_OPTION:
    case O_SHOTGUN_OPTION:
    case O_MAGNUM_OPTION:
    case O_UZI_OPTION:
    case O_EXPLOSIVE_OPTION:
    case O_MEDI_OPTION:
    case O_BIGMEDI_OPTION:
    case O_PUZZLE_OPTION1:
    case O_PUZZLE_OPTION2:
    case O_PUZZLE_OPTION3:
    case O_PUZZLE_OPTION4:
    case O_KEY_OPTION1:
    case O_KEY_OPTION2:
    case O_KEY_OPTION3:
    case O_KEY_OPTION4:
    case O_PICKUP_OPTION1:
    case O_PICKUP_OPTION2:
    case O_SCION_OPTION:
        InputDB |= IN_SELECT;
        break;

    case O_GUN_AMMO_OPTION:
    case O_SG_AMMO_OPTION:
    case O_MAG_AMMO_OPTION:
    case O_UZI_AMMO_OPTION:
        break;

    default:
        if (CHK_ANY(InputDB, (IN_DESELECT | IN_SELECT))) {
            inv_item->goal_frame = 0;
            inv_item->anim_direction = -1;
        }
        break;
    }
}

// original name: do_gamma_option
void DoGammaOption(INVENTORY_ITEM* inv_item)
{
    if (CHK_ANY(Input, IN_LEFT)) {
        IDelay = 1;
        IDCount = 10;
        OptionGammaLevel -= GAMMA_MODIFIER;
        if (OptionGammaLevel < MIN_GAMMA_LEVEL) {
            OptionGammaLevel = MIN_GAMMA_LEVEL;
        }
    }
    if (CHK_ANY(Input, IN_RIGHT)) {
        IDCount = 10;
        IDelay = 1;
        OptionGammaLevel += GAMMA_MODIFIER;
        if (OptionGammaLevel > MAX_GAMMA_LEVEL) {
            OptionGammaLevel = MAX_GAMMA_LEVEL;
        }
    }
    inv_item->sprlist = InvSprGammaList;
    InvSprGammaLevel[6].param1 = OptionGammaLevel / 2 + 63;
    // S_SetGamma(OptionGammaLevel);

    if (CHK_ANY(InputDB, IN_SELECT)) {
        inv_item->goal_frame = 0;
        inv_item->anim_direction = -1;
    }

    if (CHK_ANY(InputDB, IN_DESELECT)) {
        int32_t gamma = OptionGammaLevel - 64;
        if (gamma < -255) {
            gamma = -255;
        }
        if (InventoryMode != INV_TITLE_MODE) {
            // S_SetBackgroundGamma(gamma);
        }
    }
}

// original name: do_compass_option
void DoCompassOption(INVENTORY_ITEM* inv_item)
{
    if (CHK_ANY(InputDB, IN_DESELECT | IN_SELECT)) {
        inv_item->goal_frame = inv_item->frames_total - 1;
        inv_item->anim_direction = 1;
    }
}

void S_ShowControls()
{
#ifdef T1M_FEAT_UI
    int16_t centre = GetRenderWidthDownscaled() / 2;
#else
    int16_t centre = PhdWinWidth / 2;
#endif
    int16_t hpos;
    int16_t vpos;

    switch (HiRes) {
#ifndef T1M_FEAT_UI
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
#ifndef T1M_FEAT_UI
    case 0:
        for (int i = 0; i < 13; i++) {
            T_SetScale(CtrlTextA[i], PHD_180, PHD_ONE);
            T_SetScale(CtrlTextB[i], PHD_180, PHD_ONE);
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
#ifndef T1M_FEAT_UI
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
#ifndef T1M_FEAT_UI
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

// original name: Init_Requester
void InitRequester(REQUEST_INFO* req)
{
    req->heading = NULL;
    req->background = NULL;
    req->moreup = NULL;
    req->moredown = NULL;
    for (int i = 0; i < MAX_REQLINES; i++) {
        req->texts[i] = NULL;
    }
}

void T1MInjectGameOption()
{
    INJECT(0x0042D770, DoInventoryOptions);
    INJECT(0x0042F230, S_ShowControls);
}
