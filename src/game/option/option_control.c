#include "game/option/option_control.h"

#include "config.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/screen.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "util.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TOP_Y -60
#define BORDER 4
#define HEADER_HEIGHT 25
#define ROW_HEIGHT 17
#define BOX_PADDING 10

#define KC_TITLE -1
#define COL_END -1
#define COLOR_STEPS 5
#define RESET_ALL_KEY "R"
#define RESET_ALL_BUTTON "rightshoulder"
#define UNBIND_KEY "Backspace"
#define UNBIND_BUTTON "x"
#define UNBIND_SCANCODE 0
#define UNBIND_ENUM -1
#define BUTTON_HOLD_TIME 2
#define HOLD_DELAY_FRAMES 300 * LOGIC_FPS / 1000

typedef enum KEYMODE {
    KM_INACTIVE = 0,
    KM_BROWSE = 1,
    KM_BROWSEKEYUP = 2,
    KM_CHANGE = 3,
    KM_CHANGEKEYUP = 4,
} KEYMODE;

typedef enum CONTROL_TEXT {
    TEXT_TITLE = 0,
    TEXT_TITLE_BORDER = 1,
    TEXT_LEFT_ARROW = 2,
    TEXT_RIGHT_ARROW = 3,
    TEXT_UP_ARROW = 4,
    TEXT_DOWN_ARROW = 5,
    TEXT_RESET_BORDER = 6,
    TEXT_RESET = 7,
    TEXT_UNBIND = 8,
    TEXT_NUMBER_OF = 9,
} CONTROL_TEXT;

typedef struct LAYOUT_NUM_GS_MAP {
    INPUT_LAYOUT layout_num;
    GAME_STRING_ID layout_string;
} LAYOUT_NUM_GS_MAP;

typedef struct TEXT_COLUMN_PLACEMENT {
    int role;
    GAME_STRING_ID game_string;
    bool can_unbind;
} TEXT_COLUMN_PLACEMENT;

typedef struct MENU {
    int32_t num_options;
    int32_t vis_options;
    const TEXT_COLUMN_PLACEMENT *top_row;
    const TEXT_COLUMN_PLACEMENT *bot_row;
    const TEXT_COLUMN_PLACEMENT *cur_row;
    int32_t cur_role;
    int32_t first_role;
    int32_t last_role;
    int32_t row_num;
    int32_t prev_row_num;
    TEXTSTRING *role_texts[MAX_REQLINES];
    TEXTSTRING *key_texts[MAX_REQLINES];
} MENU;

static int32_t m_KeyMode = KM_INACTIVE;
static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = { 0 };
static int32_t m_ResetTimer = 0;
static int32_t m_ResetKeyMode = KM_INACTIVE;
static int32_t m_ResetKeyDelay = 0;
static char m_ResetGS[100];
static int32_t m_UnbindTimer = 0;
static int32_t m_UnbindKeyMode = KM_INACTIVE;
static int32_t m_UnbindKeyDelay = 0;
static char m_UnbindGS[100];

static MENU m_ControlMenu = {
    .num_options = 0,
    .vis_options = 0,
    .top_row = NULL,
    .bot_row = NULL,
    .cur_role = KC_TITLE,
    .first_role = INPUT_ROLE_UP,
    .last_role = 0,
    .row_num = KC_TITLE,
    .prev_row_num = KC_TITLE,
};

static const LAYOUT_NUM_GS_MAP m_LayoutMap[] = {
    { INPUT_LAYOUT_DEFAULT, GS_CONTROL_DEFAULT_KEYS },
    { INPUT_LAYOUT_CUSTOM_1, GS_CONTROL_CUSTOM_1 },
    { INPUT_LAYOUT_CUSTOM_2, GS_CONTROL_CUSTOM_2 },
    { INPUT_LAYOUT_CUSTOM_3, GS_CONTROL_CUSTOM_3 },
};

static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementNormal[] = {
    { INPUT_ROLE_UP, GS_KEYMAP_RUN, false },
    { INPUT_ROLE_DOWN, GS_KEYMAP_BACK, false },
    { INPUT_ROLE_LEFT, GS_KEYMAP_LEFT, false },
    { INPUT_ROLE_RIGHT, GS_KEYMAP_RIGHT, false },
    { INPUT_ROLE_DRAW, GS_KEYMAP_DRAW_WEAPON, false },
    { INPUT_ROLE_ACTION, GS_KEYMAP_ACTION, false },
    { INPUT_ROLE_JUMP, GS_KEYMAP_JUMP, false },
    { INPUT_ROLE_ROLL, GS_KEYMAP_ROLL, false },
    { INPUT_ROLE_LOOK, GS_KEYMAP_LOOK, false },
    { INPUT_ROLE_SLOW, GS_KEYMAP_WALK, false },
    { INPUT_ROLE_STEP_L, GS_KEYMAP_STEP_LEFT, true },
    { INPUT_ROLE_STEP_R, GS_KEYMAP_STEP_RIGHT, true },
    { INPUT_ROLE_OPTION, GS_KEYMAP_INVENTORY, false },
    { INPUT_ROLE_PAUSE, GS_KEYMAP_PAUSE, true },
    { INPUT_ROLE_CAMERA_UP, GS_KEYMAP_CAMERA_UP, true },
    { INPUT_ROLE_CAMERA_DOWN, GS_KEYMAP_CAMERA_DOWN, true },
    { INPUT_ROLE_CAMERA_LEFT, GS_KEYMAP_CAMERA_LEFT, true },
    { INPUT_ROLE_CAMERA_RIGHT, GS_KEYMAP_CAMERA_RIGHT, true },
    { INPUT_ROLE_CAMERA_RESET, GS_KEYMAP_CAMERA_RESET, true },
    { INPUT_ROLE_EQUIP_PISTOLS, GS_KEYMAP_EQUIP_PISTOLS, true },
    { INPUT_ROLE_EQUIP_SHOTGUN, GS_KEYMAP_EQUIP_SHOTGUN, true },
    { INPUT_ROLE_EQUIP_MAGNUMS, GS_KEYMAP_EQUIP_MAGNUMS, true },
    { INPUT_ROLE_EQUIP_UZIS, GS_KEYMAP_EQUIP_UZIS, true },
    { INPUT_ROLE_USE_SMALL_MEDI, GS_KEYMAP_USE_SMALL_MEDI, true },
    { INPUT_ROLE_USE_BIG_MEDI, GS_KEYMAP_USE_BIG_MEDI, true },
    { INPUT_ROLE_SAVE, GS_KEYMAP_SAVE, true },
    { INPUT_ROLE_LOAD, GS_KEYMAP_LOAD, true },
    { INPUT_ROLE_FPS, GS_KEYMAP_FPS, true },
    { INPUT_ROLE_BILINEAR, GS_KEYMAP_BILINEAR, true },
    { INPUT_ROLE_ENTER_CONSOLE, GS_KEYMAP_ENTER_CONSOLE, true },
    // end
    { COL_END, -1, false },
};

static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementCheats[] = {
    { INPUT_ROLE_UP, GS_KEYMAP_RUN, false },
    { INPUT_ROLE_DOWN, GS_KEYMAP_BACK, false },
    { INPUT_ROLE_LEFT, GS_KEYMAP_LEFT, false },
    { INPUT_ROLE_RIGHT, GS_KEYMAP_RIGHT, false },
    { INPUT_ROLE_DRAW, GS_KEYMAP_DRAW_WEAPON, false },
    { INPUT_ROLE_ACTION, GS_KEYMAP_ACTION, false },
    { INPUT_ROLE_JUMP, GS_KEYMAP_JUMP, false },
    { INPUT_ROLE_ROLL, GS_KEYMAP_ROLL, false },
    { INPUT_ROLE_LOOK, GS_KEYMAP_LOOK, false },
    { INPUT_ROLE_SLOW, GS_KEYMAP_WALK, false },
    { INPUT_ROLE_STEP_L, GS_KEYMAP_STEP_LEFT, true },
    { INPUT_ROLE_STEP_R, GS_KEYMAP_STEP_RIGHT, true },
    { INPUT_ROLE_OPTION, GS_KEYMAP_INVENTORY, false },
    { INPUT_ROLE_PAUSE, GS_KEYMAP_PAUSE, true },
    { INPUT_ROLE_CAMERA_UP, GS_KEYMAP_CAMERA_UP, true },
    { INPUT_ROLE_CAMERA_DOWN, GS_KEYMAP_CAMERA_DOWN, true },
    { INPUT_ROLE_CAMERA_LEFT, GS_KEYMAP_CAMERA_LEFT, true },
    { INPUT_ROLE_CAMERA_RIGHT, GS_KEYMAP_CAMERA_RIGHT, true },
    { INPUT_ROLE_CAMERA_RESET, GS_KEYMAP_CAMERA_RESET, true },
    { INPUT_ROLE_EQUIP_PISTOLS, GS_KEYMAP_EQUIP_PISTOLS, true },
    { INPUT_ROLE_EQUIP_SHOTGUN, GS_KEYMAP_EQUIP_SHOTGUN, true },
    { INPUT_ROLE_EQUIP_MAGNUMS, GS_KEYMAP_EQUIP_MAGNUMS, true },
    { INPUT_ROLE_EQUIP_UZIS, GS_KEYMAP_EQUIP_UZIS, true },
    { INPUT_ROLE_USE_SMALL_MEDI, GS_KEYMAP_USE_SMALL_MEDI, true },
    { INPUT_ROLE_USE_BIG_MEDI, GS_KEYMAP_USE_BIG_MEDI, true },
    { INPUT_ROLE_SAVE, GS_KEYMAP_SAVE, true },
    { INPUT_ROLE_LOAD, GS_KEYMAP_LOAD, true },
    { INPUT_ROLE_FPS, GS_KEYMAP_FPS, true },
    { INPUT_ROLE_BILINEAR, GS_KEYMAP_BILINEAR, true },
    { INPUT_ROLE_FLY_CHEAT, GS_KEYMAP_FLY_CHEAT, true },
    { INPUT_ROLE_ITEM_CHEAT, GS_KEYMAP_ITEM_CHEAT, true },
    { INPUT_ROLE_LEVEL_SKIP_CHEAT, GS_KEYMAP_LEVEL_SKIP_CHEAT, true },
    { INPUT_ROLE_TURBO_CHEAT, GS_KEYMAP_TURBO_CHEAT, true },
    { INPUT_ROLE_ENTER_CONSOLE, GS_KEYMAP_ENTER_CONSOLE, true },
    // end
    { COL_END, -1, false },
};

static void Option_ControlInitMenu(void);
static void Option_ControlInitText(CONTROL_MODE mode, INPUT_LAYOUT layout_num);
static void Option_ControlUpdateText(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num);
static void Option_ControlShutdownText(void);
static void Option_ControlFlashConflicts(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num);
static INPUT_LAYOUT Option_ControlChangeLayout(CONTROL_MODE mode);
static void Option_ControlCheckResetKeys(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num);
static void Option_ControlCheckUnbindKey(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num);
static void Option_ControlProgressBar(TEXTSTRING *txt, int32_t timer);

static void Option_ControlInitMenu(void)
{
    int32_t visible_lines = 0;
    if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 240) {
        visible_lines = 6;
    } else if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 384) {
        visible_lines = 8;
    } else if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 480) {
        visible_lines = 12;
    } else {
        visible_lines = 18;
    }
    m_ControlMenu.vis_options = visible_lines;

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    m_ControlMenu.top_row = cols;
    m_ControlMenu.cur_row = cols;
    m_ControlMenu.bot_row = cols + m_ControlMenu.vis_options - 1;

    for (const TEXT_COLUMN_PLACEMENT *col = cols; col->role != COL_END; col++) {
        m_ControlMenu.num_options++;
        m_ControlMenu.last_role = col->role;
    }

    m_ControlMenu.vis_options =
        MIN(m_ControlMenu.num_options, m_ControlMenu.vis_options);
}

static void Option_ControlInitText(CONTROL_MODE mode, INPUT_LAYOUT layout_num)
{
    Option_ControlInitMenu();

    m_Text[TEXT_TITLE_BORDER] = Text_Create(0, TOP_Y - BORDER, " ");
    Text_CentreH(m_Text[TEXT_TITLE_BORDER], true);
    Text_CentreV(m_Text[TEXT_TITLE_BORDER], true);

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    const TEXT_COLUMN_PLACEMENT *col = cols;
    const int16_t centre = Screen_GetResWidthDownscaled(RSR_TEXT) / 2;
    int16_t x_roles = centre - 150;
    int16_t box_width = 315;
    int16_t x_names = -centre + box_width / 2 - BORDER;
    int16_t y = TOP_Y + ROW_HEIGHT + BORDER * 2;
    for (int i = 0; i < m_ControlMenu.vis_options; i++) {
        m_ControlMenu.role_texts[i] =
            Text_Create(x_roles, y, g_GameFlow.strings[col->game_string]);
        Text_CentreV(m_ControlMenu.role_texts[i], true);

        m_ControlMenu.key_texts[i] = Text_Create(
            x_names, y, Input_GetKeyName(mode, layout_num, col->role));
        Text_CentreV(m_ControlMenu.key_texts[i], true);
        Text_AlignRight(m_ControlMenu.key_texts[i], true);

        y += ROW_HEIGHT;
        col++;
    }

    m_Text[TEXT_TITLE] = Text_Create(
        0, TOP_Y - BORDER / 2,
        g_GameFlow.strings[m_LayoutMap[layout_num].layout_string]);
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

    m_Text[TEXT_RESET_BORDER] = Text_Create(0, y + BOX_PADDING + BORDER, " ");
    Text_CentreH(m_Text[TEXT_RESET_BORDER], true);
    Text_CentreV(m_Text[TEXT_RESET_BORDER], true);
    Text_AddBackground(
        m_Text[TEXT_RESET_BORDER], box_width, ROW_HEIGHT, 0, 0, TS_BACKGROUND);

    if (mode == CM_KEYBOARD) {
        sprintf(
            m_ResetGS, g_GameFlow.strings[GS_CONTROL_RESET_DEFAULTS_KEY],
            RESET_ALL_KEY);
        sprintf(
            m_UnbindGS, g_GameFlow.strings[GS_CONTROL_UNBIND_KEY], UNBIND_KEY);
    } else {
        sprintf(
            m_ResetGS, g_GameFlow.strings[GS_CONTROL_RESET_DEFAULTS_BTN],
            Input_GetButtonName(g_Config.input.cntlr_layout, RESET_ALL_BUTTON));
        sprintf(
            m_UnbindGS, g_GameFlow.strings[GS_CONTROL_UNBIND_BTN],
            Input_GetButtonName(g_Config.input.cntlr_layout, UNBIND_BUTTON));
    }

    m_Text[TEXT_RESET] =
        Text_Create(x_roles, y + BOX_PADDING + BORDER, m_ResetGS);
    Text_CentreV(m_Text[TEXT_RESET], true);
    Text_SetScale(m_Text[TEXT_RESET], PHD_ONE * .8, PHD_ONE * .8);

    m_Text[TEXT_UNBIND] =
        Text_Create(x_names, y + BOX_PADDING + BORDER, m_UnbindGS);
    Text_CentreV(m_Text[TEXT_UNBIND], true);
    Text_AlignRight(m_Text[TEXT_UNBIND], true);
    Text_SetScale(m_Text[TEXT_UNBIND], PHD_ONE * .8, PHD_ONE * .8);

    if (layout_num == INPUT_LAYOUT_DEFAULT) {
        Text_Hide(m_Text[TEXT_RESET], true);
        Text_Hide(m_Text[TEXT_UNBIND], true);
    }

    Option_ControlUpdateText(mode, layout_num);
    Option_ControlFlashConflicts(mode, layout_num);
}

static void Option_ControlUpdateText(CONTROL_MODE mode, INPUT_LAYOUT layout_num)
{
    if (layout_num == INPUT_LAYOUT_DEFAULT) {
        Text_Hide(m_Text[TEXT_RESET], true);
        Text_Hide(m_Text[TEXT_RESET_BORDER], true);
        Text_Hide(m_Text[TEXT_UNBIND], true);
    } else {
        Text_Hide(m_Text[TEXT_RESET], false);
        Text_Hide(m_Text[TEXT_RESET_BORDER], false);

        if (m_ControlMenu.cur_role == KC_TITLE
            || !m_ControlMenu.cur_row->can_unbind) {
            Text_Hide(m_Text[TEXT_UNBIND], true);
        } else {
            Text_Hide(m_Text[TEXT_UNBIND], false);
        }
    }

    if (mode == CM_KEYBOARD) {
        sprintf(
            m_ResetGS, g_GameFlow.strings[GS_CONTROL_RESET_DEFAULTS_KEY],
            RESET_ALL_KEY);
        Text_ChangeText(m_Text[TEXT_RESET], m_ResetGS);
        sprintf(
            m_UnbindGS, g_GameFlow.strings[GS_CONTROL_UNBIND_KEY], UNBIND_KEY);
        Text_ChangeText(m_Text[TEXT_UNBIND], m_UnbindGS);
    } else {
        sprintf(
            m_ResetGS, g_GameFlow.strings[GS_CONTROL_RESET_DEFAULTS_BTN],
            Input_GetButtonName(layout_num, RESET_ALL_BUTTON));
        Text_ChangeText(m_Text[TEXT_RESET], m_ResetGS);
        sprintf(
            m_UnbindGS, g_GameFlow.strings[GS_CONTROL_UNBIND_BTN],
            Input_GetButtonName(layout_num, UNBIND_BUTTON));
        Text_ChangeText(m_Text[TEXT_UNBIND], m_UnbindGS);
    }

    if (m_ControlMenu.cur_role == KC_TITLE) {
        Text_ChangeText(
            m_Text[TEXT_TITLE],
            g_GameFlow.strings[m_LayoutMap[layout_num].layout_string]);

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

    const TEXT_COLUMN_PLACEMENT *col = m_ControlMenu.top_row;
    for (int i = 0; i < m_ControlMenu.vis_options; i++) {
        Text_ChangeText(
            m_ControlMenu.role_texts[i], g_GameFlow.strings[col->game_string]);
        Text_ChangeText(
            m_ControlMenu.key_texts[i],
            Input_GetKeyName(mode, layout_num, col->role));
        col++;
    }

    switch (m_KeyMode) {
    case KM_BROWSE:
        Text_RemoveBackground(
            m_ControlMenu.prev_row_num == KC_TITLE
                ? m_Text[TEXT_TITLE]
                : m_ControlMenu.role_texts[m_ControlMenu.prev_row_num]);
        Text_RemoveOutline(
            m_ControlMenu.prev_row_num == KC_TITLE
                ? m_Text[TEXT_TITLE]
                : m_ControlMenu.role_texts[m_ControlMenu.prev_row_num]);
        Text_AddBackground(
            m_ControlMenu.row_num == KC_TITLE
                ? m_Text[TEXT_TITLE]
                : m_ControlMenu.role_texts[m_ControlMenu.row_num],
            0, 0, 0, 0, TS_REQUESTED);
        Text_AddOutline(
            m_ControlMenu.row_num == KC_TITLE
                ? m_Text[TEXT_TITLE]
                : m_ControlMenu.role_texts[m_ControlMenu.row_num],
            true, TS_REQUESTED);
        break;

    case KM_BROWSEKEYUP:
        Text_RemoveBackground(
            m_ControlMenu.role_texts[m_ControlMenu.prev_row_num]);
        Text_RemoveOutline(
            m_ControlMenu.role_texts[m_ControlMenu.prev_row_num]);
        Text_AddBackground(
            m_ControlMenu.key_texts[m_ControlMenu.row_num], 0, 0, 0, 0,
            TS_REQUESTED);
        Text_AddOutline(
            m_ControlMenu.key_texts[m_ControlMenu.row_num], true, TS_REQUESTED);
        break;

    case KM_CHANGE:
        break;

    case KM_CHANGEKEYUP:
        Text_ChangeText(
            m_ControlMenu.key_texts[m_ControlMenu.row_num],
            Input_GetKeyName(mode, layout_num, m_ControlMenu.cur_role));
        Text_RemoveBackground(
            m_ControlMenu.key_texts[m_ControlMenu.prev_row_num]);
        Text_RemoveOutline(m_ControlMenu.key_texts[m_ControlMenu.prev_row_num]);
        Text_AddBackground(
            m_ControlMenu.role_texts[m_ControlMenu.row_num], 0, 0, 0, 0,
            TS_REQUESTED);
        Text_AddOutline(
            m_ControlMenu.role_texts[m_ControlMenu.row_num], true,
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
    m_ControlMenu.top_row = NULL;
    m_ControlMenu.bot_row = NULL;
    m_ControlMenu.cur_role = KC_TITLE;
    m_ControlMenu.first_role = INPUT_ROLE_UP;
    m_ControlMenu.last_role = 0;
    m_ControlMenu.row_num = KC_TITLE;
    m_ControlMenu.prev_row_num = KC_TITLE;

    m_ResetTimer = 0;
    m_ResetKeyMode = 0;
    m_ResetKeyDelay = 0;
    m_UnbindTimer = 0;
    m_UnbindKeyMode = 0;
    m_UnbindKeyDelay = 0;
}

static void Option_ControlFlashConflicts(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num)
{
    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    const TEXT_COLUMN_PLACEMENT *col = m_ControlMenu.top_row;
    for (int i = 0; i < m_ControlMenu.vis_options; i++) {
        Text_Flash(
            m_ControlMenu.key_texts[i],
            layout_num != INPUT_LAYOUT_DEFAULT
                && Input_IsKeyConflicted(mode, col->role),
            20);
        col++;
    }
}

static INPUT_LAYOUT Option_ControlChangeLayout(CONTROL_MODE mode)
{
    INPUT_LAYOUT layout_num = INPUT_LAYOUT_DEFAULT;
    if (mode == CM_KEYBOARD) {
        g_Config.input.layout += g_InputDB.menu_left ? -1 : 0;
        g_Config.input.layout += g_InputDB.menu_right ? 1 : 0;
        g_Config.input.layout += INPUT_LAYOUT_NUMBER_OF;
        g_Config.input.layout %= INPUT_LAYOUT_NUMBER_OF;
        layout_num = g_Config.input.layout;
    }

    if (mode == CM_CONTROLLER) {
        g_Config.input.cntlr_layout += g_InputDB.menu_left ? -1 : 0;
        g_Config.input.cntlr_layout += g_InputDB.menu_right ? 1 : 0;
        g_Config.input.cntlr_layout += INPUT_LAYOUT_NUMBER_OF;
        g_Config.input.cntlr_layout %= INPUT_LAYOUT_NUMBER_OF;
        layout_num = g_Config.input.cntlr_layout;
    }

    Input_CheckConflicts(mode, layout_num);
    Option_ControlUpdateText(mode, layout_num);
    Option_ControlFlashConflicts(mode, layout_num);
    Config_Write();
    return layout_num;
}

static void Option_ControlProgressBar(TEXTSTRING *txt, int32_t timer)
{
    int32_t width = Text_GetWidth(txt);
    int32_t height = TEXT_HEIGHT;

    int32_t x = txt->pos.x;
    int32_t y = txt->pos.y - TEXT_HEIGHT;

    if (txt->flags.centre_h) {
        x += (Screen_GetResWidthDownscaled(RSR_TEXT) - width) / 2;
    } else if (txt->flags.right) {
        x += Screen_GetResWidthDownscaled(RSR_TEXT) - width;
    }

    if (txt->flags.centre_v) {
        y += Screen_GetResHeightDownscaled(RSR_TEXT) / 2;
    } else if (txt->flags.bottom) {
        y += Screen_GetResHeightDownscaled(RSR_TEXT);
    }

    int32_t percent = (timer * 100) / (LOGIC_FPS * BUTTON_HOLD_TIME);
    CLAMP(percent, 0, 100);
    Text_AddProgressBar(
        txt, width, height, x, y, percent, g_Config.ui.menu_style);
}

static void Option_ControlCheckResetKeys(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num)
{
    if ((Input_CheckKeypress(RESET_ALL_KEY)
         || Input_CheckButtonPress(RESET_ALL_BUTTON))
        && m_ResetKeyMode != KM_CHANGEKEYUP) {
        m_ResetKeyDelay++;
        if (m_ResetKeyDelay >= HOLD_DELAY_FRAMES) {
            m_ResetKeyMode = KM_CHANGE;
            m_ResetTimer++;
            if (m_ResetTimer >= LOGIC_FPS * BUTTON_HOLD_TIME) {
                Sound_Effect(SFX_MENU_GAMEBOY, NULL, SPM_NORMAL);
                Input_ResetLayout(mode, layout_num);
                Input_CheckConflicts(mode, layout_num);
                Option_ControlUpdateText(mode, layout_num);
                Option_ControlFlashConflicts(mode, layout_num);
                Config_Write();
                m_ResetKeyMode = KM_CHANGEKEYUP;
                m_ResetTimer = 0;
            }
        }
    } else if (m_ResetKeyMode == KM_CHANGEKEYUP) {
        if (!Input_CheckKeypress(RESET_ALL_KEY)
            && !Input_CheckButtonPress(RESET_ALL_BUTTON)) {
            m_ResetKeyMode = KM_INACTIVE;
        }
    } else {
        m_ResetTimer = 0;
        m_ResetKeyMode = KM_INACTIVE;
        m_ResetKeyDelay = 0;
    }
    CLAMP(m_ResetTimer, 0, LOGIC_FPS * BUTTON_HOLD_TIME);
    Option_ControlProgressBar(m_Text[TEXT_RESET], m_ResetTimer);
}

static void Option_ControlCheckUnbindKey(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num)
{
    if ((Input_CheckKeypress(UNBIND_KEY)
         || Input_CheckButtonPress(UNBIND_BUTTON))
        && m_UnbindKeyMode != KM_CHANGEKEYUP) {
        m_UnbindKeyDelay++;
        if (m_UnbindKeyDelay >= HOLD_DELAY_FRAMES) {
            m_UnbindKeyMode = KM_CHANGE;
            m_UnbindTimer++;
            if (m_UnbindTimer >= LOGIC_FPS * BUTTON_HOLD_TIME) {
                Sound_Effect(SFX_MENU_GAMEBOY, NULL, SPM_NORMAL);
                if (mode == CM_KEYBOARD) {
                    Input_AssignScancode(
                        layout_num, m_ControlMenu.cur_role, UNBIND_SCANCODE);
                } else {
                    Input_AssignButton(
                        layout_num, m_ControlMenu.cur_role, UNBIND_ENUM);
                }
                Input_CheckConflicts(mode, layout_num);
                Option_ControlUpdateText(mode, layout_num);
                Option_ControlFlashConflicts(mode, layout_num);
                Config_Write();
                m_UnbindKeyMode = KM_CHANGEKEYUP;
                m_UnbindTimer = 0;
            }
        }
    } else if (m_UnbindKeyMode == KM_CHANGEKEYUP) {
        if (!Input_CheckKeypress(UNBIND_KEY)
            && !Input_CheckButtonPress(UNBIND_BUTTON)) {
            m_UnbindKeyMode = KM_INACTIVE;
        }
    } else {
        m_UnbindTimer = 0;
        m_UnbindKeyMode = KM_INACTIVE;
        m_UnbindKeyDelay = 0;
    }
    CLAMP(m_UnbindTimer, 0, LOGIC_FPS * BUTTON_HOLD_TIME);
    Option_ControlProgressBar(m_Text[TEXT_UNBIND], m_UnbindTimer);
}

CONTROL_MODE Option_Control(INVENTORY_ITEM *inv_item, CONTROL_MODE mode)
{
    INPUT_LAYOUT layout_num = INPUT_LAYOUT_DEFAULT;
    if (mode == CM_KEYBOARD) {
        layout_num = g_Config.input.layout;
    } else {
        layout_num = g_Config.input.cntlr_layout;
    }

    if (!m_Text[TEXT_TITLE]) {
        m_KeyMode = KM_BROWSE;
        Option_ControlInitText(mode, layout_num);
    }

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    switch (m_KeyMode) {
    case KM_BROWSE:
        if (layout_num > INPUT_LAYOUT_DEFAULT) {
            if (m_UnbindKeyMode == KM_INACTIVE) {
                Option_ControlCheckResetKeys(mode, layout_num);
            }

            if (m_ResetKeyMode == KM_INACTIVE
                && m_ControlMenu.cur_row->can_unbind) {
                Option_ControlCheckUnbindKey(mode, layout_num);
            }
        }

        if (m_ResetKeyMode != KM_INACTIVE || m_UnbindKeyMode != KM_INACTIVE) {
            break;
        }

        if (g_InputDB.menu_back
            || (g_InputDB.menu_confirm && m_ControlMenu.cur_role == KC_TITLE)) {
            Option_ControlShutdownText();
            m_KeyMode = KM_INACTIVE;
            g_Input = (INPUT_STATE) { 0 };
            g_InputDB = (INPUT_STATE) { 0 };
            return CM_PICK;
        }

        if ((g_InputDB.menu_left || g_InputDB.menu_right)
            && m_ControlMenu.cur_role == KC_TITLE) {
            layout_num = Option_ControlChangeLayout(mode);
        }

        if (g_InputDB.menu_confirm) {
            if (layout_num > INPUT_LAYOUT_DEFAULT) {
                m_KeyMode = KM_BROWSEKEYUP;
            }
        } else if (g_InputDB.menu_up) {
            if (m_ControlMenu.cur_role == KC_TITLE) {
                m_ControlMenu.row_num = m_ControlMenu.vis_options - 1;
                m_ControlMenu.cur_role = m_ControlMenu.last_role;
                m_ControlMenu.top_row = cols + m_ControlMenu.num_options - 1
                    - m_ControlMenu.vis_options + 1;
                m_ControlMenu.bot_row = cols + m_ControlMenu.num_options - 1;
                m_ControlMenu.cur_row = m_ControlMenu.bot_row;
            } else if (m_ControlMenu.cur_role == m_ControlMenu.first_role) {
                m_ControlMenu.row_num = KC_TITLE;
                m_ControlMenu.cur_role = KC_TITLE;
                m_ControlMenu.cur_row = cols + m_ControlMenu.num_options;
            } else {
                if (m_ControlMenu.row_num > 0
                    && m_ControlMenu.cur_role != m_ControlMenu.top_row->role) {
                    m_ControlMenu.row_num--;
                } else if (
                    m_ControlMenu.top_row->role != m_ControlMenu.first_role) {
                    m_ControlMenu.top_row--;
                    m_ControlMenu.bot_row--;
                } else {
                    m_ControlMenu.row_num--;
                }

                m_ControlMenu.cur_row--;
                m_ControlMenu.cur_role = m_ControlMenu.cur_row->role;
            }
            Option_ControlUpdateText(mode, layout_num);
            Option_ControlFlashConflicts(mode, layout_num);
        } else if (g_InputDB.menu_down) {
            if (m_ControlMenu.cur_role == KC_TITLE) {
                m_ControlMenu.row_num++;
                m_ControlMenu.cur_role = m_ControlMenu.first_role;
                m_ControlMenu.cur_row = m_ControlMenu.top_row;
            } else if (m_ControlMenu.cur_role == m_ControlMenu.last_role) {
                m_ControlMenu.row_num = KC_TITLE;
                m_ControlMenu.cur_role = KC_TITLE;
                m_ControlMenu.top_row = cols;
                m_ControlMenu.bot_row = cols + m_ControlMenu.vis_options - 1;
                m_ControlMenu.cur_row = cols + m_ControlMenu.num_options;
            } else {
                if (m_ControlMenu.row_num >= m_ControlMenu.vis_options - 1) {
                    m_ControlMenu.top_row++;
                    m_ControlMenu.bot_row++;
                } else {
                    m_ControlMenu.row_num++;
                }

                m_ControlMenu.cur_row++;
                m_ControlMenu.cur_role = m_ControlMenu.cur_row->role;
            }
            Option_ControlUpdateText(mode, layout_num);
            Option_ControlFlashConflicts(mode, layout_num);
        }
        break;

    case KM_BROWSEKEYUP:
        if (!g_Input.any) {
            Option_ControlUpdateText(mode, layout_num);
            m_KeyMode = KM_CHANGE;
        }
        break;

    case KM_CHANGE:
        if (Input_ReadAndAssignKey(mode, layout_num, m_ControlMenu.cur_role)) {
            Option_ControlUpdateText(mode, layout_num);
            m_KeyMode = KM_CHANGEKEYUP;
            Option_ControlFlashConflicts(mode, layout_num);
            Config_Write();
        }
        break;

    case KM_CHANGEKEYUP:
        if (!g_Input.any) {
            Option_ControlUpdateText(mode, layout_num);
            m_KeyMode = KM_BROWSE;
        }
        break;
    }

    g_Input = (INPUT_STATE) { 0 };
    g_InputDB = (INPUT_STATE) { 0 };

    m_ControlMenu.prev_row_num = m_ControlMenu.row_num;

    return mode;
}
