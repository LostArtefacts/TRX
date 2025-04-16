#include "decomp/decomp.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/inventory_ring.h"
#include "game/option/option.h"
#include "game/requester.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/ui.h>

typedef enum {
    M_ROLE_LOAD_GAME,
    M_ROLE_SAVE_GAME,
    M_ROLE_NEW_GAME,
    M_ROLE_PLAY_ANY_LEVEL,
    M_ROLE_EXIT,
} M_PAGE_ROLE;

typedef enum {
    M_MODE_BROWSE,
    M_MODE_PICK_OPTION,
} M_PAGE_MODE;

typedef struct {
    bool available;
    M_PAGE_ROLE role;
} M_PAGE;

static struct {
    M_PAGE_MODE mode;
    int32_t current_page;
    int32_t active_page;
    int32_t selection;
    M_PAGE pages[3];
    bool page_ready;
    struct {
        bool is_ready;
        UI_NEW_GAME_STATE state;
    } new_game;
} m_State = { .active_page = -1 };

TEXTSTRING *m_SubtitleText = nullptr;

static void M_ChangePageTextContent(const char *title);
static void M_SetPage(int32_t page, M_PAGE_ROLE role, bool available);
static void M_InitRequesters(void);
static void M_FreeRequesters(void);
static void M_DeterminePages(void);
static void M_RemoveAllText(void);
static void M_NewGame(void);
static void M_FlipLeft(INVENTORY_ITEM *inv_item);
static void M_FlipRight(INVENTORY_ITEM *inv_item);
static void M_Close(INVENTORY_ITEM *inv_item);
static void M_ShowPage(const INVENTORY_ITEM *inv_item);
static void M_HandleFlipInputs(void);

static void M_ChangePageTextContent(const char *const title)
{
    if (m_SubtitleText == nullptr) {
        m_SubtitleText = Text_Create(0, -16, title);
        Text_AlignBottom(m_SubtitleText, true);
        Text_CentreH(m_SubtitleText, true);
    }
}

static void M_SetPage(
    const int32_t page, const M_PAGE_ROLE role, const bool available)
{
    m_State.pages[page].role = role;
    m_State.pages[page].available = available;
}

static void M_InitRequesters(void)
{
    UI_NewGame_Init(&m_State.new_game.state);
}

static void M_FreeRequesters(void)
{
    UI_NewGame_Free(&m_State.new_game.state);
    m_State.new_game.is_ready = false;
}

static void M_DeterminePages(void)
{
    const bool has_saves = Savegame_GetTotalCount() != 0;

    for (int32_t i = 0; i < 3; i++) {
        m_State.pages[i].available = false;
    }

    switch (g_Inv_Mode) {
    case INV_TITLE_MODE:
        m_State.mode = M_MODE_BROWSE;
        M_SetPage(0, M_ROLE_LOAD_GAME, has_saves);
        M_SetPage(
            1,
            g_GameFlow.play_any_level ? M_ROLE_PLAY_ANY_LEVEL : M_ROLE_NEW_GAME,
            true);
        M_SetPage(2, M_ROLE_EXIT, true);
        break;

    case INV_GAME_MODE:
        m_State.mode = M_MODE_BROWSE;
        M_SetPage(0, M_ROLE_LOAD_GAME, has_saves);
        M_SetPage(1, M_ROLE_SAVE_GAME, true);
        M_SetPage(2, M_ROLE_EXIT, true);
        break;

    case INV_DEATH_MODE:
        m_State.mode = M_MODE_BROWSE;
        M_SetPage(0, M_ROLE_LOAD_GAME, has_saves);
        M_SetPage(2, M_ROLE_EXIT, true);
        break;

    case INV_LOAD_MODE:
        m_State.mode = M_MODE_BROWSE;
        if (has_saves) {
            M_SetPage(0, M_ROLE_LOAD_GAME, true);
        } else {
            M_SetPage(1, M_ROLE_SAVE_GAME, true);
        }
        break;

    case INV_SAVE_MODE:
        m_State.mode = M_MODE_BROWSE;
        M_SetPage(1, M_ROLE_SAVE_GAME, true);
        break;

    case INV_KEYS_MODE:
        ASSERT_FAIL();
    }

    if (Game_IsInGym()) {
        for (int32_t i = 0; i < 3; i++) {
            if (m_State.pages[i].role == M_ROLE_SAVE_GAME) {
                m_State.pages[i].role = g_GameFlow.play_any_level
                    ? M_ROLE_PLAY_ANY_LEVEL
                    : M_ROLE_NEW_GAME;
            }
        }
    }

    // disable save & load
    if (g_GameFlow.load_save_disabled) {
        for (int32_t i = 0; i < 3; i++) {
            if (m_State.pages[i].role == M_ROLE_LOAD_GAME
                || m_State.pages[i].role == M_ROLE_SAVE_GAME) {
                m_State.pages[i].available = false;
            }
        }
    }

    // select first available page
    for (int32_t i = 0; i < 3; i++) {
        if (m_State.pages[i].available) {
            m_State.active_page = i;
            break;
        }
    }

    for (int32_t i = 0; i < 3; i++) {
        LOG_DEBUG(
            "page %d: role=%d available=%d", i, m_State.pages[i].role,
            m_State.pages[i].available);
    }
}

static void M_RemoveAllText(void)
{
    if (g_LoadGameRequester.ready) {
        Requester_Shutdown(&g_LoadGameRequester);
    }
    if (g_SaveGameRequester.ready) {
        Requester_Shutdown(&g_SaveGameRequester);
    }
    M_FreeRequesters();
    if (m_SubtitleText != nullptr) {
        Text_Remove(m_SubtitleText);
        m_SubtitleText = nullptr;
    }
}

static void M_NewGame(void)
{
    M_ChangePageTextContent(GS(PASSPORT_NEW_GAME));
    m_State.selection = GF_GetFirstLevel()->num;
    if (m_State.mode == M_MODE_BROWSE) {
        if (g_InputDB.menu_confirm
            && (g_Config.gameplay.enable_game_modes
                || g_Config.profile.new_game_plus_unlock)) {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            m_State.mode = M_MODE_PICK_OPTION;
            m_State.new_game.is_ready = true;
        }
    } else if (m_State.mode == M_MODE_PICK_OPTION) {
        const int32_t choice = UI_NewGame_Control(&m_State.new_game.state);
        if (choice == UI_REQUESTER_NO_CHOICE) {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        } else if (choice == UI_REQUESTER_CANCEL) {
            m_State.mode = M_MODE_BROWSE;
            m_State.new_game.is_ready = false;
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
        }
    }
}

static void M_FlipLeft(INVENTORY_ITEM *const inv_item)
{
    M_RemoveAllText();
    inv_item->anim_direction = -1;
    inv_item->goal_frame = inv_item->open_frame + 5 * m_State.active_page;
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
}

static void M_FlipRight(INVENTORY_ITEM *const inv_item)
{
    M_RemoveAllText();
    inv_item->anim_direction = 1;
    inv_item->goal_frame = inv_item->open_frame + 5 * m_State.active_page;
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
}

static void M_Close(INVENTORY_ITEM *const inv_item)
{
    M_RemoveAllText();
    if (m_State.current_page == 2) {
        inv_item->anim_direction = 1;
        inv_item->goal_frame = inv_item->frames_total - 1;
    } else {
        inv_item->anim_direction = -1;
        inv_item->goal_frame = 0;
    }
}

static void M_ShowPage(const INVENTORY_ITEM *const inv_item)
{
    switch (m_State.pages[m_State.active_page].role) {
    case M_ROLE_LOAD_GAME:
        M_ChangePageTextContent(GS(PASSPORT_LOAD_GAME));

        if (!g_LoadGameRequester.ready) {
            Savegame_FillAvailableSaves(&g_LoadGameRequester);
            Requester_SetHeading(
                &g_LoadGameRequester, GS(PASSPORT_LOAD_GAME), 0, nullptr, 0);
            Requester_SetSize(&g_LoadGameRequester, 10, -32);
            g_LoadGameRequester.ready = true;
        }
        m_State.selection =
            Requester_Display(&g_LoadGameRequester, false, true) - 1;
        if (m_State.selection >= 0 && Savegame_IsSlotFree(m_State.selection)) {
            m_State.selection = -1;
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }
        break;

    case M_ROLE_SAVE_GAME: {
        M_ChangePageTextContent(GS(PASSPORT_SAVE_GAME));

        if (!g_LoadGameRequester.ready) {
            Requester_SetHeading(
                &g_LoadGameRequester, GS(PASSPORT_SAVE_GAME), 0, nullptr, 0);
            Requester_SetSize(&g_LoadGameRequester, 10, -32);
            g_LoadGameRequester.ready = true;
        }
        m_State.selection =
            Requester_Display(&g_LoadGameRequester, true, true) - 1;
        break;
    }

    case M_ROLE_PLAY_ANY_LEVEL: {
        if (!g_SaveGameRequester.ready) {
            Savegame_FillAvailableLevels(&g_SaveGameRequester);
        }
        M_ChangePageTextContent(GS(PASSPORT_NEW_GAME));
        m_State.selection =
            Requester_Display(&g_SaveGameRequester, true, true) - 1;
        break;
    }

    case M_ROLE_NEW_GAME:
        M_NewGame();
        break;

    case M_ROLE_EXIT:
        if (g_Inv_Mode == INV_TITLE_MODE) {
            M_ChangePageTextContent(GS(PASSPORT_EXIT_GAME));
        } else if (g_GameFlow.is_demo_version) {
            M_ChangePageTextContent(GS(PASSPORT_EXIT_DEMO));
        } else {
            M_ChangePageTextContent(GS(PASSPORT_EXIT_TO_TITLE));
        }
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

void Option_Passport_Control(INVENTORY_ITEM *const item, const bool is_busy)
{
    if (m_State.active_page == -1) {
        M_InitRequesters();
        M_DeterminePages();
    }

    if (is_busy) {
        if (g_Config.input.enable_responsive_passport) {
            M_HandleFlipInputs();
        }
        return;
    }

    InvRing_RemoveAllText();

    const int32_t frame = item->goal_frame - item->open_frame;
    const int32_t page = frame % 5 == 0 ? frame / 5 : -1;
    const bool is_flipping = page == -1;
    if (is_flipping) {
        return;
    }

    m_State.current_page = page;
    if (m_State.current_page < m_State.active_page) {
        M_FlipRight(item);
    } else if (m_State.current_page > m_State.active_page) {
        M_FlipLeft(item);
    } else {
        M_ShowPage(item);
        if (g_InputDB.menu_confirm) {
            g_Inv_ExtraData[0] = m_State.active_page;
            g_Inv_ExtraData[1] = m_State.selection;
            m_State.active_page = -1;
            M_Close(item);
        } else if (g_InputDB.menu_back) {
            if (g_Inv_Mode != INV_DEATH_MODE) {
                m_State.active_page = -1;
                M_Close(item);
            } else {
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            }
        } else {
            M_HandleFlipInputs();
        }
    }
}

void Option_Passport_Draw(INVENTORY_ITEM *const item)
{
    switch (m_State.pages[m_State.active_page].role) {
    case M_ROLE_NEW_GAME:
        if (m_State.new_game.is_ready) {
            UI_NewGame(&m_State.new_game.state);
        }
        break;
    default:
        break;
    }
}

void Option_Passport_Shutdown(void)
{
    M_RemoveAllText();
    UI_NewGame_Free(&m_State.new_game.state);
    m_State.active_page = -1;
}
