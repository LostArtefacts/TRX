#include "game/option/option_control.h"

#include "config.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/screen.h"
#include "game/text.h"
#include "global/types.h"
#include "util.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TOP_Y -60
#define BORDER 4
#define HEADER_HEIGHT 25
#define ROW_HEIGHT 18
#define BOX_PADDING 10

#define KC_TITLE -1
#define COL_END -1

typedef struct LAYOUT_NUM_GS_MAP {
    INPUT_LAYOUT layout_num;
    GAME_STRING_ID layout_string;
} LAYOUT_NUM_GS_MAP;

static const LAYOUT_NUM_GS_MAP m_LayoutMap[] = {
    { INPUT_LAYOUT_DEFAULT, GS_CONTROL_DEFAULT_KEYS },
    { INPUT_LAYOUT_CUSTOM_1, GS_CONTROL_CUSTOM_1 },
    { INPUT_LAYOUT_CUSTOM_2, GS_CONTROL_CUSTOM_2 },
    { INPUT_LAYOUT_CUSTOM_3, GS_CONTROL_CUSTOM_3 },
};

typedef enum KEYMODE {
    KM_BROWSE = 0,
    KM_BROWSEKEYUP = 1,
    KM_CHANGE = 2,
    KM_CHANGEKEYUP = 3,
} KEYMODE;

typedef enum CONTROL_TEXT {
    TEXT_TITLE = 0,
    TEXT_TITLE_BORDER = 1,
    TEXT_LEFT_ARROW = 2,
    TEXT_RIGHT_ARROW = 3,
    TEXT_UP_ARROW = 4,
    TEXT_DOWN_ARROW = 5,
    TEXT_NUMBER_OF = 6,
} CONTROL_TEXT;

typedef struct TEXT_COLUMN_PLACEMENT {
    int option;
    GAME_STRING_ID game_string;
} TEXT_COLUMN_PLACEMENT;

typedef struct MENU {
    int32_t num_options;
    int32_t vis_options;
    const TEXT_COLUMN_PLACEMENT *head;
    const TEXT_COLUMN_PLACEMENT *tail;
    int32_t cur_option;
    int32_t prev_option;
    int32_t first_option;
    int32_t last_option;
    int32_t cur_row;
    int32_t prev_row;
    TEXTSTRING *role_texts[MAX_REQLINES];
    TEXTSTRING *key_texts[MAX_REQLINES];
} MENU;

static bool m_ControlLock = false;
static int32_t m_KeyMode = KM_BROWSE;
static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = { 0 };

static MENU m_ControlMenu = {
    .num_options = 0,
    .vis_options = 0,
    .head = NULL,
    .tail = NULL,
    .cur_option = KC_TITLE,
    .prev_option = KC_TITLE,
    .first_option = INPUT_ROLE_UP,
    .last_option = 0,
    .cur_row = KC_TITLE,
    .prev_row = KC_TITLE,
};

static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementNormal[] = {
    { INPUT_ROLE_UP, GS_KEYMAP_RUN },
    { INPUT_ROLE_DOWN, GS_KEYMAP_BACK },
    { INPUT_ROLE_LEFT, GS_KEYMAP_LEFT },
    { INPUT_ROLE_RIGHT, GS_KEYMAP_RIGHT },
    { INPUT_ROLE_STEP_L, GS_KEYMAP_STEP_LEFT },
    { INPUT_ROLE_STEP_R, GS_KEYMAP_STEP_RIGHT },
    { INPUT_ROLE_LOOK, GS_KEYMAP_LOOK },
    { INPUT_ROLE_EQUIP_PISTOLS, GS_KEYMAP_EQUIP_PISTOLS },
    { INPUT_ROLE_EQUIP_SHOTGUN, GS_KEYMAP_EQUIP_SHOTGUN },
    { INPUT_ROLE_EQUIP_MAGNUMS, GS_KEYMAP_EQUIP_MAGNUMS },
    { INPUT_ROLE_EQUIP_UZIS, GS_KEYMAP_EQUIP_UZIS },
    { INPUT_ROLE_CAMERA_UP, GS_KEYMAP_CAMERA_UP },
    { INPUT_ROLE_CAMERA_DOWN, GS_KEYMAP_CAMERA_DOWN },
    { INPUT_ROLE_CAMERA_LEFT, GS_KEYMAP_CAMERA_LEFT },
    { INPUT_ROLE_CAMERA_RIGHT, GS_KEYMAP_CAMERA_RIGHT },
    { INPUT_ROLE_CAMERA_RESET, GS_KEYMAP_CAMERA_RESET },
    { INPUT_ROLE_SLOW, GS_KEYMAP_WALK },
    { INPUT_ROLE_JUMP, GS_KEYMAP_JUMP },
    { INPUT_ROLE_ACTION, GS_KEYMAP_ACTION },
    { INPUT_ROLE_DRAW, GS_KEYMAP_DRAW_WEAPON },
    { INPUT_ROLE_ROLL, GS_KEYMAP_ROLL },
    { INPUT_ROLE_OPTION, GS_KEYMAP_INVENTORY },
    { INPUT_ROLE_PAUSE, GS_KEYMAP_PAUSE },
    { INPUT_ROLE_USE_SMALL_MEDI, GS_KEYMAP_USE_SMALL_MEDI },
    { INPUT_ROLE_USE_BIG_MEDI, GS_KEYMAP_USE_BIG_MEDI },
    { INPUT_ROLE_SAVE, GS_KEYMAP_SAVE },
    { INPUT_ROLE_LOAD, GS_KEYMAP_LOAD },
    { INPUT_ROLE_FPS, GS_KEYMAP_FPS },
    { INPUT_ROLE_BILINEAR, GS_KEYMAP_BILINEAR },
    // end
    { COL_END, -1 },
};

static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementCheats[] = {
    { INPUT_ROLE_UP, GS_KEYMAP_RUN },
    { INPUT_ROLE_DOWN, GS_KEYMAP_BACK },
    { INPUT_ROLE_LEFT, GS_KEYMAP_LEFT },
    { INPUT_ROLE_RIGHT, GS_KEYMAP_RIGHT },
    { INPUT_ROLE_STEP_L, GS_KEYMAP_STEP_LEFT },
    { INPUT_ROLE_STEP_R, GS_KEYMAP_STEP_RIGHT },
    { INPUT_ROLE_LOOK, GS_KEYMAP_LOOK },
    { INPUT_ROLE_EQUIP_PISTOLS, GS_KEYMAP_EQUIP_PISTOLS },
    { INPUT_ROLE_EQUIP_SHOTGUN, GS_KEYMAP_EQUIP_SHOTGUN },
    { INPUT_ROLE_EQUIP_MAGNUMS, GS_KEYMAP_EQUIP_MAGNUMS },
    { INPUT_ROLE_EQUIP_UZIS, GS_KEYMAP_EQUIP_UZIS },
    { INPUT_ROLE_CAMERA_UP, GS_KEYMAP_CAMERA_UP },
    { INPUT_ROLE_CAMERA_DOWN, GS_KEYMAP_CAMERA_DOWN },
    { INPUT_ROLE_CAMERA_LEFT, GS_KEYMAP_CAMERA_LEFT },
    { INPUT_ROLE_CAMERA_RIGHT, GS_KEYMAP_CAMERA_RIGHT },
    { INPUT_ROLE_CAMERA_RESET, GS_KEYMAP_CAMERA_RESET },
    { INPUT_ROLE_SLOW, GS_KEYMAP_WALK },
    { INPUT_ROLE_JUMP, GS_KEYMAP_JUMP },
    { INPUT_ROLE_ACTION, GS_KEYMAP_ACTION },
    { INPUT_ROLE_DRAW, GS_KEYMAP_DRAW_WEAPON },
    { INPUT_ROLE_ROLL, GS_KEYMAP_ROLL },
    { INPUT_ROLE_OPTION, GS_KEYMAP_INVENTORY },
    { INPUT_ROLE_PAUSE, GS_KEYMAP_PAUSE },
    { INPUT_ROLE_USE_SMALL_MEDI, GS_KEYMAP_USE_SMALL_MEDI },
    { INPUT_ROLE_USE_BIG_MEDI, GS_KEYMAP_USE_BIG_MEDI },
    { INPUT_ROLE_SAVE, GS_KEYMAP_SAVE },
    { INPUT_ROLE_LOAD, GS_KEYMAP_LOAD },
    { INPUT_ROLE_FPS, GS_KEYMAP_FPS },
    { INPUT_ROLE_BILINEAR, GS_KEYMAP_BILINEAR },
    { INPUT_ROLE_FLY_CHEAT, GS_KEYMAP_FLY_CHEAT },
    { INPUT_ROLE_ITEM_CHEAT, GS_KEYMAP_ITEM_CHEAT },
    { INPUT_ROLE_LEVEL_SKIP_CHEAT, GS_KEYMAP_LEVEL_SKIP_CHEAT },
    { INPUT_ROLE_TURBO_CHEAT, GS_KEYMAP_TURBO_CHEAT },
    // end
    { COL_END, -1 },
};

static void Option_ControlInitMenu(void);
static void Option_ControlInitText(void);
static void Option_ControlUpdateText(void);
static void Option_ControlShutdownText(void);
static void Option_ControlFlashConflicts(void);
static void Option_ControlChangeLayout(void);

static void Option_ControlInitMenu(void)
{
    int32_t visible_lines = 0;
    if (Screen_GetResHeightDownscaled() <= 240) {
        visible_lines = 5;
    } else if (Screen_GetResHeightDownscaled() <= 384) {
        visible_lines = 8;
    } else if (Screen_GetResHeightDownscaled() <= 480) {
        visible_lines = 10;
    } else {
        visible_lines = 12;
    }
    m_ControlMenu.vis_options = visible_lines;

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    for (const TEXT_COLUMN_PLACEMENT *col = cols; col->option != COL_END;
         col++) {
        m_ControlMenu.num_options++;
    }

    m_ControlMenu.vis_options =
        MIN(m_ControlMenu.num_options, m_ControlMenu.vis_options);
}

static void Option_ControlInitText(void)
{
    Option_ControlInitMenu();

    m_Text[TEXT_TITLE_BORDER] = Text_Create(0, TOP_Y - BORDER, " ");
    Text_CentreH(m_Text[TEXT_TITLE_BORDER], true);
    Text_CentreV(m_Text[TEXT_TITLE_BORDER], true);

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    const TEXT_COLUMN_PLACEMENT *col = cols;
    m_ControlMenu.head = col;
    const int16_t centre = Screen_GetResWidthDownscaled() / 2;
    int16_t x_roles = centre - 150;
    int16_t box_width = 315;
    int16_t x_names = -centre + box_width / 2 - BORDER;
    int16_t y = TOP_Y + ROW_HEIGHT + BORDER * 2;
    for (int i = 0; i < m_ControlMenu.vis_options; i++) {
        m_ControlMenu.role_texts[i] =
            Text_Create(x_roles, y, g_GameFlow.strings[col->game_string]);
        Text_CentreV(m_ControlMenu.role_texts[i], true);

        m_ControlMenu.key_texts[i] = Text_Create(
            x_names, y, Input_GetKeyName(g_Config.input.layout, col->option));
        Text_CentreV(m_ControlMenu.key_texts[i], true);
        Text_AlignRight(m_ControlMenu.key_texts[i], true);

        y += ROW_HEIGHT;
        m_ControlMenu.tail = col;
        col++;
    }

    m_Text[TEXT_TITLE] = Text_Create(
        0, TOP_Y - BORDER / 2,
        g_GameFlow.strings[m_LayoutMap[g_Config.input.layout].layout_string]);
    Text_CentreH(m_Text[TEXT_TITLE], true);
    Text_CentreV(m_Text[TEXT_TITLE], true);
    Text_AddBackground(m_Text[TEXT_TITLE], 0, 0, 0, 0, TS_REQUESTED);
    Text_AddOutline(m_Text[TEXT_TITLE], true, TS_REQUESTED);

    int32_t tw = Text_GetWidth(m_Text[TEXT_TITLE]);

    m_Text[TEXT_LEFT_ARROW] = Text_Create(
        m_Text[TEXT_TITLE]->pos.x - tw / 2 - 20, m_Text[TEXT_TITLE]->pos.y,
        "\200");
    Text_CentreH(m_Text[TEXT_LEFT_ARROW], true);
    Text_CentreV(m_Text[TEXT_LEFT_ARROW], true);

    m_Text[TEXT_RIGHT_ARROW] = Text_Create(
        m_Text[TEXT_TITLE]->pos.x + tw / 2 + 15, m_Text[TEXT_TITLE]->pos.y,
        "\201");
    Text_CentreH(m_Text[TEXT_RIGHT_ARROW], true);
    Text_CentreV(m_Text[TEXT_RIGHT_ARROW], true);

    m_Text[TEXT_UP_ARROW] =
        Text_Create(0, m_ControlMenu.key_texts[0]->pos.y - 12, "[");
    Text_SetScale(m_Text[TEXT_UP_ARROW], PHD_ONE * 2 / 3, PHD_ONE * 2 / 3);
    Text_CentreH(m_Text[TEXT_UP_ARROW], true);
    Text_CentreV(m_Text[TEXT_UP_ARROW], true);

    m_Text[TEXT_DOWN_ARROW] = Text_Create(
        0, m_ControlMenu.key_texts[m_ControlMenu.vis_options - 1]->pos.y + 12,
        "]");
    Text_SetScale(m_Text[TEXT_DOWN_ARROW], PHD_ONE * 2 / 3, PHD_ONE * 2 / 3);
    Text_CentreH(m_Text[TEXT_DOWN_ARROW], true);
    Text_CentreV(m_Text[TEXT_DOWN_ARROW], true);

    int16_t lines_height = m_ControlMenu.vis_options * ROW_HEIGHT;
    int16_t box_height = lines_height + ROW_HEIGHT + BOX_PADDING * 2 + BORDER;

    Text_AddBackground(
        m_Text[TEXT_TITLE_BORDER], box_width, box_height, 0, 0, TS_BACKGROUND);
    Text_AddOutline(m_Text[TEXT_TITLE_BORDER], true, TS_BACKGROUND);

    Option_ControlFlashConflicts();

    for (const TEXT_COLUMN_PLACEMENT *col = cols; col->option != COL_END;
         col++) {
        if (col->option != COL_END) {
            m_ControlMenu.last_option = col->option;
        }
    }
}

static void Option_ControlUpdateText(void)
{
    if (m_ControlMenu.cur_option == KC_TITLE) {
        Text_ChangeText(
            m_Text[TEXT_TITLE],
            g_GameFlow
                .strings[m_LayoutMap[g_Config.input.layout].layout_string]);

        int32_t title_w = Text_GetWidth(m_Text[TEXT_TITLE]);
        Text_SetPos(
            m_Text[TEXT_LEFT_ARROW],
            m_Text[TEXT_TITLE]->pos.x - title_w / 2 - 20,
            m_Text[TEXT_TITLE]->pos.y);
        Text_SetPos(
            m_Text[TEXT_RIGHT_ARROW],
            m_Text[TEXT_TITLE]->pos.x + title_w / 2 + 15,
            m_Text[TEXT_TITLE]->pos.y);

        Text_Hide(m_Text[TEXT_LEFT_ARROW], false);
        Text_Hide(m_Text[TEXT_RIGHT_ARROW], false);
    } else {
        Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
        Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
    }

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    const TEXT_COLUMN_PLACEMENT *col = m_ControlMenu.head;
    for (int i = 0; i < m_ControlMenu.vis_options; i++) {
        Text_ChangeText(
            m_ControlMenu.role_texts[i], g_GameFlow.strings[col->game_string]);
        Text_ChangeText(
            m_ControlMenu.key_texts[i],
            Input_GetKeyName(g_Config.input.layout, col->option));
        col++;
    }

    switch (m_KeyMode) {
    case KM_BROWSE:
        Text_RemoveBackground(
            m_ControlMenu.prev_row == KC_TITLE
                ? m_Text[TEXT_TITLE]
                : m_ControlMenu.role_texts[m_ControlMenu.prev_row]);
        Text_RemoveOutline(
            m_ControlMenu.prev_row == KC_TITLE
                ? m_Text[TEXT_TITLE]
                : m_ControlMenu.role_texts[m_ControlMenu.prev_row]);
        Text_AddBackground(
            m_ControlMenu.cur_row == KC_TITLE
                ? m_Text[TEXT_TITLE]
                : m_ControlMenu.role_texts[m_ControlMenu.cur_row],
            0, 0, 0, 0, TS_REQUESTED);
        Text_AddOutline(
            m_ControlMenu.cur_row == KC_TITLE
                ? m_Text[TEXT_TITLE]
                : m_ControlMenu.role_texts[m_ControlMenu.cur_row],
            true, TS_REQUESTED);
        break;
    case KM_BROWSEKEYUP:
        Text_RemoveBackground(m_ControlMenu.role_texts[m_ControlMenu.prev_row]);
        Text_RemoveOutline(m_ControlMenu.role_texts[m_ControlMenu.prev_row]);
        Text_AddBackground(
            m_ControlMenu.key_texts[m_ControlMenu.cur_row], 0, 0, 0, 0,
            TS_REQUESTED);
        Text_AddOutline(
            m_ControlMenu.key_texts[m_ControlMenu.cur_row], true, TS_REQUESTED);
        break;
    case KM_CHANGE:
        break;
    case KM_CHANGEKEYUP:
        Text_ChangeText(
            m_ControlMenu.key_texts[m_ControlMenu.cur_row],
            Input_GetKeyName(g_Config.input.layout, m_ControlMenu.cur_option));
        Text_RemoveBackground(m_ControlMenu.key_texts[m_ControlMenu.prev_row]);
        Text_RemoveOutline(m_ControlMenu.key_texts[m_ControlMenu.prev_row]);
        Text_AddBackground(
            m_ControlMenu.role_texts[m_ControlMenu.cur_row], 0, 0, 0, 0,
            TS_REQUESTED);
        Text_AddOutline(
            m_ControlMenu.role_texts[m_ControlMenu.cur_row], true,
            TS_REQUESTED);
        break;
    }
}

static void Option_ControlShutdownText(void)
{
    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_Remove(m_Text[i]);
        m_Text[TEXT_TITLE] = NULL;
    }
    for (int i = 0; i < m_ControlMenu.vis_options; i++) {
        Text_Remove(m_ControlMenu.role_texts[i]);
        m_ControlMenu.role_texts[i] = NULL;
        Text_Remove(m_ControlMenu.key_texts[i]);
        m_ControlMenu.key_texts[i] = NULL;
    }
    m_ControlMenu.num_options = 0;
    m_ControlMenu.vis_options = 0;
    m_ControlMenu.head = NULL;
    m_ControlMenu.tail = NULL;
    m_ControlMenu.cur_option = KC_TITLE;
    m_ControlMenu.prev_option = KC_TITLE;
    m_ControlMenu.first_option = INPUT_ROLE_UP;
    m_ControlMenu.last_option = 0;
    m_ControlMenu.cur_row = KC_TITLE;
    m_ControlMenu.prev_row = KC_TITLE;
}

static void Option_ControlFlashConflicts(void)
{
    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    const TEXT_COLUMN_PLACEMENT *col = m_ControlMenu.head;
    for (int i = 0; i < m_ControlMenu.vis_options; i++) {
        Text_Flash(
            m_ControlMenu.key_texts[i],
            g_Config.input.layout != INPUT_LAYOUT_DEFAULT
                && Input_IsKeyConflictedWithUser(col->option),
            20);
        col++;
    }
}

static void Option_ControlChangeLayout(void)
{
    if (g_InputDB.left || g_InputDB.right) {
        g_Config.input.layout += g_InputDB.left ? -1 : 0;
        g_Config.input.layout += g_InputDB.right ? 1 : 0;
        g_Config.input.layout += INPUT_LAYOUT_NUMBER_OF;
        g_Config.input.layout %= INPUT_LAYOUT_NUMBER_OF;
    }

    Input_CheckConflicts(g_Config.input.layout);
    Option_ControlUpdateText();
    Option_ControlFlashConflicts();
    Config_Write();
}

bool Option_ControlIsLocked(void) {
    return m_ControlLock;
}

void Option_Control(INVENTORY_ITEM *inv_item)
{
    if (!m_Text[TEXT_TITLE]) {
        m_ControlLock = true;
        Option_ControlInitText();
    }

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    switch (m_KeyMode) {
    case KM_BROWSE:
        if (g_InputDB.deselect
            || (g_InputDB.select && m_ControlMenu.cur_option == KC_TITLE)) {
            Option_ControlShutdownText();
            m_ControlLock = false;
            return;
        }

        if ((g_InputDB.left || g_InputDB.right)
            && m_ControlMenu.cur_option == KC_TITLE) {
            Option_ControlChangeLayout();
        }

        if (g_Config.input.layout > INPUT_LAYOUT_DEFAULT) {
            if (g_InputDB.select) {
                m_KeyMode = KM_BROWSEKEYUP;
            } else if (g_InputDB.forward) {
                if (m_ControlMenu.cur_option == KC_TITLE) {
                    m_ControlMenu.cur_row = m_ControlMenu.vis_options - 1;
                    m_ControlMenu.cur_option = m_ControlMenu.last_option;
                    m_ControlMenu.head = cols + m_ControlMenu.num_options - 1
                        - m_ControlMenu.vis_options + 1;
                    m_ControlMenu.tail = cols + m_ControlMenu.num_options - 1;
                } else if (
                    m_ControlMenu.cur_option == m_ControlMenu.first_option) {
                    m_ControlMenu.cur_row = KC_TITLE;
                    m_ControlMenu.cur_option = KC_TITLE;
                } else {
                    if (m_ControlMenu.cur_row > 0
                        && m_ControlMenu.cur_option
                            != m_ControlMenu.head->option) {
                        m_ControlMenu.cur_row--;
                    } else if (
                        m_ControlMenu.head->option
                        != m_ControlMenu.first_option) {
                        m_ControlMenu.head--;
                        m_ControlMenu.tail--;
                    } else {
                        m_ControlMenu.cur_row--;
                    }

                    const TEXT_COLUMN_PLACEMENT *sel_col;
                    for (sel_col = cols; sel_col->option != COL_END;
                         sel_col++) {
                        if (sel_col->option == m_ControlMenu.cur_option) {
                            break;
                        }
                    }
                    sel_col--;
                    m_ControlMenu.cur_option = sel_col->option;
                }
                Option_ControlUpdateText();
                Option_ControlFlashConflicts();
            } else if (g_InputDB.back) {
                if (m_ControlMenu.cur_option == KC_TITLE) {
                    m_ControlMenu.cur_row++;
                    m_ControlMenu.cur_option = m_ControlMenu.first_option;
                } else if (
                    m_ControlMenu.cur_option == m_ControlMenu.last_option) {
                    m_ControlMenu.cur_row = KC_TITLE;
                    m_ControlMenu.cur_option = KC_TITLE;
                    m_ControlMenu.head = cols;
                    m_ControlMenu.tail = cols + m_ControlMenu.vis_options - 1;
                } else {
                    if (m_ControlMenu.cur_row
                        >= m_ControlMenu.vis_options - 1) {
                        m_ControlMenu.head++;
                        m_ControlMenu.tail++;
                    } else {
                        m_ControlMenu.cur_row++;
                    }

                    const TEXT_COLUMN_PLACEMENT *sel_col;
                    for (sel_col = cols; sel_col->option != COL_END;
                         sel_col++) {
                        if (sel_col->option == m_ControlMenu.cur_option) {
                            break;
                        }
                    }
                    sel_col++;
                    m_ControlMenu.cur_option = sel_col->option;
                }
                Option_ControlUpdateText();
                Option_ControlFlashConflicts();
            }
        }
        break;

    case KM_BROWSEKEYUP:
        if (!g_Input.any) {
            Option_ControlUpdateText();
            m_KeyMode = KM_CHANGE;
        }
        break;

    case KM_CHANGE:
        if (Input_ReadAndAssignKey(
                g_Config.input.layout, m_ControlMenu.cur_option)) {
            Option_ControlUpdateText();
            m_KeyMode = KM_CHANGEKEYUP;
            Option_ControlFlashConflicts();
            Config_Write();
        }
        break;

    case KM_CHANGEKEYUP:
        if (!g_Input.any) {
            Option_ControlUpdateText();
            m_KeyMode = KM_BROWSE;
        }
        break;
    }

    g_Input = (INPUT_STATE) { 0 };
    g_InputDB = (INPUT_STATE) { 0 };

    m_ControlMenu.prev_option = m_ControlMenu.cur_option;
    m_ControlMenu.prev_row = m_ControlMenu.cur_row;
}
