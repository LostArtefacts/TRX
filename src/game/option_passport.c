#include "game/option.h"

#include "game/game.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/requester.h"
#include "game/screen.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/vars.h"

#define PASSPORT_PAGE_COUNT 3
#define MAX_GAME_MODES 4
#define MAX_GAME_MODE_LENGTH 20

static int32_t m_PassportMode = 0;
static TEXTSTRING *m_PassportText = NULL;

static char m_NewGameStrings[MAX_GAME_MODES][MAX_GAME_MODE_LENGTH] = { 0 };
static REQUEST_INFO m_NewGameRequester = {
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
    .item_texts = &m_NewGameStrings[0][0],
    .item_text_len = MAX_GAME_MODE_LENGTH,
    0,
};

static char m_LoadSaveGameStrings[MAX_SAVE_SLOTS][MAX_LEVEL_NAME_LENGTH] = {
    0
};
REQUEST_INFO g_LoadSaveGameRequester = {
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
    .item_texts = &m_LoadSaveGameStrings[0][0],
    .item_text_len = MAX_LEVEL_NAME_LENGTH,
    0,
};

static void InitNewGameRequester()
{
    REQUEST_INFO *req = &m_NewGameRequester;
    InitRequester(req);
    SetRequesterHeading(req, g_GameFlow.strings[GS_PASSPORT_SELECT_MODE]);
    AddRequesterItem(req, g_GameFlow.strings[GS_PASSPORT_MODE_NEW_GAME], 0);
    AddRequesterItem(
        req, g_GameFlow.strings[GS_PASSPORT_MODE_NEW_GAME_PLUS], 0);
    AddRequesterItem(req, g_GameFlow.strings[GS_PASSPORT_MODE_NEW_GAME_JP], 0);
    AddRequesterItem(
        req, g_GameFlow.strings[GS_PASSPORT_MODE_NEW_GAME_JP_PLUS], 0);
    req->y = -30 * Screen_GetResHeightDownscaled() / 100;
    req->vis_lines = MAX_GAME_MODES;
}

static void InitLoadSaveGameRequester()
{
    REQUEST_INFO *req = &g_LoadSaveGameRequester;
    InitRequester(req);
    GetSavedGamesList(req);
    SetRequesterHeading(req, g_GameFlow.strings[GS_PASSPORT_SELECT_LEVEL]);

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

void Option_Passport(INVENTORY_ITEM *inv_item)
{
    Text_Remove(g_InvItemText[0]);
    g_InvItemText[IT_NAME] = NULL;

    int16_t page = (inv_item->goal_frame - inv_item->open_frame) / 5;
    if ((inv_item->goal_frame - inv_item->open_frame) % 5) {
        page = -1;
    }

    if (g_InvMode == INV_LOAD_MODE || g_InvMode == INV_SAVE_MODE
        || g_InvMode == INV_SAVE_CRYSTAL_MODE) {
        g_InputDB.left = 0;
        g_InputDB.right = 0;
    }

    switch (page) {
    case 0:
        if (m_PassportMode == 1) {
            int32_t select = DisplayRequester(&g_LoadSaveGameRequester);
            if (select) {
                if (select > 0) {
                    g_InvExtraData[1] = select - 1;
                } else if (
                    g_InvMode != INV_SAVE_MODE
                    && g_InvMode != INV_SAVE_CRYSTAL_MODE
                    && g_InvMode != INV_LOAD_MODE) {
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                }
                m_PassportMode = 0;
            } else {
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            }
        } else if (m_PassportMode == 0) {
            if (!g_SavedGamesCount || g_InvMode == INV_SAVE_MODE
                || g_InvMode == INV_SAVE_CRYSTAL_MODE) {
                g_InputDB = (INPUT_STATE) { 0, .right = 1 };
            } else {
                if (!m_PassportText) {
                    m_PassportText = Text_Create(
                        0, -16, g_GameFlow.strings[GS_PASSPORT_LOAD_GAME]);
                    Text_AlignBottom(m_PassportText, 1);
                    Text_CentreH(m_PassportText, 1);
                }
                if (g_InputDB.select || g_InvMode == INV_LOAD_MODE) {
                    Text_Remove(g_InvRingText);
                    g_InvRingText = NULL;
                    Text_Remove(g_InvItemText[IT_NAME]);
                    g_InvItemText[IT_NAME] = NULL;

                    g_LoadSaveGameRequester.flags |= RIF_BLOCKABLE;
                    InitLoadSaveGameRequester();
                    m_PassportMode = 1;
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                }
            }
        }
        break;

    case 1:
        if (m_PassportMode == 2) {
            int32_t select = DisplayRequester(&m_NewGameRequester);
            if (select) {
                if (select > 0) {
                    g_InvExtraData[1] = select - 1;
                } else if (g_InvMode != INV_GAME_MODE) {
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                }
                m_PassportMode = 0;
            } else {
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            }
        } else if (m_PassportMode == 1) {
            int32_t select = DisplayRequester(&g_LoadSaveGameRequester);
            if (select) {
                if (select > 0) {
                    m_PassportMode = 0;
                    g_InvExtraData[1] = select - 1;
                } else {
                    if (g_InvMode != INV_SAVE_MODE
                        && g_InvMode != INV_SAVE_CRYSTAL_MODE
                        && g_InvMode != INV_LOAD_MODE) {
                        g_Input = (INPUT_STATE) { 0 };
                        g_InputDB = (INPUT_STATE) { 0 };
                    }
                    m_PassportMode = 0;
                }
            } else {
                g_Input = (INPUT_STATE) { 0 };
                g_InputDB = (INPUT_STATE) { 0 };
            }
        } else if (m_PassportMode == 0) {
            if (g_InvMode == INV_DEATH_MODE) {
                if (inv_item->anim_direction == -1) {
                    g_InputDB = (INPUT_STATE) { 0, .left = 1 };
                } else {
                    g_InputDB = (INPUT_STATE) { 0, .right = 1 };
                }
            }
            if (!m_PassportText) {
                if (g_InvMode == INV_TITLE_MODE
                    || g_CurrentLevel == g_GameFlow.gym_level_num) {
                    m_PassportText = Text_Create(
                        0, -16, g_GameFlow.strings[GS_PASSPORT_NEW_GAME]);
                } else {
                    m_PassportText = Text_Create(
                        0, -16, g_GameFlow.strings[GS_PASSPORT_SAVE_GAME]);
                }
                Text_AlignBottom(m_PassportText, 1);
                Text_CentreH(m_PassportText, 1);
            }
            if (g_InputDB.select || g_InvMode == INV_SAVE_MODE
                || g_InvMode == INV_SAVE_CRYSTAL_MODE) {
                if (g_InvMode == INV_TITLE_MODE
                    || g_CurrentLevel == g_GameFlow.gym_level_num) {
                    Text_Remove(g_InvRingText);
                    g_InvRingText = NULL;
                    Text_Remove(g_InvItemText[IT_NAME]);
                    g_InvItemText[IT_NAME] = NULL;

                    if (g_GameFlow.enable_game_modes) {
                        InitNewGameRequester();
                        m_PassportMode = 2;
                        g_Input = (INPUT_STATE) { 0 };
                        g_InputDB = (INPUT_STATE) { 0 };
                    } else {
                        g_InvExtraData[1] = g_SaveGame.bonus_flag;
                    }
                } else {
                    Text_Remove(g_InvRingText);
                    g_InvRingText = NULL;
                    Text_Remove(g_InvItemText[IT_NAME]);
                    g_InvItemText[IT_NAME] = NULL;

                    g_LoadSaveGameRequester.flags &= ~RIF_BLOCKABLE;
                    InitLoadSaveGameRequester();
                    m_PassportMode = 1;
                    g_Input = (INPUT_STATE) { 0 };
                    g_InputDB = (INPUT_STATE) { 0 };
                }
            }
        }
        break;

    case 2:
        if (!m_PassportText) {
            if (g_InvMode == INV_TITLE_MODE) {
                m_PassportText = Text_Create(
                    0, -16, g_GameFlow.strings[GS_PASSPORT_EXIT_GAME]);
            } else {
                m_PassportText = Text_Create(
                    0, -16, g_GameFlow.strings[GS_PASSPORT_EXIT_TO_TITLE]);
            }
            Text_AlignBottom(m_PassportText, 1);
            Text_CentreH(m_PassportText, 1);
        }
        break;
    }

    bool pages_available[3] = {
        g_SavedGamesCount > 0,
        g_InvMode == INV_TITLE_MODE || g_InvMode == INV_SAVE_CRYSTAL_MODE
            || !g_GameFlow.enable_save_crystals,
        true,
    };

    if (g_InputDB.left && (g_InvMode != INV_DEATH_MODE || g_SavedGamesCount)) {
        while (--page >= 0) {
            if (pages_available[page]) {
                break;
            }
        }

        if (page >= 0) {
            inv_item->anim_direction = -1;
            inv_item->goal_frame = inv_item->open_frame + 5 * page;
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            if (m_PassportText) {
                Text_Remove(m_PassportText);
                m_PassportText = NULL;
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
            if (m_PassportText) {
                Text_Remove(m_PassportText);
                m_PassportText = NULL;
            }
        }
    }

    if (g_InputDB.deselect) {
        if (g_InvMode == INV_DEATH_MODE) {
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
            if (m_PassportText) {
                Text_Remove(m_PassportText);
                m_PassportText = NULL;
            }
        }
    }

    if (g_InputDB.select) {
        g_InvExtraData[0] = page;
        if (page == 2) {
            inv_item->anim_direction = 1;
            inv_item->goal_frame = inv_item->frames_total - 1;
        } else {
            inv_item->goal_frame = 0;
            inv_item->anim_direction = -1;
        }
        if (m_PassportText) {
            Text_Remove(m_PassportText);
            m_PassportText = NULL;
        }
    }
}
