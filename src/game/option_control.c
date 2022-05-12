#include "game/option.h"

#include "config.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/screen.h"
#include "game/settings.h"
#include "game/text.h"
#include "global/types.h"
#include "specific/s_input.h"
#include "util.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TOP_Y -60
#define BORDER 4
#define HEADER_HEIGHT 25
#define ROW_HEIGHT 17

typedef struct TEXT_COLUMN_PLACEMENT {
    int option;
    int col_num;
} TEXT_COLUMN_PLACEMENT;

static int32_t m_KeyMode = 0;
static int32_t m_KeyChange = 0;

static TEXTSTRING *m_Text[2] = { 0 };
static TEXTSTRING *m_TextA[INPUT_KEY_NUMBER_OF] = { 0 };
static TEXTSTRING *m_TextB[INPUT_KEY_NUMBER_OF] = { 0 };
static TEXTSTRING *m_TextArrowLeft = NULL;
static TEXTSTRING *m_TextArrowRight = NULL;

static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementNormal[] = {
    // left column
    { INPUT_KEY_UP, 0 },
    { INPUT_KEY_DOWN, 0 },
    { INPUT_KEY_LEFT, 0 },
    { INPUT_KEY_RIGHT, 0 },
    { INPUT_KEY_STEP_L, 0 },
    { INPUT_KEY_STEP_R, 0 },
    { INPUT_KEY_LOOK, 0 },
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
    { INPUT_KEY_ROLL, 1 },
    { INPUT_KEY_OPTION, 1 },
    { INPUT_KEY_PAUSE, 1 },
    { -1, 1 },
    { -1, 1 },
    { -1, 1 },
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
    { INPUT_KEY_LOOK, 0 },
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
    { INPUT_KEY_ROLL, 1 },
    { INPUT_KEY_OPTION, 1 },
    { INPUT_KEY_PAUSE, 1 },
    { -1, 1 },
    { INPUT_KEY_FLY_CHEAT, 1 },
    { INPUT_KEY_ITEM_CHEAT, 1 },
    { INPUT_KEY_LEVEL_SKIP_CHEAT, 1 },
    { INPUT_KEY_TURBO_CHEAT, 1 },
    // end
    { -1, -1 },
};

static void Option_ControlInitText(void);
static void Option_ControlUpdateText(void);
static void Option_ControlShutdownText(void);

static void Option_ControlInitText(void)
{
    m_Text[0] = Text_Create(
        0, TOP_Y - BORDER + (HEADER_HEIGHT + BORDER - ROW_HEIGHT) / 2,
        g_GameFlow.strings
            [g_Config.input.layout ? GS_CONTROL_USER_KEYS
                                   : GS_CONTROL_DEFAULT_KEYS]);
    Text_CentreH(m_Text[0], 1);
    Text_CentreV(m_Text[0], 1);

    const int16_t centre = Screen_GetResWidthDownscaled() / 2;
    int16_t max_y = 0;

    m_TextArrowLeft = Text_Create(
        -75, TOP_Y - BORDER + (HEADER_HEIGHT + BORDER - ROW_HEIGHT) / 2,
        "\200");
    Text_CentreH(m_TextArrowLeft, 1);
    Text_CentreV(m_TextArrowLeft, 1);
    m_TextArrowRight = Text_Create(
        70, TOP_Y - BORDER + (HEADER_HEIGHT + BORDER - ROW_HEIGHT) / 2, "\201");
    Text_CentreH(m_TextArrowRight, 1);
    Text_CentreV(m_TextArrowRight, 1);

    m_Text[1] = Text_Create(0, TOP_Y - BORDER, " ");
    Text_CentreH(m_Text[1], 1);
    Text_CentreV(m_Text[1], 1);

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    if (!m_TextB[0]) {
        int16_t xs[2] = { centre - 200, centre + 20 };
        int16_t ys[2] = { TOP_Y + HEADER_HEIGHT, TOP_Y + HEADER_HEIGHT };

        for (const TEXT_COLUMN_PLACEMENT *col = cols;
             col->col_num >= 0 && col->col_num <= 1; col++) {
            int16_t x = xs[col->col_num];
            int16_t y = ys[col->col_num];

            const char *scancode_name = S_Input_GetKeyCodeName(
                S_Input_GetAssignedKeyCode(g_Config.input.layout, col->option));
            if (col->option != -1 && scancode_name) {
                m_TextB[col->option] = Text_Create(x, y, scancode_name);
                Text_CentreV(m_TextB[col->option], 1);
            }

            ys[col->col_num] += ROW_HEIGHT;
            max_y = MAX(max_y, ys[col->col_num]);
        }

        m_KeyChange = 0;
    }

    if (!m_TextA[0]) {
        int16_t xs[2] = { centre - 130, centre + 90 };
        int16_t ys[2] = { TOP_Y + HEADER_HEIGHT, TOP_Y + HEADER_HEIGHT };

        for (const TEXT_COLUMN_PLACEMENT *col = cols;
             col->col_num >= 0 && col->col_num <= 1; col++) {
            int16_t x = xs[col->col_num];
            int16_t y = ys[col->col_num];

            if (col->option != -1) {
                m_TextA[col->option] = Text_Create(
                    x, y,
                    g_GameFlow
                        .strings[col->option + GS_KEYMAP_RUN - INPUT_KEY_UP]);
                Text_CentreV(m_TextA[col->option], 1);
            }

            ys[col->col_num] += ROW_HEIGHT;
            max_y = MAX(max_y, ys[col->col_num]);
        }
    }

    int16_t width = 420;
    int16_t height = max_y + BORDER * 2 - TOP_Y;
    Text_AddBackground(m_Text[1], width, height, 0, 0);
    Text_AddOutline(m_Text[1], 1);

    Option_FlashConflicts();

    m_KeyChange = -1;
    Text_AddBackground(m_Text[0], 0, 0, 0, 0);
    Text_AddOutline(m_Text[0], 1);
}

static void Option_ControlUpdateText(void)
{
    Text_ChangeText(
        m_Text[0],
        g_GameFlow.strings
            [g_Config.input.layout ? GS_CONTROL_USER_KEYS
                                   : GS_CONTROL_DEFAULT_KEYS]);

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    for (const TEXT_COLUMN_PLACEMENT *col = cols;
         col->col_num >= 0 && col->col_num <= 1; col++) {
        const char *scancode_name = S_Input_GetKeyCodeName(
            S_Input_GetAssignedKeyCode(g_Config.input.layout, col->option));
        if (col->option != -1 && scancode_name) {
            Text_ChangeText(m_TextB[col->option], scancode_name);
        }
    }
}

static void Option_ControlShutdownText(void)
{
    Text_Remove(m_Text[0]);
    Text_Remove(m_Text[1]);
    m_Text[0] = NULL;
    m_Text[1] = NULL;
    Text_Remove(m_TextArrowLeft);
    m_TextArrowLeft = NULL;
    Text_Remove(m_TextArrowRight);
    m_TextArrowRight = NULL;
    for (int i = 0; i < INPUT_KEY_NUMBER_OF; i++) {
        Text_Remove(m_TextA[i]);
        Text_Remove(m_TextB[i]);
        m_TextB[i] = NULL;
        m_TextA[i] = NULL;
    }
}

void Option_FlashConflicts(void)
{
    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    for (const TEXT_COLUMN_PLACEMENT *item = cols; item->col_num != -1;
         item++) {
        Text_Flash(m_TextB[item->option], 0, 0);
    }

    for (const TEXT_COLUMN_PLACEMENT *item1 = cols; item1->col_num != -1;
         item1++) {
        if (item1->option == -1) {
            continue;
        }
        S_INPUT_KEYCODE key_code1 =
            S_Input_GetAssignedKeyCode(g_Config.input.layout, item1->option);
        for (const TEXT_COLUMN_PLACEMENT *item2 = item1 + 1;
             item2->col_num != -1; item2++) {
            if (item2->option == -1) {
                continue;
            }
            S_INPUT_KEYCODE key_code2 = S_Input_GetAssignedKeyCode(
                g_Config.input.layout, item2->option);
            if (item1 != item2 && key_code1 == key_code2) {
                Text_Flash(m_TextB[item1->option], 1, 20);
                Text_Flash(m_TextB[item2->option], 1, 20);
            }
        }
    }
}

void Option_DefaultConflict(void)
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

void Option_Control(INVENTORY_ITEM *inv_item)
{
    if (!m_Text[0]) {
        Option_ControlInitText();
    }

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
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

    switch (m_KeyMode) {
    case 0:
        if (g_InputDB.left || g_InputDB.right) {
            if (m_KeyChange == -1) {
                g_Config.input.layout ^= 1;
                Option_ControlUpdateText();
                Option_FlashConflicts();
                Settings_Write();
            } else {
                Text_RemoveBackground(m_TextA[m_KeyChange]);
                Text_RemoveOutline(m_TextA[m_KeyChange]);

                int col_idx[2] = { 0, 0 };
                const TEXT_COLUMN_PLACEMENT *sel_col;

                for (sel_col = cols;
                     sel_col->col_num >= 0 && sel_col->col_num <= 1;
                     sel_col++) {
                    col_idx[sel_col->col_num]++;
                    if (sel_col->option == m_KeyChange) {
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
                            m_KeyChange = dst_col->option;
                            break;
                        }
                    }
                }

                Text_AddBackground(m_TextA[m_KeyChange], 0, 0, 0, 0);
                Text_AddOutline(m_TextA[m_KeyChange], 1);
            }
        } else if (
            g_InputDB.deselect || (g_InputDB.select && m_KeyChange == -1)) {
            Option_ControlShutdownText();
            Option_DefaultConflict();
            return;
        }

        if (g_Config.input.layout) {
            if (g_InputDB.select) {
                m_KeyMode = 1;
                Text_RemoveBackground(m_TextA[m_KeyChange]);
                Text_AddBackground(m_TextB[m_KeyChange], 0, 0, 0, 0);
                Text_RemoveOutline(m_TextA[m_KeyChange]);
                Text_AddOutline(m_TextB[m_KeyChange], 1);
            } else if (g_InputDB.forward) {
                Text_RemoveBackground(
                    m_KeyChange == -1 ? m_Text[0] : m_TextA[m_KeyChange]);
                Text_RemoveOutline(
                    m_KeyChange == -1 ? m_Text[0] : m_TextA[m_KeyChange]);
                Text_Hide(m_TextArrowLeft, true);
                Text_Hide(m_TextArrowRight, true);
                if (m_KeyChange == -1) {
                    m_KeyChange = last_col->option;
                } else if (m_KeyChange == first_col->option) {
                    m_KeyChange = -1;
                    Text_Hide(m_TextArrowLeft, false);
                    Text_Hide(m_TextArrowRight, false);
                } else {
                    const TEXT_COLUMN_PLACEMENT *sel_col;
                    for (sel_col = cols;
                         sel_col->col_num >= 0 && sel_col->col_num <= 1;
                         sel_col++) {
                        if (sel_col->option == m_KeyChange) {
                            break;
                        }
                    }
                    sel_col--;
                    while (sel_col >= cols) {
                        if (sel_col->option != -1) {
                            m_KeyChange = sel_col->option;
                            break;
                        }
                        sel_col--;
                    }
                }

                Text_AddBackground(
                    m_KeyChange == -1 ? m_Text[0] : m_TextA[m_KeyChange], 0, 0,
                    0, 0);
                Text_AddOutline(
                    m_KeyChange == -1 ? m_Text[0] : m_TextA[m_KeyChange], 1);
            } else if (g_InputDB.back) {
                Text_RemoveBackground(
                    m_KeyChange == -1 ? m_Text[0] : m_TextA[m_KeyChange]);
                Text_RemoveOutline(
                    m_KeyChange == -1 ? m_Text[0] : m_TextA[m_KeyChange]);
                Text_Hide(m_TextArrowLeft, true);
                Text_Hide(m_TextArrowRight, true);
                if (m_KeyChange == -1) {
                    m_KeyChange = first_col->option;
                } else if (m_KeyChange == last_col->option) {
                    m_KeyChange = -1;
                    Text_Hide(m_TextArrowLeft, false);
                    Text_Hide(m_TextArrowRight, false);
                } else {
                    const TEXT_COLUMN_PLACEMENT *sel_col;
                    for (sel_col = cols;
                         sel_col->col_num >= 0 && sel_col->col_num <= 1;
                         sel_col++) {
                        if (sel_col->option == m_KeyChange) {
                            break;
                        }
                    }
                    sel_col++;
                    while (sel_col >= cols) {
                        if (sel_col->option != -1) {
                            m_KeyChange = sel_col->option;
                            break;
                        }
                        sel_col++;
                    }
                }

                Text_AddBackground(
                    m_KeyChange == -1 ? m_Text[0] : m_TextA[m_KeyChange], 0, 0,
                    0, 0);
                Text_AddOutline(
                    m_KeyChange == -1 ? m_Text[0] : m_TextA[m_KeyChange], 1);
            }
        }
        break;

    case 1:
        if (!g_Input.select) {
            m_KeyMode = 2;
        }
        break;

    case 2: {
        S_INPUT_KEYCODE key_code = S_Input_ReadKeyCode();

        const char *scancode_name = S_Input_GetKeyCodeName(key_code);
        if (key_code >= 0 && scancode_name) {
            S_Input_AssignKeyCode(g_Config.input.layout, m_KeyChange, key_code);
            Text_ChangeText(m_TextB[m_KeyChange], scancode_name);
            Text_RemoveBackground(m_TextB[m_KeyChange]);
            Text_RemoveOutline(m_TextB[m_KeyChange]);
            Text_AddBackground(m_TextA[m_KeyChange], 0, 0, 0, 0);
            Text_AddOutline(m_TextA[m_KeyChange], 1);
            m_KeyMode = 3;
            Option_FlashConflicts();
            Settings_Write();
        }
        break;
    }

    case 3: {
        S_INPUT_KEYCODE key_code =
            S_Input_GetAssignedKeyCode(g_Config.input.layout, m_KeyChange);

        if (S_Input_ReadKeyCode() < 0 || S_Input_ReadKeyCode() != key_code) {
            m_KeyMode = 0;
            Option_FlashConflicts();
            Settings_Write();
        }

        m_KeyMode = 0;
        break;
    }
    }

    g_Input = (INPUT_STATE) { 0 };
    g_InputDB = (INPUT_STATE) { 0 };
}
