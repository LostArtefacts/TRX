#include "game/option/option_passport.h"

#include "config.h"
#include "game/game_string.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inventory/inventory_vars.h"
#include "game/requester.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/memory.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_GAME_MODES 4

typedef enum PASSPORT_TEXT {
    TEXT_PAGE_NAME = 0,
    TEXT_LEFT_ARROW = 1,
    TEXT_RIGHT_ARROW = 2,
    TEXT_LEVEL_ARROW_RIGHT = 3,
    TEXT_LEVEL_ARROW_LEFT = 4,
    TEXT_NUMBER_OF = 5,
} PASSPORT_TEXT;

typedef struct PASSPORT_STATUS {
    PASSPORT_PAGE page;
    PASSPORT_MODE mode;
    bool page_available[PAGE_COUNT];
    PASSPORT_MODE page_role[PAGE_COUNT];
} PASSPORT_STATUS;

static PASSPORT_STATUS m_PassportStatus = {
    .page = PAGE_1,
    .mode = PASSPORT_MODE_BROWSE,
    .page_available = { false, true, true },
    .page_role = { PASSPORT_MODE_UNAVAILABLE, PASSPORT_MODE_NEW_GAME,
                   PASSPORT_MODE_EXIT_TITLE },
};

static bool m_IsTextInit = false;
static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = { 0 };

static REQUEST_INFO m_NewGameRequester = {
    .items_used = 0,
    .max_items = MAX_GAME_MODES,
    .requested = 0,
    .vis_lines = MAX_GAME_MODES,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 162,
    .line_height = TEXT_HEIGHT + 7,
    .is_blockable = false,
    .x = 0,
    .y = 0,
    .heading_text = NULL,
    .items = NULL,
};

static REQUEST_INFO m_SelectLevelRequester = {
    .items_used = 0,
    .max_items = 2,
    .requested = 0,
    .vis_lines = -1,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 292,
    .line_height = TEXT_HEIGHT + 7,
    .is_blockable = false,
    .x = 0,
    .y = -32,
    .heading_text = NULL,
    .items = NULL,
};

REQUEST_INFO g_SavegameRequester = {
    .items_used = 0,
    .max_items = 1,
    .requested = 0,
    .vis_lines = -1,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 292,
    .line_height = TEXT_HEIGHT + 7,
    .is_blockable = false,
    .x = 0,
    .y = -32,
    .heading_text = NULL,
    .items = NULL,
};

static void Option_PassportInitText(void);
static void Option_PassportShutdownText(void);
static void Option_PassportClose(INVENTORY_ITEM *inv_item);
static void Option_PassportDeterminePages(void);
static void Option_PassportInitSaveRequester(int16_t page_num);
static void Option_PassportInitSelectLevelRequester(void);
static void Option_PassportInitNewGameRequester(void);
static void Option_PassportShowSaves(PASSPORT_MODE pending_mode);
static void Option_PassportShowSelectLevel(void);
static void Option_PassportLoadGame(void);
static void Option_PassportSelectLevel(void);
static void Option_PassportSaveGame(void);
static void Option_PassportNewGame(void);
static void Option_PassportRestart(INVENTORY_ITEM *inv_item);
static void Option_PassportFlipRight(INVENTORY_ITEM *inv_item);
static void Option_PassportFlipLeft(INVENTORY_ITEM *inv_item);

void Option_PassportInit(void)
{
    Requester_Init(&g_SavegameRequester, g_Config.maximum_save_slots);
    Requester_Init(&m_SelectLevelRequester, g_GameFlow.level_count + 1);
    Requester_Init(&m_NewGameRequester, MAX_GAME_MODES);
}

void Option_PassportShutdown(void)
{
    Requester_Shutdown(&g_SavegameRequester);
    Requester_Shutdown(&m_SelectLevelRequester);
    Requester_Shutdown(&m_NewGameRequester);
}

static void Option_PassportInitText(void)
{
    m_Text[TEXT_LEFT_ARROW] = Text_Create(-85, -15, "\200");
    Text_Hide(m_Text[TEXT_LEFT_ARROW], true);

    m_Text[TEXT_RIGHT_ARROW] = Text_Create(70, -15, "\201");
    Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);

    m_Text[TEXT_LEVEL_ARROW_LEFT] = Text_Create(0, 0, "\200");
    Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], true);

    m_Text[TEXT_LEVEL_ARROW_RIGHT] = Text_Create(0, 0, "\201");
    Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);

    m_Text[TEXT_PAGE_NAME] = Text_Create(0, -16, "");

    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_AlignBottom(m_Text[i], 1);
        Text_CentreH(m_Text[i], 1);
    }
}

static void Option_PassportShutdownText(void)
{
    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_Remove(m_Text[i]);
        m_Text[i] = NULL;
    }
    m_IsTextInit = false;
}

static void Option_PassportClose(INVENTORY_ITEM *inv_item)
{
    if (m_PassportStatus.page == PAGE_3) {
        inv_item->anim_direction = 1;
        inv_item->goal_frame = inv_item->frames_total - 1;
    } else {
        inv_item->anim_direction = -1;
        inv_item->goal_frame = 0;
    }
    Option_PassportShutdownText();
}

static void Option_PassportDeterminePages(void)
{
    switch (g_InvMode) {
    case INV_TITLE_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        m_PassportStatus.page_available[PAGE_1] = g_SavedGamesCount > 0;
        m_PassportStatus.page_available[PAGE_2] = true;
        m_PassportStatus.page_available[PAGE_3] = true;
        m_PassportStatus.page_role[PAGE_1] = PASSPORT_MODE_LOAD_GAME;
        m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_NEW_GAME;
        m_PassportStatus.page_role[PAGE_3] = PASSPORT_MODE_EXIT_GAME;
        break;

    case INV_GAME_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        m_PassportStatus.page_available[PAGE_1] = g_SavedGamesCount > 0;
        m_PassportStatus.page_available[PAGE_2] = true;
        m_PassportStatus.page_available[PAGE_3] = true;
        m_PassportStatus.page_role[PAGE_1] = PASSPORT_MODE_LOAD_GAME;
        m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_SAVE_GAME;
        m_PassportStatus.page_role[PAGE_3] = PASSPORT_MODE_EXIT_TITLE;
        if (g_CurrentLevel == g_GameFlow.gym_level_num) {
            m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_NEW_GAME;
        } else if (g_Config.enable_save_crystals) {
            m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_RESTART;
        }
        break;

    case INV_LOAD_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_LOAD_GAME;
        m_PassportStatus.page_available[PAGE_1] = true;
        m_PassportStatus.page_available[PAGE_2] = false;
        m_PassportStatus.page_available[PAGE_3] = false;
        m_PassportStatus.page_role[PAGE_1] = PASSPORT_MODE_LOAD_GAME;
        m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_UNAVAILABLE;
        m_PassportStatus.page_role[PAGE_3] = PASSPORT_MODE_UNAVAILABLE;
        Option_PassportInitSaveRequester(PAGE_1);
        break;

    case INV_SAVE_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_SAVE_GAME;
        m_PassportStatus.page_available[PAGE_1] = false;
        m_PassportStatus.page_available[PAGE_2] = true;
        m_PassportStatus.page_available[PAGE_3] = false;
        m_PassportStatus.page_role[PAGE_1] = PASSPORT_MODE_UNAVAILABLE;
        m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_SAVE_GAME;
        m_PassportStatus.page_role[PAGE_3] = PASSPORT_MODE_UNAVAILABLE;
        if (g_CurrentLevel == g_GameFlow.gym_level_num) {
            m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
            m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_NEW_GAME;
        } else if (g_Config.enable_save_crystals) {
            m_PassportStatus.mode = PASSPORT_MODE_RESTART;
            m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_RESTART;
            m_PassportStatus.page_available[PAGE_3] = true;
            m_PassportStatus.page_role[PAGE_3] = PASSPORT_MODE_EXIT_TITLE;
        }
        Option_PassportInitSaveRequester(PAGE_2);
        break;

    case INV_SAVE_CRYSTAL_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_SAVE_GAME;
        m_PassportStatus.page_available[PAGE_1] = false;
        m_PassportStatus.page_available[PAGE_2] = true;
        m_PassportStatus.page_available[PAGE_3] = false;
        m_PassportStatus.page_role[PAGE_1] = PASSPORT_MODE_UNAVAILABLE;
        m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_SAVE_GAME;
        m_PassportStatus.page_role[PAGE_3] = PASSPORT_MODE_UNAVAILABLE;
        Option_PassportInitSaveRequester(PAGE_2);
        break;

    case INV_DEATH_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        m_PassportStatus.page_available[PAGE_1] = g_SavedGamesCount > 0;
        m_PassportStatus.page_available[PAGE_2] = true;
        m_PassportStatus.page_available[PAGE_3] = true;
        m_PassportStatus.page_role[PAGE_1] = PASSPORT_MODE_LOAD_GAME;
        m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_RESTART;
        m_PassportStatus.page_role[PAGE_3] = PASSPORT_MODE_EXIT_TITLE;
        break;

    default:
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        m_PassportStatus.page_available[PAGE_1] = g_SavedGamesCount > 0;
        m_PassportStatus.page_available[PAGE_2] = true;
        m_PassportStatus.page_available[PAGE_3] = true;
        m_PassportStatus.page_role[PAGE_1] = PASSPORT_MODE_LOAD_GAME;
        m_PassportStatus.page_role[PAGE_2] = PASSPORT_MODE_NEW_GAME;
        m_PassportStatus.page_role[PAGE_3] = PASSPORT_MODE_EXIT_TITLE;
        break;
    }
}

static void Option_PassportInitSaveRequester(int16_t page_num)
{
    REQUEST_INFO *req = &g_SavegameRequester;
    Requester_ClearTextstrings(req);
    Requester_SetHeading(
        req,
        page_num == PAGE_1 ? GS(PASSPORT_LOAD_GAME) : GS(PASSPORT_SAVE_GAME));

    if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 240) {
        req->vis_lines = 5;
    } else if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 384) {
        req->vis_lines = 7;
    } else if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 480) {
        req->vis_lines = 10;
    } else {
        req->vis_lines = 12;
    }

    // Title screen passport is at a different pitch.
    if (g_InvMode == INV_TITLE_MODE) {
        req->y = (-Screen_GetResHeightDownscaled(RSR_TEXT) / 2)
            + (req->line_height * req->vis_lines);
    } else {
        req->y = (-Screen_GetResHeightDownscaled(RSR_TEXT) / 1.73)
            + (req->line_height * req->vis_lines);
    }

    Savegame_ScanSavedGames();
}

static void Option_PassportInitSelectLevelRequester(void)
{
    REQUEST_INFO *req = &m_SelectLevelRequester;
    req->is_blockable = true;
    Requester_ClearTextstrings(req);
    Requester_SetHeading(req, GS(PASSPORT_SELECT_LEVEL));

    if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 240) {
        req->vis_lines = 5;
    } else if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 384) {
        req->vis_lines = 7;
    } else if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 480) {
        req->vis_lines = 10;
    } else {
        req->vis_lines = 12;
    }

    // Title screen passport is at a different pitch.
    if (g_InvMode == INV_TITLE_MODE) {
        req->y = (-Screen_GetResHeightDownscaled(RSR_TEXT) / 2)
            + (req->line_height * req->vis_lines);
    } else {
        req->y = (-Screen_GetResHeightDownscaled(RSR_TEXT) / 1.73)
            + (req->line_height * req->vis_lines);
    }

    Savegame_ScanAvailableLevels(req);
}

static void Option_PassportInitNewGameRequester(void)
{
    REQUEST_INFO *req = &m_NewGameRequester;
    Requester_ClearTextstrings(req);
    Requester_SetHeading(req, GS(PASSPORT_SELECT_MODE));
    Requester_AddItem(req, false, "%s", GS(PASSPORT_MODE_NEW_GAME));
    Requester_AddItem(req, false, "%s", GS(PASSPORT_MODE_NEW_GAME_PLUS));
    Requester_AddItem(req, false, "%s", GS(PASSPORT_MODE_NEW_GAME_JP));
    Requester_AddItem(req, false, "%s", GS(PASSPORT_MODE_NEW_GAME_JP_PLUS));
    req->vis_lines = MAX_GAME_MODES;

    req->line_offset = 0;
    req->requested = 0;

    // Title screen passport is at a different pitch.
    if (g_InvMode == INV_TITLE_MODE) {
        req->y = (-Screen_GetResHeightDownscaled(RSR_TEXT) / 2.4)
            + (req->line_height * req->vis_lines + 1);
    } else {
        req->y = (-Screen_GetResHeightDownscaled(RSR_TEXT) / 2)
            + (req->line_height * req->vis_lines);
    }
}

static void Option_PassportShowSaves(PASSPORT_MODE pending_mode)
{
    int32_t select = Requester_Display(&g_SavegameRequester);
    if (select == 0) {
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
    } else if (select > 0) {
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        g_GameInfo.current_save_slot = select - 1;
        g_GameInfo.passport_selection = pending_mode;
    } else if (
        g_InvMode != INV_SAVE_MODE && g_InvMode != INV_SAVE_CRYSTAL_MODE
        && g_InvMode != INV_LOAD_MODE) {
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
    } else {
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
    }
}

static void Option_PassportShowSelectLevel(void)
{
    int32_t select = Requester_Display(&m_SelectLevelRequester);
    if (select) {
        if (select - 1 + g_GameFlow.first_level_num
            == Savegame_GetLevelNumber(g_GameInfo.current_save_slot) + 1) {
            g_GameInfo.passport_selection = PASSPORT_MODE_STORY_SO_FAR;
        } else if (select > 0) {
            g_GameInfo.select_level_num =
                select - 1 + g_GameFlow.first_level_num;
            g_GameInfo.passport_selection = PASSPORT_MODE_SELECT_LEVEL;
        } else if (
            g_InvMode != INV_SAVE_MODE && g_InvMode != INV_SAVE_CRYSTAL_MODE
            && g_InvMode != INV_LOAD_MODE) {
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
        }
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
    } else {
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
    }
}

static void Option_PassportLoadGame(void)
{
    Text_ChangeText(m_Text[TEXT_PAGE_NAME], GS(PASSPORT_LOAD_GAME));
    g_SavegameRequester.is_blockable = true;

    if (m_PassportStatus.mode == PASSPORT_MODE_BROWSE) {
        if (g_InputDB.menu_confirm) {
            Option_PassportInitSaveRequester(m_PassportStatus.page);
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
            m_PassportStatus.mode = PASSPORT_MODE_LOAD_GAME;
        }
    } else if (m_PassportStatus.mode == PASSPORT_MODE_LOAD_GAME) {
        if (!g_SavegameRequester.items[g_SavegameRequester.requested].is_blocked
            || !g_SavegameRequester.is_blockable) {
            if (g_InputDB.menu_right) {
                g_GameInfo.current_save_slot = g_SavegameRequester.requested;
                Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
                Requester_ClearTextstrings(&g_SavegameRequester);
                Option_PassportInitSelectLevelRequester();
                m_PassportStatus.mode = PASSPORT_MODE_SELECT_LEVEL;
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
                Option_PassportShowSelectLevel();
            } else {
                Option_PassportShowSaves(PASSPORT_MODE_LOAD_GAME);
                if (m_PassportStatus.mode == PASSPORT_MODE_LOAD_GAME) {
                    Text_SetPos(
                        m_Text[TEXT_LEVEL_ARROW_RIGHT], 130,
                        g_SavegameRequester
                            .items
                                [g_SavegameRequester.requested
                                 - g_SavegameRequester.line_offset]
                            .content->pos.y);
                    Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], false);
                } else {
                    Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
                }
            }
        } else {
            Option_PassportShowSaves(PASSPORT_MODE_LOAD_GAME);
            Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
        }

        if (g_SavegameRequester.items[g_SavegameRequester.requested].is_blocked
            && g_SavegameRequester.is_blockable) {
            Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
        }
    } else if (m_PassportStatus.mode == PASSPORT_MODE_SELECT_LEVEL) {
        Option_PassportSelectLevel();
    }
}

static void Option_PassportSelectLevel(void)
{
    if (g_InputDB.menu_left) {
        Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], true);
        Requester_ClearTextstrings(&m_SelectLevelRequester);
        Option_PassportInitSaveRequester(m_PassportStatus.page);
        m_PassportStatus.mode = PASSPORT_MODE_LOAD_GAME;
        g_Input = (INPUT_STATE) { 0 };
        g_InputDB = (INPUT_STATE) { 0 };
        Option_PassportShowSaves(PASSPORT_MODE_LOAD_GAME);
    } else {
        Option_PassportShowSelectLevel();
        if (m_PassportStatus.mode == PASSPORT_MODE_SELECT_LEVEL) {
            Text_SetPos(
                m_Text[TEXT_LEVEL_ARROW_LEFT], -130,
                m_SelectLevelRequester
                    .items
                        [m_SelectLevelRequester.requested
                         - m_SelectLevelRequester.line_offset]
                    .content->pos.y);
            Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], false);
        } else {
            Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], true);
        }
    }
}

static void Option_PassportSaveGame(void)
{
    Text_ChangeText(m_Text[TEXT_PAGE_NAME], GS(PASSPORT_SAVE_GAME));
    g_SavegameRequester.is_blockable = false;

    if (m_PassportStatus.mode == PASSPORT_MODE_BROWSE) {
        if (g_InputDB.menu_confirm) {
            Option_PassportInitSaveRequester(m_PassportStatus.page);
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
            m_PassportStatus.mode = PASSPORT_MODE_SAVE_GAME;
        }
    } else if (m_PassportStatus.mode == PASSPORT_MODE_SAVE_GAME) {
        Option_PassportShowSaves(PASSPORT_MODE_SAVE_GAME);
    }
}

static void Option_PassportNewGame(void)
{
    Text_ChangeText(m_Text[TEXT_PAGE_NAME], GS(PASSPORT_NEW_GAME));

    if (m_PassportStatus.mode == PASSPORT_MODE_BROWSE) {
        if (g_InputDB.menu_confirm
            && (g_Config.enable_game_modes
                || g_Config.profile.new_game_plus_unlock)) {
            Option_PassportInitNewGameRequester();
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
            m_PassportStatus.mode = PASSPORT_MODE_NEW_GAME;
        } else {
            g_GameInfo.save_initial_version = SAVEGAME_CURRENT_VERSION;
            g_GameInfo.bonus_level_unlock = false;
            g_GameInfo.current_save_slot = -1;
            g_GameInfo.passport_selection = PASSPORT_MODE_NEW_GAME;
        }
    } else if (m_PassportStatus.mode == PASSPORT_MODE_NEW_GAME) {
        int32_t select = Requester_Display(&m_NewGameRequester);
        if (select) {
            if (select > 0) {
                switch (select - 1) {
                case 0:
                    g_GameInfo.bonus_flag = 0;
                    break;
                case 1:
                    g_GameInfo.bonus_flag = GBF_NGPLUS;
                    break;
                case 2:
                    g_GameInfo.bonus_flag = GBF_JAPANESE;
                    break;
                case 3:
                    g_GameInfo.bonus_flag = GBF_JAPANESE | GBF_NGPLUS;
                    break;
                default:
                    g_GameInfo.bonus_flag = 0;
                    break;
                }
                g_GameInfo.bonus_level_unlock = false;
                g_GameInfo.current_save_slot = -1;
                g_GameInfo.passport_selection = PASSPORT_MODE_NEW_GAME;
                g_GameInfo.save_initial_version = SAVEGAME_CURRENT_VERSION;
            } else if (
                g_InvMode != INV_SAVE_MODE && g_InvMode != INV_SAVE_CRYSTAL_MODE
                && g_InvMode != INV_LOAD_MODE) {
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            }
            m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        } else {
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
        }
    }
}

static void Option_PassportRestart(INVENTORY_ITEM *inv_item)
{
    Text_ChangeText(m_Text[TEXT_PAGE_NAME], GS(PASSPORT_RESTART_LEVEL));

    if (Savegame_RestartAvailable(g_GameInfo.current_save_slot)) {
        if (g_InputDB.menu_confirm) {
            g_GameInfo.passport_selection = PASSPORT_MODE_RESTART;
        }
    } else {
        inv_item->anim_direction = 1;
        g_InputDB = (INPUT_STATE) { 0, .menu_right = 1 };
    }
}

static void Option_PassportFlipRight(INVENTORY_ITEM *inv_item)
{
    g_Input = (INPUT_STATE) { 0 };
    g_InputDB = (INPUT_STATE) { 0 };

    while (m_PassportStatus.page < PAGE_3) {
        m_PassportStatus.page++;
        if (m_PassportStatus.page_available[m_PassportStatus.page]) {
            inv_item->anim_direction = 1;
            inv_item->goal_frame =
                inv_item->open_frame + 5 * m_PassportStatus.page;
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            break;
        }
    }
}

static void Option_PassportFlipLeft(INVENTORY_ITEM *inv_item)
{
    g_Input = (INPUT_STATE) { 0 };
    g_InputDB = (INPUT_STATE) { 0 };

    while (m_PassportStatus.page > PAGE_1) {
        m_PassportStatus.page--;
        if (m_PassportStatus.page_available[m_PassportStatus.page]) {
            inv_item->anim_direction = -1;
            inv_item->goal_frame =
                inv_item->open_frame + 5 * m_PassportStatus.page;
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            break;
        }
    }
}

void Option_Passport(INVENTORY_ITEM *inv_item)
{
    if (!m_IsTextInit) {
        Text_Remove(g_InvItemText[IT_NAME]);
        g_InvItemText[IT_NAME] = NULL;
        Text_Remove(g_InvRingText);
        g_InvRingText = NULL;
        Option_PassportInitText();
        m_IsTextInit = true;
        Option_PassportDeterminePages();
    }

    m_PassportStatus.page = (inv_item->goal_frame - inv_item->open_frame) / 5;
    if (!m_PassportStatus.page_available[m_PassportStatus.page]) {
        Option_PassportFlipRight(inv_item);
        return;
    }

    if (m_PassportStatus.mode == PASSPORT_MODE_BROWSE) {
        if (m_PassportStatus.page > PAGE_1
            && m_PassportStatus.page_available[m_PassportStatus.page - 1]) {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], false);
        } else {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
        }

        if (m_PassportStatus.page < PAGE_3
            && m_PassportStatus.page_available[m_PassportStatus.page + 1]) {
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], false);
        } else {
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
        }
    } else {
        Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
        Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
    }

    switch (m_PassportStatus.page_role[m_PassportStatus.page]) {
    case PASSPORT_MODE_LOAD_GAME:
        Option_PassportLoadGame();
        break;

    case PASSPORT_MODE_SAVE_GAME:
        Option_PassportSaveGame();
        break;

    case PASSPORT_MODE_NEW_GAME:
        Option_PassportNewGame();
        break;

    case PASSPORT_MODE_RESTART:
        Option_PassportRestart(inv_item);
        break;

    case PASSPORT_MODE_EXIT_TITLE:
        Text_ChangeText(m_Text[TEXT_PAGE_NAME], GS(PASSPORT_EXIT_TO_TITLE));
        if (g_InputDB.menu_confirm) {
            g_GameInfo.passport_selection = PASSPORT_MODE_EXIT_TITLE;
        }
        break;

    case PASSPORT_MODE_EXIT_GAME:
        Text_ChangeText(m_Text[TEXT_PAGE_NAME], GS(PASSPORT_EXIT_GAME));
        if (g_InputDB.menu_confirm) {
            g_GameInfo.passport_selection = PASSPORT_MODE_EXIT_GAME;
        }
        break;

    case PASSPORT_MODE_BROWSE:
    case PASSPORT_MODE_SELECT_LEVEL:
    case PASSPORT_MODE_STORY_SO_FAR:
    case PASSPORT_MODE_UNAVAILABLE:
    default:
        break;
    }

    if (g_InputDB.menu_right
        || !m_PassportStatus.page_available[m_PassportStatus.page]) {
        Option_PassportFlipRight(inv_item);
    }

    if (g_InputDB.menu_left) {
        Option_PassportFlipLeft(inv_item);
    }

    if (g_InputDB.menu_back) {
        if (g_InvMode == INV_DEATH_MODE) {
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
        } else {
            Option_PassportClose(inv_item);
        }
    }

    if (g_InputDB.menu_confirm) {
        Option_PassportClose(inv_item);
    }
}
