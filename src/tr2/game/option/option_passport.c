#include "decomp/decomp.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/inventory_ring.h"
#include "game/option/option.h"
#include "game/savegame.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/input.h>
#include <libtrx/game/overlay.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/ui.h>

#define M_PAGE_COUNT 3

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
    bool is_ready;
    struct {
        bool is_ready;
        UI_NEW_GAME_STATE state;
    } new_game;
    struct {
        UI_PLAY_ANY_LEVEL_DIALOG_STATE *state;
    } play_any_level;
    struct {
        UI_SAVE_SLOT_DIALOG_STATE *state;
    } save_slot;
} m_State = { .active_page = -1 };

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
    if (m_State.save_slot.state != nullptr) {
        UI_SaveSlotDialog_Free(m_State.save_slot.state);
        m_State.save_slot.state = nullptr;
    }
    if (m_State.play_any_level.state != nullptr) {
        UI_PlayAnyLevelDialog_Free(m_State.play_any_level.state);
        m_State.play_any_level.state = nullptr;
    }
    M_FreeRequesters();
}

static void M_SyncArrowsVisibility(void)
{
    bool has_pages_to_left = false;
    bool has_pages_to_right = false;
    for (int32_t page = 0; page < M_PAGE_COUNT; page++) {
        has_pages_to_left |=
            (page < m_State.active_page) && m_State.pages[page].available;
        has_pages_to_right |=
            (page > m_State.active_page) && m_State.pages[page].available;
    }
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BCL, has_pages_to_left);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BCR, has_pages_to_right);
}

static void M_ChangePageTextContent(const char *const content)
{
    InvRing_RemoveAllText();
    Overlay_SetBottomText(content, false);
}

static void M_SetPage(
    const int32_t page, const M_PAGE_ROLE role, const bool available)
{
    m_State.pages[page].role = role;
    m_State.pages[page].available = available;
}

static void M_DeterminePages(void)
{
    const bool has_saves = Savegame_GetTotalCount() != 0;

    for (int32_t i = 0; i < M_PAGE_COUNT; i++) {
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
        for (int32_t i = 0; i < M_PAGE_COUNT; i++) {
            if (m_State.pages[i].role == M_ROLE_SAVE_GAME) {
                m_State.pages[i].role = g_GameFlow.play_any_level
                    ? M_ROLE_PLAY_ANY_LEVEL
                    : M_ROLE_NEW_GAME;
            }
        }
    }

    // disable save & load
    if (g_GameFlow.load_save_disabled) {
        for (int32_t i = 0; i < M_PAGE_COUNT; i++) {
            if (m_State.pages[i].role == M_ROLE_LOAD_GAME
                || m_State.pages[i].role == M_ROLE_SAVE_GAME) {
                m_State.pages[i].available = false;
            }
        }
    }

    // select first available page
    for (int32_t i = 0; i < M_PAGE_COUNT; i++) {
        if (m_State.pages[i].available) {
            m_State.active_page = i;
            break;
        }
    }

    for (int32_t i = 0; i < M_PAGE_COUNT; i++) {
        LOG_DEBUG(
            "page %d: role=%d available=%d", i, m_State.pages[i].role,
            m_State.pages[i].available);
    }
}

static void M_InitSaveRequester(const M_PAGE_ROLE role)
{
    int32_t save_slot = Savegame_GetMostRecentlyUsedSlot();
    if (save_slot == -1) {
        save_slot = Savegame_GetMostRecentlyCreatedSlot();
    }
    if (save_slot == -1) {
        save_slot = 0;
    }

    const UI_SAVE_SLOT_DIALOG_TYPE dialog_type = role == M_ROLE_LOAD_GAME
        ? UI_SAVE_SLOT_DIALOG_LOAD_GAME
        : UI_SAVE_SLOT_DIALOG_SAVE_GAME;
    m_State.save_slot.state = UI_SaveSlotDialog_Init(dialog_type, save_slot);
}

static void M_Close(INVENTORY_ITEM *const inv_item)
{
    m_State.active_page = -1;
    M_RemoveAllText();
    if (m_State.current_page == 2) {
        inv_item->anim_direction = 1;
        inv_item->goal_frame = inv_item->frames_total - 1;
    } else {
        inv_item->anim_direction = -1;
        inv_item->goal_frame = 0;
    }
}

static void M_ShowSaves(INVENTORY_ITEM *const inv_item)
{
    if (m_State.save_slot.state == nullptr) {
        M_InitSaveRequester(m_State.pages[m_State.active_page].role);
    }
    const UI_SAVE_SLOT_DIALOG_CHOICE choice =
        UI_SaveSlotDialog_Control(m_State.save_slot.state);
    switch (choice.action) {
    case UI_SAVE_SLOT_DIALOG_NO_CHOICE:
        // prevent propagating confirmation input up
        if (g_Input.menu_confirm) {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }
        break;

    case UI_SAVE_SLOT_DIALOG_CANCEL:
        if (g_Inv_Mode != INV_SAVE_MODE && g_Inv_Mode != INV_LOAD_MODE
            && g_Inv_Mode != INV_DEATH_MODE) {
            M_Close(inv_item);
        }
        break;

    case UI_SAVE_SLOT_DIALOG_DETAILS:
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
        break;

    case UI_SAVE_SLOT_DIALOG_CONFIRM:
        m_State.selection = choice.slot_num;
        break;
    }
}

static void M_LoadGame(INVENTORY_ITEM *const inv_item)
{
    M_ChangePageTextContent(GS(PASSPORT_LOAD_GAME));
    M_ShowSaves(inv_item);
}

static void M_SaveGame(INVENTORY_ITEM *const inv_item)
{
    M_ChangePageTextContent(GS(PASSPORT_SAVE_GAME));
    M_ShowSaves(inv_item);
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

static void M_PlayAnyLevel(INVENTORY_ITEM *const inv_item)
{
    M_ChangePageTextContent(GS(PASSPORT_NEW_GAME));
    if (m_State.play_any_level.state == nullptr) {
        m_State.play_any_level.state = UI_PlayAnyLevelDialog_Init();
    }
    const int32_t choice =
        UI_PlayAnyLevelDialog_Control(m_State.play_any_level.state);
    if (choice != UI_PLAY_ANY_LEVEL_CHOICE_NO_CHOICE) {
        m_State.selection = choice;
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

static void M_ShowPage(INVENTORY_ITEM *const inv_item)
{
    switch (m_State.pages[m_State.active_page].role) {
    case M_ROLE_LOAD_GAME:
        M_LoadGame(inv_item);
        break;

    case M_ROLE_SAVE_GAME:
        M_SaveGame(inv_item);
        break;

    case M_ROLE_NEW_GAME:
        M_NewGame();
        break;

    case M_ROLE_PLAY_ANY_LEVEL:
        M_PlayAnyLevel(inv_item);
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
        for (int32_t page = m_State.active_page + 1; page < M_PAGE_COUNT;
             page++) {
            if (m_State.pages[page].available) {
                m_State.active_page = page;
                break;
            }
        }
    }
}

void Option_Passport_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
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
            g_Inv_ExtraData[0] = m_State.active_page;
            g_Inv_ExtraData[1] = m_State.selection;
            M_Close(inv_item);
        } else if (g_InputDB.menu_back) {
            if (g_Inv_Mode == INV_DEATH_MODE) {
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            } else {
                M_Close(inv_item);
            }
        } else {
            M_HandleFlipInputs();
        }
    }
}

void Option_Passport_Draw(INVENTORY_ITEM *const inv_item)
{
    switch (m_State.pages[m_State.active_page].role) {
    case M_ROLE_NEW_GAME:
        if (m_State.new_game.is_ready) {
            UI_NewGame(&m_State.new_game.state);
        }
        break;

    case M_ROLE_PLAY_ANY_LEVEL:
        if (m_State.play_any_level.state != nullptr) {
            UI_PlayAnyLevelDialog(m_State.play_any_level.state);
        }
        break;

    case M_ROLE_LOAD_GAME:
    case M_ROLE_SAVE_GAME:
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
    UI_NewGame_Free(&m_State.new_game.state);
    m_State.active_page = -1;
}
