#include "game/option.h"

#include "config.h"
#include "game/game.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/music.h"
#include "game/requester.h"
#include "game/screen.h"
#include "game/settings.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/vars.h"
#include "specific/s_input.h"

#include <stdio.h>

#define GAMMA_MODIFIER 8
#define MIN_GAMMA_LEVEL -127
#define MAX_GAMMA_LEVEL 127
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
#define CONTROLS_ROW_HEIGHT 17

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
    DETAIL_HW_PERSPECTIVE = 0,
    DETAIL_HW_BILINEAR = 1,
    DETAIL_HW_BRIGHTNESS = 2,
    DETAIL_HW_UI_TEXT_SCALE = 3,
    DETAIL_HW_UI_BAR_SCALE = 4,
    DETAIL_HW_RESOLUTION = 5,
    DETAIL_HW_TITLE = 6,
    DETAIL_HW_TITLE_BORDER = 7,
    DETAIL_HW_NUMBER_OF = 8,
    DETAIL_HW_OPTION_MIN = DETAIL_HW_PERSPECTIVE,
    DETAIL_HW_OPTION_MAX = DETAIL_HW_RESOLUTION,
} DETAIL_HW_TEXT;

typedef enum SOUND_TEXT {
    SOUND_MUSIC_VOLUME = 0,
    SOUND_SOUND_VOLUME = 1,
    SOUND_TITLE = 2,
    SOUND_TITLE_BORDER = 3,
    SOUND_NUMBER_OF = 4,
    SOUND_OPTION_MIN = SOUND_MUSIC_VOLUME,
    SOUND_OPTION_MAX = SOUND_SOUND_VOLUME,
} SOUND_TEXT;

typedef struct TEXT_COLUMN_PLACEMENT {
    int option;
    int col_num;
} TEXT_COLUMN_PLACEMENT;

static int32_t PassportMode = 0;
static int32_t KeyMode = 0;
static int32_t KeyChange = 0;

static TEXTSTRING *PassportText = NULL;
static TEXTSTRING *DetailTextHW[DETAIL_HW_NUMBER_OF] = { 0 };
static TEXTSTRING *SoundText[4] = { 0 };
static TEXTSTRING *CompassText[COMPASS_NUMBER_OF] = { 0 };
static TEXTSTRING *CtrlText[2] = { 0 };
static TEXTSTRING *CtrlTextA[INPUT_KEY_NUMBER_OF] = { 0 };
static TEXTSTRING *CtrlTextB[INPUT_KEY_NUMBER_OF] = { 0 };

static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementNormal[] = {
    // left column
    { INPUT_KEY_UP, 0 },
    { INPUT_KEY_DOWN, 0 },
    { INPUT_KEY_LEFT, 0 },
    { INPUT_KEY_RIGHT, 0 },
    { INPUT_KEY_STEP_L, 0 },
    { INPUT_KEY_STEP_R, 0 },
    { INPUT_KEY_CAMERA_UP, 0 },
    { INPUT_KEY_CAMERA_DOWN, 0 },
    { INPUT_KEY_CAMERA_LEFT, 0 },
    { INPUT_KEY_CAMERA_RIGHT, 0 },
    { INPUT_KEY_CAMERA_RESET, 0 },
    // right column
    { INPUT_KEY_SLOW, 1 },
    { INPUT_KEY_JUMP, 1 },
    { INPUT_KEY_ACTION, 1 },
    { INPUT_KEY_DRAW, 1 },
    { INPUT_KEY_LOOK, 1 },
    { INPUT_KEY_ROLL, 1 },
    { -1, 1 },
    { INPUT_KEY_OPTION, 1 },
    { INPUT_KEY_PAUSE, 1 },
    { -1, 1 },
    { -1, 1 },
    // end
    { -1, -1 },
};

static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementCheats[] = {
    // left column
    { INPUT_KEY_UP, 0 },
    { INPUT_KEY_DOWN, 0 },
    { INPUT_KEY_LEFT, 0 },
    { INPUT_KEY_RIGHT, 0 },
    { INPUT_KEY_STEP_L, 0 },
    { INPUT_KEY_STEP_R, 0 },
    { INPUT_KEY_CAMERA_UP, 0 },
    { INPUT_KEY_CAMERA_DOWN, 0 },
    { INPUT_KEY_CAMERA_LEFT, 0 },
    { INPUT_KEY_CAMERA_RIGHT, 0 },
    { INPUT_KEY_CAMERA_RESET, 0 },
    // right column
    { INPUT_KEY_SLOW, 1 },
    { INPUT_KEY_JUMP, 1 },
    { INPUT_KEY_ACTION, 1 },
    { INPUT_KEY_DRAW, 1 },
    { INPUT_KEY_LOOK, 1 },
    { INPUT_KEY_ROLL, 1 },
    { INPUT_KEY_OPTION, 1 },
    { INPUT_KEY_PAUSE, 1 },
    { INPUT_KEY_FLY_CHEAT, 1 },
    { INPUT_KEY_ITEM_CHEAT, 1 },
    { INPUT_KEY_LEVEL_SKIP_CHEAT, 1 },
    // end
    { -1, -1 },
};

static char NewGameStrings[MAX_GAME_MODES][MAX_GAME_MODE_LENGTH] = { 0 };
static REQUEST_INFO NewGameRequester = {
    .items = MAX_GAME_MODES,
    .requested = 0,
    .vis_lines = MAX_GAME_MODES,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 162,
    .line_height = TEXT_HEIGHT + 7,
    .x = 0,
    .y = 0,
    .z = 0,
    .flags = 0,
    .heading_text = NULL,
    .item_texts = &NewGameStrings[0][0],
    .item_text_len = MAX_GAME_MODE_LENGTH,
    0,
};

static char LoadSaveGameStrings[MAX_SAVE_SLOTS][MAX_LEVEL_NAME_LENGTH] = { 0 };
REQUEST_INFO LoadSaveGameRequester = {
    .items = 1,
    .requested = 0,
    .vis_lines = -1,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 272,
    .line_height = TEXT_HEIGHT + 7,
    .x = 0,
    .y = -32,
    .z = 0,
    .flags = 0,
    .heading_text = NULL,
    .item_texts = &LoadSaveGameStrings[0][0],
    .item_text_len = MAX_LEVEL_NAME_LENGTH,
    0,
};

static void InitLoadSaveGameRequester()
{
    REQUEST_INFO *req = &LoadSaveGameRequester;
    InitRequester(req);
    GetSavedGamesList(req);
    SetRequesterHeading(req, GF.strings[GS_PASSPORT_SELECT_LEVEL]);

    if (Screen_GetResHeightDownscaled() <= 240) {
        req->y = -30;
        req->vis_lines = 5;
    } else if (Screen_GetResHeightDownscaled() <= 384) {
        req->y = -30;
        req->vis_lines = 8;
    } else if (Screen_GetResHeightDownscaled() <= 480) {
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
    req->y = -30 * Screen_GetResHeightDownscaled() / 100;
    req->vis_lines = MAX_GAME_MODES;
}

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
        // not implemented in TombATI
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
        g_InputDB.select = 1;
        break;

    case O_GUN_AMMO_OPTION:
    case O_SG_AMMO_OPTION:
    case O_MAG_AMMO_OPTION:
    case O_UZI_AMMO_OPTION:
        break;

    default:
        if (g_InputDB.deselect || g_InputDB.select) {
            inv_item->goal_frame = 0;
            inv_item->anim_direction = -1;
        }
        break;
    }
}

void DoPassportOption(INVENTORY_ITEM *inv_item)
{
    Text_Remove(InvItemText[0]);
    InvItemText[IT_NAME] = NULL;

    int16_t page = (inv_item->goal_frame - inv_item->open_frame) / 5;
    if ((inv_item->goal_frame - inv_item->open_frame) % 5) {
        page = -1;
    }

    if (InvMode == INV_LOAD_MODE || InvMode == INV_SAVE_MODE
        || InvMode == INV_SAVE_CRYSTAL_MODE) {
        g_InputDB.left = 0;
        g_InputDB.right = 0;
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
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                }
                PassportMode = 0;
            } else {
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            }
        } else if (PassportMode == 0) {
            if (!SavedGamesCount || InvMode == INV_SAVE_MODE
                || InvMode == INV_SAVE_CRYSTAL_MODE) {
                g_InputDB = (INPUT_STATE) { 0, .right = 1 };
            } else {
                if (!PassportText) {
                    PassportText =
                        Text_Create(0, -16, GF.strings[GS_PASSPORT_LOAD_GAME]);
                    Text_AlignBottom(PassportText, 1);
                    Text_CentreH(PassportText, 1);
                }
                if (g_InputDB.select || InvMode == INV_LOAD_MODE) {
                    Text_Remove(InvRingText);
                    InvRingText = NULL;
                    Text_Remove(InvItemText[IT_NAME]);
                    InvItemText[IT_NAME] = NULL;

                    LoadSaveGameRequester.flags |= RIF_BLOCKABLE;
                    InitLoadSaveGameRequester();
                    PassportMode = 1;
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
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
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                }
                PassportMode = 0;
            } else {
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
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
                        g_Input = (INPUT_STATE) { 0 };
                        g_InputDB = (INPUT_STATE) { 0 };
                    }
                    PassportMode = 0;
                }
            } else {
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            }
        } else if (PassportMode == 0) {
            if (InvMode == INV_DEATH_MODE) {
                if (inv_item->anim_direction == -1) {
                    g_InputDB = (INPUT_STATE) { 0, .left = 1 };
                } else {
                    g_InputDB = (INPUT_STATE) { 0, .right = 1 };
                }
            }
            if (!PassportText) {
                if (InvMode == INV_TITLE_MODE
                    || CurrentLevel == GF.gym_level_num) {
                    PassportText =
                        Text_Create(0, -16, GF.strings[GS_PASSPORT_NEW_GAME]);
                } else {
                    PassportText =
                        Text_Create(0, -16, GF.strings[GS_PASSPORT_SAVE_GAME]);
                }
                Text_AlignBottom(PassportText, 1);
                Text_CentreH(PassportText, 1);
            }
            if (g_InputDB.select || InvMode == INV_SAVE_MODE
                || InvMode == INV_SAVE_CRYSTAL_MODE) {
                if (InvMode == INV_TITLE_MODE
                    || CurrentLevel == GF.gym_level_num) {
                    Text_Remove(InvRingText);
                    InvRingText = NULL;
                    Text_Remove(InvItemText[IT_NAME]);
                    InvItemText[IT_NAME] = NULL;

                    if (GF.enable_game_modes) {
                        InitNewGameRequester();
                        PassportMode = 2;
                        g_Input = (INPUT_STATE) { 0 };
                        g_InputDB = (INPUT_STATE) { 0 };
                    } else {
                        InvExtraData[1] = SaveGame.bonus_flag;
                    }
                } else {
                    Text_Remove(InvRingText);
                    InvRingText = NULL;
                    Text_Remove(InvItemText[IT_NAME]);
                    InvItemText[IT_NAME] = NULL;

                    LoadSaveGameRequester.flags &= ~RIF_BLOCKABLE;
                    InitLoadSaveGameRequester();
                    PassportMode = 1;
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                }
            }
        }
        break;

    case 2:
        if (!PassportText) {
            if (InvMode == INV_TITLE_MODE) {
                PassportText =
                    Text_Create(0, -16, GF.strings[GS_PASSPORT_EXIT_GAME]);
            } else {
                PassportText =
                    Text_Create(0, -16, GF.strings[GS_PASSPORT_EXIT_TO_TITLE]);
            }
            Text_AlignBottom(PassportText, 1);
            Text_CentreH(PassportText, 1);
        }
        break;
    }

    bool pages_available[3] = {
        SavedGamesCount > 0,
        InvMode == INV_TITLE_MODE || InvMode == INV_SAVE_CRYSTAL_MODE
            || !GF.enable_save_crystals,
        true,
    };

    if (g_InputDB.left && (InvMode != INV_DEATH_MODE || SavedGamesCount)) {
        while (--page >= 0) {
            if (pages_available[page]) {
                break;
            }
        }

        if (page >= 0) {
            inv_item->anim_direction = -1;
            inv_item->goal_frame = inv_item->open_frame + 5 * page;
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            if (PassportText) {
                Text_Remove(PassportText);
                PassportText = NULL;
            }
        }

        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
    }

    if (g_InputDB.right) {
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };

        while (++page < PASSPORT_PAGE_COUNT) {
            if (pages_available[page]) {
                break;
            }
        }

        if (page < PASSPORT_PAGE_COUNT) {
            inv_item->anim_direction = 1;
            inv_item->goal_frame = inv_item->open_frame + 5 * page;
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            if (PassportText) {
                Text_Remove(PassportText);
                PassportText = NULL;
            }
        }
    }

    if (g_InputDB.deselect) {
        if (InvMode == INV_DEATH_MODE) {
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
        } else {
            if (page == 2) {
                inv_item->anim_direction = 1;
                inv_item->goal_frame = inv_item->frames_total - 1;
            } else {
                inv_item->goal_frame = 0;
                inv_item->anim_direction = -1;
            }
            if (PassportText) {
                Text_Remove(PassportText);
                PassportText = NULL;
            }
        }
    }

    if (g_InputDB.select) {
        InvExtraData[0] = page;
        if (page == 2) {
            inv_item->anim_direction = 1;
            inv_item->goal_frame = inv_item->frames_total - 1;
        } else {
            inv_item->goal_frame = 0;
            inv_item->anim_direction = -1;
        }
        if (PassportText) {
            Text_Remove(PassportText);
            PassportText = NULL;
        }
    }
}

void DoDetailOption(INVENTORY_ITEM *inv_item)
{
    char buf[256];

    if (!DetailTextHW[DETAIL_HW_TITLE_BORDER]) {
        int32_t y = DETAIL_HW_TOP_Y;
        DetailTextHW[DETAIL_HW_TITLE_BORDER] = Text_Create(0, y - 2, " ");

        DetailTextHW[DETAIL_HW_TITLE] =
            Text_Create(0, y, GF.strings[GS_DETAIL_SELECT_DETAIL]);
        y += DETAIL_HW_ROW_HEIGHT;

        sprintf(
            buf, GF.strings[GS_DETAIL_PERSPECTIVE_FMT],
            GF.strings
                [T1MConfig.render_flags.perspective ? GS_MISC_ON
                                                    : GS_MISC_OFF]);
        DetailTextHW[DETAIL_HW_PERSPECTIVE] = Text_Create(0, y, buf);
        y += DETAIL_HW_ROW_HEIGHT;

        sprintf(
            buf, GF.strings[GS_DETAIL_BILINEAR_FMT],
            GF.strings
                [T1MConfig.render_flags.bilinear ? GS_MISC_ON : GS_MISC_OFF]);
        DetailTextHW[DETAIL_HW_BILINEAR] = Text_Create(0, y, buf);
        y += DETAIL_HW_ROW_HEIGHT;

        sprintf(
            buf, GF.strings[GS_DETAIL_BRIGHTNESS_FMT], T1MConfig.brightness);
        DetailTextHW[DETAIL_HW_BRIGHTNESS] = Text_Create(0, y, buf);
        y += DETAIL_HW_ROW_HEIGHT;

        sprintf(
            buf, GF.strings[GS_DETAIL_UI_TEXT_SCALE_FMT],
            T1MConfig.ui.text_scale);
        DetailTextHW[DETAIL_HW_UI_TEXT_SCALE] = Text_Create(0, y, buf);
        y += DETAIL_HW_ROW_HEIGHT;

        sprintf(
            buf, GF.strings[GS_DETAIL_UI_BAR_SCALE_FMT],
            T1MConfig.ui.bar_scale);
        DetailTextHW[DETAIL_HW_UI_BAR_SCALE] = Text_Create(0, y, buf);
        y += DETAIL_HW_ROW_HEIGHT;

        char tmp[10];
        sprintf(
            tmp, "%dx%d", Screen_GetGameResWidth(), Screen_GetGameResHeight());
        sprintf(buf, GF.strings[GS_DETAIL_VIDEO_MODE_FMT], tmp);
        DetailTextHW[DETAIL_HW_RESOLUTION] = Text_Create(0, y, buf);
        y += DETAIL_HW_ROW_HEIGHT;

        if (OptionSelected < DETAIL_HW_OPTION_MIN) {
            OptionSelected = DETAIL_HW_OPTION_MIN;
        }
        if (OptionSelected > DETAIL_HW_OPTION_MAX) {
            OptionSelected = DETAIL_HW_OPTION_MAX;
        }

        Text_AddBackground(
            DetailTextHW[DETAIL_HW_TITLE_BORDER], DETAIL_HW_ROW_WIDHT,
            y - DETAIL_HW_TOP_Y, 0, 0);
        Text_AddOutline(DetailTextHW[DETAIL_HW_TITLE_BORDER], 1);

        Text_AddBackground(
            DetailTextHW[DETAIL_HW_TITLE], DETAIL_HW_ROW_WIDHT - 4, 0, 0, 0);
        Text_AddOutline(DetailTextHW[DETAIL_HW_TITLE], 1);

        Text_AddBackground(
            DetailTextHW[OptionSelected], DETAIL_HW_ROW_WIDHT - 12, 0, 0, 0);
        Text_AddOutline(DetailTextHW[OptionSelected], 1);

        for (int i = 0; i < DETAIL_HW_NUMBER_OF; i++) {
            Text_CentreH(DetailTextHW[i], 1);
            Text_CentreV(DetailTextHW[i], 1);
        }
    }

    if (g_InputDB.forward && OptionSelected > DETAIL_HW_OPTION_MIN) {
        Text_RemoveOutline(DetailTextHW[OptionSelected]);
        Text_RemoveBackground(DetailTextHW[OptionSelected]);
        OptionSelected--;
        Text_AddOutline(DetailTextHW[OptionSelected], 1);
        Text_AddBackground(
            DetailTextHW[OptionSelected], DETAIL_HW_ROW_WIDHT - 12, 0, 0, 0);
    }

    if (g_InputDB.back && OptionSelected < DETAIL_HW_OPTION_MAX) {
        Text_RemoveOutline(DetailTextHW[OptionSelected]);
        Text_RemoveBackground(DetailTextHW[OptionSelected]);
        OptionSelected++;
        Text_AddOutline(DetailTextHW[OptionSelected], 1);
        Text_AddBackground(
            DetailTextHW[OptionSelected], DETAIL_HW_ROW_WIDHT - 12, 0, 0, 0);
    }

    bool reset = false;

    if (g_InputDB.right) {
        switch (OptionSelected) {
        case DETAIL_HW_PERSPECTIVE:
            if (!T1MConfig.render_flags.perspective) {
                T1MConfig.render_flags.perspective = 1;
                reset = true;
            }
            break;

        case DETAIL_HW_BILINEAR:
            if (!T1MConfig.render_flags.bilinear) {
                T1MConfig.render_flags.bilinear = 1;
                reset = true;
            }
            break;

        case DETAIL_HW_BRIGHTNESS:
            if (T1MConfig.brightness < MAX_BRIGHTNESS) {
                T1MConfig.brightness += 0.1f;
                reset = 1;
            }
            break;

        case DETAIL_HW_UI_TEXT_SCALE:
            if (T1MConfig.ui.text_scale < MAX_UI_SCALE) {
                T1MConfig.ui.text_scale += 0.1;
                reset = true;
            }
            break;

        case DETAIL_HW_UI_BAR_SCALE:
            if (T1MConfig.ui.bar_scale < MAX_UI_SCALE) {
                T1MConfig.ui.bar_scale += 0.1;
                reset = true;
            }
            break;

        case DETAIL_HW_RESOLUTION:
            if (Screen_SetNextGameRes()) {
                reset = true;
            }
            break;
        }
    }

    if (g_InputDB.left) {
        switch (OptionSelected) {
        case DETAIL_HW_PERSPECTIVE:
            if (T1MConfig.render_flags.perspective) {
                T1MConfig.render_flags.perspective = 0;
                reset = true;
            }
            break;

        case DETAIL_HW_BILINEAR:
            if (T1MConfig.render_flags.bilinear) {
                T1MConfig.render_flags.bilinear = 0;
                reset = true;
            }
            break;

        case DETAIL_HW_BRIGHTNESS:
            if (T1MConfig.brightness > MIN_BRIGHTNESS) {
                T1MConfig.brightness -= 0.1f;
                reset = 1;
            }
            break;

        case DETAIL_HW_UI_TEXT_SCALE:
            if (T1MConfig.ui.text_scale > MIN_UI_SCALE) {
                T1MConfig.ui.text_scale -= 0.1;
                reset = true;
            }
            break;

        case DETAIL_HW_UI_BAR_SCALE:
            if (T1MConfig.ui.bar_scale > MIN_UI_SCALE) {
                T1MConfig.ui.bar_scale -= 0.1;
                reset = true;
            }
            break;

        case DETAIL_HW_RESOLUTION:
            if (Screen_SetPrevGameRes()) {
                reset = true;
            }
            break;
        }
    }

    if (g_InputDB.deselect || g_InputDB.select) {
        reset = true;
    }

    if (reset) {
        for (int i = 0; i < DETAIL_HW_NUMBER_OF; i++) {
            Text_Remove(DetailTextHW[i]);
            DetailTextHW[i] = NULL;
        }
        S_WriteUserSettings();
    }
}

void DoSoundOption(INVENTORY_ITEM *inv_item)
{
    char buf[20];

    if (!SoundText[0]) {
        if (T1MConfig.music_volume > 10) {
            T1MConfig.music_volume = 10;
        }
        sprintf(buf, "| %2d", T1MConfig.music_volume);
        SoundText[SOUND_MUSIC_VOLUME] = Text_Create(0, 0, buf);

        if (T1MConfig.sound_volume > 10) {
            T1MConfig.sound_volume = 10;
        }
        sprintf(buf, "} %2d", T1MConfig.sound_volume);
        SoundText[SOUND_SOUND_VOLUME] = Text_Create(0, 25, buf);

        SoundText[SOUND_TITLE] =
            Text_Create(0, -30, GF.strings[GS_SOUND_SET_VOLUMES]);
        SoundText[SOUND_TITLE_BORDER] = Text_Create(0, -32, " ");

        Text_AddBackground(SoundText[OptionSelected], 128, 0, 0, 0);
        Text_AddOutline(SoundText[OptionSelected], 1);
        Text_AddBackground(SoundText[SOUND_TITLE], 136, 0, 0, 0);
        Text_AddOutline(SoundText[SOUND_TITLE], 1);
        Text_AddBackground(SoundText[SOUND_TITLE_BORDER], 140, 85, 0, 0);
        Text_AddOutline(SoundText[SOUND_TITLE_BORDER], 1);

        for (int i = 0; i < SOUND_NUMBER_OF; i++) {
            Text_CentreH(SoundText[i], 1);
            Text_CentreV(SoundText[i], 1);
        }
    }

    if (g_InputDB.forward && OptionSelected > SOUND_OPTION_MIN) {
        Text_RemoveOutline(SoundText[OptionSelected]);
        Text_RemoveBackground(SoundText[OptionSelected]);
        Text_AddBackground(SoundText[--OptionSelected], 128, 0, 0, 0);
        Text_AddOutline(SoundText[OptionSelected], 1);
    }

    if (g_InputDB.back && OptionSelected < SOUND_OPTION_MAX) {
        Text_RemoveOutline(SoundText[OptionSelected]);
        Text_RemoveBackground(SoundText[OptionSelected]);
        Text_AddBackground(SoundText[++OptionSelected], 128, 0, 0, 0);
        Text_AddOutline(SoundText[OptionSelected], 1);
    }

    switch (OptionSelected) {
    case SOUND_MUSIC_VOLUME:
        if (g_Input.left && T1MConfig.music_volume > 0) {
            T1MConfig.music_volume--;
            IDelay = true;
            IDCount = 10;
            sprintf(buf, "| %2d", T1MConfig.music_volume);
            Text_ChangeText(SoundText[SOUND_MUSIC_VOLUME], buf);
            S_WriteUserSettings();
        } else if (g_Input.right && T1MConfig.music_volume < 10) {
            T1MConfig.music_volume++;
            IDelay = true;
            IDCount = 10;
            sprintf(buf, "| %2d", T1MConfig.music_volume);
            Text_ChangeText(SoundText[SOUND_MUSIC_VOLUME], buf);
            S_WriteUserSettings();
        }

        if (g_Input.left || g_Input.right) {
            Music_SetVolume(T1MConfig.music_volume);
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
        }
        break;

    case SOUND_SOUND_VOLUME:
        if (g_Input.left && T1MConfig.sound_volume > 0) {
            T1MConfig.sound_volume--;
            IDelay = true;
            IDCount = 10;
            sprintf(buf, "} %2d", T1MConfig.sound_volume);
            Text_ChangeText(SoundText[SOUND_SOUND_VOLUME], buf);
            S_WriteUserSettings();
        } else if (g_Input.right && T1MConfig.sound_volume < 10) {
            T1MConfig.sound_volume++;
            IDelay = true;
            IDCount = 10;
            sprintf(buf, "} %2d", T1MConfig.sound_volume);
            Text_ChangeText(SoundText[SOUND_SOUND_VOLUME], buf);
            S_WriteUserSettings();
        }

        if (g_Input.left || g_Input.right) {
            Sound_SetMasterVolume(T1MConfig.sound_volume);
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
        }
        break;
    }

    if (g_InputDB.deselect || g_InputDB.select) {
        for (int i = 0; i < SOUND_NUMBER_OF; i++) {
            Text_Remove(SoundText[i]);
            SoundText[i] = NULL;
        }
    }
}

void DoCompassOption(INVENTORY_ITEM *inv_item)
{
    char buf[100];
    char time_buf[100];

    if (T1MConfig.enable_compass_stats) {
        if (!CompassText[0]) {
            int32_t y = COMPASS_TOP_Y;

            CompassText[COMPASS_TITLE_BORDER] = Text_Create(0, y - 2, " ");

            sprintf(buf, "%s", GF.levels[CurrentLevel].level_title);
            CompassText[COMPASS_TITLE] = Text_Create(0, y, buf);
            y += COMPASS_ROW_HEIGHT;

            CompassText[COMPASS_TIME] = Text_Create(0, y, " ");
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
            CompassText[COMPASS_SECRETS] = Text_Create(0, y, buf);
            y += COMPASS_ROW_HEIGHT;

            sprintf(buf, GF.strings[GS_STATS_PICKUPS_FMT], SaveGame.pickups);
            CompassText[COMPASS_PICKUPS] = Text_Create(0, y, buf);
            y += COMPASS_ROW_HEIGHT;

            sprintf(buf, GF.strings[GS_STATS_KILLS_FMT], SaveGame.kills);
            CompassText[COMPASS_KILLS] = Text_Create(0, y, buf);
            y += COMPASS_ROW_HEIGHT;

            Text_AddBackground(
                CompassText[COMPASS_TITLE_BORDER], COMPASS_ROW_WIDTH,
                y - COMPASS_TOP_Y, 0, 0);
            Text_AddOutline(CompassText[COMPASS_TITLE_BORDER], 1);
            Text_AddBackground(
                CompassText[COMPASS_TITLE], COMPASS_ROW_WIDTH - 4, 0, 0, 0);
            Text_AddOutline(CompassText[COMPASS_TITLE], 1);

            for (int i = 0; i < COMPASS_NUMBER_OF; i++) {
                Text_CentreH(CompassText[i], 1);
                Text_CentreV(CompassText[i], 1);
            }
        }

        int32_t seconds = SaveGame.timer / 30;
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
        Text_ChangeText(CompassText[COMPASS_TIME], buf);
    }

    if (g_InputDB.deselect || g_InputDB.select) {
        for (int i = 0; i < COMPASS_NUMBER_OF; i++) {
            Text_Remove(CompassText[i]);
            CompassText[i] = NULL;
        }
        inv_item->goal_frame = inv_item->frames_total - 1;
        inv_item->anim_direction = 1;
    }
}

void FlashConflicts()
{
    const TEXT_COLUMN_PLACEMENT *cols = T1MConfig.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    for (const TEXT_COLUMN_PLACEMENT *item = cols; item->col_num != -1;
         item++) {
        Text_Flash(CtrlTextB[item->option], 0, 0);
    }

    for (const TEXT_COLUMN_PLACEMENT *item1 = cols; item1->col_num != -1;
         item1++) {
        if (item1->option == -1) {
            continue;
        }
        S_INPUT_KEYCODE key_code1 =
            S_Input_GetAssignedKeyCode(T1MConfig.input.layout, item1->option);
        for (const TEXT_COLUMN_PLACEMENT *item2 = item1 + 1;
             item2->col_num != -1; item2++) {
            if (item2->option == -1) {
                continue;
            }
            S_INPUT_KEYCODE key_code2 = S_Input_GetAssignedKeyCode(
                T1MConfig.input.layout, item2->option);
            if (item1 != item2 && key_code1 == key_code2) {
                Text_Flash(CtrlTextB[item1->option], 1, 20);
                Text_Flash(CtrlTextB[item2->option], 1, 20);
            }
        }
    }
}

void DefaultConflict()
{
    for (int i = 0; i < INPUT_KEY_NUMBER_OF; i++) {
        S_INPUT_KEYCODE key_code =
            S_Input_GetAssignedKeyCode(INPUT_LAYOUT_DEFAULT, i);
        S_Input_SetKeyAsConflicted(i, false);
        for (int j = 0; j < INPUT_KEY_NUMBER_OF; j++) {
            if (key_code == S_Input_GetAssignedKeyCode(INPUT_LAYOUT_USER, j)) {
                S_Input_SetKeyAsConflicted(i, true);
                break;
            }
        }
    }
}

void DoControlOption(INVENTORY_ITEM *inv_item)
{
    if (!CtrlText[0]) {
        CtrlText[0] = Text_Create(
            0,
            CONTROLS_TOP_Y - CONTROLS_BORDER
                + (CONTROLS_HEADER_HEIGHT + CONTROLS_BORDER
                   - CONTROLS_ROW_HEIGHT)
                    / 2,
            GF.strings
                [T1MConfig.input.layout ? GS_CONTROL_USER_KEYS
                                        : GS_CONTROL_DEFAULT_KEYS]);
        Text_CentreH(CtrlText[0], 1);
        Text_CentreV(CtrlText[0], 1);
        S_ShowControls();

        KeyChange = -1;
        Text_AddBackground(CtrlText[0], 0, 0, 0, 0);
        Text_AddOutline(CtrlText[0], 1);
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
        if (g_InputDB.left || g_InputDB.right) {
            if (KeyChange == -1) {
                T1MConfig.input.layout ^= 1;
                S_ChangeCtrlText();
                FlashConflicts();
                S_WriteUserSettings();
            } else {
                Text_RemoveBackground(CtrlTextA[KeyChange]);
                Text_RemoveOutline(CtrlTextA[KeyChange]);

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

                Text_AddBackground(CtrlTextA[KeyChange], 0, 0, 0, 0);
                Text_AddOutline(CtrlTextA[KeyChange], 1);
            }
        } else if (
            g_InputDB.deselect || (g_InputDB.select && KeyChange == -1)) {
            S_RemoveCtrl();
            DefaultConflict();
            return;
        }

        if (T1MConfig.input.layout) {
            if (g_InputDB.select) {
                KeyMode = 1;
                Text_RemoveBackground(CtrlTextA[KeyChange]);
                Text_AddBackground(CtrlTextB[KeyChange], 0, 0, 0, 0);
                Text_RemoveOutline(CtrlTextA[KeyChange]);
                Text_AddOutline(CtrlTextB[KeyChange], 1);
            } else if (g_InputDB.forward) {
                Text_RemoveBackground(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange]);
                Text_RemoveOutline(
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

                Text_AddBackground(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange], 0, 0,
                    0, 0);
                Text_AddOutline(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange], 1);
            } else if (g_InputDB.back) {
                Text_RemoveBackground(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange]);
                Text_RemoveOutline(
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

                Text_AddBackground(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange], 0, 0,
                    0, 0);
                Text_AddOutline(
                    KeyChange == -1 ? CtrlText[0] : CtrlTextA[KeyChange], 1);
            }
        }
        break;

    case 1:
        if (!g_Input.select) {
            KeyMode = 2;
        }
        break;

    case 2: {
        S_INPUT_KEYCODE key_code = S_Input_ReadKeyCode();

        const char *scancode_name = S_Input_GetKeyCodeName(key_code);
        if (key_code >= 0 && scancode_name) {
            S_Input_AssignKeyCode(T1MConfig.input.layout, KeyChange, key_code);
            Text_ChangeText(CtrlTextB[KeyChange], scancode_name);
            Text_RemoveBackground(CtrlTextB[KeyChange]);
            Text_RemoveOutline(CtrlTextB[KeyChange]);
            Text_AddBackground(CtrlTextA[KeyChange], 0, 0, 0, 0);
            Text_AddOutline(CtrlTextA[KeyChange], 1);
            KeyMode = 3;
            FlashConflicts();
            S_WriteUserSettings();
        }
        break;
    }

    case 3: {
        S_INPUT_KEYCODE key_code =
            S_Input_GetAssignedKeyCode(T1MConfig.input.layout, KeyChange);

        if (S_Input_ReadKeyCode() < 0 || S_Input_ReadKeyCode() != key_code) {
            KeyMode = 0;
            FlashConflicts();
            S_WriteUserSettings();
        }

        KeyMode = 0;
        break;
    }
    }

    g_Input = (INPUT_STATE) { 0 };
    g_InputDB = (INPUT_STATE) { 0 };
}

void S_ShowControls()
{
    const int16_t centre = Screen_GetResWidthDownscaled() / 2;
    int16_t max_y = 0;

    CtrlText[1] = Text_Create(0, CONTROLS_TOP_Y - CONTROLS_BORDER, " ");
    Text_CentreH(CtrlText[1], 1);
    Text_CentreV(CtrlText[1], 1);

    const TEXT_COLUMN_PLACEMENT *cols = T1MConfig.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    if (!CtrlTextB[0]) {
        int16_t xs[2] = { centre - 200, centre + 20 };
        int16_t ys[2] = { CONTROLS_TOP_Y + CONTROLS_HEADER_HEIGHT,
                          CONTROLS_TOP_Y + CONTROLS_HEADER_HEIGHT };

        for (const TEXT_COLUMN_PLACEMENT *col = cols;
             col->col_num >= 0 && col->col_num <= 1; col++) {
            int16_t x = xs[col->col_num];
            int16_t y = ys[col->col_num];

            const char *scancode_name =
                S_Input_GetKeyCodeName(S_Input_GetAssignedKeyCode(
                    T1MConfig.input.layout, col->option));
            if (col->option != -1 && scancode_name) {
                CtrlTextB[col->option] = Text_Create(x, y, scancode_name);
                Text_CentreV(CtrlTextB[col->option], 1);
            }

            ys[col->col_num] += CONTROLS_ROW_HEIGHT;
            max_y = MAX(max_y, ys[col->col_num]);
        }

        KeyChange = 0;
    }

    if (!CtrlTextA[0]) {
        int16_t xs[2] = { centre - 130, centre + 90 };
        int16_t ys[2] = { CONTROLS_TOP_Y + CONTROLS_HEADER_HEIGHT,
                          CONTROLS_TOP_Y + CONTROLS_HEADER_HEIGHT };

        for (const TEXT_COLUMN_PLACEMENT *col = cols;
             col->col_num >= 0 && col->col_num <= 1; col++) {
            int16_t x = xs[col->col_num];
            int16_t y = ys[col->col_num];

            if (col->option != -1) {
                CtrlTextA[col->option] = Text_Create(
                    x, y,
                    GF.strings[col->option + GS_KEYMAP_RUN - INPUT_KEY_UP]);
                Text_CentreV(CtrlTextA[col->option], 1);
            }

            ys[col->col_num] += CONTROLS_ROW_HEIGHT;
            max_y = MAX(max_y, ys[col->col_num]);
        }
    }

    int16_t width = 420;
    int16_t height = max_y + CONTROLS_BORDER * 2 - CONTROLS_TOP_Y;
    Text_AddBackground(CtrlText[1], width, height, 0, 0);
    Text_AddOutline(CtrlText[1], 1);

    FlashConflicts();
}

void S_ChangeCtrlText()
{
    Text_ChangeText(
        CtrlText[0],
        GF.strings
            [T1MConfig.input.layout ? GS_CONTROL_USER_KEYS
                                    : GS_CONTROL_DEFAULT_KEYS]);

    const TEXT_COLUMN_PLACEMENT *cols = T1MConfig.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    for (const TEXT_COLUMN_PLACEMENT *col = cols;
         col->col_num >= 0 && col->col_num <= 1; col++) {
        const char *scancode_name = S_Input_GetKeyCodeName(
            S_Input_GetAssignedKeyCode(T1MConfig.input.layout, col->option));
        if (col->option != -1 && scancode_name) {
            Text_ChangeText(CtrlTextB[col->option], scancode_name);
        }
    }
}

void S_RemoveCtrlText()
{
    for (int i = 0; i < INPUT_KEY_NUMBER_OF; i++) {
        Text_Remove(CtrlTextA[i]);
        Text_Remove(CtrlTextB[i]);
        CtrlTextB[i] = NULL;
        CtrlTextA[i] = NULL;
    }
}

void S_RemoveCtrl()
{
    Text_Remove(CtrlText[0]);
    Text_Remove(CtrlText[1]);
    CtrlText[0] = NULL;
    CtrlText[1] = NULL;
    S_RemoveCtrlText();
}
