#include "game/ui/widgets/graphics_dialog.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/game_string.h>
#include <libtrx/game/input.h>
#include <libtrx/game/text.h>
#include <libtrx/game/ui/common.h>
#include <libtrx/game/ui/widgets/frame.h>
#include <libtrx/game/ui/widgets/label.h>
#include <libtrx/game/ui/widgets/stack.h>
#include <libtrx/game/ui/widgets/window.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>

typedef struct {
    CONFIG_OPTION_TYPE option_type;
    GAME_STRING_ID label_id;
    void *target;
    int32_t min_value;
    int32_t max_value;
    int32_t delta_slow;
    int32_t delta_fast;
    int32_t misc;
} M_OPTION;

typedef struct {
    void *user_data;
    UI_WIDGET *frame;
    UI_WIDGET *stack;
    UI_WIDGET *title_label;
    UI_WIDGET *value_label;
    UI_WIDGET *arrow_left_label;
    UI_WIDGET *arrow_right_label;
} M_ROW;

typedef struct {
    UI_WIDGET_VTABLE vtable;
    UI_WIDGET *window;
    UI_WIDGET *outer_stack;

    int32_t visible_rows;

    bool is_confirmed;
    int32_t selected_row_offset;
    int32_t visible_row_offset;
    int32_t row_count;
    M_ROW *rows;

    int32_t selection_margin;
    int32_t selection_padding;

    int32_t listener;
} M_WIDGET;

static M_OPTION m_Options[] = {
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(DETAIL_FOG_START),
        .target = &g_Config.visuals.fog_start,
        .min_value = 1,
        .max_value = 100,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(DETAIL_FOG_END),
        .target = &g_Config.visuals.fog_end,
        .min_value = 1,
        .max_value = 100,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(DETAIL_WATER_COLOR_R),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = 0,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(DETAIL_WATER_COLOR_G),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = 1,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(DETAIL_WATER_COLOR_B),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = 2,
    },

    {
        .target = nullptr,
    },
};

static uint8_t *M_GetColorComponent(const M_OPTION *option);
static void M_ClearRows(M_WIDGET *self);
static char *M_FormatRowValue(int32_t row_idx);
static bool M_CanChangeValue(int32_t row_idx, int32_t dir);
static bool M_RequestChangeValue(M_WIDGET *self, int32_t row_idx, int32_t dir);
static void M_DeselectRow(M_WIDGET *self, int32_t row_idx);
static void M_SelectRow(M_WIDGET *self, int32_t row_idx);
static void M_SyncRows(M_WIDGET *self);
static M_ROW *M_AddRow(
    M_WIDGET *self, const char *left_text, const char *right_text,
    void *user_data);
static void M_DoLayout(M_WIDGET *self);
static void M_HandleCanvasResize(const EVENT *event, void *data);

static int32_t M_GetWidth(const M_WIDGET *self);
static int32_t M_GetHeight(const M_WIDGET *self);
static void M_SetPosition(M_WIDGET *self, int32_t x, int32_t y);
static void M_Control(M_WIDGET *self);
static void M_Draw(M_WIDGET *self);
static void M_Free(M_WIDGET *self);

static uint8_t *M_GetColorComponent(const M_OPTION *const option)
{
    RGB_888 *const color = option->target;
    switch (option->misc) {
    case 0:
        return &color->r;
    case 1:
        return &color->g;
    case 2:
        return &color->b;
    }
    ASSERT_FAIL();
    return nullptr;
}

static void M_ClearRows(M_WIDGET *const self)
{
    for (int32_t i = 0; i < self->row_count; i++) {
        self->rows[i].title_label->free(self->rows[i].title_label);
        self->rows[i].value_label->free(self->rows[i].value_label);
        self->rows[i].arrow_left_label->free(self->rows[i].arrow_left_label);
        self->rows[i].arrow_right_label->free(self->rows[i].arrow_right_label);
        self->rows[i].frame->free(self->rows[i].frame);
        self->rows[i].stack->free(self->rows[i].stack);
    }
    UI_Stack_ClearChildren(self->outer_stack);
    self->visible_row_offset = 0;
    self->row_count = 0;
    self->selected_row_offset = -1;
    self->is_confirmed = false;
}

static char *M_FormatRowValue(const int32_t row_idx)
{
    const M_OPTION *const option = &m_Options[row_idx];
    switch (option->option_type) {
    case COT_INT32:
        return String_Format(
            GS(DETAIL_INTEGER_FMT), *(int32_t *)option->target);
    case COT_RGB888: {
        const uint8_t *const component = M_GetColorComponent(option);
        return String_Format("%d", *component);
    }
    default:
        break;
    }
    return nullptr;
}

static bool M_CanChangeValue(const int32_t row_idx, const int32_t dir)
{
    const M_OPTION *const option = &m_Options[row_idx];
    switch (option->option_type) {
    case COT_INT32:
        if (dir < 0) {
            return *(int32_t *)option->target > option->min_value;
        } else if (dir > 0) {
            return *(int32_t *)option->target < option->max_value;
        }
        break;
    case COT_RGB888: {
        const uint8_t *const component = M_GetColorComponent(option);
        if (dir < 0) {
            return *component > option->min_value;
        } else if (dir > 0) {
            return *component < option->max_value;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

static bool M_RequestChangeValue(
    M_WIDGET *const self, const int32_t row_idx, const int32_t dir)
{
    if (!M_CanChangeValue(row_idx, dir)) {
        return false;
    }

    const M_OPTION *const option = &m_Options[row_idx];
    int32_t delta = g_Input.slow ? option->delta_slow : option->delta_fast;
    if (delta == 0) {
        delta = 1;
    }
    delta *= dir;

    switch (option->option_type) {
    case COT_INT32:
        *(int32_t *)option->target += delta;
        break;
    case COT_RGB888: {
        uint8_t *const component = M_GetColorComponent(option);
        *component += delta;
        break;
    }
    default:
        return false;
    }
    Config_Write();

    M_ROW *const row = &self->rows[row_idx];
    UI_Label_SetVisible(row->arrow_left_label, M_CanChangeValue(row_idx, -1));
    UI_Label_SetVisible(row->arrow_right_label, M_CanChangeValue(row_idx, +1));
    M_SyncRows(self);
    return true;
}

static void M_DeselectRow(M_WIDGET *const self, const int32_t row_idx)
{
    M_ROW *const row = &self->rows[row_idx];
    UI_Label_SetVisible(row->arrow_left_label, false);
    UI_Label_SetVisible(row->arrow_right_label, false);
    UI_Frame_SetFrameVisible(row->frame, false);
}

static void M_SelectRow(M_WIDGET *const self, const int32_t row_idx)
{
    M_ROW *const row = &self->rows[row_idx];
    UI_Label_SetVisible(row->arrow_left_label, M_CanChangeValue(row_idx, -1));
    UI_Label_SetVisible(row->arrow_right_label, M_CanChangeValue(row_idx, +1));
    UI_Frame_SetFrameVisible(row->frame, true);
}

static void M_SyncRows(M_WIDGET *const self)
{
    for (int32_t row_idx = 0; row_idx < self->row_count; row_idx++) {
        M_ROW *const row = &self->rows[row_idx];
        char *value_text = M_FormatRowValue(row_idx);
        UI_Label_ChangeText(row->value_label, value_text);
        Memory_Free(value_text);
    }
    M_DoLayout(self);
}

static M_ROW *M_AddRow(
    M_WIDGET *const self, const char *const left_text,
    const char *const right_text, void *const user_data)
{
    self->row_count++;
    self->rows = Memory_Realloc(self->rows, sizeof(M_ROW) * self->row_count);
    M_ROW *const row = &self->rows[self->row_count - 1];

    row->stack =
        UI_Stack_Create(UI_STACK_LAYOUT_HORIZONTAL, 220, UI_STACK_AUTO_SIZE);
    UI_Stack_SetHAlign(row->stack, UI_STACK_H_ALIGN_DISTRIBUTE);

    row->frame = UI_Frame_Create(row->stack, 0, 0);
    UI_Frame_SetFrameVisible(row->frame, false);

    row->title_label = UI_Label_Create(left_text, 150, UI_LABEL_AUTO_SIZE);
    UI_Stack_AddChild(row->stack, row->title_label);

    row->arrow_left_label = UI_Label_Create(
        "\\{button left} ", UI_LABEL_AUTO_SIZE, UI_LABEL_AUTO_SIZE);
    UI_Label_SetVisible(row->arrow_left_label, false);
    UI_Stack_AddChild(row->stack, row->arrow_left_label);

    row->value_label =
        UI_Label_Create(right_text, UI_LABEL_AUTO_SIZE, UI_LABEL_AUTO_SIZE);
    UI_Stack_AddChild(row->stack, row->value_label);

    row->arrow_right_label = UI_Label_Create(
        " \\{button right}", UI_LABEL_AUTO_SIZE, UI_LABEL_AUTO_SIZE);
    UI_Label_SetVisible(row->arrow_right_label, false);
    UI_Stack_AddChild(row->stack, row->arrow_right_label);

    row->user_data = user_data;

    for (int32_t y = 0; y < self->row_count; y++) {
        self->rows[y].stack->is_hidden = y < self->visible_row_offset
            || y >= self->visible_row_offset + self->visible_rows;
    }
    if (self->selected_row_offset == -1) {
        self->selected_row_offset = 0;
        M_SelectRow(self, self->selected_row_offset);
    }

    UI_Stack_AddChild(self->outer_stack, row->frame);
    return row;
}

static void M_DoLayout(M_WIDGET *const self)
{
    M_SetPosition(
        self, (UI_GetCanvasWidth() - M_GetWidth(self)) / 2,
        (UI_GetCanvasHeight() - M_GetHeight(self)) * 3 / 5);
}

static void M_HandleCanvasResize(const EVENT *event, void *data)
{
    M_WIDGET *const self = (M_WIDGET *)data;
    M_DoLayout(self);
}

static int32_t M_GetWidth(const M_WIDGET *const self)
{
    return self->window->get_width(self->window);
}

static int32_t M_GetHeight(const M_WIDGET *const self)
{
    return self->window->get_height(self->window);
}

static void M_SetPosition(
    M_WIDGET *const self, const int32_t x, const int32_t y)
{
    self->window->set_position(self->window, x, y);
}

static void M_Control(M_WIDGET *const self)
{
    if (self->window->control != nullptr) {
        self->window->control(self->window);
    }

    bool update = false;
    if (g_InputDB.menu_down) {
        if (self->visible_row_offset + self->visible_rows < self->row_count) {
            self->rows[self->visible_row_offset].stack->is_hidden = true;
            self->rows[self->visible_row_offset + self->visible_rows]
                .stack->is_hidden = false;
            self->visible_row_offset++;
            update = true;
        }
    } else if (g_InputDB.menu_up) {
        if (self->visible_row_offset > 0) {
            self->rows[self->visible_row_offset + self->visible_rows - 1]
                .stack->is_hidden = true;
            self->rows[self->visible_row_offset - 1].stack->is_hidden = false;
            self->visible_row_offset--;
            update = true;
        }
    }

    if (g_InputDB.menu_down
        && self->selected_row_offset + 1 < self->row_count) {
        if (self->selected_row_offset != -1) {
            M_DeselectRow(self, self->selected_row_offset);
        }
        self->selected_row_offset++;
        M_SelectRow(self, self->selected_row_offset);
        update = true;
    } else if (g_InputDB.menu_up && self->selected_row_offset > 0) {
        if (self->selected_row_offset != -1) {
            M_DeselectRow(self, self->selected_row_offset);
        }
        self->selected_row_offset--;
        M_SelectRow(self, self->selected_row_offset);
        update = true;
    }
    if (g_InputDB.menu_left && self->selected_row_offset >= 0) {
        M_RequestChangeValue(self, self->selected_row_offset, -1);
    } else if (g_InputDB.menu_right && self->selected_row_offset >= 0) {
        M_RequestChangeValue(self, self->selected_row_offset, +1);
    }

    if (g_InputDB.menu_confirm) {
        self->is_confirmed = true;
    }

    if (update) {
        M_DoLayout(self);
    }
}

static void M_Draw(M_WIDGET *const self)
{
    if (self->window->draw != nullptr) {
        self->window->draw(self->window);
    }
}

static void M_Free(M_WIDGET *const self)
{
    M_ClearRows(self);
    self->outer_stack->free(self->outer_stack);
    self->window->free(self->window);
    UI_Events_Unsubscribe(self->listener);
    Memory_Free(self);
}

UI_WIDGET *UI_GraphicsDialog_Create(void)
{
    M_WIDGET *const self = Memory_Alloc(sizeof(M_WIDGET));

    self->vtable = (UI_WIDGET_VTABLE) {
        .get_width = (UI_WIDGET_GET_WIDTH)M_GetWidth,
        .get_height = (UI_WIDGET_GET_HEIGHT)M_GetHeight,
        .set_position = (UI_WIDGET_SET_POSITION)M_SetPosition,
        .control = (UI_WIDGET_CONTROL)M_Control,
        .draw = (UI_WIDGET_DRAW)M_Draw,
        .free = (UI_WIDGET_FREE)M_Free,
    };

    self->outer_stack = UI_Stack_Create(
        UI_STACK_LAYOUT_VERTICAL, UI_STACK_AUTO_SIZE, UI_STACK_AUTO_SIZE);
    UI_Stack_SetHAlign(self->outer_stack, UI_STACK_H_ALIGN_CENTER);

    self->window = UI_Window_Create(self->outer_stack, 8, 8, 8, 8);
    UI_Window_SetTitle(self->window, GS(DETAIL_TITLE));

    self->selected_row_offset = -1;
    self->listener = UI_Events_Subscribe(
        "canvas_resize", nullptr, M_HandleCanvasResize, self);

    self->visible_rows = 0;
    for (int32_t i = 0; m_Options[i].target != nullptr; i++) {
        self->visible_rows++;
    }

    for (int32_t i = 0; m_Options[i].target != nullptr; i++) {
        char *value_text = M_FormatRowValue(i);
        M_AddRow(
            self, GameString_Get(m_Options[i].label_id), value_text, nullptr);
        Memory_Free(value_text);
    }

    M_DoLayout(self);
    return (UI_WIDGET *)self;
}
