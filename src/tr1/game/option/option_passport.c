#include "game/option/option_passport.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/inventory_ring.h"
#include "game/requester.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/memory.h>

#include <stdint.h>

#define MAX_GAME_MODES 4
#define MIN_NAME_WIDTH 140
#define NAME_SPACER 15

typedef enum {
    TEXT_PAGE_NAME = 0,
    TEXT_LEFT_ARROW = 1,
    TEXT_RIGHT_ARROW = 2,
    TEXT_LEVEL_ARROW_RIGHT = 3,
    TEXT_LEVEL_ARROW_LEFT = 4,
    TEXT_NUMBER_OF = 5,
} PASSPORT_TEXT;

typedef struct {
    bool available;
    PASSPORT_MODE role;
} M_PAGE;

typedef struct {
    PASSPORT_PAGE page;
    PASSPORT_MODE mode;
    M_PAGE pages[PAGE_COUNT];
} PASSPORT_STATUS;

static PASSPORT_STATUS m_PassportStatus = {
    .page = PAGE_1,
    .mode = PASSPORT_MODE_BROWSE,
    .pages = {
        { .role = PASSPORT_MODE_UNAVAILABLE, .available = false },
        { .role = PASSPORT_MODE_NEW_GAME, .available = true },
        { .role = PASSPORT_MODE_EXIT_TITLE, .available = true },
    },
};

static bool m_IsTextInit = false;
static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = {};

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
    .heading_text = nullptr,
    .items = nullptr,
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
    .heading_text = nullptr,
    .items = nullptr,
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
    .heading_text = nullptr,
    .items = nullptr,
};

static void M_InitRequesters(void);
static void M_InitText(void);
static void M_ShutdownText(void);
static void M_Close(INVENTORY_ITEM *inv_item);
static void M_ChangePageTextContent(const char *text);
static void M_SetPage(int32_t page, PASSPORT_MODE role, bool available);
static void M_DeterminePages(void);
static void M_InitSaveRequester(int16_t page_num);
static void M_RestoreSaveRequester(void);
static void M_InitSelectLevelRequester(void);
static void M_InitNewGameRequester(void);
static void M_ShowSaves(PASSPORT_MODE pending_mode);
static void M_ShowSelectLevel(void);
static void M_LoadGame(void);
static void M_SelectLevel(void);
static void M_SaveGame(void);
static void M_NewGame(void);
static void M_Restart(INVENTORY_ITEM *inv_item);
static void M_FlipRight(INVENTORY_ITEM *inv_item);
static void M_FlipLeft(INVENTORY_ITEM *inv_item);

void M_InitRequesters(void)
{
    Requester_Shutdown(&m_SelectLevelRequester);
    Requester_Shutdown(&m_NewGameRequester);
    Requester_Shutdown(&g_SavegameRequester);
    Requester_Init(&g_SavegameRequester, Savegame_GetSlotCount());
    Requester_Init(
        &m_SelectLevelRequester, g_GameFlow.level_tables[GFLT_MAIN].count + 1);
    Requester_Init(&m_NewGameRequester, MAX_GAME_MODES);
}

static void M_InitText(void)
{
    m_Text[TEXT_LEFT_ARROW] = Text_Create(-85, -15, "\\{button left}");
    Text_Hide(m_Text[TEXT_LEFT_ARROW], true);

    m_Text[TEXT_RIGHT_ARROW] = Text_Create(70, -15, "\\{button right}");
    Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);

    m_Text[TEXT_LEVEL_ARROW_LEFT] = Text_Create(0, 0, "\\{button left}");
    Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], true);

    m_Text[TEXT_LEVEL_ARROW_RIGHT] = Text_Create(0, 0, "\\{button right}");
    Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);

    m_Text[TEXT_PAGE_NAME] = Text_Create(0, -16, "");

    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_AlignBottom(m_Text[i], 1);
        Text_CentreH(m_Text[i], 1);
    }
}

static void M_ShutdownText(void)
{
    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_Remove(m_Text[i]);
        m_Text[i] = nullptr;
    }
    m_IsTextInit = false;
}

static void M_Close(INVENTORY_ITEM *inv_item)
{
    if (m_PassportStatus.page == PAGE_3) {
        inv_item->anim_direction = 1;
        inv_item->goal_frame = inv_item->frames_total - 1;
    } else {
        inv_item->anim_direction = -1;
        inv_item->goal_frame = 0;
    }
    M_ShutdownText();
}

static void M_ChangePageTextContent(const char *const content)
{
    Text_ChangeText(m_Text[TEXT_PAGE_NAME], content);
    const int32_t width = Text_GetWidth(m_Text[TEXT_PAGE_NAME]);
    const int32_t x_pos = MAX(width, MIN_NAME_WIDTH) / 2 + NAME_SPACER;
    Text_SetPos(m_Text[TEXT_LEFT_ARROW], -x_pos, -15);
    Text_SetPos(m_Text[TEXT_RIGHT_ARROW], x_pos, -15);
}

static void M_SetPage(
    const int32_t page, const PASSPORT_MODE role, const bool available)
{
    m_PassportStatus.pages[page].role = role;
    m_PassportStatus.pages[page].available = available;
}

static void M_DeterminePages(void)
{
    const bool has_saves = g_SavedGamesCount > 0 && Savegame_GetSlotCount() > 0;

    switch (g_InvMode) {
    case INV_TITLE_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        M_SetPage(PAGE_1, PASSPORT_MODE_LOAD_GAME, has_saves);
        M_SetPage(PAGE_2, PASSPORT_MODE_NEW_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_EXIT_GAME, true);
        break;

    case INV_GAME_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        M_SetPage(PAGE_1, PASSPORT_MODE_LOAD_GAME, has_saves);
        M_SetPage(PAGE_2, PASSPORT_MODE_SAVE_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_EXIT_TITLE, true);
        if (Game_GetCurrentLevel() == GF_GetGymLevel()
            || Savegame_GetSlotCount() <= 0) {
            m_PassportStatus.pages[PAGE_2].role = PASSPORT_MODE_NEW_GAME;
        } else if (g_Config.gameplay.enable_save_crystals) {
            m_PassportStatus.pages[PAGE_2].role = PASSPORT_MODE_RESTART;
        }
        break;

    case INV_LOAD_MODE:
        m_PassportStatus.mode = Savegame_GetSlotCount() > 0
            ? PASSPORT_MODE_LOAD_GAME
            : PASSPORT_MODE_RESTART;
        M_SetPage(PAGE_1, m_PassportStatus.mode, true);
        M_SetPage(PAGE_2, PASSPORT_MODE_UNAVAILABLE, false);
        M_SetPage(PAGE_3, PASSPORT_MODE_UNAVAILABLE, false);
        if (m_PassportStatus.mode == PASSPORT_MODE_RESTART) {
            M_InitNewGameRequester();
        } else {
            M_InitSaveRequester(PAGE_1);
        }
        break;

    case INV_SAVE_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_SAVE_GAME;
        M_SetPage(PAGE_1, PASSPORT_MODE_UNAVAILABLE, false);
        M_SetPage(PAGE_2, PASSPORT_MODE_SAVE_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_UNAVAILABLE, false);
        if (Game_GetCurrentLevel() == GF_GetGymLevel()
            || Savegame_GetSlotCount() <= 0) {
            m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
            m_PassportStatus.pages[PAGE_2].role = PASSPORT_MODE_NEW_GAME;
        } else if (g_Config.gameplay.enable_save_crystals) {
            m_PassportStatus.mode = PASSPORT_MODE_RESTART;
            m_PassportStatus.pages[PAGE_2].role = PASSPORT_MODE_RESTART;
            m_PassportStatus.pages[PAGE_3].role = PASSPORT_MODE_EXIT_TITLE;
            m_PassportStatus.pages[PAGE_3].available = true;
        }
        M_InitSaveRequester(PAGE_2);
        break;

    case INV_SAVE_CRYSTAL_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_SAVE_GAME;
        M_SetPage(PAGE_1, PASSPORT_MODE_UNAVAILABLE, false);
        M_SetPage(PAGE_2, PASSPORT_MODE_SAVE_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_UNAVAILABLE, false);
        M_InitSaveRequester(PAGE_2);
        break;

    case INV_DEATH_MODE:
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        M_SetPage(PAGE_1, PASSPORT_MODE_LOAD_GAME, has_saves);
        M_SetPage(PAGE_2, PASSPORT_MODE_RESTART, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_EXIT_TITLE, true);
        break;

    default:
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        M_SetPage(PAGE_1, PASSPORT_MODE_LOAD_GAME, has_saves);
        M_SetPage(PAGE_2, PASSPORT_MODE_NEW_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_EXIT_TITLE, true);
        break;
    }
}

static void M_InitSaveRequester(int16_t page_num)
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
    req->vis_lines = MIN(req->max_items, req->vis_lines);

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

static void M_RestoreSaveRequester(void)
{
    CLAMP(g_SavegameRequester.requested, 0, g_SavegameRequester.items_used - 1);
}

static void M_InitSelectLevelRequester(void)
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

static void M_InitNewGameRequester(void)
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

static void M_ShowSaves(PASSPORT_MODE pending_mode)
{
    int32_t select = Requester_Display(&g_SavegameRequester);
    if (select == 0) {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    } else if (select > 0) {
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        g_GameInfo.select_save_slot = select - 1;
        g_GameInfo.passport_selection = pending_mode;
    } else if (
        g_InvMode != INV_SAVE_MODE && g_InvMode != INV_SAVE_CRYSTAL_MODE
        && g_InvMode != INV_LOAD_MODE) {
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    } else {
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
    }
}

static void M_ShowSelectLevel(void)
{
    int32_t select = Requester_Display(&m_SelectLevelRequester);
    if (select) {
        if (select - 1 + GF_GetFirstLevel()->num
            == Savegame_GetLevelNumber(g_GameInfo.select_save_slot) + 1) {
            g_GameInfo.passport_selection = PASSPORT_MODE_STORY_SO_FAR;
        } else if (select > 0) {
            g_GameInfo.select_level_num = select - 1 + GF_GetFirstLevel()->num;
            g_GameInfo.passport_selection = PASSPORT_MODE_SELECT_LEVEL;
        } else if (
            g_InvMode != INV_SAVE_MODE && g_InvMode != INV_SAVE_CRYSTAL_MODE
            && g_InvMode != INV_LOAD_MODE) {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }
        m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
    } else {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    }
}

static void M_LoadGame(void)
{
    M_ChangePageTextContent(GS(PASSPORT_LOAD_GAME));
    g_SavegameRequester.is_blockable = true;

    if (m_PassportStatus.mode == PASSPORT_MODE_BROWSE) {
        if (g_InputDB.menu_confirm) {
            M_InitSaveRequester(m_PassportStatus.page);
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            m_PassportStatus.mode = PASSPORT_MODE_LOAD_GAME;
        }
    } else if (m_PassportStatus.mode == PASSPORT_MODE_LOAD_GAME) {
        M_RestoreSaveRequester();
        if (!g_SavegameRequester.items[g_SavegameRequester.requested].is_blocked
            || !g_SavegameRequester.is_blockable) {
            if (g_InputDB.menu_right) {
                g_GameInfo.select_save_slot = g_SavegameRequester.requested;
                Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
                Requester_ClearTextstrings(&g_SavegameRequester);
                M_InitSelectLevelRequester();
                m_PassportStatus.mode = PASSPORT_MODE_SELECT_LEVEL;
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
                M_ShowSelectLevel();
            } else {
                M_ShowSaves(PASSPORT_MODE_LOAD_GAME);
                if (m_PassportStatus.mode == PASSPORT_MODE_LOAD_GAME) {
                    const REQUESTER_ITEM *const row_item =
                        &g_SavegameRequester.items
                             [g_SavegameRequester.requested
                              - g_SavegameRequester.line_offset];
                    if (row_item->content != nullptr) {
                        Text_SetPos(
                            m_Text[TEXT_LEVEL_ARROW_RIGHT], 130,
                            row_item->content->pos.y);
                        Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], false);
                    }
                } else {
                    Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
                }
            }
        } else {
            M_ShowSaves(PASSPORT_MODE_LOAD_GAME);
            Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
        }

        if (g_SavegameRequester.items[g_SavegameRequester.requested].is_blocked
            && g_SavegameRequester.is_blockable) {
            Text_Hide(m_Text[TEXT_LEVEL_ARROW_RIGHT], true);
        }
    } else if (m_PassportStatus.mode == PASSPORT_MODE_SELECT_LEVEL) {
        M_SelectLevel();
    }
}

static void M_SelectLevel(void)
{
    if (g_InputDB.menu_left) {
        Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], true);
        Requester_ClearTextstrings(&m_SelectLevelRequester);
        M_InitSaveRequester(m_PassportStatus.page);
        m_PassportStatus.mode = PASSPORT_MODE_LOAD_GAME;
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
        M_ShowSaves(PASSPORT_MODE_LOAD_GAME);
    } else {
        M_ShowSelectLevel();
        if (m_PassportStatus.mode == PASSPORT_MODE_SELECT_LEVEL) {
            const TEXTSTRING *const sel_item =
                m_SelectLevelRequester
                    .items
                        [m_SelectLevelRequester.requested
                         - m_SelectLevelRequester.line_offset]
                    .content;
            if (sel_item != nullptr) {
                Text_SetPos(
                    m_Text[TEXT_LEVEL_ARROW_LEFT], -130, sel_item->pos.y);
                Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], false);
            } else {
                Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], true);
            }
        } else {
            Text_Hide(m_Text[TEXT_LEVEL_ARROW_LEFT], true);
        }
    }
}

static void M_SaveGame(void)
{
    M_ChangePageTextContent(GS(PASSPORT_SAVE_GAME));
    g_SavegameRequester.is_blockable = false;

    if (m_PassportStatus.mode == PASSPORT_MODE_BROWSE) {
        if (g_InputDB.menu_confirm) {
            M_InitSaveRequester(m_PassportStatus.page);
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            m_PassportStatus.mode = PASSPORT_MODE_SAVE_GAME;
        }
    } else if (m_PassportStatus.mode == PASSPORT_MODE_SAVE_GAME) {
        M_RestoreSaveRequester();
        M_ShowSaves(PASSPORT_MODE_SAVE_GAME);
    }
}

static void M_NewGame(void)
{
    M_ChangePageTextContent(GS(PASSPORT_NEW_GAME));

    if (m_PassportStatus.mode == PASSPORT_MODE_BROWSE) {
        if (g_InputDB.menu_confirm
            && (g_Config.gameplay.enable_game_modes
                || g_Config.profile.new_game_plus_unlock)) {
            M_InitNewGameRequester();
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            m_PassportStatus.mode = PASSPORT_MODE_NEW_GAME;
        } else {
            g_GameInfo.save_initial_version = SAVEGAME_CURRENT_VERSION;
            g_GameInfo.bonus_level_unlock = false;
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
                g_GameInfo.passport_selection = PASSPORT_MODE_NEW_GAME;
                g_GameInfo.save_initial_version = SAVEGAME_CURRENT_VERSION;
            } else if (
                g_InvMode != INV_SAVE_MODE && g_InvMode != INV_SAVE_CRYSTAL_MODE
                && g_InvMode != INV_LOAD_MODE) {
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            }
            m_PassportStatus.mode = PASSPORT_MODE_BROWSE;
        } else {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }
    }
}

static void M_Restart(INVENTORY_ITEM *inv_item)
{
    M_ChangePageTextContent(GS(PASSPORT_RESTART_LEVEL));

    if (Savegame_RestartAvailable(Savegame_GetBoundSlot())) {
        if (g_InputDB.menu_confirm) {
            g_GameInfo.passport_selection = PASSPORT_MODE_RESTART;
        }
    } else {
        inv_item->anim_direction = 1;
        g_InputDB = (INPUT_STATE) { .menu_right = 1 };
    }
}

static void M_FlipRight(INVENTORY_ITEM *inv_item)
{
    g_Input = (INPUT_STATE) {};
    g_InputDB = (INPUT_STATE) {};

    while (m_PassportStatus.page < PAGE_3) {
        m_PassportStatus.page++;
        if (m_PassportStatus.pages[m_PassportStatus.page].available) {
            inv_item->anim_direction = 1;
            inv_item->goal_frame =
                inv_item->open_frame + 5 * m_PassportStatus.page;
            Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
            break;
        }
    }
}

static void M_FlipLeft(INVENTORY_ITEM *inv_item)
{
    g_Input = (INPUT_STATE) {};
    g_InputDB = (INPUT_STATE) {};

    while (m_PassportStatus.page > PAGE_1) {
        m_PassportStatus.page--;
        if (m_PassportStatus.pages[m_PassportStatus.page].available) {
            inv_item->anim_direction = -1;
            inv_item->goal_frame =
                inv_item->open_frame + 5 * m_PassportStatus.page;
            Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
            break;
        }
    }
}

void Option_Passport_Control(INVENTORY_ITEM *inv_item)
{
    if (!m_IsTextInit) {
        M_InitRequesters();
        InvRing_RemoveAllText();
        M_InitText();
        m_IsTextInit = true;
        M_DeterminePages();
    }

    m_PassportStatus.page = (inv_item->goal_frame - inv_item->open_frame) / 5;
    if (!m_PassportStatus.pages[m_PassportStatus.page].available) {
        M_FlipRight(inv_item);
        return;
    }

    if (m_PassportStatus.mode == PASSPORT_MODE_BROWSE) {
        if (m_PassportStatus.page > PAGE_1
            && m_PassportStatus.pages[m_PassportStatus.page - 1].available) {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], false);
        } else {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
        }

        if (m_PassportStatus.page < PAGE_3
            && m_PassportStatus.pages[m_PassportStatus.page + 1].available) {
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], false);
        } else {
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
        }
    } else {
        Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
        Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
    }

    switch (m_PassportStatus.pages[m_PassportStatus.page].role) {
    case PASSPORT_MODE_LOAD_GAME:
        M_LoadGame();
        break;

    case PASSPORT_MODE_SAVE_GAME:
        M_SaveGame();
        break;

    case PASSPORT_MODE_NEW_GAME:
        M_NewGame();
        break;

    case PASSPORT_MODE_RESTART:
        M_Restart(inv_item);
        break;

    case PASSPORT_MODE_EXIT_TITLE:
        M_ChangePageTextContent(GS(PASSPORT_EXIT_TO_TITLE));
        if (g_InputDB.menu_confirm) {
            g_GameInfo.passport_selection = PASSPORT_MODE_EXIT_TITLE;
        }
        break;

    case PASSPORT_MODE_EXIT_GAME:
        M_ChangePageTextContent(GS(PASSPORT_EXIT_GAME));
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
        || !m_PassportStatus.pages[m_PassportStatus.page].available) {
        M_FlipRight(inv_item);
    }

    if (g_InputDB.menu_left) {
        M_FlipLeft(inv_item);
    }

    if (g_InputDB.menu_back) {
        if (g_InvMode == INV_DEATH_MODE) {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        } else {
            M_Close(inv_item);
        }
    }

    if (g_InputDB.menu_confirm) {
        M_Close(inv_item);
    }
}

void Option_Passport_Shutdown(void)
{
    M_ShutdownText();
    Requester_Shutdown(&m_SelectLevelRequester);
    Requester_Shutdown(&m_NewGameRequester);
    Requester_ClearTextstrings(&g_SavegameRequester);
}
