#include "game/option/option_controls.h"

#include "game/clock.h"
#include "game/game_string.h"
#include "game/overlay.h"
#include "game/screen.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/types.h"

#include <libtrx/config.h>
#include <libtrx/log.h>

#include <stdio.h>

#define TOP_Y -60
#define BORDER 4
#define ROW_HEIGHT 17
#define BOX_PADDING 10

#define KC_TITLE -1
#define COL_END -1
#define BUTTON_HOLD_TIME 2
#define HOLD_DELAY_FRAMES 300 * LOGIC_FPS / 1000

typedef enum {
    M_PROGERSS_BAR_RESET_ALL,
    M_PROGERSS_BAR_UNBIND,
    M_PROGRESS_BAR_NUMBER_OF,
} M_PROGRESS_BAR;

typedef enum {
    KM_INACTIVE = 0,
    KM_BROWSE = 1,
    KM_BROWSEKEYUP = 2,
    KM_CHANGE = 3,
    KM_CHANGEKEYUP = 4,
} KEYMODE;

typedef enum {
    TEXT_TITLE = 0,
    TEXT_TITLE_BORDER = 1,
    TEXT_LEFT_ARROW = 2,
    TEXT_RIGHT_ARROW = 3,
    TEXT_UP_ARROW = 4,
    TEXT_DOWN_ARROW = 5,
    TEXT_RESET = 7,
    TEXT_UNBIND = 8,
    TEXT_NUMBER_OF = 9,
} CONTROL_TEXT;

typedef struct {
    int role;
    GAME_STRING_ID game_string;
    bool can_unbind;
} TEXT_COLUMN_PLACEMENT;

typedef struct {
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
static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = {};
static int32_t m_ResetTimer = 0;
static int32_t m_ResetKeyMode = KM_INACTIVE;
static int32_t m_ResetKeyDelay = 0;
static char m_ResetGS[100];
static int32_t m_UnbindTimer = 0;
static int32_t m_UnbindKeyMode = KM_INACTIVE;
static int32_t m_UnbindKeyDelay = 0;
static char m_UnbindGS[100];
static BAR_INFO m_ProgressBars[M_PROGRESS_BAR_NUMBER_OF];

static MENU m_ControlMenu = {
    .num_options = 0,
    .vis_options = 0,
    .top_row = nullptr,
    .bot_row = nullptr,
    .cur_role = KC_TITLE,
    .first_role = INPUT_ROLE_UP,
    .last_role = 0,
    .row_num = KC_TITLE,
    .prev_row_num = KC_TITLE,
};

static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementNormal[] = {
    { INPUT_ROLE_UP, GS_ID(KEYMAP_RUN), false },
    { INPUT_ROLE_DOWN, GS_ID(KEYMAP_BACK), false },
    { INPUT_ROLE_LEFT, GS_ID(KEYMAP_LEFT), false },
    { INPUT_ROLE_RIGHT, GS_ID(KEYMAP_RIGHT), false },
    { INPUT_ROLE_DRAW, GS_ID(KEYMAP_DRAW_WEAPON), false },
    { INPUT_ROLE_ACTION, GS_ID(KEYMAP_ACTION), false },
    { INPUT_ROLE_JUMP, GS_ID(KEYMAP_JUMP), false },
    { INPUT_ROLE_ROLL, GS_ID(KEYMAP_ROLL), false },
    { INPUT_ROLE_LOOK, GS_ID(KEYMAP_LOOK), false },
    { INPUT_ROLE_SLOW, GS_ID(KEYMAP_WALK), false },
    { INPUT_ROLE_STEP_L, GS_ID(KEYMAP_STEP_LEFT), true },
    { INPUT_ROLE_STEP_R, GS_ID(KEYMAP_STEP_RIGHT), true },
    { INPUT_ROLE_OPTION, GS_ID(KEYMAP_INVENTORY), false },
    { INPUT_ROLE_PAUSE, GS_ID(KEYMAP_PAUSE), true },
    { INPUT_ROLE_CHANGE_TARGET, GS_ID(KEYMAP_CHANGE_TARGET), true },
    { INPUT_ROLE_TOGGLE_PHOTO_MODE, GS_ID(KEYMAP_TOGGLE_PHOTO_MODE), true },
    { INPUT_ROLE_CAMERA_UP, GS_ID(KEYMAP_CAMERA_UP), true },
    { INPUT_ROLE_CAMERA_DOWN, GS_ID(KEYMAP_CAMERA_DOWN), true },
    { INPUT_ROLE_CAMERA_FORWARD, GS_ID(KEYMAP_CAMERA_FORWARD), true },
    { INPUT_ROLE_CAMERA_BACK, GS_ID(KEYMAP_CAMERA_BACK), true },
    { INPUT_ROLE_CAMERA_LEFT, GS_ID(KEYMAP_CAMERA_LEFT), true },
    { INPUT_ROLE_CAMERA_RIGHT, GS_ID(KEYMAP_CAMERA_RIGHT), true },
    { INPUT_ROLE_EQUIP_PISTOLS, GS_ID(KEYMAP_EQUIP_PISTOLS), true },
    { INPUT_ROLE_EQUIP_SHOTGUN, GS_ID(KEYMAP_EQUIP_SHOTGUN), true },
    { INPUT_ROLE_EQUIP_MAGNUMS, GS_ID(KEYMAP_EQUIP_MAGNUMS), true },
    { INPUT_ROLE_EQUIP_UZIS, GS_ID(KEYMAP_EQUIP_UZIS), true },
    { INPUT_ROLE_USE_SMALL_MEDI, GS_ID(KEYMAP_USE_SMALL_MEDI), true },
    { INPUT_ROLE_USE_BIG_MEDI, GS_ID(KEYMAP_USE_BIG_MEDI), true },
    { INPUT_ROLE_SAVE, GS_ID(KEYMAP_SAVE), true },
    { INPUT_ROLE_LOAD, GS_ID(KEYMAP_LOAD), true },
    { INPUT_ROLE_FPS, GS_ID(KEYMAP_FPS), true },
    { INPUT_ROLE_BILINEAR, GS_ID(KEYMAP_BILINEAR), true },
    { INPUT_ROLE_ENTER_CONSOLE, GS_ID(KEYMAP_ENTER_CONSOLE), true },
    { INPUT_ROLE_TOGGLE_UI, GS_ID(KEYMAP_TOGGLE_UI), true },
    // end
    { COL_END, nullptr, false },
};

static const TEXT_COLUMN_PLACEMENT CtrlTextPlacementCheats[] = {
    { INPUT_ROLE_UP, GS_ID(KEYMAP_RUN), false },
    { INPUT_ROLE_DOWN, GS_ID(KEYMAP_BACK), false },
    { INPUT_ROLE_LEFT, GS_ID(KEYMAP_LEFT), false },
    { INPUT_ROLE_RIGHT, GS_ID(KEYMAP_RIGHT), false },
    { INPUT_ROLE_DRAW, GS_ID(KEYMAP_DRAW_WEAPON), false },
    { INPUT_ROLE_ACTION, GS_ID(KEYMAP_ACTION), false },
    { INPUT_ROLE_JUMP, GS_ID(KEYMAP_JUMP), false },
    { INPUT_ROLE_ROLL, GS_ID(KEYMAP_ROLL), false },
    { INPUT_ROLE_LOOK, GS_ID(KEYMAP_LOOK), false },
    { INPUT_ROLE_SLOW, GS_ID(KEYMAP_WALK), false },
    { INPUT_ROLE_STEP_L, GS_ID(KEYMAP_STEP_LEFT), true },
    { INPUT_ROLE_STEP_R, GS_ID(KEYMAP_STEP_RIGHT), true },
    { INPUT_ROLE_OPTION, GS_ID(KEYMAP_INVENTORY), false },
    { INPUT_ROLE_PAUSE, GS_ID(KEYMAP_PAUSE), true },
    { INPUT_ROLE_CHANGE_TARGET, GS_ID(KEYMAP_CHANGE_TARGET), true },
    { INPUT_ROLE_TOGGLE_PHOTO_MODE, GS_ID(KEYMAP_TOGGLE_PHOTO_MODE), true },
    { INPUT_ROLE_CAMERA_UP, GS_ID(KEYMAP_CAMERA_UP), true },
    { INPUT_ROLE_CAMERA_DOWN, GS_ID(KEYMAP_CAMERA_DOWN), true },
    { INPUT_ROLE_CAMERA_FORWARD, GS_ID(KEYMAP_CAMERA_FORWARD), true },
    { INPUT_ROLE_CAMERA_BACK, GS_ID(KEYMAP_CAMERA_BACK), true },
    { INPUT_ROLE_CAMERA_LEFT, GS_ID(KEYMAP_CAMERA_LEFT), true },
    { INPUT_ROLE_CAMERA_RIGHT, GS_ID(KEYMAP_CAMERA_RIGHT), true },
    { INPUT_ROLE_EQUIP_PISTOLS, GS_ID(KEYMAP_EQUIP_PISTOLS), true },
    { INPUT_ROLE_EQUIP_SHOTGUN, GS_ID(KEYMAP_EQUIP_SHOTGUN), true },
    { INPUT_ROLE_EQUIP_MAGNUMS, GS_ID(KEYMAP_EQUIP_MAGNUMS), true },
    { INPUT_ROLE_EQUIP_UZIS, GS_ID(KEYMAP_EQUIP_UZIS), true },
    { INPUT_ROLE_USE_SMALL_MEDI, GS_ID(KEYMAP_USE_SMALL_MEDI), true },
    { INPUT_ROLE_USE_BIG_MEDI, GS_ID(KEYMAP_USE_BIG_MEDI), true },
    { INPUT_ROLE_SAVE, GS_ID(KEYMAP_SAVE), true },
    { INPUT_ROLE_LOAD, GS_ID(KEYMAP_LOAD), true },
    { INPUT_ROLE_FPS, GS_ID(KEYMAP_FPS), true },
    { INPUT_ROLE_BILINEAR, GS_ID(KEYMAP_BILINEAR), true },
    { INPUT_ROLE_FLY_CHEAT, GS_ID(KEYMAP_FLY_CHEAT), true },
    { INPUT_ROLE_ITEM_CHEAT, GS_ID(KEYMAP_ITEM_CHEAT), true },
    { INPUT_ROLE_LEVEL_SKIP_CHEAT, GS_ID(KEYMAP_LEVEL_SKIP_CHEAT), true },
    { INPUT_ROLE_TURBO_CHEAT, GS_ID(KEYMAP_TURBO_CHEAT), true },
    { INPUT_ROLE_ENTER_CONSOLE, GS_ID(KEYMAP_ENTER_CONSOLE), true },
    { INPUT_ROLE_TOGGLE_UI, GS_ID(KEYMAP_TOGGLE_UI), true },
    // end
    { COL_END, nullptr, false },
};

static void M_InitMenu(void);
static void M_InitText(INPUT_BACKEND backend, INPUT_LAYOUT layout);
static void M_UpdateText(INPUT_BACKEND backend, INPUT_LAYOUT layout);
static void M_ShutdownText(void);
static void M_FlashConflicts(INPUT_BACKEND backend, INPUT_LAYOUT layout);
static INPUT_LAYOUT M_ChangeLayout(INPUT_BACKEND backend);
static void M_CheckResetKeys(INPUT_BACKEND backend, INPUT_LAYOUT layout);
static void M_CheckUnbindKey(INPUT_BACKEND backend, INPUT_LAYOUT layout);
static void M_ProgressBar(const TEXTSTRING *txt, BAR_INFO *bar, int32_t timer);

static void M_InitMenu(void)
{
    int32_t visible_lines = 0;
    if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 240) {
        visible_lines = 6;
    } else if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 384) {
        visible_lines = 8;
    } else if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 600) {
        visible_lines = 12;
    } else {
        visible_lines = 18;
    }
    m_ControlMenu.vis_options = visible_lines;

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.gameplay.enable_cheats
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

static void M_InitText(INPUT_BACKEND backend, INPUT_LAYOUT layout)
{
    M_InitMenu();

    m_Text[TEXT_TITLE_BORDER] = Text_Create(0, TOP_Y - BORDER, " ");
    Text_CentreH(m_Text[TEXT_TITLE_BORDER], true);
    Text_CentreV(m_Text[TEXT_TITLE_BORDER], true);

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.gameplay.enable_cheats
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
            Text_Create(x_roles, y, GameString_Get(col->game_string));
        Text_CentreV(m_ControlMenu.role_texts[i], true);

        m_ControlMenu.key_texts[i] = Text_Create(
            x_names, y, Input_GetKeyName(backend, layout, col->role));
        Text_CentreV(m_ControlMenu.key_texts[i], true);
        Text_AlignRight(m_ControlMenu.key_texts[i], true);

        y += ROW_HEIGHT;
        col++;
    }

    m_Text[TEXT_TITLE] =
        Text_Create(0, TOP_Y - BORDER / 2, Input_GetLayoutName(layout));
    Text_CentreH(m_Text[TEXT_TITLE], true);
    Text_CentreV(m_Text[TEXT_TITLE], true);
    Text_AddBackground(m_Text[TEXT_TITLE], 0, 0, 0, 0, TS_REQUESTED);
    Text_AddOutline(m_Text[TEXT_TITLE], TS_REQUESTED);

    int32_t tw = Text_GetWidth(m_Text[TEXT_TITLE]);

    m_Text[TEXT_LEFT_ARROW] = Text_Create(
        m_Text[TEXT_TITLE]->pos.x - tw / 2 - 20, m_Text[TEXT_TITLE]->pos.y,
        "\\{button left}");
    Text_CentreH(m_Text[TEXT_LEFT_ARROW], true);
    Text_CentreV(m_Text[TEXT_LEFT_ARROW], true);

    m_Text[TEXT_RIGHT_ARROW] = Text_Create(
        m_Text[TEXT_TITLE]->pos.x + tw / 2 + 15, m_Text[TEXT_TITLE]->pos.y,
        "\\{button right}");
    Text_CentreH(m_Text[TEXT_RIGHT_ARROW], true);
    Text_CentreV(m_Text[TEXT_RIGHT_ARROW], true);

    m_Text[TEXT_UP_ARROW] =
        Text_Create(0, m_ControlMenu.key_texts[0]->pos.y - 12, "\\{arrow up}");
    Text_SetScale(
        m_Text[TEXT_UP_ARROW], TEXT_BASE_SCALE * 2 / 3,
        TEXT_BASE_SCALE * 2 / 3);
    Text_CentreH(m_Text[TEXT_UP_ARROW], true);
    Text_CentreV(m_Text[TEXT_UP_ARROW], true);

    m_Text[TEXT_DOWN_ARROW] = Text_Create(
        0, m_ControlMenu.key_texts[m_ControlMenu.vis_options - 1]->pos.y + 12,
        "\\{arrow down}");
    Text_SetScale(
        m_Text[TEXT_DOWN_ARROW], TEXT_BASE_SCALE * 2 / 3,
        TEXT_BASE_SCALE * 2 / 3);
    Text_CentreH(m_Text[TEXT_DOWN_ARROW], true);
    Text_CentreV(m_Text[TEXT_DOWN_ARROW], true);

    int16_t lines_height = m_ControlMenu.vis_options * ROW_HEIGHT;
    int16_t box_height = lines_height + ROW_HEIGHT + BOX_PADDING * 2 + BORDER;

    Text_AddBackground(
        m_Text[TEXT_TITLE_BORDER], box_width, box_height, 0, 0, TS_BACKGROUND);
    Text_AddOutline(m_Text[TEXT_TITLE_BORDER], TS_BACKGROUND);

    sprintf(
        m_ResetGS, GS(CONTROL_RESET_DEFAULTS),
        Input_GetKeyName(backend, layout, INPUT_ROLE_RESET_BINDINGS));
    sprintf(
        m_UnbindGS, GS(CONTROL_UNBIND),
        Input_GetKeyName(backend, layout, INPUT_ROLE_UNBIND_KEY));

    m_Text[TEXT_RESET] =
        Text_Create(x_roles, y + BOX_PADDING + BORDER, m_ResetGS);
    Text_CentreV(m_Text[TEXT_RESET], true);
    Text_SetScale(
        m_Text[TEXT_RESET], TEXT_BASE_SCALE * .8, TEXT_BASE_SCALE * .8);

    m_Text[TEXT_UNBIND] =
        Text_Create(x_names, y + BOX_PADDING + BORDER, m_UnbindGS);
    Text_CentreV(m_Text[TEXT_UNBIND], true);
    Text_AlignRight(m_Text[TEXT_UNBIND], true);
    Text_SetScale(
        m_Text[TEXT_UNBIND], TEXT_BASE_SCALE * .8, TEXT_BASE_SCALE * .8);

    if (layout == INPUT_LAYOUT_DEFAULT) {
        Text_Hide(m_Text[TEXT_RESET], true);
        Text_Hide(m_Text[TEXT_UNBIND], true);
    }

    M_UpdateText(backend, layout);
    M_FlashConflicts(backend, layout);
}

static void M_UpdateText(INPUT_BACKEND backend, INPUT_LAYOUT layout)
{
    if (layout == INPUT_LAYOUT_DEFAULT) {
        Text_Hide(m_Text[TEXT_RESET], true);
        Text_Hide(m_Text[TEXT_UNBIND], true);
    } else {
        Text_Hide(m_Text[TEXT_RESET], false);

        if (m_ControlMenu.cur_role == KC_TITLE
            || !m_ControlMenu.cur_row->can_unbind) {
            Text_Hide(m_Text[TEXT_UNBIND], true);
        } else {
            Text_Hide(m_Text[TEXT_UNBIND], false);
        }
    }

    sprintf(
        m_ResetGS, GS(CONTROL_RESET_DEFAULTS),
        Input_GetKeyName(backend, layout, INPUT_ROLE_RESET_BINDINGS));
    Text_ChangeText(m_Text[TEXT_RESET], m_ResetGS);
    sprintf(
        m_UnbindGS, GS(CONTROL_UNBIND),
        Input_GetKeyName(backend, layout, INPUT_ROLE_UNBIND_KEY));
    Text_ChangeText(m_Text[TEXT_UNBIND], m_UnbindGS);

    if (m_ControlMenu.cur_role == KC_TITLE) {
        Text_ChangeText(m_Text[TEXT_TITLE], Input_GetLayoutName(layout));

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
            m_ControlMenu.role_texts[i], GameString_Get(col->game_string));
        Text_ChangeText(
            m_ControlMenu.key_texts[i],
            Input_GetKeyName(backend, layout, col->role));
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
            TS_REQUESTED);
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
            m_ControlMenu.key_texts[m_ControlMenu.row_num], TS_REQUESTED);
        break;

    case KM_CHANGE:
        break;

    case KM_CHANGEKEYUP:
        Text_ChangeText(
            m_ControlMenu.key_texts[m_ControlMenu.row_num],
            Input_GetKeyName(backend, layout, m_ControlMenu.cur_role));
        Text_RemoveBackground(
            m_ControlMenu.key_texts[m_ControlMenu.prev_row_num]);
        Text_RemoveOutline(m_ControlMenu.key_texts[m_ControlMenu.prev_row_num]);
        Text_AddBackground(
            m_ControlMenu.role_texts[m_ControlMenu.row_num], 0, 0, 0, 0,
            TS_REQUESTED);
        Text_AddOutline(
            m_ControlMenu.role_texts[m_ControlMenu.row_num], TS_REQUESTED);
        break;
    }
}

static void M_ShutdownText(void)
{
    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_Remove(m_Text[i]);
        m_Text[TEXT_TITLE] = nullptr;
    }
    for (int i = 0; i < m_ControlMenu.vis_options; i++) {
        Text_Remove(m_ControlMenu.role_texts[i]);
        m_ControlMenu.role_texts[i] = nullptr;
        Text_Remove(m_ControlMenu.key_texts[i]);
        m_ControlMenu.key_texts[i] = nullptr;
    }
    m_ControlMenu.num_options = 0;
    m_ControlMenu.vis_options = 0;
    m_ControlMenu.top_row = nullptr;
    m_ControlMenu.bot_row = nullptr;
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

static void M_FlashConflicts(INPUT_BACKEND backend, INPUT_LAYOUT layout)
{
    const TEXT_COLUMN_PLACEMENT *cols = g_Config.gameplay.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    const TEXT_COLUMN_PLACEMENT *col = m_ControlMenu.top_row;
    for (int i = 0; i < m_ControlMenu.vis_options; i++) {
        Text_Flash(
            m_ControlMenu.key_texts[i],
            layout != INPUT_LAYOUT_DEFAULT
                && Input_IsKeyConflicted(backend, layout, col->role),
            20);
        col++;
    }
}

static INPUT_LAYOUT M_ChangeLayout(INPUT_BACKEND backend)
{
    INPUT_LAYOUT layout = INPUT_LAYOUT_DEFAULT;
    if (backend == INPUT_BACKEND_KEYBOARD) {
        g_Config.input.keyboard_layout += g_InputDB.menu_left ? -1 : 0;
        g_Config.input.keyboard_layout += g_InputDB.menu_right ? 1 : 0;
        g_Config.input.keyboard_layout += INPUT_LAYOUT_NUMBER_OF;
        g_Config.input.keyboard_layout %= INPUT_LAYOUT_NUMBER_OF;
        layout = g_Config.input.keyboard_layout;
    }

    if (backend == INPUT_BACKEND_CONTROLLER) {
        g_Config.input.controller_layout += g_InputDB.menu_left ? -1 : 0;
        g_Config.input.controller_layout += g_InputDB.menu_right ? 1 : 0;
        g_Config.input.controller_layout += INPUT_LAYOUT_NUMBER_OF;
        g_Config.input.controller_layout %= INPUT_LAYOUT_NUMBER_OF;
        layout = g_Config.input.controller_layout;
    }

    M_UpdateText(backend, layout);
    M_FlashConflicts(backend, layout);
    Config_Write();
    return layout;
}

static void M_ProgressBar(
    const TEXTSTRING *const txt, BAR_INFO *const bar, const int32_t timer)
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

    bar->custom_width = width;
    bar->custom_height = height;
    bar->custom_x = x;
    bar->custom_y = y;
    bar->blink = false;
    bar->location = BL_CUSTOM;
    bar->max_value = 100;
    bar->type = BT_PROGRESS;
    bar->value = percent;
    if (g_Config.ui.menu_style == UI_STYLE_PC) {
        bar->color = BC_GOLD;
    } else {
        bar->color = BC_PURPLE;
    }
}

static void M_CheckResetKeys(INPUT_BACKEND backend, INPUT_LAYOUT layout)
{
    const double time = Clock_GetSimTime() * LOGIC_FPS;

    if (Input_IsPressed(backend, layout, INPUT_ROLE_RESET_BINDINGS)
        && m_ResetKeyMode != KM_CHANGEKEYUP) {
        if (m_ResetKeyDelay == 0) {
            m_ResetKeyDelay = time;
        } else if (time - m_ResetKeyDelay >= HOLD_DELAY_FRAMES) {
            m_ResetKeyMode = KM_CHANGE;
            if (m_ResetTimer == 0) {
                m_ResetTimer = time;
            } else if (time - m_ResetTimer >= LOGIC_FPS * BUTTON_HOLD_TIME) {
                Sound_Effect(SFX_MENU_GAMEBOY, nullptr, SPM_NORMAL);
                Input_ResetLayout(backend, layout);
                M_UpdateText(backend, layout);
                M_FlashConflicts(backend, layout);
                Config_Write();
                m_ResetKeyMode = KM_CHANGEKEYUP;
                m_ResetTimer = 0;
            }
        }
    } else if (m_ResetKeyMode == KM_CHANGEKEYUP) {
        if (!Input_IsPressed(backend, layout, INPUT_ROLE_RESET_BINDINGS)) {
            m_ResetKeyMode = KM_INACTIVE;
        }
    } else {
        m_ResetTimer = 0;
        m_ResetKeyMode = KM_INACTIVE;
        m_ResetKeyDelay = 0;
    }

    int32_t progress = m_ResetTimer > 0 ? time - m_ResetTimer : 0;
    CLAMP(progress, 0, LOGIC_FPS * BUTTON_HOLD_TIME);
    M_ProgressBar(
        m_Text[TEXT_RESET], &m_ProgressBars[M_PROGERSS_BAR_RESET_ALL],
        progress);
}

static void M_CheckUnbindKey(INPUT_BACKEND backend, INPUT_LAYOUT layout)
{
    const int32_t time = Clock_GetSimTime() * LOGIC_FPS;

    if (Input_IsPressed(backend, layout, INPUT_ROLE_UNBIND_KEY)
        && m_UnbindKeyMode != KM_CHANGEKEYUP) {
        if (m_UnbindKeyDelay == 0) {
            m_UnbindKeyDelay = time;
        } else if (time - m_UnbindKeyDelay >= HOLD_DELAY_FRAMES) {
            m_UnbindKeyMode = KM_CHANGE;
            if (m_UnbindTimer == 0) {
                m_UnbindTimer = time;
            } else if (time - m_UnbindTimer >= LOGIC_FPS * BUTTON_HOLD_TIME) {
                Sound_Effect(SFX_MENU_GAMEBOY, nullptr, SPM_NORMAL);
                Input_UnassignRole(backend, layout, m_ControlMenu.cur_role);
                M_UpdateText(backend, layout);
                M_FlashConflicts(backend, layout);
                Config_Write();
                m_UnbindKeyMode = KM_CHANGEKEYUP;
                m_UnbindTimer = 0;
            }
        }
    } else if (m_UnbindKeyMode == KM_CHANGEKEYUP) {
        if (!Input_IsPressed(backend, layout, INPUT_ROLE_UNBIND_KEY)) {
            m_UnbindKeyMode = KM_INACTIVE;
        }
    } else {
        m_UnbindTimer = 0;
        m_UnbindKeyMode = KM_INACTIVE;
        m_UnbindKeyDelay = 0;
    }

    int32_t progress = m_UnbindTimer > 0 ? time - m_UnbindTimer : 0;
    CLAMP(progress, 0, LOGIC_FPS * BUTTON_HOLD_TIME);
    M_ProgressBar(
        m_Text[TEXT_UNBIND], &m_ProgressBars[M_PROGERSS_BAR_UNBIND], progress);
}

CONTROL_MODE Option_Controls_Control(
    INVENTORY_ITEM *inv_item, const bool is_busy, INPUT_BACKEND backend)
{
    if (is_busy) {
        return CM_PICK;
    }

    INPUT_LAYOUT layout = INPUT_LAYOUT_DEFAULT;
    if (backend == INPUT_BACKEND_KEYBOARD) {
        layout = g_Config.input.keyboard_layout;
    } else {
        layout = g_Config.input.controller_layout;
    }

    if (!m_Text[TEXT_TITLE]) {
        m_KeyMode = KM_BROWSE;
        M_InitText(backend, layout);
    }

    const TEXT_COLUMN_PLACEMENT *cols = g_Config.gameplay.enable_cheats
        ? CtrlTextPlacementCheats
        : CtrlTextPlacementNormal;

    switch (m_KeyMode) {
    case KM_BROWSE:
        if (layout > INPUT_LAYOUT_DEFAULT) {
            if (m_UnbindKeyMode == KM_INACTIVE) {
                M_CheckResetKeys(backend, layout);
            }

            if (m_ResetKeyMode == KM_INACTIVE
                && m_ControlMenu.cur_row->can_unbind) {
                M_CheckUnbindKey(backend, layout);
            }
        }

        if (m_ResetKeyMode != KM_INACTIVE || m_UnbindKeyMode != KM_INACTIVE) {
            break;
        }

        if (g_InputDB.menu_back
            || (g_InputDB.menu_confirm && m_ControlMenu.cur_role == KC_TITLE)) {
            M_ShutdownText();
            m_KeyMode = KM_INACTIVE;
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            return CM_PICK;
        }

        if ((g_InputDB.menu_left || g_InputDB.menu_right)
            && m_ControlMenu.cur_role == KC_TITLE) {
            layout = M_ChangeLayout(backend);
        }

        if (g_InputDB.menu_confirm) {
            if (layout > INPUT_LAYOUT_DEFAULT) {
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
            M_UpdateText(backend, layout);
            M_FlashConflicts(backend, layout);
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
            M_UpdateText(backend, layout);
            M_FlashConflicts(backend, layout);
        }
        break;

    case KM_BROWSEKEYUP:
        if (!g_Input.any) {
            M_UpdateText(backend, layout);
            m_KeyMode = KM_CHANGE;
        }
        break;

    case KM_CHANGE:
        if (Input_ReadAndAssignRole(backend, layout, m_ControlMenu.cur_role)) {
            M_UpdateText(backend, layout);
            m_KeyMode = KM_CHANGEKEYUP;
            M_FlashConflicts(backend, layout);
            Config_Write();
        }
        break;

    case KM_CHANGEKEYUP:
        if (!g_Input.any) {
            M_UpdateText(backend, layout);
            m_KeyMode = KM_BROWSE;
        }
        break;
    }

    g_Input = (INPUT_STATE) {};
    g_InputDB = (INPUT_STATE) {};

    m_ControlMenu.prev_row_num = m_ControlMenu.row_num;

    return backend == INPUT_BACKEND_KEYBOARD ? CM_KEYBOARD : CM_CONTROLLER;
}

void Option_Controls_Draw(INVENTORY_ITEM *inv_item, INPUT_BACKEND backend)
{
    if (m_ProgressBars[M_PROGERSS_BAR_RESET_ALL].value > 0) {
        Overlay_BarDraw(&m_ProgressBars[M_PROGERSS_BAR_RESET_ALL], RSR_TEXT);
    }
    if (m_ProgressBars[M_PROGERSS_BAR_UNBIND].value > 0) {
        Overlay_BarDraw(&m_ProgressBars[M_PROGERSS_BAR_UNBIND], RSR_TEXT);
    }
}

void Option_Control_Shutdown(void)
{
    M_ShutdownText();
}
