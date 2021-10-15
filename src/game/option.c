#include "game/option.h"

#include "config.h"
#include "game/game.h"
#include "game/inv.h"
#include "game/mnsound.h"
#include "game/requester.h"
#include "game/settings.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/vars.h"
#include "specific/input.h"
#include "specific/output.h"
#include "specific/sndpc.h"
#include "specific/clock.h"
#include "util.h"

#include <dinput.h>
#include <stdio.h>

#define GAMMA_MODIFIER 8
#define MIN_GAMMA_LEVEL -127
#define MAX_GAMMA_LEVEL 127
#define PASSPORT_2FRONT IN_LEFT
#define PASSPORT_2BACK IN_RIGHT
#define PASSPORT_PAGE_COUNT 3
#define MAX_GAME_MODES 4
#define MAX_GAME_MODE_LENGTH 20

#define COMPASS_TOP_Y -100
#define COMPASS_ROW_HEIGHT 25
#define COMPASS_ROW_WIDTH 225
#define DETAIL_HW_TOP_Y -55
#define DETAIL_HW_ROW_HEIGHT 25
#define DETAIL_HW_ROW_WIDHT 280
#define CONTROLS_TOP_Y -60
#define CONTROLS_BORDER 4
#define CONTROLS_HEADER_HEIGHT 25
#define CONTROLS_ROW_HEIGHT 15

typedef enum COMPASS_TEXT {
    COMPASS_TITLE = 0,
    COMPASS_TITLE_BORDER = 1,
    COMPASS_TIME = 2,
    COMPASS_SECRETS = 3,
    COMPASS_PICKUPS = 4,
    COMPASS_KILLS = 5,
    COMPASS_NUMBER_OF = 6,
} COMPASS_TEXT;

typedef enum DETAIL_HW_TEXT {
    DETAIL_HW_TITLE = 0,
    DETAIL_HW_TITLE_BORDER = 1,
    DETAIL_HW_PERSPECTIVE = 2,
    DETAIL_HW_BILINEAR = 3,
    DETAIL_HW_UI_TEXT_SCALE = 4,
    DETAIL_HW_UI_BAR_SCALE = 5,
    DETAIL_HW_RESOLUTION = 6,
    DETAIL_HW_FPS = 7,
    DETAIL_HW_NUMBER_OF = 8,
} DETAIL_HW_TEXT;

typedef struct TEXT_COLUMN_PLACEMENT {
    int option;
    int col_num;
} TEXT_COLUMN_PLACEMENT;

static TEXTSTRING *PassportText = NULL;
static TEXTSTRING *DetailTextHW[DETAIL_HW_NUMBER_OF] = { 0 };
static TEXTSTRING *SoundText[4] = { 0 };
static TEXTSTRING *CompassText[COMPASS_NUMBER_OF] = { 0 };
static TEXTSTRING *CtrlText[2] = { 0 };
static TEXTSTRING *CtrlTextA[KEY_NUMBER_OF] = { 0 };
static TEXTSTRING *CtrlTextB[KEY_NUMBER_OF] = { 0 };

static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementNormal[] = {
    { KEY_UP, 0 },     { KEY_DOWN, 0 },   { KEY_LEFT, 0 }, { KEY_RIGHT, 0 },
    { KEY_STEP_L, 0 }, { KEY_STEP_R, 0 }, { KEY_SLOW, 0 }, { KEY_JUMP, 1 },
    { KEY_ACTION, 1 }, { KEY_DRAW, 1 },   { KEY_LOOK, 1 }, { KEY_ROLL, 1 },
    { -1, 1 },         { KEY_OPTION, 1 }, { -1, -1 },
};
static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementCheats[] = {
    { KEY_UP, 0 },
    { KEY_DOWN, 0 },
    { KEY_LEFT, 0 },
    { KEY_RIGHT, 0 },
    { KEY_STEP_L, 0 },
    { KEY_STEP_R, 0 },
    { KEY_SLOW, 0 },
    { KEY_JUMP, 0 },
    { KEY_ACTION, 1 },
    { KEY_DRAW, 1 },
    { KEY_LOOK, 1 },
    { KEY_ROLL, 1 },
    { KEY_OPTION, 1 },
    { KEY_FLY_CHEAT, 1 },
    { KEY_ITEM_CHEAT, 1 },
    { KEY_LEVEL_SKIP_CHEAT, 1 },
    { -1, -1 },
};

static int32_t PassportMode = 0;
static int32_t KeyMode = 0;

static char NewGameStrings[MAX_GAME_MODES][MAX_GAME_MODE_LENGTH];
REQUEST_INFO NewGameRequester = {
    MAX_GAME_MODES, // items
    0, // requested
    MAX_GAME_MODES, // vis_lines
    0, // line_offset
    0, // line_old_offset
    162, // pix_width
    TEXT_HEIGHT + 7, // line_height
    0, // x
    0, // y
    0, // z
    0, // flags
    NULL, // heading_text
    &NewGameStrings[0][0], // item_texts
    MAX_GAME_MODE_LENGTH, // item_text_len
};

static char LoadSaveGameStrings[MAX_SAVE_SLOTS][MAX_LEVEL_NAME_LENGTH];
REQUEST_INFO LoadSaveGameRequester = {
    1, // items
    0, // requested
    -1, // vis_lines
    0, // line_offset
    0, // line_old_offset
    272, // pix_width
    TEXT_HEIGHT + 7, // line_height
    0, // x
    -32, // y
    0, // z
    0, // flags
    NULL, // heading_text
    &LoadSaveGameStrings[0][0], // item_texts
    MAX_LEVEL_NAME_LENGTH, // item_text_len
};

static void InitLoadSaveGameRequester()
{
    REQUEST_INFO *req = &LoadSaveGameRequester;
    InitRequester(req);
    GetSavedGamesList(req);
    SetRequesterHeading(req, GF.strings[GS_PASSPORT_SELECT_LEVEL]);

    if (GetRenderHeightDownscaled() <= 240) {
        req->y = -30;
        req->vis_lines = 5;
    } else if (GetRenderHeightDownscaled() <= 384) {
        req->y = -30;
        req->vis_lines = 8;
    } else if (GetRenderHeightDownscaled() <= 480) {
        req->y = -80;
        req->vis_lines = 10;
    } else {
        req->y = -120;
        req->vis_lines = 12;
    }

    S_FrontEndCheck();
}

static void InitNewGameRequester()
{
    REQUEST_INFO *req = &NewGameRequester;
    InitRequester(req);
    SetRequesterHeading(req, GF.strings[GS_PASSPORT_SELECT_MODE]);
    AddRequesterItem(req, GF.strings[GS_PASSPORT_MODE_NEW_GAME], 0);
    AddRequesterItem(req, GF.strings[GS_PASSPORT_MODE_NEW_GAME_PLUS], 0);
    AddRequesterItem(req, GF.strings[GS_PASSPORT_MODE_JAPANESE_NEW_GAME], 0);
    AddRequesterItem(
        req, GF.strings[GS_PASSPORT_MODE_JAPANESE_NEW_GAME_PLUS], 0);
    req->y = -30 * GetRenderHeightDownscaled() / 100;
    req->vis_lines = MAX_GAME_MODES;
}

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

    if (InvMode == INV_LOAD_MODE || InvMode == INV_SAVE_MODE
        || InvMode == INV_SAVE_CRYSTAL_MODE) {
        InputDB &= ~(PASSPORT_2FRONT | PASSPORT_2BACK);
    }

    switch (page) {
    case 0:
        if (PassportMode == 1) {
            int32_t select = DisplayRequester(&LoadSaveGameRequester);
            if (select) {
                if (select > 0) {
                    InvExtraData[1] = select - 1;
                } else if (
                    InvMode != INV_SAVE_MODE && InvMode != INV_SAVE_CRYSTAL_MODE
                    && InvMode != INV_LOAD_MODE) {
                    Input = 0;
                    InputDB = 0;
                }
                PassportMode = 0;
            } else {
                Input = 0;
                InputDB = 0;
            }
        } else if (PassportMode == 0) {
            if (!SavedGamesCount || InvMode == INV_SAVE_MODE
                || InvMode == INV_SAVE_CRYSTAL_MODE) {
                InputDB = PASSPORT_2BACK;
            } else {
                if (!PassportText) {
                    PassportText =
                        T_Print(0, -16, GF.strings[GS_PASSPORT_LOAD_GAME]);
                    T_BottomAlign(PassportText, 1);
                    T_CentreH(PassportText, 1);
                }
                if (CHK_ANY(InputDB, IN_SELECT) || InvMode == INV_LOAD_MODE) {
                    T_RemovePrint(InvRingText);
                    InvRingText = NULL;
                    T_RemovePrint(InvItemText[IT_NAME]);
                    InvItemText[IT_NAME] = NULL;

                    LoadSaveGameRequester.flags |= RIF_BLOCKABLE;
                    InitLoadSaveGameRequester();
                    PassportMode = 1;
                    Input = 0;
                    InputDB = 0;
                }
            }
        }
        break;

    case 1:
        if (PassportMode == 2) {
            int32_t select = DisplayRequester(&NewGameRequester);
            if (select) {
                if (select > 0) {
                    InvExtraData[1] = select - 1;
                } else if (InvMode != INV_GAME_MODE) {
                    Input = 0;
                    InputDB = 0;
                }
                PassportMode = 0;
            } else {
                Input = 0;
                InputDB = 0;
            }
        } else if (PassportMode == 1) {
            int32_t select = DisplayRequester(&LoadSaveGameRequester);
            if (select) {
                if (select > 0) {
                    PassportMode = 0;
                    InvExtraData[1] = select - 1;
                } else {
                    if (InvMode != INV_SAVE_MODE
                        && InvMode != INV_SAVE_CRYSTAL_MODE
                        && InvMode != INV_LOAD_MODE) {
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
            if (InvMode == INV_DEATH_MODE) {
                InputDB = inv_item->anim_direction == -1 ? PASSPORT_2FRONT
                                                         : PASSPORT_2BACK;
            }
            if (!PassportText) {
                if (InvMode == INV_TITLE_MODE
                    || CurrentLevel == GF.gym_level_num) {
                    PassportText =
                        T_Print(0, -16, GF.strings[GS_PASSPORT_NEW_GAME]);
                } else {
                    PassportText =
                        T_Print(0, -16, GF.strings[GS_PASSPORT_SAVE_GAME]);
                }
                T_BottomAlign(PassportText, 1);
                T_CentreH(PassportText, 1);
            }
            if (CHK_ANY(InputDB, IN_SELECT) || InvMode == INV_SAVE_MODE
                || InvMode == INV_SAVE_CRYSTAL_MODE) {
                if (InvMode == INV_TITLE_MODE
                    || CurrentLevel == GF.gym_level_num) {
                    T_RemovePrint(InvRingText);
                    InvRingText = NULL;
                    T_RemovePrint(InvItemText[IT_NAME]);
                    InvItemText[IT_NAME] = NULL;

                    if (GF.enable_game_modes) {
                        InitNewGameRequester();
                        PassportMode = 2;
                        Input = 0;
                        InputDB = 0;
                    } else {
                        InvExtraData[1] = SaveGame.bonus_flag;
                    }
                } else {
                    T_RemovePrint(InvRingText);
                    InvRingText = NULL;
                    T_RemovePrint(InvItemText[IT_NAME]);
                    InvItemText[IT_NAME] = NULL;

                    LoadSaveGameRequester.flags &= ~RIF_BLOCKABLE;
                    InitLoadSaveGameRequester();
                    PassportMode = 1;
                    Input = 0;
                    InputDB = 0;
                }
            }
        }
        break;

    case 2:
        if (!PassportText) {
            if (InvMode == INV_TITLE_MODE) {
                PassportText =
                    T_Print(0, -16, GF.strings[GS_PASSPORT_EXIT_GAME]);
            } else {
                PassportText =
                    T_Print(0, -16, GF.strings[GS_PASSPORT_EXIT_TO_TITLE]);
            }
            T_BottomAlign(PassportText, 1);
            T_CentreH(PassportText, 1);
        }
        break;
    }

    int8_t pages_available[3] = {
        SavedGamesCount,
        InvMode == INV_TITLE_MODE || InvMode == INV_SAVE_CRYSTAL_MODE
            || !GF.enable_save_crystals,
        1,
    };

    if (CHK_ANY(InputDB, PASSPORT_2FRONT)
        && (InvMode != INV_DEATH_MODE || SavedGamesCount)) {

        while (--page >= 0) {
            if (pages_available[page]) {
                break;
            }
        }

        if (page >= 0) {
            inv_item->anim_direction = -1;
            inv_item->goal_frame = inv_item->open_frame + 5 * page;
            SoundEffect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            if (PassportText) {
                T_RemovePrint(PassportText);
                PassportText = NULL;
            }
        }

        Input = 0;
        InputDB = 0;
    }

    if (CHK_ANY(InputDB, PASSPORT_2BACK)) {
        Input = 0;
        InputDB = 0;

        while (++page < PASSPORT_PAGE_COUNT) {
            if (pages_available[page]) {
                break;
            }
        }

        if (page < PASSPORT_PAGE_COUNT) {
            inv_item->anim_direction = 1;
            inv_item->goal_frame = inv_item->open_frame + 5 * page;
            SoundEffect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            if (PassportText) {
                T_RemovePrint(PassportText);
                PassportText = NULL;
            }
        }
    }

    if (CHK_ANY(InputDB, IN_DESELECT)) {
        if (InvMode == INV_DEATH_MODE) {
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
        InvExtraData[0] = page;
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
        if (InvMode != INV_TITLE_MODE) {
            // S_SetBackgroundGamma(gamma);
        }
    }
}

// original name: do_detail_option?
void DoDetailOptionHW(INVENTORY_ITEM *inv_item)
{
    static char buf[256];
    static int32_t current_row = DETAIL_HW_PERSPECTIVE;
    const int32_t min_row = DETAIL_HW_PERSPECTIVE;
    static int32_t max_row = DETAIL_HW_RESOLUTION;

    if (!DetailTextHW[DETAIL_HW_TITLE_BORDER]) {
        int32_t y = DETAIL_HW_TOP_Y;
        DetailTextHW[DETAIL_HW_TITLE_BORDER] = T_Print(0, y - 2, " ");

        DetailTextHW[DETAIL_HW_TITLE] =
            T_Print(0, y, GF.strings[GS_DETAIL_SELECT_DETAIL]);
        y += DETAIL_HW_ROW_HEIGHT;

        sprintf(
            buf, GF.strings[GS_DETAIL_PERSPECTIVE_FMT],
            GF.strings
                [RenderSettings & RSF_PERSPECTIVE ? GS_MISC_ON : GS_MISC_OFF]);
        DetailTextHW[DETAIL_HW_PERSPECTIVE] = T_Print(0, y, buf);
        y += DETAIL_HW_ROW_HEIGHT;

        sprintf(
            buf, GF.strings[GS_DETAIL_BILINEAR_FMT],
            GF.strings
                [RenderSettings & RSF_BILINEAR ? GS_MISC_ON : GS_MISC_OFF]);
        DetailTextHW[DETAIL_HW_BILINEAR] = T_Print(0, y, buf);
        y += DETAIL_HW_ROW_HEIGHT;

        sprintf(buf, GF.strings[GS_DETAIL_UI_TEXT_SCALE_FMT], UITextScale);
        DetailTextHW[DETAIL_HW_UI_TEXT_SCALE] = T_Print(0, y, buf);
        y += DETAIL_HW_ROW_HEIGHT;

        sprintf(buf, GF.strings[GS_DETAIL_UI_BAR_SCALE_FMT], UIBarScale);
        DetailTextHW[DETAIL_HW_UI_BAR_SCALE] = T_Print(0, y, buf);
        y += DETAIL_HW_ROW_HEIGHT;

        if (dword_45B940) {
            DetailTextHW[DETAIL_HW_RESOLUTION] = T_Print(0, y, " ");
            max_row = DETAIL_HW_UI_BAR_SCALE;
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
            sprintf(buf, GF.strings[GS_DETAIL_VIDEO_MODE_FMT], tmp);
            DetailTextHW[DETAIL_HW_RESOLUTION] = T_Print(0, y, buf);
            max_row = DETAIL_HW_RESOLUTION;
        }
        y += DETAIL_HW_ROW_HEIGHT;
        
        if ( InvMode == INV_TITLE_MODE ) {
			// this can only be changed in the title menu and not in game, bad things will happen
			sprintf(buf, GF.strings[GS_DETAIL_VIDEO_FPS_RATE], (1+((AppSettings & ASF_60FPS)>>ASF_60FPS_SHIFT)) * 30);
			DetailTextHW[DETAIL_HW_FPS] = T_Print(0, y, buf);
			y += DETAIL_HW_ROW_HEIGHT;
			max_row = DETAIL_HW_FPS;
        }

        if (current_row < min_row) {
            current_row = min_row;
        }
        if (current_row > max_row) {
            current_row = max_row;
        }

        T_AddBackground(
            DetailTextHW[DETAIL_HW_TITLE_BORDER], DETAIL_HW_ROW_WIDHT,
            y - DETAIL_HW_TOP_Y, 0, 0);
        T_AddOutline(DetailTextHW[DETAIL_HW_TITLE_BORDER], 1);

        T_AddBackground(
            DetailTextHW[DETAIL_HW_TITLE], DETAIL_HW_ROW_WIDHT - 4, 0, 0, 0);
        T_AddOutline(DetailTextHW[DETAIL_HW_TITLE], 1);

        T_AddBackground(
            DetailTextHW[current_row], DETAIL_HW_ROW_WIDHT - 12, 0, 0, 0);
        T_AddOutline(DetailTextHW[current_row], 1);

        for (int i = 0; i < DETAIL_HW_NUMBER_OF; i++) {
            T_CentreH(DetailTextHW[i], 1);
            T_CentreV(DetailTextHW[i], 1);
        }
    }

    if (CHK_ANY(InputDB, IN_FORWARD) && current_row > min_row) {
        T_RemoveOutline(DetailTextHW[current_row]);
        T_RemoveBackground(DetailTextHW[current_row]);
        current_row--;
        T_AddOutline(DetailTextHW[current_row], 1);
        T_AddBackground(
            DetailTextHW[current_row], DETAIL_HW_ROW_WIDHT - 12, 0, 0, 0);
    }

    if (CHK_ANY(InputDB, IN_BACK) && current_row < max_row) {
        T_RemoveOutline(DetailTextHW[current_row]);
        T_RemoveBackground(DetailTextHW[current_row]);
        current_row++;
        T_AddOutline(DetailTextHW[current_row], 1);
        T_AddBackground(
            DetailTextHW[current_row], DETAIL_HW_ROW_WIDHT - 12, 0, 0, 0);
    }

    int8_t reset = 0;

    if (CHK_ANY(InputDB, IN_RIGHT)) {
        switch (current_row) {
        case DETAIL_HW_PERSPECTIVE:
            if (!(RenderSettings & RSF_PERSPECTIVE)) {
                RenderSettings |= RSF_PERSPECTIVE;
                reset = 1;
            }
            break;

        case DETAIL_HW_BILINEAR:
            if (!(RenderSettings & RSF_BILINEAR)) {
                RenderSettings |= RSF_BILINEAR;
                reset = 1;
            }
            break;

        case DETAIL_HW_UI_TEXT_SCALE:
            if (UITextScale < MAX_UI_SCALE) {
                UITextScale += 0.1;
                reset = 1;
            }
            break;

        case DETAIL_HW_UI_BAR_SCALE:
            if (UIBarScale < MAX_UI_SCALE) {
                UIBarScale += 0.1;
                reset = 1;
            }
            break;

        case DETAIL_HW_RESOLUTION:
            if (GameHiRes < 3) {
                GameHiRes++;
                reset = 1;
            }
            break;
            
        case DETAIL_HW_FPS:
			AppSettings ^= ASF_60FPS;
			reset = 1;
			break;
        }
    }

    if (CHK_ANY(InputDB, IN_LEFT)) {
        switch (current_row) {
        case DETAIL_HW_PERSPECTIVE:
            if (RenderSettings & RSF_PERSPECTIVE) {
                RenderSettings &= ~RSF_PERSPECTIVE;
                reset = 1;
            }
            break;

        case DETAIL_HW_BILINEAR:
            if (RenderSettings & RSF_BILINEAR) {
                RenderSettings &= ~RSF_BILINEAR;
                reset = 1;
            }
            break;

        case DETAIL_HW_UI_TEXT_SCALE:
            if (UITextScale > MIN_UI_SCALE) {
                UITextScale -= 0.1;
                reset = 1;
            }
            break;

        case DETAIL_HW_UI_BAR_SCALE:
            if (UIBarScale > MIN_UI_SCALE) {
                UIBarScale -= 0.1;
                reset = 1;
            }
            break;

        case DETAIL_HW_RESOLUTION:
            if (GameHiRes > 0) {
                GameHiRes--;
                reset = 1;
            }
            break;
            
        case DETAIL_HW_FPS:
			AppSettings ^= ASF_60FPS;
			reset = 1;
			break;
        }
    }

    if (CHK_ANY(InputDB, IN_DESELECT | IN_SELECT)) {
        reset = 1;
    }

    if (reset) {
        for (int i = 0; i < DETAIL_HW_NUMBER_OF; i++) {
            T_RemovePrint(DetailTextHW[i]);
            DetailTextHW[i] = NULL;
        }
        S_WriteUserSettings();
    }
}

void DoDetailOption(INVENTORY_ITEM *inv_item)
{
    DoDetailOptionHW(inv_item);
}

// original name: do_sound_option
void DoSoundOption(INVENTORY_ITEM *inv_item)
{
    static char buf[20];

    if (!SoundText[0]) {
        if (OptionMusicVolume > 10) {
            OptionMusicVolume = 10;
        }
        sprintf(buf, "| %2d", OptionMusicVolume);
        SoundText[0] = T_Print(0, 0, buf);

        if (OptionSoundFXVolume > 10) {
            OptionSoundFXVolume = 10;
        }
        sprintf(buf, "} %2d", OptionSoundFXVolume);
        SoundText[1] = T_Print(0, 25, buf);

        SoundText[2] = T_Print(0, -32, " ");
        SoundText[3] = T_Print(0, -30, GF.strings[GS_SOUND_SET_VOLUMES]);

        T_AddBackground(SoundText[0], 128, 0, 0, 0);
        T_AddOutline(SoundText[0], 1);
        T_AddBackground(SoundText[2], 140, 85, 0, 0);
        T_AddOutline(SoundText[2], 1);
        T_AddBackground(SoundText[3], 136, 0, 0, 0);
        T_AddOutline(SoundText[3], 1);

        for (int i = 0; i < 4; i++) {
            T_CentreH(SoundText[i], 1);
            T_CentreV(SoundText[i], 1);
        }
    }

    if (CHK_ANY(InputDB, IN_FORWARD) && Item_Data > 0) {
        T_RemoveOutline(SoundText[Item_Data]);
        T_RemoveBackground(SoundText[Item_Data]);
        T_AddBackground(SoundText[--Item_Data], 128, 0, 0, 0);
        T_AddOutline(SoundText[Item_Data], 1);
    }

    if (CHK_ANY(InputDB, IN_BACK) && Item_Data < 1) {
        T_RemoveOutline(SoundText[Item_Data]);
        T_RemoveBackground(SoundText[Item_Data]);
        T_AddBackground(SoundText[++Item_Data], 128, 0, 0, 0);
        T_AddOutline(SoundText[Item_Data], 1);
    }

    switch (Item_Data) {
    case 0:
        if (CHK_ANY(Input, IN_LEFT) && OptionMusicVolume > 0) {
            OptionMusicVolume--;
            IDelay = 1;
            IDCount = 10;
            sprintf(buf, "| %2d", OptionMusicVolume);
            T_ChangeText(SoundText[0], buf);
            S_WriteUserSettings();
        } else if (CHK_ANY(Input, IN_RIGHT) && OptionMusicVolume < 10) {
            OptionMusicVolume++;
            IDelay = 1;
            IDCount = 10;
            sprintf(buf, "| %2d", OptionMusicVolume);
            T_ChangeText(SoundText[0], buf);
            S_WriteUserSettings();
        }

        if (CHK_ANY(Input, IN_LEFT | IN_RIGHT)) {
            if (OptionMusicVolume) {
                S_MusicVolume(25 * OptionMusicVolume + 5);
            } else {
                S_MusicVolume(0);
            }
            SoundEffect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
        }
        break;

    case 1:
        if (CHK_ANY(Input, IN_LEFT) && OptionSoundFXVolume > 0) {
            OptionSoundFXVolume--;
            IDelay = 1;
            IDCount = 10;
            sprintf(buf, "} %2d", OptionSoundFXVolume);
            T_ChangeText(SoundText[1], buf);
            S_WriteUserSettings();
        } else if (CHK_ANY(Input, IN_RIGHT) && OptionSoundFXVolume < 10) {
            OptionSoundFXVolume++;
            IDelay = 1;
            IDCount = 10;
            sprintf(buf, "} %2d", OptionSoundFXVolume);
            T_ChangeText(SoundText[1], buf);
            S_WriteUserSettings();
        }

        if (CHK_ANY(Input, IN_LEFT | IN_RIGHT)) {
            if (OptionSoundFXVolume) {
                mn_adjust_master_volume(6 * OptionSoundFXVolume + 3);
            } else {
                mn_adjust_master_volume(0);
            }
            SoundEffect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
        }
        break;
    }

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
    static char buf[100];
    static char time_buf[100];

    if (T1MConfig.enable_compass_stats) {
        if (!CompassText[COMPASS_TITLE_BORDER]) {
            int32_t y = COMPASS_TOP_Y;

            CompassText[COMPASS_TITLE_BORDER] = T_Print(0, y - 2, " ");

            sprintf(buf, "%s", GF.levels[CurrentLevel].level_title);
            CompassText[COMPASS_TITLE] = T_Print(0, y, buf);
            y += COMPASS_ROW_HEIGHT;

            CompassText[COMPASS_TIME] = T_Print(0, y, " ");
            y += COMPASS_ROW_HEIGHT;

            int32_t secrets_taken = 0;
            int32_t secrets_total = MAX_SECRETS;
            int32_t secrets_flags = SaveGame.secrets;
            do {
                if (secrets_flags & 1) {
                    secrets_taken++;
                }
                secrets_flags >>= 1;
                secrets_total--;
            } while (secrets_total);
            sprintf(
                buf, GF.strings[GS_STATS_SECRETS_FMT], secrets_taken,
                GF.levels[CurrentLevel].secrets);
            CompassText[COMPASS_SECRETS] = T_Print(0, y, buf);
            y += COMPASS_ROW_HEIGHT;

            sprintf(buf, GF.strings[GS_STATS_PICKUPS_FMT], SaveGame.pickups);
            CompassText[COMPASS_PICKUPS] = T_Print(0, y, buf);
            y += COMPASS_ROW_HEIGHT;

            sprintf(buf, GF.strings[GS_STATS_KILLS_FMT], SaveGame.kills);
            CompassText[COMPASS_KILLS] = T_Print(0, y, buf);
            y += COMPASS_ROW_HEIGHT;

            T_AddBackground(
                CompassText[COMPASS_TITLE_BORDER], COMPASS_ROW_WIDTH,
                y - COMPASS_TOP_Y, 0, 0);
            T_AddOutline(CompassText[COMPASS_TITLE_BORDER], 1);
            T_AddBackground(
                CompassText[COMPASS_TITLE], COMPASS_ROW_WIDTH - 4, 0, 0, 0);
            T_AddOutline(CompassText[COMPASS_TITLE], 1);

            for (int i = 0; i < COMPASS_NUMBER_OF; i++) {
                T_CentreH(CompassText[i], 1);
                T_CentreV(CompassText[i], 1);
            }
        }

        int32_t seconds = SaveGame.timer / 30/ANIM_SCALE;
        int32_t hours = seconds / 3600;
        int32_t minutes = (seconds / 60) % 60;
        seconds %= 60;
        if (hours) {
            sprintf(
                time_buf, "%d:%d%d:%d%d", hours, minutes / 10, minutes % 10,
                seconds / 10, seconds % 10);
        } else {
            sprintf(time_buf, "%d:%d%d", minutes, seconds / 10, seconds % 10);
        }
        sprintf(buf, GF.strings[GS_STATS_TIME_TAKEN_FMT], time_buf);
        T_ChangeText(CompassText[COMPASS_TIME], buf);
    }

    if (CHK_ANY(InputDB, IN_DESELECT | IN_SELECT)) {
        for (int i = 0; i < COMPASS_NUMBER_OF; i++) {
            T_RemovePrint(CompassText[i]);
            CompassText[i] = NULL;
        }
        inv_item->goal_frame = inv_item->frames_total - 1;
        inv_item->anim_direction = 1;
    }
}

void FlashConflicts()
{
    for (int i = 0; i < KEY_NUMBER_OF; i++) {
        int16_t key = Layout[IConfig][i];
        T_FlashText(CtrlTextB[i], 0, 0);
        for (int j = 0; j < KEY_NUMBER_OF; j++) {
            if (i != j && key == Layout[IConfig][j]) {
                T_FlashText(CtrlTextB[i], 1, 20);
                break;
            }
        }
    }
}

void DefaultConflict()
{
    for (int i = 0; i < KEY_NUMBER_OF; i++) {
        int16_t key = Layout[INPUT_LAYOUT_DEFAULT][i];
        ConflictLayout[i] = 0;
        for (int j = 0; j < KEY_NUMBER_OF; j++) {
            if (key == Layout[INPUT_LAYOUT_USER][j]) {
                ConflictLayout[i] = 1;
                break;
            }
        }
    }
}

// original name: do_control_option
void DoControlOption(INVENTORY_ITEM *inv_item)
{
    int16_t key;

    if (!CtrlText[0]) {
        CtrlText[0] = T_Print(
            0,
            CONTROLS_TOP_Y - CONTROLS_BORDER
                + (CONTROLS_HEADER_HEIGHT + CONTROLS_BORDER
                   - CONTROLS_ROW_HEIGHT)
                    / 2,
            GF.strings
                [IConfig ? GS_CONTROL_USER_KEYS : GS_CONTROL_DEFAULT_KEYS]);
        T_CentreH(CtrlText[0], 1);
        T_CentreV(CtrlText[0], 1);
        S_ShowControls();

        KeyChange = -1;
        T_AddBackground(CtrlText[0], 0, 0, 0, 0);
        T_AddOutline(CtrlText[0], 1);
    }

    const TEXT_COLUMN_PLACEMENT *cols = T1MConfig.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    const TEXT_COLUMN_PLACEMENT *first_col = NULL;
    const TEXT_COLUMN_PLACEMENT *last_col = NULL;
    for (const TEXT_COLUMN_PLACEMENT *col = cols;
         col->col_num >= 0 && col->col_num <= 1; col++) {
        if (col->option != -1) {
            if (first_col == NULL) {
                first_col = col;
            }
            last_col = col;
        }
    }

    switch (KeyMode) {
    case 0:
        if (CHK_ANY(InputDB, IN_LEFT | IN_RIGHT)) {
            if (KeyChange == -1) {
                IConfig = IConfig ? 0 : 1;
                S_ChangeCtrlText();
                FlashConflicts();
                S_WriteUserSettings();
            } else {
                T_RemoveBackground(CtrlTextA[KeyChange]);
                T_RemoveOutline(CtrlTextA[KeyChange]);

                int col_idx[2] = { 0, 0 };
                const TEXT_COLUMN_PLACEMENT *sel_col;

                for (sel_col = cols;
                     sel_col->col_num >= 0 && sel_col->col_num <= 1;
                     sel_col++) {
                    col_idx[sel_col->col_num]++;
                    if (sel_col->option == KeyChange) {
                        break;
                    }
                }

                col_idx[!sel_col->col_num] = 0;
                for (const TEXT_COLUMN_PLACEMENT *dst_col = cols;
                     dst_col->col_num >= 0 && dst_col->col_num <= 1;
                     dst_col++) {
                    if (dst_col->col_num != sel_col->col_num) {
                        col_idx[dst_col->col_num]++;
                        if (dst_col->option != -1
                            && col_idx[dst_col->col_num]
                                >= col_idx[sel_col->col_num]) {
                            KeyChange = dst_col->option;
                            break;
                        }
                    }
                }

                T_AddBackground(CtrlTextA[KeyChange], 0, 0, 0, 0);
                T_AddOutline(CtrlTextA[KeyChange], 1);
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
                KeyMode = 1;
                T_RemoveBackground(CtrlTextA[KeyChange]);
                T_AddBackground(CtrlTextB[KeyChange], 0, 0, 0, 0);
                T_RemoveOutline(CtrlTextA[KeyChange]);
                T_AddOutline(CtrlTextB[KeyChange], 1);
            } else if (CHK_ANY(InputDB, IN_FORWARD)) {
                T_RemoveBackground(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange]);
                T_RemoveOutline(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange]);

                if (KeyChange == -1) {
                    KeyChange = last_col->option;
                } else if (KeyChange == first_col->option) {
                    KeyChange = -1;
                } else {
                    const TEXT_COLUMN_PLACEMENT *sel_col;
                    for (sel_col = cols;
                         sel_col->col_num >= 0 && sel_col->col_num <= 1;
                         sel_col++) {
                        if (sel_col->option == KeyChange) {
                            break;
                        }
                    }
                    sel_col--;
                    while (sel_col >= cols) {
                        if (sel_col->option != -1) {
                            KeyChange = sel_col->option;
                            break;
                        }
                        sel_col--;
                    }
                }

                T_AddBackground(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange], 0, 0,
                    0, 0);
                T_AddOutline(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange], 1);
            } else if (CHK_ANY(InputDB, IN_BACK)) {
                T_RemoveBackground(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange]);
                T_RemoveOutline(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange]);

                if (KeyChange == -1) {
                    KeyChange = first_col->option;
                } else if (KeyChange == last_col->option) {
                    KeyChange = -1;
                } else {
                    const TEXT_COLUMN_PLACEMENT *sel_col;
                    for (sel_col = cols;
                         sel_col->col_num >= 0 && sel_col->col_num <= 1;
                         sel_col++) {
                        if (sel_col->option == KeyChange) {
                            break;
                        }
                    }
                    sel_col++;
                    while (sel_col >= cols) {
                        if (sel_col->option != -1) {
                            KeyChange = sel_col->option;
                            break;
                        }
                        sel_col++;
                    }
                }

                T_AddBackground(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange], 0, 0,
                    0, 0);
                T_AddOutline(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange], 1);
            }
        }
        break;

    case 1:
        if (!CHK_ANY(Input, IN_SELECT)) {
            KeyMode = 2;
        }
        break;

    case 2:
        key = KeyGet();

        if (key >= 0 && ScanCodeNames[key] && key != DIK_ESCAPE
            && key != DIK_RETURN && key != DIK_LEFT && key != DIK_RIGHT
            && key != DIK_UP && key != DIK_DOWN) {
            Layout[IConfig][KeyChange] = key;
            T_ChangeText(CtrlTextB[KeyChange], ScanCodeNames[key]);
            T_RemoveBackground(CtrlTextB[KeyChange]);
            T_RemoveOutline(CtrlTextB[KeyChange]);
            T_AddBackground(CtrlTextA[KeyChange], 0, 0, 0, 0);
            T_AddOutline(CtrlTextA[KeyChange], 1);
            KeyMode = 3;
            FlashConflicts();
            S_WriteUserSettings();
        }
        break;

    case 3:
        key = Layout[IConfig][KeyChange];

        if (!CHK_ANY(key, IN_OPTION)) {
            if (KeyGet() < 0 || KeyGet() != key) {
                KeyMode = 0;
                FlashConflicts();
                S_WriteUserSettings();
            }
        }

        KeyMode = 0;
        break;
    }

    Input = 0;
    InputDB = 0;
}

void S_ShowControls()
{
    const int16_t centre = GetRenderWidthDownscaled() / 2;
    int16_t max_y = 0;

    CtrlText[1] = T_Print(0, CONTROLS_TOP_Y - CONTROLS_BORDER, " ");
    T_CentreH(CtrlText[1], 1);
    T_CentreV(CtrlText[1], 1);

    const TEXT_COLUMN_PLACEMENT *cols = T1MConfig.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    if (!CtrlTextB[KEY_UP]) {
        int16_t *layout = Layout[IConfig];
        int16_t xs[2] = { centre - 200, centre + 20 };
        int16_t ys[2] = { CONTROLS_TOP_Y + CONTROLS_HEADER_HEIGHT,
                          CONTROLS_TOP_Y + CONTROLS_HEADER_HEIGHT };

        for (const TEXT_COLUMN_PLACEMENT *col = cols;
             col->col_num >= 0 && col->col_num <= 1; col++) {
            int16_t x = xs[col->col_num];
            int16_t y = ys[col->col_num];

            if (col->option != -1) {
                CtrlTextB[col->option] =
                    T_Print(x, y, ScanCodeNames[layout[col->option]]);
                T_CentreV(CtrlTextB[col->option], 1);
            }

            ys[col->col_num] += CONTROLS_ROW_HEIGHT;
            max_y = MAX(max_y, ys[col->col_num]);
        }

        KeyChange = 0;
    }

    if (!CtrlTextA[KEY_UP]) {
        int16_t xs[2] = { centre - 130, centre + 90 };
        int16_t ys[2] = { CONTROLS_TOP_Y + CONTROLS_HEADER_HEIGHT,
                          CONTROLS_TOP_Y + CONTROLS_HEADER_HEIGHT };

        for (const TEXT_COLUMN_PLACEMENT *col = cols;
             col->col_num >= 0 && col->col_num <= 1; col++) {
            int16_t x = xs[col->col_num];
            int16_t y = ys[col->col_num];

            if (col->option != -1) {
                CtrlTextA[col->option] = T_Print(
                    x, y, GF.strings[col->option + GS_KEYMAP_RUN - KEY_UP]);
                T_CentreV(CtrlTextA[col->option], 1);
            }

            ys[col->col_num] += CONTROLS_ROW_HEIGHT;
            max_y = MAX(max_y, ys[col->col_num]);
        }
    }

    int16_t width = 420;
    int16_t height = max_y + CONTROLS_BORDER * 2 - CONTROLS_TOP_Y;
    T_AddBackground(CtrlText[1], width, height, 0, 0);
    T_AddOutline(CtrlText[1], 1);

    FlashConflicts();
}

void S_ChangeCtrlText()
{
    T_ChangeText(
        CtrlText[0],
        GF.strings[IConfig ? GS_CONTROL_USER_KEYS : GS_CONTROL_DEFAULT_KEYS]);

    const TEXT_COLUMN_PLACEMENT *cols = T1MConfig.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    int16_t *layout = Layout[IConfig];
    for (const TEXT_COLUMN_PLACEMENT *col = cols;
         col->col_num >= 0 && col->col_num <= 1; col++) {
        if (col->option != -1) {
            T_ChangeText(
                CtrlTextB[col->option], ScanCodeNames[layout[col->option]]);
        }
    }
}

void S_RemoveCtrlText()
{
    for (int i = 0; i < KEY_NUMBER_OF; i++) {
        T_RemovePrint(CtrlTextA[i]);
        T_RemovePrint(CtrlTextB[i]);
        CtrlTextB[i] = NULL;
        CtrlTextA[i] = NULL;
    }
}

void S_RemoveCtrl()
{
    T_RemovePrint(CtrlText[0]);
    T_RemovePrint(CtrlText[1]);
    CtrlText[0] = NULL;
    CtrlText[1] = NULL;
    S_RemoveCtrlText();
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
