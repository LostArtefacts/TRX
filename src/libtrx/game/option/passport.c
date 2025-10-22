#include "game/option/passport.h"

#include "config.h"
#include "debug.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/inventory_ring.h"
#include "game/overlay.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/ui.h"
#include "version.h"

#define M_IMMEDIATE (g_TRVersion == 2)

typedef enum {
    PAGE_UNDETERMINED = -1,
    PAGE_1 = 0,
    PAGE_2 = 1,
    PAGE_3 = 2,
    PAGE_COUNT = 3,
} M_PAGE_NUMBER;

typedef enum {
    M_MODE_BROWSE,
    M_MODE_PICK_OPTION,
} M_PAGE_MODE;

typedef struct {
    bool available;
    PASSPORT_ROLE role;
} M_PAGE;

static struct {
    M_PAGE_MODE mode;
    M_PAGE pages[PAGE_COUNT];
    M_PAGE_NUMBER current_page;
    M_PAGE_NUMBER active_page;
    int32_t selection;
    int32_t outer_selection; // multi-level navigation: load game→story so far
    bool is_ready;
    struct {
        UI_NEW_GAME_STATE *state;
    } new_game;
    struct {
        UI_SELECT_LEVEL_DIALOG_STATE *state;
    } select_level;
    struct {
        UI_PLAY_ANY_LEVEL_DIALOG_STATE *state;
    } play_any_level;
    struct {
        UI_SAVE_SLOT_DIALOG_STATE *state;
    } save_slot;
} m_Priv = {
    .active_page = PAGE_UNDETERMINED,
    .selection = -1,
    .outer_selection = -1,
};

PASSPORT g_Passport = {
    .select_slot = -1,
};

static void M_FreeRequesters(void)
{
    m_Priv.is_ready = false;
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
    if (m_Priv.select_level.state != nullptr) {
        UI_SelectLevelDialog_Free(m_Priv.select_level.state);
        m_Priv.select_level.state = nullptr;
    }
    if (m_Priv.play_any_level.state != nullptr) {
        UI_PlayAnyLevelDialog_Free(m_Priv.play_any_level.state);
        m_Priv.play_any_level.state = nullptr;
    }
    if (m_Priv.save_slot.state != nullptr) {
        UI_SaveSlotDialog_Free(m_Priv.save_slot.state);
        m_Priv.save_slot.state = nullptr;
    }
    if (m_Priv.new_game.state != nullptr) {
        UI_NewGame_Free(m_Priv.new_game.state);
        m_Priv.new_game.state = nullptr;
    }
    M_FreeRequesters();
}

static void M_SyncArrowsVisibility(void)
{
    if (m_Priv.mode == M_MODE_PICK_OPTION && !M_IMMEDIATE) {
        Overlay_ShowArrow(UI_OVERLAY_ARROW_BCL, false);
        Overlay_ShowArrow(UI_OVERLAY_ARROW_BCR, false);
    } else {
        bool has_pages_to_left = false;
        bool has_pages_to_right = false;
        for (M_PAGE_NUMBER page = PAGE_1; page < PAGE_COUNT; page++) {
            has_pages_to_left |=
                (page < m_Priv.active_page) && m_Priv.pages[page].available;
            has_pages_to_right |=
                (page > m_Priv.active_page) && m_Priv.pages[page].available;
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
    inv_item->goal_frame = inv_item->open_frame + 5 * m_Priv.active_page;
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
}

static void M_FlipRight(INVENTORY_ITEM *const inv_item)
{
    M_RemoveAllText();
    inv_item->anim_direction = 1;
    inv_item->goal_frame = inv_item->open_frame + 5 * m_Priv.active_page;
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
}

static void M_NavigateInto(const int16_t slot_num)
{
    m_Priv.outer_selection = slot_num;
}

static void M_Close(INVENTORY_ITEM *const inv_item)
{
    m_Priv.active_page = PAGE_UNDETERMINED;
    M_RemoveAllText();
    if (m_Priv.current_page == PAGE_3) {
        inv_item->anim_direction = 1;
        inv_item->goal_frame = inv_item->frames_total - 1;
    } else {
        inv_item->anim_direction = -1;
        inv_item->goal_frame = 0;
    }
}

static void M_SoftClose(INVENTORY_ITEM *const inv_item)
{
    if (g_Inv_Mode == INV_DEATH_MODE) {
        if (!M_IMMEDIATE && m_Priv.mode != M_MODE_BROWSE) {
            m_Priv.mode = M_MODE_BROWSE;
        }
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
        return;
    }
    if (m_Priv.mode == M_MODE_BROWSE || M_IMMEDIATE
        || (g_Inv_Mode != INV_GAME_MODE && g_Inv_Mode != INV_TITLE_MODE)) {
        M_Close(inv_item);
    } else {
        m_Priv.mode = M_MODE_BROWSE;
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    }
}

static void M_SetPage(
    const M_PAGE_NUMBER page, const PASSPORT_ROLE role, const bool available)
{
    m_Priv.pages[page].role = role;
    m_Priv.pages[page].available = available;
}

static void M_DeterminePages(void)
{
    const bool can_restart = Savegame_RestartAvailable(Savegame_GetBoundSlot());
    const bool saving_enabled =
        Savegame_GetSlotCount() > 0 && !g_GameFlow.load_save_disabled;
    const bool has_saves = Savegame_GetTotalCount() > 0 && saving_enabled;

    m_Priv.selection = -1;
    m_Priv.outer_selection = -1;
    for (M_PAGE_NUMBER i = PAGE_1; i < PAGE_COUNT; i++) {
        m_Priv.pages[i].available = false;
    }

    switch (g_Inv_Mode) {
    case INV_TITLE_MODE:
        m_Priv.mode = M_IMMEDIATE ? M_MODE_PICK_OPTION : M_MODE_BROWSE;
        M_SetPage(PAGE_1, PASSPORT_ROLE_LOAD_GAME, has_saves);
        M_SetPage(PAGE_2, PASSPORT_ROLE_NEW_GAME, true);
        M_SetPage(PAGE_3, PASSPORT_ROLE_EXIT_GAME, true);
        break;

    case INV_GAME_MODE:
        m_Priv.mode = M_IMMEDIATE ? M_MODE_PICK_OPTION : M_MODE_BROWSE;
        if (!saving_enabled) {
            M_SetPage(PAGE_2, PASSPORT_ROLE_RESTART, can_restart);
        } else {
            M_SetPage(PAGE_1, PASSPORT_ROLE_LOAD_GAME, has_saves);
            M_SetPage(PAGE_2, PASSPORT_ROLE_SAVE_GAME, true);
        }
        M_SetPage(PAGE_3, PASSPORT_ROLE_EXIT_TITLE, true);
        break;

    case INV_LOAD_MODE:
        m_Priv.mode = M_MODE_PICK_OPTION;
        if (!saving_enabled) {
            M_SetPage(PAGE_2, PASSPORT_ROLE_RESTART, can_restart);
        } else if (has_saves) {
            M_SetPage(PAGE_1, PASSPORT_ROLE_LOAD_GAME, true);
        } else {
            M_SetPage(PAGE_2, PASSPORT_ROLE_SAVE_GAME, true);
        }
        break;

    case INV_SAVE_MODE:
    case INV_SAVE_CRYSTAL_MODE:
        m_Priv.mode = M_MODE_PICK_OPTION;
        if (!saving_enabled) {
            M_SetPage(PAGE_2, PASSPORT_ROLE_RESTART, can_restart);
        } else {
            M_SetPage(PAGE_2, PASSPORT_ROLE_SAVE_GAME, true);
        }
        break;

    case INV_DEATH_MODE:
        m_Priv.mode = M_IMMEDIATE ? M_MODE_PICK_OPTION : M_MODE_BROWSE;
        M_SetPage(PAGE_1, PASSPORT_ROLE_LOAD_GAME, has_saves);
        M_SetPage(PAGE_2, PASSPORT_ROLE_RESTART, can_restart);
        M_SetPage(PAGE_3, PASSPORT_ROLE_EXIT_TITLE, true);
        break;

    case INV_KEYS_MODE:
        ASSERT_FAIL();
    }

    // Disable saves in gym and save crystals mode.
    // Offer New Game or Restart instead.
    for (M_PAGE_NUMBER i = PAGE_1; i < PAGE_COUNT; i++) {
        if (m_Priv.pages[i].role != PASSPORT_ROLE_SAVE_GAME) {
            continue;
        }
        if (Game_IsInGym()) {
            m_Priv.pages[i].role = PASSPORT_ROLE_NEW_GAME;
        } else if (
            g_Config.gameplay.enable_save_crystals
            && g_Inv_Mode != INV_SAVE_CRYSTAL_MODE) {
            if (can_restart) {
                m_Priv.pages[i].role = PASSPORT_ROLE_RESTART;
            } else {
                m_Priv.pages[i].available = false;
            }
        }
    }

// If play any level is enabled, replace New Game with Play Any Level.
#if TR_VERSION == 2
    if (g_GameFlow.play_any_level) {
        for (M_PAGE_NUMBER i = PAGE_1; i < PAGE_COUNT; i++) {
            if (m_Priv.pages[i].role == PASSPORT_ROLE_NEW_GAME) {
                m_Priv.pages[i].role = PASSPORT_ROLE_SELECT_LEVEL;
            }
        }
    }
#endif

    // Select first available page
    m_Priv.active_page = PAGE_UNDETERMINED;
    for (M_PAGE_NUMBER i = PAGE_1; i < PAGE_COUNT; i++) {
        if (m_Priv.pages[i].available) {
            m_Priv.active_page = i;
            break;
        }
    }

    // Guard: if no pages are available, force-add exit game or exit to title
    if (m_Priv.active_page == PAGE_UNDETERMINED) {
        M_SetPage(
            PAGE_3,
            g_Inv_Mode == INV_TITLE_MODE ? PASSPORT_ROLE_EXIT_GAME
                                         : PASSPORT_ROLE_EXIT_TITLE,
            true);
        m_Priv.active_page = PAGE_3;
    }

    for (M_PAGE_NUMBER i = PAGE_1; i < PAGE_COUNT; i++) {
        LOG_DEBUG(
            "page %d: role=%d available=%d", i, m_Priv.pages[i].role,
            m_Priv.pages[i].available);
    }
}

static void M_ShowStorySoFar(void)
{
    if (m_Priv.select_level.state == nullptr) {
        m_Priv.select_level.state =
            UI_SelectLevelDialog_Init(m_Priv.outer_selection);
    }
    const int32_t choice =
        UI_SelectLevelDialog_Control(m_Priv.select_level.state);
    if (choice == UI_SELECT_LEVEL_CHOICE_PLAY_STORY_SO_FAR) {
        m_Priv.selection = m_Priv.outer_selection;
        m_Priv.pages[m_Priv.active_page].role = PASSPORT_ROLE_STORY_SO_FAR;
    } else if (choice != UI_SELECT_LEVEL_CHOICE_NOOP) {
        m_Priv.selection = choice + GF_GetFirstLevel()->num;
        m_Priv.pages[m_Priv.active_page].role = PASSPORT_ROLE_SELECT_LEVEL;
        Savegame_BindSlot(m_Priv.outer_selection);
    } else {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    }
}

static void M_PlayAnyLevel(INVENTORY_ITEM *const inv_item)
{
    M_ChangePageTextContent(GS(PASSPORT_NEW_GAME));
    if (m_Priv.play_any_level.state == nullptr) {
        m_Priv.play_any_level.state = UI_PlayAnyLevelDialog_Init();
    }
    const int32_t choice =
        UI_PlayAnyLevelDialog_Control(m_Priv.play_any_level.state);
    if (choice != UI_PLAY_ANY_LEVEL_CHOICE_NO_CHOICE) {
        m_Priv.selection = choice;
    }
}

static void M_InitLoadSaveRequester(const PASSPORT_ROLE role)
{
    int32_t save_slot = m_Priv.selection;
    if (save_slot == -1) {
        save_slot = Savegame_GetMostRecentlyUsedSlot();
    }
    if (save_slot == -1) {
        save_slot = Savegame_GetMostRecentlyCreatedSlot();
    }
    if (save_slot == -1) {
        save_slot = 0;
    }

    const UI_SAVE_SLOT_DIALOG_TYPE dialog_type = role == PASSPORT_ROLE_LOAD_GAME
        ? UI_SAVE_SLOT_DIALOG_LOAD_GAME
        : UI_SAVE_SLOT_DIALOG_SAVE_GAME;
    m_Priv.save_slot.state = UI_SaveSlotDialog_Init(dialog_type, save_slot);
}

static void M_ShowSaves(INVENTORY_ITEM *const inv_item)
{
    if (m_Priv.save_slot.state == nullptr) {
        M_InitLoadSaveRequester(m_Priv.pages[m_Priv.active_page].role);
    }
    const UI_SAVE_SLOT_DIALOG_CHOICE choice =
        UI_SaveSlotDialog_Control(m_Priv.save_slot.state);
    switch (choice.action) {
    case UI_SAVE_SLOT_DIALOG_NO_CHOICE:
        if (M_IMMEDIATE) {
            // Make sure it's not possible to confirm empty slots
            g_Input.menu_confirm = false;
            g_InputDB.menu_confirm = false;
        } else {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }
        break;

    case UI_SAVE_SLOT_DIALOG_CANCEL:
        M_SoftClose(inv_item);
        break;

    case UI_SAVE_SLOT_DIALOG_DETAILS:
        M_NavigateInto(choice.slot_num);
        m_Priv.pages[m_Priv.active_page].role = PASSPORT_ROLE_STORY_SO_FAR;
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
        M_ShowStorySoFar();
        break;

    case UI_SAVE_SLOT_DIALOG_CONFIRM:
        m_Priv.selection = choice.slot_num;
        break;
    }
}

static void M_StorySoFar(INVENTORY_ITEM *const inv_item)
{
    if (g_InputDB.menu_left || g_InputDB.menu_back) {
        UI_SelectLevelDialog_Free(m_Priv.select_level.state);
        m_Priv.select_level.state = nullptr;
        m_Priv.pages[m_Priv.active_page].role = PASSPORT_ROLE_LOAD_GAME;
        m_Priv.selection = m_Priv.outer_selection;
        M_InitLoadSaveRequester(m_Priv.pages[m_Priv.active_page].role);
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
        M_ShowSaves(PASSPORT_ROLE_LOAD_GAME);
    } else {
        M_ShowStorySoFar();
    }
}

static void M_LoadSaveGame(INVENTORY_ITEM *const inv_item)
{
    if (m_Priv.mode == M_MODE_BROWSE) {
        if (g_InputDB.menu_confirm) {
            M_InitLoadSaveRequester(m_Priv.pages[m_Priv.active_page].role);
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            m_Priv.mode = M_MODE_PICK_OPTION;
        }
    } else {
        M_ShowSaves(inv_item);
    }
}

static void M_LoadGame(INVENTORY_ITEM *const inv_item)
{
    M_ChangePageTextContent(GS(PASSPORT_LOAD_GAME));
    M_LoadSaveGame(inv_item);
}

static void M_SaveGame(INVENTORY_ITEM *const inv_item)
{
    M_ChangePageTextContent(GS(PASSPORT_SAVE_GAME));
    M_LoadSaveGame(inv_item);
}

static void M_NewGame(INVENTORY_ITEM *const inv_item)
{
    M_ChangePageTextContent(GS(PASSPORT_NEW_GAME));
    if (m_Priv.mode == M_MODE_BROWSE) {
        if (g_InputDB.menu_confirm
            && (g_Config.gameplay.enable_game_modes
                || g_Config.profile.new_game_plus_unlock)) {
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            m_Priv.selection = GF_GetFirstLevel()->num;
            m_Priv.mode = M_MODE_PICK_OPTION;
        }
    } else if (m_Priv.mode == M_MODE_PICK_OPTION) {
        if (m_Priv.new_game.state == nullptr) {
            m_Priv.new_game.state = UI_NewGame_Init();
        }
        const int32_t choice = UI_NewGame_Control(m_Priv.new_game.state);
        if (choice == UI_REQUESTER_NO_CHOICE) {
            if (!M_IMMEDIATE) {
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            }
        } else if (choice == UI_REQUESTER_CANCEL) {
            M_SoftClose(inv_item);
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

static void M_Restart(INVENTORY_ITEM *const inv_item)
{
    M_ChangePageTextContent(GS(PASSPORT_RESTART_LEVEL));
}

static void M_ShowPage(INVENTORY_ITEM *const inv_item)
{
    switch (m_Priv.pages[m_Priv.active_page].role) {
    case PASSPORT_ROLE_LOAD_GAME:
        M_LoadGame(inv_item);
        break;

    case PASSPORT_ROLE_SAVE_GAME:
        M_SaveGame(inv_item);
        break;

    case PASSPORT_ROLE_STORY_SO_FAR:
        M_StorySoFar(inv_item);
        break;

    case PASSPORT_ROLE_NEW_GAME:
        M_NewGame(inv_item);
        break;

    case PASSPORT_ROLE_SELECT_LEVEL:
        M_PlayAnyLevel(inv_item);
        break;

    case PASSPORT_ROLE_RESTART:
        M_Restart(inv_item);
        break;

    case PASSPORT_ROLE_EXIT_GAME:
        M_ChangePageTextContent(GS(PASSPORT_EXIT_GAME));
        break;

    case PASSPORT_ROLE_EXIT_TITLE:
#if TR_VERSION == 2
        if (g_GameFlow.is_demo_version) {
            M_ChangePageTextContent(GS(PASSPORT_EXIT_DEMO));
        } else {
            M_ChangePageTextContent(GS(PASSPORT_EXIT_TO_TITLE));
        }
#else
        M_ChangePageTextContent(GS(PASSPORT_EXIT_TO_TITLE));
#endif
        break;

    default:
        break;
    }
}

static void M_HandleFlipInputs(void)
{
    if (g_InputDB.menu_left) {
        for (M_PAGE_NUMBER page = m_Priv.active_page - 1; page >= PAGE_1;
             page--) {
            if (m_Priv.pages[page].available) {
                m_Priv.active_page = page;
                break;
            }
        }
    } else if (g_InputDB.menu_right) {
        for (M_PAGE_NUMBER page = m_Priv.active_page + 1; page < PAGE_COUNT;
             page++) {
            if (m_Priv.pages[page].available) {
                m_Priv.active_page = page;
                break;
            }
        }
    }
}

void Option_Passport_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    if (m_Priv.active_page == PAGE_UNDETERMINED) {
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

    m_Priv.current_page = M_GetCurrentPage(inv_item);
    if (m_Priv.current_page < m_Priv.active_page) {
        M_FlipRight(inv_item);
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    } else if (m_Priv.current_page > m_Priv.active_page) {
        M_FlipLeft(inv_item);
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    } else {
        m_Priv.is_ready = true;
        M_SyncArrowsVisibility();
        M_ShowPage(inv_item);
        if (g_InputDB.menu_confirm) {
            g_Passport.select_role = m_Priv.pages[m_Priv.active_page].role;
            g_Passport.select_slot = m_Priv.selection;
            M_Close(inv_item);
        } else if (g_InputDB.menu_back) {
            if (g_Inv_Mode == INV_DEATH_MODE) {
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            } else {
                M_SoftClose(inv_item);
            }
        } else {
            M_HandleFlipInputs();
        }
    }
}

void Option_Passport_Draw(INVENTORY_ITEM *const inv_item)
{
    if (m_Priv.mode == M_MODE_BROWSE) {
        return;
    }

    switch (m_Priv.pages[m_Priv.active_page].role) {
    case PASSPORT_ROLE_NEW_GAME:
        if (m_Priv.new_game.state != nullptr) {
            UI_NewGame(m_Priv.new_game.state);
        }
        break;

    case PASSPORT_ROLE_SELECT_LEVEL:
        if (m_Priv.play_any_level.state != nullptr) {
            UI_PlayAnyLevelDialog(m_Priv.play_any_level.state);
        }
        break;

    case PASSPORT_ROLE_STORY_SO_FAR:
        if (m_Priv.select_level.state != nullptr) {
            UI_SelectLevelDialog(m_Priv.select_level.state);
        }
        break;

    case PASSPORT_ROLE_LOAD_GAME:
    case PASSPORT_ROLE_SAVE_GAME:
        if (m_Priv.is_ready && m_Priv.save_slot.state != nullptr) {
            UI_SaveSlotDialog(m_Priv.save_slot.state);
        }
        break;

    case PASSPORT_ROLE_RESTART:
    case PASSPORT_ROLE_EXIT_TITLE:
    case PASSPORT_ROLE_EXIT_GAME:
        break;
    }
}

void Option_Passport_Close(void)
{
    M_RemoveAllText();
    m_Priv.active_page = -1;
}
