#include "game/option/option_passport.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/inventory_ring.h"
#include "game/savegame.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/input.h>
#include <libtrx/game/overlay.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/ui.h>
#include <libtrx/memory.h>

#include <stdint.h>

typedef enum {
    PAGE_UNDETERMINED = -1,
    PAGE_1 = 0,
    PAGE_2 = 1,
    PAGE_3 = 2,
    PAGE_COUNT = 3,
} M_PAGE_NUMBER;

typedef struct {
    bool available;
    PASSPORT_MODE role;
} M_PAGE;

static struct {
    PASSPORT_MODE mode;
    M_PAGE pages[PAGE_COUNT];
    M_PAGE_NUMBER current_page;
    M_PAGE_NUMBER active_page;
    bool is_ready;
    struct {
        bool is_ready;
        UI_NEW_GAME_STATE state;
    } new_game;
    struct {
        UI_SELECT_LEVEL_DIALOG_STATE *state;
    } select_level;
    struct {
        UI_SAVE_SLOT_DIALOG_STATE *state;
    } save_slot;
} m_State = {
    .current_page = PAGE_1,
    .active_page = -1,
    .mode = PASSPORT_MODE_BROWSE,
    .pages = {
        { .role = PASSPORT_MODE_UNAVAILABLE, .available = false },
        { .role = PASSPORT_MODE_NEW_GAME, .available = true },
        { .role = PASSPORT_MODE_EXIT_TITLE, .available = true },
    },
};

static void M_InitRequesters(void)
{
    UI_NewGame_Init(&m_State.new_game.state);
}

static void M_FreeRequesters(void)
{
    UI_NewGame_Free(&m_State.new_game.state);
    m_State.new_game.is_ready = false;
    m_State.is_ready = false;
}

static void M_InitText(void)
{
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BCL, false);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BCR, false);
    Overlay_SetBottomText(nullptr, false);
}

static void M_RemoveAllText(void)
{
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BCL, false);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BCR, false);
    Overlay_SetBottomText(nullptr, false);
    if (m_State.select_level.state != nullptr) {
        UI_SelectLevelDialog_Free(m_State.select_level.state);
        m_State.select_level.state = nullptr;
    }
    if (m_State.save_slot.state != nullptr) {
        UI_SaveSlotDialog_Free(m_State.save_slot.state);
        m_State.save_slot.state = nullptr;
    }
    M_FreeRequesters();
}

static void M_SyncArrowsVisibility(void)
{
    if (m_State.mode != PASSPORT_MODE_BROWSE) {
        Overlay_ShowArrow(UI_OVERLAY_ARROW_BCL, false);
        Overlay_ShowArrow(UI_OVERLAY_ARROW_BCR, false);
    } else {
        bool has_pages_to_left = false;
        bool has_pages_to_right = false;
        for (int32_t page = PAGE_1; page < PAGE_COUNT; page++) {
            has_pages_to_left |=
                (page < m_State.active_page) && m_State.pages[page].available;
            has_pages_to_right |=
                (page > m_State.active_page) && m_State.pages[page].available;
        }
        Overlay_ShowArrow(UI_OVERLAY_ARROW_BCL, has_pages_to_left);
        Overlay_ShowArrow(UI_OVERLAY_ARROW_BCR, has_pages_to_right);
    }
}

static void M_ChangePageTextContent(const char *const content)
{
    InvRing_RemoveAllText();
    Overlay_SetBottomText(content, false);
}

static void M_SetPage(
    const int32_t page, const PASSPORT_MODE role, const bool available)
{
    m_State.pages[page].role = role;
    m_State.pages[page].available = available;
}

static void M_InitSaveRequester(const int16_t page_num)
{
    int32_t save_slot = g_GameInfo.select_save_slot;
    if (save_slot == -1) {
        save_slot = Savegame_GetMostRecentlyUsedSlot();
    }
    if (save_slot == -1) {
        save_slot = Savegame_GetMostRecentlyCreatedSlot();
    }
    if (save_slot == -1) {
        save_slot = 0;
    }

    const UI_SAVE_SLOT_DIALOG_TYPE dialog_type = page_num == PAGE_1
        ? UI_SAVE_SLOT_DIALOG_LOAD_GAME
        : UI_SAVE_SLOT_DIALOG_SAVE_GAME;
    m_State.save_slot.state = UI_SaveSlotDialog_Init(dialog_type, save_slot);
}

static void M_DeterminePages(void)
{
    const bool has_saves =
        Savegame_GetTotalCount() > 0 && Savegame_GetSlotCount() > 0;

    switch (g_Inv_Mode) {
    case INV_TITLE_MODE:
        m_State.mode = PASSPORT_MODE_BROWSE;
        M_SetPage(PAGE_1, PASSPORT_MODE_LOAD_GAME, has_saves);
        M_SetPage(PAGE_2, PASSPORT_MODE_NEW_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_EXIT_GAME, true);
        break;

    case INV_GAME_MODE:
        m_State.mode = PASSPORT_MODE_BROWSE;
        M_SetPage(PAGE_1, PASSPORT_MODE_LOAD_GAME, has_saves);
        M_SetPage(PAGE_2, PASSPORT_MODE_SAVE_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_EXIT_TITLE, true);
        if (Game_GetCurrentLevel() == GF_GetGymLevel()
            || Savegame_GetSlotCount() <= 0) {
            m_State.pages[PAGE_2].role = PASSPORT_MODE_NEW_GAME;
        } else if (g_Config.gameplay.enable_save_crystals) {
            m_State.pages[PAGE_2].role = PASSPORT_MODE_RESTART;
        }
        break;

    case INV_LOAD_MODE:
        m_State.mode = Savegame_GetSlotCount() > 0 ? PASSPORT_MODE_LOAD_GAME
                                                   : PASSPORT_MODE_RESTART;
        M_SetPage(PAGE_1, m_State.mode, true);
        M_SetPage(PAGE_2, PASSPORT_MODE_UNAVAILABLE, false);
        M_SetPage(PAGE_3, PASSPORT_MODE_UNAVAILABLE, false);
        if (m_State.mode == PASSPORT_MODE_RESTART) {
            m_State.new_game.is_ready = true;
        } else {
            M_InitSaveRequester(PAGE_1);
        }
        break;

    case INV_SAVE_MODE:
        m_State.mode = PASSPORT_MODE_SAVE_GAME;
        M_SetPage(PAGE_1, PASSPORT_MODE_UNAVAILABLE, false);
        M_SetPage(PAGE_2, PASSPORT_MODE_SAVE_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_UNAVAILABLE, false);
        if (Game_GetCurrentLevel() == GF_GetGymLevel()
            || Savegame_GetSlotCount() <= 0) {
            m_State.mode = PASSPORT_MODE_BROWSE;
            m_State.pages[PAGE_2].role = PASSPORT_MODE_NEW_GAME;
        } else if (g_Config.gameplay.enable_save_crystals) {
            m_State.mode = PASSPORT_MODE_RESTART;
            m_State.pages[PAGE_2].role = PASSPORT_MODE_RESTART;
            m_State.pages[PAGE_3].role = PASSPORT_MODE_EXIT_TITLE;
            m_State.pages[PAGE_3].available = true;
        }
        M_InitSaveRequester(PAGE_2);
        break;

    case INV_SAVE_CRYSTAL_MODE:
        m_State.mode = PASSPORT_MODE_SAVE_GAME;
        M_SetPage(PAGE_1, PASSPORT_MODE_UNAVAILABLE, false);
        M_SetPage(PAGE_2, PASSPORT_MODE_SAVE_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_UNAVAILABLE, false);
        M_InitSaveRequester(PAGE_2);
        break;

    case INV_DEATH_MODE:
        m_State.mode = PASSPORT_MODE_BROWSE;
        M_SetPage(PAGE_1, PASSPORT_MODE_LOAD_GAME, has_saves);
        M_SetPage(PAGE_2, PASSPORT_MODE_RESTART, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_EXIT_TITLE, true);
        break;

    default:
        m_State.mode = PASSPORT_MODE_BROWSE;
        M_SetPage(PAGE_1, PASSPORT_MODE_LOAD_GAME, has_saves);
        M_SetPage(PAGE_2, PASSPORT_MODE_NEW_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_MODE_EXIT_TITLE, true);
        break;
    }

    // select first available page
    for (int32_t i = 0; i < 3; i++) {
        if (m_State.pages[i].available) {
            m_State.active_page = i;
            break;
        }
    }
}

static void M_InitSelectLevelRequester(void)
{
    m_State.select_level.state =
        UI_SelectLevelDialog_Init(g_GameInfo.select_save_slot);
}

static void M_ShowSelectLevel(void)
{
    const int32_t choice =
        UI_SelectLevelDialog_Control(m_State.select_level.state);
    if (choice == UI_SELECT_LEVEL_CHOICE_PLAY_STORY_SO_FAR) {
        g_GameInfo.passport_selection = PASSPORT_MODE_STORY_SO_FAR;
    } else if (choice != UI_SELECT_LEVEL_CHOICE_NOOP) {
        g_GameInfo.select_level_num = choice + GF_GetFirstLevel()->num;
        g_GameInfo.passport_selection = PASSPORT_MODE_SELECT_LEVEL;
        Savegame_BindSlot(g_GameInfo.select_save_slot);
    } else {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    }
}

static void M_ShowSaves(const PASSPORT_MODE pending_mode)
{
    const UI_SAVE_SLOT_DIALOG_CHOICE choice =
        UI_SaveSlotDialog_Control(m_State.save_slot.state);
    switch (choice.action) {
    case UI_SAVE_SLOT_DIALOG_NO_CHOICE:
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
        break;

    case UI_SAVE_SLOT_DIALOG_CANCEL:
        if (g_Inv_Mode != INV_SAVE_MODE && g_Inv_Mode != INV_SAVE_CRYSTAL_MODE
            && g_Inv_Mode != INV_LOAD_MODE) {
            m_State.mode = PASSPORT_MODE_BROWSE;
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        } else {
            m_State.mode = PASSPORT_MODE_BROWSE;
        }
        break;

    case UI_SAVE_SLOT_DIALOG_DETAILS:
        g_GameInfo.select_save_slot = choice.slot_num;
        M_InitSelectLevelRequester();
        m_State.mode = PASSPORT_MODE_SELECT_LEVEL;
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
        M_ShowSelectLevel();
        break;

    case UI_SAVE_SLOT_DIALOG_CONFIRM:
        m_State.mode = PASSPORT_MODE_BROWSE;
        g_GameInfo.select_save_slot = choice.slot_num;
        g_GameInfo.passport_selection = pending_mode;
        break;
    }
}

static void M_SelectLevel(void)
{
    if (g_InputDB.menu_left || g_InputDB.menu_back) {
        M_InitSaveRequester(m_State.active_page);
        m_State.mode = PASSPORT_MODE_LOAD_GAME;
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
        M_ShowSaves(PASSPORT_MODE_LOAD_GAME);
    } else {
        M_ShowSelectLevel();
    }
}

static void M_LoadGame(void)
{
    M_ChangePageTextContent(GS(PASSPORT_LOAD_GAME));
    if (m_State.mode == PASSPORT_MODE_BROWSE) {
        if (g_InputDB.menu_confirm) {
            M_InitSaveRequester(m_State.active_page);
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            m_State.mode = PASSPORT_MODE_LOAD_GAME;
        }
    } else if (m_State.mode == PASSPORT_MODE_LOAD_GAME) {
        M_ShowSaves(PASSPORT_MODE_LOAD_GAME);
    } else if (m_State.mode == PASSPORT_MODE_SELECT_LEVEL) {
        M_SelectLevel();
    }
}

static void M_SaveGame(void)
{
    M_ChangePageTextContent(GS(PASSPORT_SAVE_GAME));
    if (m_State.mode == PASSPORT_MODE_BROWSE) {
        if (g_InputDB.menu_confirm) {
            M_InitSaveRequester(m_State.active_page);
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            m_State.mode = PASSPORT_MODE_SAVE_GAME;
        }
    } else if (m_State.mode == PASSPORT_MODE_SAVE_GAME) {
        M_ShowSaves(PASSPORT_MODE_SAVE_GAME);
    }
}

static void M_NewGame(void)
{
    M_ChangePageTextContent(GS(PASSPORT_NEW_GAME));
    if (m_State.mode == PASSPORT_MODE_BROWSE) {
        if (g_InputDB.menu_confirm
            && (g_Config.gameplay.enable_game_modes
                || g_Config.profile.new_game_plus_unlock)) {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            m_State.mode = PASSPORT_MODE_NEW_GAME;
            m_State.new_game.is_ready = true;
        } else {
            Savegame_SetInitialVersion(SAVEGAME_CURRENT_VERSION);
            g_GameInfo.passport_selection = PASSPORT_MODE_NEW_GAME;
        }
    } else if (m_State.mode == PASSPORT_MODE_NEW_GAME) {
        const int32_t choice = UI_NewGame_Control(&m_State.new_game.state);
        if (choice == UI_REQUESTER_NO_CHOICE) {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        } else if (choice == UI_REQUESTER_CANCEL) {
            m_State.new_game.is_ready = false;
            m_State.mode = PASSPORT_MODE_BROWSE;
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        } else {
            switch (choice) {
            case 0:
                Game_SetBonusFlag(GBF_NONE);
                break;
            case 1:
                Game_SetBonusFlag(GBF_NGPLUS);
                break;
            case 2:
                Game_SetBonusFlag(GBF_JAPANESE);
                break;
            case 3:
                Game_SetBonusFlag(GBF_JAPANESE | GBF_NGPLUS);
                break;
            default:
                Game_SetBonusFlag(GBF_NONE);
                break;
            }
            g_GameInfo.passport_selection = PASSPORT_MODE_NEW_GAME;
            Savegame_SetInitialVersion(SAVEGAME_CURRENT_VERSION);
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

static int32_t M_GetCurrentPage(const INVENTORY_ITEM *const inv_item)
{
    const int32_t frame = inv_item->goal_frame - inv_item->open_frame;
    return frame % 5 == 0 ? frame / 5 : -1;
}

static bool M_IsFlipping(const INVENTORY_ITEM *const inv_item)
{
    return M_GetCurrentPage(inv_item) == -1;
}

static void M_FlipLeft(INVENTORY_ITEM *inv_item)
{
    inv_item->anim_direction = -1;
    inv_item->goal_frame = inv_item->open_frame + 5 * m_State.active_page;
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
}

static void M_FlipRight(INVENTORY_ITEM *inv_item)
{
    inv_item->anim_direction = 1;
    inv_item->goal_frame = inv_item->open_frame + 5 * m_State.active_page;
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
}

static void M_Close(INVENTORY_ITEM *inv_item)
{
    M_RemoveAllText();
    if (m_State.current_page == PAGE_3) {
        inv_item->anim_direction = 1;
        inv_item->goal_frame = inv_item->frames_total - 1;
    } else {
        inv_item->anim_direction = -1;
        inv_item->goal_frame = 0;
    }
}

static void M_ShowPage(INVENTORY_ITEM *const inv_item)
{
    switch (m_State.pages[m_State.active_page].role) {
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
}

static void M_HandleFlipInputs(void)
{
    if (g_InputDB.menu_left) {
        for (int32_t page = m_State.active_page - 1; page >= 0; page--) {
            if (m_State.pages[page].available) {
                m_State.active_page = page;
                break;
            }
        }
    } else if (g_InputDB.menu_right) {
        for (int32_t page = m_State.active_page + 1; page < 3; page++) {
            if (m_State.pages[page].available) {
                m_State.active_page = page;
                break;
            }
        }
    }
}

void Option_Passport_Control(INVENTORY_ITEM *inv_item, const bool is_busy)
{
    if (m_State.active_page == -1) {
        M_InitRequesters();
        M_InitText();
        M_DeterminePages();
    }

    if (is_busy) {
        if (g_Config.input.enable_responsive_passport) {
            M_HandleFlipInputs();
        }
        return;
    }

    if (M_IsFlipping(inv_item)) {
        return;
    }

    m_State.current_page = M_GetCurrentPage(inv_item);
    if (m_State.current_page < m_State.active_page) {
        M_FlipRight(inv_item);
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    } else if (m_State.current_page > m_State.active_page) {
        M_FlipLeft(inv_item);
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    } else {
        m_State.is_ready = true;
        M_SyncArrowsVisibility();
        M_ShowPage(inv_item);
        if (g_InputDB.menu_confirm) {
            M_Close(inv_item);
            m_State.active_page = -1;
        } else if (g_InputDB.menu_back) {
            if (g_Inv_Mode != INV_DEATH_MODE
                && (m_State.mode == PASSPORT_MODE_BROWSE
                    || m_State.mode == PASSPORT_MODE_RESTART)) {
                M_Close(inv_item);
                m_State.active_page = -1;
            } else {
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            }
        } else {
            M_HandleFlipInputs();
        }
    }
}

void Option_Passport_Draw(INVENTORY_ITEM *const inv_item)
{
    switch (m_State.mode) {
    case PASSPORT_MODE_NEW_GAME:
        if (m_State.new_game.is_ready) {
            UI_NewGame(&m_State.new_game.state);
        }
        break;

    case PASSPORT_MODE_SELECT_LEVEL:
        if (m_State.select_level.state != nullptr) {
            UI_SelectLevelDialog(m_State.select_level.state);
        }
        break;

    case PASSPORT_MODE_LOAD_GAME:
    case PASSPORT_MODE_SAVE_GAME:
        if (m_State.is_ready && m_State.save_slot.state != nullptr) {
            UI_SaveSlotDialog(m_State.save_slot.state);
        }
        break;

    default:
        break;
    }
}

void Option_Passport_Close(void)
{
    M_RemoveAllText();
    m_State.active_page = -1;
}
