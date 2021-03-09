#include "game/effects.h"
#include "game/game.h"
#include "game/option.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/input.h"
#include "specific/output.h"
#include "specific/shed.h"
#include "specific/sndpc.h"
#include "config.h"
#include "util.h"
#include <dinput.h>

#define BOX_PADDING 10
#define BOX_BORDER 1
#define GAMMA_MODIFIER 8
#define MIN_GAMMA_LEVEL -127
#define MAX_GAMMA_LEVEL 127
#define PASSPORT_2FRONT IN_LEFT
#define PASSPORT_2BACK IN_RIGHT

static TEXTSTRING *PassportText = NULL;
static TEXTSTRING *DetailText[5] = { NULL, NULL, NULL, NULL, NULL };
static TEXTSTRING *SoundText[4] = { NULL, NULL, NULL, NULL };
static int32_t PassportMode = 0;
static int32_t SelectKey = 0;

#ifdef T1M_FEAT_GAMEPLAY
static char NewGameStrings[][20] = {
    { "New Game" },
    { "New Game+" },
};

static REQUEST_INFO NewGameRequester = {
    2, // items
    0, // requested
    2, // vis_lines
    0, // line_offset
    0, // line_old_offset
    162, // pix_width
    TEXT_HEIGHT + 7, // line_height
    0, // x
    -30, // y
    0, // z
    RIF_FIXED_HEIGHT,
    "Select Mode", // heading_text
    &NewGameStrings[0][0], // item_texts
    20, // item_text_len
};
#endif

static char LoadGameStrings[MAX_SAVE_SLOTS][MAX_LEVEL_NAME_LENGTH];
REQUEST_INFO LoadGameRequester = {
    1,
    0,
    5,
    0,
    0,
    272,
    TEXT_HEIGHT + 7,
    0,
    -32,
    0,
    0,
    "Select Level",
    &LoadGameStrings[0][0],
    MAX_LEVEL_NAME_LENGTH,
};

// original name: do_inventory_options
void DoInventoryOptions(INVENTORY_ITEM *inv_item)
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

void DoPassportOption(INVENTORY_ITEM *inv_item)
{
    T_RemovePrint(InvItemText[0]);
    InvItemText[IT_NAME] = NULL;

    int16_t page = (inv_item->goal_frame - inv_item->open_frame) / 5;
    if ((inv_item->goal_frame - inv_item->open_frame) % 5) {
        page = -1;
    }

    if (InventoryMode == INV_LOAD_MODE || InventoryMode == INV_SAVE_MODE) {
        InputDB &= ~(PASSPORT_2FRONT | PASSPORT_2BACK);
    }

    switch (page) {
    case 0:
        if (PassportMode == 1) {
            int32_t select = DisplayRequester(&LoadGameRequester);
            if (select) {
                if (select > 0) {
                    InventoryExtraData[1] = select - 1;
                } else if (
                    InventoryMode != INV_SAVE_MODE
                    && InventoryMode != INV_LOAD_MODE) {
                    Input = 0;
                    InputDB = 0;
                }
                PassportMode = 0;
            } else {
                Input = 0;
                InputDB = 0;
            }
        } else if (PassportMode == 0) {
            if (!SavedGamesCount || InventoryMode == INV_SAVE_MODE) {
                InputDB = PASSPORT_2BACK;
            } else {
                if (!PassportText) {
                    PassportText = T_Print(0, -16, 0, "Load Game");
                    T_BottomAlign(PassportText, 1);
                    T_CentreH(PassportText, 1);
                }
                if (CHK_ANY(InputDB, IN_SELECT)
                    || InventoryMode == INV_LOAD_MODE) {
                    T_RemovePrint(InvRingText);
                    InvRingText = NULL;
                    T_RemovePrint(InvItemText[IT_NAME]);
                    InvItemText[IT_NAME] = NULL;
                    GetSavedGamesList(&LoadGameRequester);
                    InitRequester(&LoadGameRequester);
                    PassportMode = 1;
                    Input = 0;
                    InputDB = 0;
                }
            }
        }
        break;

    case 1:
#ifdef T1M_FEAT_GAMEPLAY
        if (PassportMode == 2) {
            int32_t select = DisplayRequester(&NewGameRequester);
            if (select) {
                if (select > 0) {
                    InventoryExtraData[1] = select - 1;
                } else if (InventoryMode != INV_GAME_MODE) {
                    Input = 0;
                    InputDB = 0;
                }
                PassportMode = 0;
            } else {
                Input = 0;
                InputDB = 0;
            }
        } else
#endif
            if (PassportMode == 1) {
            int32_t select = DisplayRequester(&LoadGameRequester);
            if (select) {
                if (select > 0) {
                    PassportMode = 0;
                    InventoryExtraData[1] = select - 1;
                } else {
                    if (InventoryMode != INV_SAVE_MODE
                        && InventoryMode != INV_LOAD_MODE) {
                        Input = 0;
                        InputDB = 0;
                    }
                    PassportMode = 0;
                }
            } else {
                Input = 0;
                InputDB = 0;
            }
        } else if (PassportMode == 0) {
            if (InventoryMode == INV_DEATH_MODE) {
                InputDB = inv_item->anim_direction == -1 ? PASSPORT_2FRONT
                                                         : PASSPORT_2BACK;
            }
            if (!PassportText) {
                if (InventoryMode == INV_TITLE_MODE || !CurrentLevel) {
                    PassportText = T_Print(0, -16, 0, "New Game");
                } else {
                    PassportText = T_Print(0, -16, 0, "Save Game");
                }
                T_BottomAlign(PassportText, 1);
                T_CentreH(PassportText, 1);
            }
            if (CHK_ANY(InputDB, IN_SELECT) || InventoryMode == INV_SAVE_MODE) {
                if (InventoryMode == INV_TITLE_MODE || !CurrentLevel) {
#ifdef T1M_FEAT_GAMEPLAY
                    T_RemovePrint(InvRingText);
                    InvRingText = NULL;
                    T_RemovePrint(InvItemText[IT_NAME]);
                    InvItemText[IT_NAME] = NULL;
                    InitRequester(&NewGameRequester);
                    PassportMode = 2;
                    Input = 0;
                    InputDB = 0;
#else
                    InventoryExtraData[1] = CurrentLevel;
#endif
                } else {
                    T_RemovePrint(InvRingText);
                    InvRingText = NULL;
                    T_RemovePrint(InvItemText[IT_NAME]);
                    InvItemText[IT_NAME] = NULL;
                    GetSavedGamesList(&LoadGameRequester);
                    InitRequester(&LoadGameRequester);
                    PassportMode = 1;
                    Input = 0;
                    InputDB = 0;
                }
            }
        }
        break;

    case 2:
        if (!PassportText) {
            if (InventoryMode == INV_TITLE_MODE) {
                PassportText = T_Print(0, -16, 0, "Exit Game");
            } else {
                PassportText = T_Print(0, -16, 0, "Exit to Title");
            }
            T_BottomAlign(PassportText, 1);
            T_CentreH(PassportText, 1);
        }
        break;
    }

    if (CHK_ANY(InputDB, PASSPORT_2FRONT)
        && (InventoryMode != INV_DEATH_MODE || SavedGamesCount)) {
        inv_item->anim_direction = -1;
        inv_item->goal_frame -= 5;

        Input = 0;
        InputDB = 0;

        if (!SavedGamesCount) {
            if (inv_item->goal_frame < inv_item->open_frame + 5) {
                inv_item->goal_frame = inv_item->open_frame + 5;
            } else if (PassportText) {
                T_RemovePrint(PassportText);
                PassportText = NULL;
            }
        } else {
            if (inv_item->goal_frame < inv_item->open_frame) {
                inv_item->goal_frame = inv_item->open_frame;
            } else {
                SoundEffect(115, NULL, SFX_ALWAYS);
                if (PassportText) {
                    T_RemovePrint(PassportText);
                    PassportText = NULL;
                }
            }
        }
    }

    if (CHK_ANY(InputDB, PASSPORT_2BACK)) {
        inv_item->goal_frame += 5;
        inv_item->anim_direction = 1;

        Input = 0;
        InputDB = 0;

        if (inv_item->goal_frame > inv_item->frames_total - 6) {
            inv_item->goal_frame = inv_item->frames_total - 6;
        } else {
            SoundEffect(115, NULL, SFX_ALWAYS);
            if (PassportText) {
                T_RemovePrint(PassportText);
                PassportText = NULL;
            }
        }
    }

    if (CHK_ANY(InputDB, IN_DESELECT)) {
        if (InventoryMode == INV_DEATH_MODE) {
            Input = 0;
            InputDB = 0;
        } else {
            if (page == 2) {
                inv_item->anim_direction = 1;
                inv_item->goal_frame = inv_item->frames_total - 1;
            } else {
                inv_item->goal_frame = 0;
                inv_item->anim_direction = -1;
            }
            if (PassportText) {
                T_RemovePrint(PassportText);
                PassportText = NULL;
            }
        }
    }

    if (CHK_ANY(InputDB, IN_SELECT)) {
        InventoryExtraData[0] = page;
        if (page == 2) {
            inv_item->anim_direction = 1;
            inv_item->goal_frame = inv_item->frames_total - 1;
        } else {
            inv_item->goal_frame = 0;
            inv_item->anim_direction = -1;
        }
        if (PassportText) {
            T_RemovePrint(PassportText);
            PassportText = NULL;
        }
    }
}

// original name: do_gamma_option
void DoGammaOption(INVENTORY_ITEM *inv_item)
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

// original name: do_detail_option?
void DoDetailOptionHW(INVENTORY_ITEM *inv_item)
{
    static int32_t current_row = 0;
    static int32_t max_row = 0;
    char buf[256];

    if (!DetailText[0]) {
        sprintf(
            buf, "Perspective     %s",
            AppSettings & ASF_PERSPECTIVE ? "On" : "Off");
        DetailText[0] = T_Print(0, 0, 0, buf);
        sprintf(
            buf, "Bilinear        %s",
            AppSettings & ASF_BILINEAR ? "On" : "Off");
        DetailText[1] = T_Print(0, 25, 0, buf);
        if (dword_45B940) {
            DetailText[2] = T_Print(0, 50, 0, " ");
            max_row = 1;
        } else {
            const char *tmp;
            switch (GameHiRes) {
            case 0:
                tmp = "320x200";
                break;
            case 1:
                tmp = "512x384";
                break;
            case 3:
                tmp = "800x600";
                break;
            default:
                tmp = "640x480";
                break;
            }
            sprintf(buf, "Game Video Mode %s", tmp);
            DetailText[2] = T_Print(0, 50, 0, buf);
            max_row = 2;
        }

        DetailText[3] = T_Print(0, -32, 0, " ");
        DetailText[4] = T_Print(0, -30, 0, "Select Detail");

        if (current_row > max_row) {
            current_row = max_row;
        }

        T_AddBackground(DetailText[4], 276, 0, 0, 0, 8, IC_BLACK, NULL, 0);
        T_AddOutline(DetailText[4], 1, IC_ORANGE, NULL, 0);
        T_AddBackground(
            DetailText[current_row], 268, 0, 0, 0, 8, IC_BLACK, NULL, 0);
        T_AddOutline(DetailText[current_row], 1, IC_ORANGE, NULL, 0);
        T_AddBackground(DetailText[3], 280, 122, 0, 0, 16, IC_BLACK, NULL, 0);
        T_AddOutline(DetailText[3], 1, IC_BLUE, NULL, 0);

        for (int i = 0; i < 5; i++) {
            T_CentreH(DetailText[i], 1);
            T_CentreV(DetailText[i], 1);
        }
    }

    if (CHK_ANY(InputDB, IN_FORWARD) && current_row > 0) {
        T_RemoveOutline(DetailText[current_row]);
        T_RemoveBackground(DetailText[current_row]);
        current_row--;
        T_AddOutline(DetailText[current_row], 1, IC_ORANGE, NULL, 0);
        T_AddBackground(
            DetailText[current_row], 268, 0, 0, 0, 8, IC_BLACK, NULL, 0);
    }

    if (CHK_ANY(InputDB, IN_BACK) && current_row < max_row) {
        T_RemoveOutline(DetailText[current_row]);
        T_RemoveBackground(DetailText[current_row]);
        current_row++;
        T_AddOutline(DetailText[current_row], 1, IC_ORANGE, NULL, 0);
        T_AddBackground(
            DetailText[current_row], 268, 0, 0, 0, 8, IC_BLACK, NULL, 0);
    }

    int8_t reset = 0;

    if (CHK_ANY(InputDB, IN_RIGHT)) {
        switch (current_row) {
        case 0:
            if (!(AppSettings & ASF_PERSPECTIVE)) {
                AppSettings |= ASF_PERSPECTIVE;
                reset = 1;
            }
            break;
        case 1:
            if (!(AppSettings & ASF_BILINEAR)) {
                AppSettings |= ASF_BILINEAR;
                reset = 1;
            }
            break;
        case 2:
            if (GameHiRes < 3) {
                GameHiRes++;
                reset = 1;
            }
            break;
        }
    }

    if (CHK_ANY(InputDB, IN_LEFT)) {
        switch (current_row) {
        case 0:
            if (AppSettings & ASF_PERSPECTIVE) {
                AppSettings &= ~ASF_PERSPECTIVE;
                reset = 1;
            }
            break;
        case 1:
            if (AppSettings & ASF_BILINEAR) {
                AppSettings &= ~ASF_BILINEAR;
                reset = 1;
            }
            break;
        case 2:
            if (GameHiRes > 0) {
                --GameHiRes;
                reset = 1;
            }
            break;
        }
    }

    if (CHK_ANY(InputDB, IN_DESELECT | IN_SELECT)) {
        reset = 1;
    }

    if (reset) {
        for (int i = 0; i < 5; i++) {
            T_RemovePrint(DetailText[i]);
            DetailText[i] = NULL;
        }
    }
}

void DoDetailOption(INVENTORY_ITEM *inv_item)
{
    if (IsHardwareRenderer) {
        DoDetailOptionHW(inv_item);
        return;
    }

    if (!DetailText[0]) {
        DetailText[2] = T_Print(0, 0, 0, "High");
        DetailText[1] = T_Print(0, 25, 0, "Medium");
        DetailText[0] = T_Print(0, 50, 0, "Low");
        DetailText[3] = T_Print(0, -32, 0, " ");
        DetailText[4] = T_Print(0, -30, 0, "Select Detail");
        T_AddBackground(DetailText[4], 156, 0, 0, 0, 8, IC_BLACK, NULL, 0);
        T_AddOutline(DetailText[4], 1, IC_ORANGE, NULL, 0);
        T_AddBackground(
            DetailText[AppSettings], 148, 0, 0, 0, 8, IC_BLACK, NULL, 0);
        T_AddOutline(DetailText[AppSettings], 1, IC_ORANGE, NULL, 0);
        T_AddBackground(DetailText[3], 160, 107, 0, 0, 16, IC_BLACK, NULL, 0);
        T_AddOutline(DetailText[3], 1, IC_BLUE, NULL, 0);
        for (int i = 0; i < 5; i++) {
            T_CentreH(DetailText[i], 1);
            T_CentreV(DetailText[i], 1);
        }
    }

    if (CHK_ANY(InputDB, IN_BACK) && AppSettings > 0) {
        T_RemoveOutline(DetailText[AppSettings]);
        T_RemoveBackground(DetailText[AppSettings]);
        AppSettings--;
        T_AddOutline(DetailText[AppSettings], 1, IC_ORANGE, NULL, 0);
        T_AddBackground(
            DetailText[AppSettings], 148, 0, 0, 0, 8, IC_BLACK, NULL, 0);
    }

    if (CHK_ANY(InputDB, IN_FORWARD) && AppSettings < 2) {
        T_RemoveOutline(DetailText[AppSettings]);
        T_RemoveBackground(DetailText[AppSettings]);
        AppSettings++;
        T_AddOutline(DetailText[AppSettings], 1, IC_ORANGE, NULL, 0);
        T_AddBackground(
            DetailText[AppSettings], 148, 0, 0, 0, 8, IC_BLACK, NULL, 0);
    }

    if (AppSettings == 0) {
        Quality = 0;
    } else if (AppSettings == 1) {
        Quality = 0x3000000;
    } else if (AppSettings == 2) {
        Quality = 0x6000000;
    }

    if (CHK_ANY(InputDB, IN_DESELECT | IN_SELECT)) {
        for (int i = 0; i < 5; i++) {
            T_RemovePrint(DetailText[i]);
            DetailText[0] = NULL;
        }
    }
}

// original name: do_sound_option
void DoSoundOption(INVENTORY_ITEM *inv_item)
{
    char buf[20];

    if (!SoundText[0]) {
        if (OptionMusicVolume > 10) {
            OptionMusicVolume = 10;
        }
        sprintf(buf, "| %2d", OptionMusicVolume);
        SoundText[0] = T_Print(0, 0, 0, buf);

        if (OptionSoundFXVolume > 10) {
            OptionSoundFXVolume = 10;
        }
        sprintf(buf, "} %2d", OptionSoundFXVolume);
        SoundText[1] = T_Print(0, 25, 0, buf);

        SoundText[2] = T_Print(0, -32, 0, " ");
        SoundText[3] = T_Print(0, -30, 0, "Set Volumes");

        T_AddBackground(SoundText[0], 128, 0, 0, 0, 8, IC_BLACK, NULL, 0);
        T_AddOutline(SoundText[0], 1, IC_ORANGE, NULL, 0);
        T_AddBackground(SoundText[2], 140, 85, 0, 0, 48, IC_BLACK, NULL, 0);
        T_AddOutline(SoundText[2], 1, IC_BLUE, NULL, 0);
        T_AddBackground(SoundText[3], 136, 0, 0, 0, 8, IC_BLACK, NULL, 0);
        T_AddOutline(SoundText[3], 1, IC_BLUE, NULL, 0);

        for (int i = 0; i < 4; i++) {
            T_CentreH(SoundText[i], 1);
            T_CentreV(SoundText[i], 1);
        }
    }

    if (CHK_ANY(InputDB, IN_FORWARD) && Item_Data > 0) {
        T_RemoveOutline(SoundText[Item_Data]);
        T_RemoveBackground(SoundText[Item_Data]);
        T_AddBackground(
            SoundText[--Item_Data], 128, 0, 0, 0, 8, IC_BLACK, NULL, 0);
        T_AddOutline(SoundText[Item_Data], 1, IC_ORANGE, NULL, 0);
    }

    if (CHK_ANY(InputDB, IN_BACK) && Item_Data < 1) {
        T_RemoveOutline(SoundText[Item_Data]);
        T_RemoveBackground(SoundText[Item_Data]);
        T_AddBackground(
            SoundText[++Item_Data], 128, 0, 0, 0, 8, IC_BLACK, NULL, 0);
        T_AddOutline(SoundText[Item_Data], 1, IC_ORANGE, NULL, 0);
    }

    switch (Item_Data) {
    case 0:
        if (CHK_ANY(Input, IN_LEFT) && OptionMusicVolume > 0) {
            OptionMusicVolume--;
            IDelay = 1;
            IDCount = 10;
            sprintf(buf, "| %2d", OptionMusicVolume);
            T_ChangeText(SoundText[0], buf);
        } else if (CHK_ANY(Input, IN_RIGHT) && OptionMusicVolume < 10) {
            OptionMusicVolume++;
            IDelay = 1;
            IDCount = 10;
            sprintf(buf, "| %2d", OptionMusicVolume);
            T_ChangeText(SoundText[0], buf);
        }

        if (CHK_ANY(Input, IN_LEFT | IN_RIGHT)) {
            if (OptionMusicVolume) {
                S_CDVolume(25 * OptionMusicVolume + 5);
            } else {
                S_CDVolume(0);
            }
            SoundEffect(115, NULL, SFX_ALWAYS);
        }
        break;

    case 1:
        if (CHK_ANY(Input, IN_LEFT) && OptionSoundFXVolume > 0) {
            OptionSoundFXVolume--;
            IDelay = 1;
            IDCount = 10;
            sprintf(buf, "} %2d", OptionSoundFXVolume);
            T_ChangeText(SoundText[1], buf);
        } else if (CHK_ANY(Input, IN_RIGHT) && OptionSoundFXVolume < 10) {
            OptionSoundFXVolume++;
            IDelay = 1;
            IDCount = 10;
            sprintf(buf, "} %2d", OptionSoundFXVolume);
            T_ChangeText(SoundText[1], buf);
        }

        if (CHK_ANY(Input, IN_LEFT | IN_RIGHT)) {
            if (OptionSoundFXVolume) {
                adjust_master_volume(6 * OptionSoundFXVolume + 3);
            } else {
                adjust_master_volume(0);
            }
            SoundEffect(115, NULL, SFX_ALWAYS);
        }
        break;
    }

    // NOTE: removed dead code (unused INVENTORY_SPRITE* member assignments)

    if (CHK_ANY(InputDB, IN_DESELECT | IN_SELECT)) {
        for (int i = 0; i < 4; i++) {
            T_RemovePrint(SoundText[i]);
            SoundText[i] = NULL;
        }
    }
}

// original name: do_compass_option
void DoCompassOption(INVENTORY_ITEM *inv_item)
{
    if (CHK_ANY(InputDB, IN_DESELECT | IN_SELECT)) {
        inv_item->goal_frame = inv_item->frames_total - 1;
        inv_item->anim_direction = 1;
    }
}

void FlashConflicts()
{
    for (int i = 0; i < 13; i++) {
        int16_t key = Layout[IConfig][i];
        T_FlashText(CtrlTextB[i], 0, 0);
        for (int j = 0; j < 13; j++) {
            if (i != j && key == Layout[IConfig][j]) {
                T_FlashText(CtrlTextB[i], 1, 20);
                break;
            }
        }
    }
}

void DefaultConflict()
{
    for (int i = 0; i < 13; i++) {
        int16_t key = Layout[0][i];
        Conflict[i] = 0;
        for (int j = 0; j < 13; j++) {
            if (key == Layout[1][j]) {
                Conflict[i] = 1;
                break;
            }
        }
    }
}

// original name: do_control_option
void DoControlOption(INVENTORY_ITEM *inv_item)
{
    int16_t key;

    if (!ControlText[0]) {
        ControlText[0] =
            T_Print(0, -50, 0, IConfig ? "User Keys" : "Default Keys");
        T_CentreH(ControlText[0], 1);
        T_CentreV(ControlText[0], 1);
        S_ShowControls();

        KeyChange = -1;
        T_AddBackground(ControlText[0], 0, 0, 0, 0, 48, IC_BLACK, NULL, 0);
    }

    switch (SelectKey) {
    case 0:
        if (CHK_ANY(InputDB, IN_LEFT | IN_RIGHT)) {
            if (KeyChange == -1) {
                IConfig = IConfig ? 0 : 1;
                S_ChangeCtrlText();
                FlashConflicts();
            } else {
                T_RemoveBackground(CtrlTextA[KeyChange]);
                if (KeyChange <= 6) {
                    KeyChange += 7;
                    if (KeyChange == 13) {
                        KeyChange = 12;
                    }
                } else if (KeyChange == 12) {
                    KeyChange = 6;
                } else {
                    KeyChange -= 7;
                }
                T_AddBackground(
                    CtrlTextA[KeyChange], 0, 0, 0, 0, 48, IC_BLACK, NULL, 0);
            }
        } else if (
            CHK_ANY(InputDB, IN_DESELECT)
            || (CHK_ANY(InputDB, IN_SELECT) && KeyChange == -1)) {
            S_RemoveCtrl();
            DefaultConflict();
            return;
        }

        if (IConfig) {
            if (CHK_ANY(InputDB, IN_SELECT)) {
                SelectKey = 1;
                T_RemoveBackground(CtrlTextA[KeyChange]);
                T_AddBackground(
                    CtrlTextB[KeyChange], 0, 0, 0, 0, 48, IC_BLACK, NULL, 0);
            } else if (CHK_ANY(InputDB, IN_FORWARD)) {
                T_RemoveBackground(
                    KeyChange == -1 ? ControlText[0] : CtrlTextA[KeyChange]);

                KeyChange--;
                if (KeyChange < -1) {
                    KeyChange = 12;
                }

                T_AddBackground(
                    KeyChange == -1 ? ControlText[0] : CtrlTextA[KeyChange], 0,
                    0, 0, 0, 48, IC_BLACK, NULL, 0);
            } else if (CHK_ANY(InputDB, IN_BACK)) {
                T_RemoveBackground(
                    KeyChange == -1 ? ControlText[0] : CtrlTextA[KeyChange]);

                KeyChange++;
                if (KeyChange > 12) {
                    KeyChange = -1;
                }

                T_AddBackground(
                    KeyChange == -1 ? ControlText[0] : CtrlTextA[KeyChange], 0,
                    0, 0, 0, 48, IC_BLACK, NULL, 0);
            }
        }
        break;

    case 1:
#ifdef T1M_FEAT_INPUT
        if (!CHK_ANY(Input, IN_SELECT)) {
#else
        if (!CHK_ANY(InputDB, IN_SELECT)) {
#endif
            SelectKey = 2;
        }
        KeyClearBuffer();
        break;

    case 2:
        if (JoyThrottle) {
            key = 272;
        } else if (JoyHat) {
            key = 256;
        } else if (JoyFire) {
            key = JoyFire + 256;
        } else {
            key = KeyGet();
        }

        if (key >= 0 && ScanCodeNames[key] && key != DIK_ESCAPE
            && key != DIK_RETURN && key != DIK_LEFT && key != DIK_RIGHT
            && key != DIK_UP && key != DIK_DOWN) {
            Layout[IConfig][KeyChange] = key;
            T_ChangeText(CtrlTextB[KeyChange], ScanCodeNames[key]);
            T_RemoveBackground(CtrlTextB[KeyChange]);
            T_AddBackground(
                CtrlTextA[KeyChange], 0, 0, 0, 0, 48, IC_BLACK, NULL, 0);
            SelectKey = 3;
            FlashConflicts();
        }
        break;

    case 3:
        key = Layout[IConfig][KeyChange];

        if (!CHK_ANY(key, IN_OPTION)) {
            if (!KeyData->keymap[key]) {
                SelectKey = 0;
                if (Layout[IConfig][key] == DIK_LCONTROL) {
                    Layout[IConfig][key] = DIK_RCONTROL;
                }
                if (Layout[IConfig][key] == DIK_LSHIFT) {
                    Layout[IConfig][key] = DIK_RSHIFT;
                }
                if (Layout[IConfig][key] == DIK_LMENU) {
                    Layout[IConfig][key] = DIK_RMENU;
                }
                FlashConflicts();
            }
        }

        if (key == 256) {
            if (!JoyHat) {
                SelectKey = 0;
            }
        } else if (key == 272) {
            if (!JoyThrottle) {
                SelectKey = 0;
            }
        } else {
            if (JoyFire != key) {
                SelectKey = 0;
            }
        }
        break;
    }

    Input = 0;
    InputDB = 0;
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
    T_AddBackground(ControlText[1], hpos, vpos, 0, 0, 48, IC_BLACK, NULL, 0);

    if (!CtrlTextB[0]) {
        int16_t *layout = Layout[IConfig];

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

void S_ChangeCtrlText()
{
    T_ChangeText(ControlText[0], IConfig ? "User Keys" : "Default Keys");
    for (int i = 0; i < 13; ++i) {
        T_ChangeText(CtrlTextB[i], ScanCodeNames[Layout[IConfig][i]]);
    }
}

void S_RemoveCtrlText()
{
    for (int i = 0; i < 13; ++i) {
        T_RemovePrint(CtrlTextA[i]);
        T_RemovePrint(CtrlTextB[i]);
        CtrlTextB[i] = NULL;
        CtrlTextA[i] = NULL;
    }
}

void S_RemoveCtrl()
{
    T_RemovePrint(ControlText[0]);
    T_RemovePrint(ControlText[1]);
    ControlText[0] = NULL;
    ControlText[1] = NULL;
    S_RemoveCtrlText();
}

// original name: Init_Requester
void InitRequester(REQUEST_INFO *req)
{
    req->heading = NULL;
    req->background = NULL;
    req->moreup = NULL;
    req->moredown = NULL;
    for (int i = 0; i < MAX_REQLINES; i++) {
        req->texts[i] = NULL;
    }
}

// original name: Remove_Requester
void RemoveRequester(REQUEST_INFO *req)
{
    T_RemovePrint(req->heading);
    req->heading = NULL;
    T_RemovePrint(req->background);
    req->background = NULL;
    T_RemovePrint(req->moreup);
    req->moreup = NULL;
    T_RemovePrint(req->moredown);
    req->moredown = NULL;
    for (int i = 0; i < MAX_REQLINES; i++) {
        T_RemovePrint(req->texts[i]);
        req->texts[i] = NULL;
    }
}

// original name: Display_Requester
int32_t DisplayRequester(REQUEST_INFO *req)
{
    int32_t edge_y;

    // NOTE: RIF_FIXED_HEIGHT is missing in original game
    if (req->flags & RIF_FIXED_HEIGHT) {
        edge_y = req->y * GetRenderHeightDownscaled() / 100;
    } else {
        switch (HiRes) {
        case 0:
            req->y = -20;
            req->vis_lines = 5;
            break;
        case 1:
            req->y = -60;
            req->vis_lines = 8;
            break;
        case 3:
            req->y = -120;
            req->vis_lines = 12;
            break;
        default:
            req->y = -80;
            req->vis_lines = 10;
            break;
        }
        edge_y = req->y;
    }

    int32_t lines_height = req->vis_lines * req->line_height;
    int32_t box_width = req->pix_width;
    int32_t box_height =
        req->line_height + lines_height + BOX_PADDING * 2 + BOX_BORDER * 2;

    int32_t line_one_off = edge_y - lines_height - BOX_PADDING;
    int32_t box_y = line_one_off - req->line_height - BOX_PADDING - BOX_BORDER;
    int32_t line_qty = req->vis_lines;
    if (req->items < req->vis_lines) {
        line_qty = req->items;
    }

    if (!req->heading) {
        req->heading = T_Print(
            req->x, line_one_off - req->line_height - BOX_PADDING, req->z,
            req->heading_text);
        T_CentreH(req->heading, 1);
        T_BottomAlign(req->heading, 1);
        T_AddBackground(
            req->heading, req->pix_width - 4, 0, 0, 0, 8, IC_BLACK,
            ReqMainGour1, D_TRANS2);
        T_AddOutline(req->heading, 1, IC_ORANGE, ReqMainGour2, 0);
    }

    if (!req->background) {
        req->background = T_Print(req->x, box_y, 0, " ");
        T_CentreH(req->background, 1);
        T_BottomAlign(req->background, 1);
        T_AddBackground(
            req->background, box_width, box_height, 0, 0, 48, IC_BLACK,
            ReqBgndGour1, D_TRANS1);
        T_AddOutline(req->background, 1, IC_BLUE, ReqBgndGour2, 0);
    }

    if (req->line_offset) {
        if (!req->moreup) {
#if T1M_FEAT_UI
            req->moreup =
                T_Print(req->x, line_one_off - req->line_height + 2, 0, "[");
            T_SetScale(req->moreup, PHD_ONE * 2 / 3, PHD_ONE * 2 / 3);
#else
            req->moreup =
                T_Print(req->x, line_one_off - req->line_height, 0, " ");
#endif
            T_CentreH(req->moreup, 1);
            T_BottomAlign(req->moreup, 1);
            T_AddBackground(
                req->moreup, 16, 6, 0, 8, 8, IC_BLACK, ReqBgndMoreUp, D_TRANS1);
        }
    } else {
        T_RemovePrint(req->moreup);
        req->moreup = NULL;
    }

    if (req->items > req->vis_lines + req->line_offset) {
        if (!req->moredown) {
#if T1M_FEAT_UI
            req->moredown = T_Print(req->x, edge_y - 12, 0, "]");
            T_SetScale(req->moredown, PHD_ONE * 2 / 3, PHD_ONE * 2 / 3);
#else
            req->moredown = T_Print(req->x, edge_y - 8, 0, " ");
#endif
            T_CentreH(req->moredown, 1);
            T_BottomAlign(req->moredown, 1);
            T_AddBackground(
                req->moredown, 16, 6, 0, 0, 8, IC_BLACK, ReqBgndMoreDown,
                D_TRANS1);
        }
    } else {
        T_RemovePrint(req->moredown);
        req->moredown = NULL;
    }

    for (int i = 0; i < line_qty; i++) {
        if (!req->texts[i]) {
            req->texts[i] = T_Print(
                0, line_one_off + req->line_height * i, 0,
                &req->item_texts[req->item_text_len * (req->line_offset + i)]);
            T_CentreH(req->texts[i], 1);
            T_BottomAlign(req->texts[i], 1);
        }
        if (req->line_offset + i == req->requested) {
            T_AddBackground(
                req->texts[i], req->pix_width - BOX_PADDING - 2 * BOX_BORDER, 0,
                0, 0, 16, IC_BLACK, ReqUnselGour1, D_TRANS1);
            T_AddOutline(req->texts[i], 1, IC_ORANGE, ReqUnselGour2, 0);
        } else {
            T_RemoveBackground(req->texts[i]);
            T_RemoveOutline(req->texts[i]);
        }
    }

    if (req->line_offset != req->line_old_offset) {
        for (int i = 0; i < line_qty; i++) {
            if (req->texts[i]) {
                T_ChangeText(
                    req->texts[i],
                    &req->item_texts
                         [req->item_text_len * (req->line_offset + i)]);
            }
        }
    }

    if (CHK_ANY(InputDB, IN_BACK)) {
        if (req->requested < req->items - 1) {
            req->requested++;
        }
        req->line_old_offset = req->line_offset;
        if (req->requested > req->line_offset + req->vis_lines - 1) {
            req->line_offset++;
            return 0;
        }
        return 0;
    }

    if (CHK_ANY(InputDB, IN_FORWARD)) {
        if (req->requested) {
            req->requested--;
        }
        req->line_old_offset = req->line_offset;
        if (req->requested < req->line_offset) {
            req->line_offset--;
            return 0;
        }
        return 0;
    }

    if (CHK_ANY(InputDB, IN_SELECT)) {
        // TODO: refactor me
        if (!strncmp(
                req->texts[req->requested - req->line_offset]->string,
                "- EMPTY SLOT", 12)
            && !strcmp(PassportText->string, "Load Game")) {
            Input = 0;
            return 0;
        } else {
            RemoveRequester(req);
            return req->requested + 1;
        }
    } else if (InputDB & IN_DESELECT) {
        RemoveRequester(req);
        return -1;
    }

    return 0;
}

void T1MInjectGameOption()
{
    INJECT(0x0042D770, DoInventoryOptions);
    INJECT(0x0042D9C0, DoPassportOption);
    INJECT(0x0042DE90, DoDetailOptionHW);
    INJECT(0x0042E2D0, DoDetailOption);
    INJECT(0x0042E5C0, DoSoundOption);
    INJECT(0x0042F230, S_ShowControls);
    INJECT(0x0042F6F0, DisplayRequester);
}
