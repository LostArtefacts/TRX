#include "decomp/decomp.h"
#include "decomp/savegame.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/inventory_ring.h"
#include "game/option/option.h"
#include "game/requester.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>

typedef enum {
    M_ROLE_LOAD_GAME,
    M_ROLE_SAVE_GAME,
    M_ROLE_NEW_GAME,
    M_ROLE_PLAY_ANY_LEVEL,
    M_ROLE_EXIT,
} M_PAGE_ROLE;

typedef struct {
    bool available;
    M_PAGE_ROLE role;
} M_PAGE;

static struct {
    int32_t current_page;
    int32_t active_page;
    int32_t selection;
    M_PAGE pages[3];
    bool page_ready;
} m_State = { .active_page = -1 };

TEXTSTRING *m_SubtitleText = nullptr;

static void M_SetPage(int32_t page, M_PAGE_ROLE role, bool available);
static void M_DeterminePages(void);
static void M_RemoveAllText(void);
static void M_FlipLeft(INVENTORY_ITEM *inv_item);
static void M_FlipRight(INVENTORY_ITEM *inv_item);
static void M_Close(INVENTORY_ITEM *inv_item);
static void M_ShowPage(const INVENTORY_ITEM *inv_item);
static void M_HandleFlipInputs(void);

static void M_SetPage(
    const int32_t page, const M_PAGE_ROLE role, const bool available)
{
    m_State.pages[page].role = role;
    m_State.pages[page].available = available;
}

static void M_DeterminePages(void)
{
    const bool has_saves = g_SavedGames != 0;

    for (int32_t i = 0; i < 3; i++) {
        m_State.pages[i].available = false;
    }

    switch (g_Inv_Mode) {
    case INV_TITLE_MODE:
        M_SetPage(0, M_ROLE_LOAD_GAME, has_saves);
        M_SetPage(
            1,
            g_GameFlow.play_any_level ? M_ROLE_PLAY_ANY_LEVEL : M_ROLE_NEW_GAME,
            true);
        M_SetPage(2, M_ROLE_EXIT, true);
        break;

    case INV_GAME_MODE:
        M_SetPage(0, M_ROLE_LOAD_GAME, has_saves);
        M_SetPage(1, M_ROLE_SAVE_GAME, true);
        M_SetPage(2, M_ROLE_EXIT, true);
        break;

    case INV_DEATH_MODE:
        M_SetPage(0, M_ROLE_LOAD_GAME, has_saves);
        M_SetPage(2, M_ROLE_EXIT, true);
        break;

    case INV_LOAD_MODE:
        if (has_saves) {
            M_SetPage(0, M_ROLE_LOAD_GAME, true);
        } else {
            M_SetPage(1, M_ROLE_SAVE_GAME, true);
        }
        break;

    case INV_SAVE_MODE:
        M_SetPage(1, M_ROLE_SAVE_GAME, true);
        break;

    case INV_KEYS_MODE:
        ASSERT_FAIL();
    }

    if (Game_IsInGym()) {
        for (int32_t i = 0; i < 3; i++) {
            if (m_State.pages[i].role == M_ROLE_SAVE_GAME) {
                m_State.pages[i].role = M_ROLE_NEW_GAME;
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
    if (m_SubtitleText != nullptr) {
        Text_Remove(m_SubtitleText);
        m_SubtitleText = nullptr;
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
        if (m_SubtitleText == nullptr) {
            m_SubtitleText = Text_Create(0, -16, GS(PASSPORT_LOAD_GAME));
            Text_AlignBottom(m_SubtitleText, true);
            Text_CentreH(m_SubtitleText, true);
        }

        if (!g_LoadGameRequester.ready) {
            GetSavedGamesList(&g_LoadGameRequester);
            Requester_SetHeading(
                &g_LoadGameRequester, GS(PASSPORT_LOAD_GAME), 0, nullptr, 0);
            Requester_SetSize(&g_LoadGameRequester, 10, -32);
            g_LoadGameRequester.ready = true;
        }
        m_State.selection =
            Requester_Display(&g_LoadGameRequester, false, true) - 1;
        if (m_State.selection >= 0 && !g_SavedLevels[m_State.selection]) {
            m_State.selection = -1;
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }
        break;

    case M_ROLE_SAVE_GAME: {
        if (m_SubtitleText == nullptr) {
            m_SubtitleText = Text_Create(0, -16, GS(PASSPORT_SAVE_GAME));
            Text_AlignBottom(m_SubtitleText, true);
            Text_CentreH(m_SubtitleText, true);
        }

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
            Requester_Init(&g_SaveGameRequester);
            Requester_SetSize(&g_SaveGameRequester, 10, -32);
            GetValidLevelsList(&g_SaveGameRequester);
            Requester_SetHeading(
                &g_SaveGameRequester, GS(PASSPORT_SELECT_LEVEL), 0, nullptr, 0);
            g_SaveGameRequester.ready = true;
        }
        m_State.selection =
            Requester_Display(&g_SaveGameRequester, true, true) - 1;
        break;
    }

    case M_ROLE_NEW_GAME:
        if (m_SubtitleText == nullptr) {
            m_SubtitleText = Text_Create(0, -16, GS(PASSPORT_NEW_GAME));
            Text_AlignBottom(m_SubtitleText, true);
            Text_CentreH(m_SubtitleText, true);
        }
        m_State.selection = GF_GetFirstLevel()->num;
        break;

    case M_ROLE_EXIT:
        if (m_SubtitleText == nullptr) {
            if (g_Inv_Mode == INV_TITLE_MODE) {
                m_SubtitleText = Text_Create(0, -16, GS(PASSPORT_EXIT_GAME));
            } else if (g_GameFlow.is_demo_version) {
                m_SubtitleText = Text_Create(0, -16, GS(PASSPORT_EXIT_DEMO));
            } else {
                m_SubtitleText =
                    Text_Create(0, -16, GS(PASSPORT_EXIT_TO_TITLE));
            }
            Text_AlignBottom(m_SubtitleText, true);
            Text_CentreH(m_SubtitleText, true);
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
        M_DeterminePages();
    }

    if (!is_busy || g_Config.input.enable_responsive_passport) {
        M_HandleFlipInputs();
    }
    if (is_busy) {
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
    }

    M_ShowPage(item);
    if (g_InputDB.menu_confirm) {
        g_Inv_ExtraData[0] = m_State.active_page;
        g_Inv_ExtraData[1] = m_State.selection;
        m_State.active_page = -1;
        M_Close(item);
    } else if (g_InputDB.menu_back) {
        if (g_Inv_Mode != INV_DEATH_MODE) {
            M_Close(item);
            m_State.active_page = -1;
        } else {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }
    }
}

void Option_Passport_Draw(INVENTORY_ITEM *const item)
{
}

void Option_Passport_Shutdown(void)
{
    M_RemoveAllText();
    m_State.active_page = -1;
}
