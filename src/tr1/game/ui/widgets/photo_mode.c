#include "game/ui/widgets/photo_mode.h"

#include "game/input.h"
#include "game/text.h"

#include <libtrx/game/game_string.h>
#include <libtrx/game/ui/common.h>
#include <libtrx/game/ui/widgets/label.h>
#include <libtrx/game/ui/widgets/spacer.h>
#include <libtrx/game/ui/widgets/stack.h>
#include <libtrx/game/ui/widgets/window.h>
#include <libtrx/memory.h>

#include <stdio.h>

#define TITLE_MARGIN 5
#define WINDOW_MARGIN 10
#define DIALOG_PADDING 5

typedef struct {
    UI_WIDGET_VTABLE vtable;
    UI_WIDGET *window;
    UI_WIDGET *title;
    UI_WIDGET *outer_stack;
    UI_WIDGET *inner_stack;
    UI_WIDGET *left_stack;
    UI_WIDGET *right_stack;
    UI_WIDGET *spacer;
    bool shown;
    int32_t label_count;
    UI_WIDGET **left_labels;
    UI_WIDGET **right_labels;
} UI_PHOTO_MODE;

static int32_t M_GetWidth(const UI_PHOTO_MODE *self);
static int32_t M_GetHeight(const UI_PHOTO_MODE *self);
static void M_SetPosition(UI_PHOTO_MODE *self, int32_t x, int32_t y);
static void M_Control(UI_PHOTO_MODE *self);
static void M_Draw(UI_PHOTO_MODE *self);
static void M_Free(UI_PHOTO_MODE *self);

static int32_t M_GetWidth(const UI_PHOTO_MODE *const self)
{
    return UI_GetCanvasWidth() - 2 * WINDOW_MARGIN;
}

static int32_t M_GetHeight(const UI_PHOTO_MODE *const self)
{
    return UI_GetCanvasHeight() - 2 * WINDOW_MARGIN;
}

static void M_SetPosition(UI_PHOTO_MODE *const self, int32_t x, int32_t y)
{
    return self->window->set_position(self->window, x, y);
}

static void M_Control(UI_PHOTO_MODE *const self)
{
    if (self->window->control != NULL) {
        self->window->control(self->window);
    }

    self->shown = g_Config.ui.enable_photo_mode_ui;
}

static void M_Draw(UI_PHOTO_MODE *const self)
{
    if (self->shown && self->window->draw != NULL) {
        self->window->draw(self->window);
    }
}

static void M_Free(UI_PHOTO_MODE *const self)
{
    for (int32_t i = 0; i < self->label_count; i++) {
        self->left_labels[i]->free(self->left_labels[i]);
        self->right_labels[i]->free(self->right_labels[i]);
    }
    self->spacer->free(self->spacer);
    self->title->free(self->title);
    self->outer_stack->free(self->outer_stack);
    self->inner_stack->free(self->inner_stack);
    self->left_stack->free(self->left_stack);
    self->right_stack->free(self->right_stack);
    self->window->free(self->window);
    Memory_Free(self->left_labels);
    Memory_Free(self->right_labels);
    Memory_Free(self);
}

UI_WIDGET *UI_PhotoMode_Create(void)
{
    UI_PHOTO_MODE *const self = Memory_Alloc(sizeof(UI_PHOTO_MODE));
    self->vtable = (UI_WIDGET_VTABLE) {
        .control = (UI_WIDGET_CONTROL)M_Control,
        .draw = (UI_WIDGET_DRAW)M_Draw,
        .get_width = (UI_WIDGET_GET_WIDTH)M_GetWidth,
        .get_height = (UI_WIDGET_GET_HEIGHT)M_GetHeight,
        .set_position = (UI_WIDGET_SET_POSITION)M_SetPosition,
        .free = (UI_WIDGET_FREE)M_Free,
    };

    self->outer_stack = UI_Stack_Create(
        UI_STACK_LAYOUT_VERTICAL, UI_STACK_AUTO_SIZE, UI_STACK_AUTO_SIZE);
    self->inner_stack = UI_Stack_Create(
        UI_STACK_LAYOUT_HORIZONTAL, UI_STACK_AUTO_SIZE, UI_STACK_AUTO_SIZE);
    self->left_stack = UI_Stack_Create(
        UI_STACK_LAYOUT_VERTICAL, UI_STACK_AUTO_SIZE, UI_STACK_AUTO_SIZE);
    self->right_stack = UI_Stack_Create(
        UI_STACK_LAYOUT_VERTICAL, UI_STACK_AUTO_SIZE, UI_STACK_AUTO_SIZE);

    const char *const title = GS(PHOTO_MODE_TITLE);
    self->title = UI_Label_Create(title, UI_LABEL_AUTO_SIZE, TEXT_HEIGHT_FIXED);
    UI_Stack_AddChild(self->outer_stack, self->title);

    self->spacer = UI_Spacer_Create(TITLE_MARGIN, TITLE_MARGIN);
    UI_Stack_AddChild(self->outer_stack, self->spacer);
    UI_Stack_AddChild(self->outer_stack, self->inner_stack);
    UI_Stack_AddChild(self->inner_stack, self->left_stack);
    UI_Stack_AddChild(self->inner_stack, self->right_stack);

    char move_role[50];
    sprintf(
        move_role, "%s%s%s%s%s%s: ",
        Input_GetKeyName(
            CM_KEYBOARD, g_Config.input.layout, INPUT_ROLE_CAMERA_UP),
        Input_GetKeyName(
            CM_KEYBOARD, g_Config.input.layout, INPUT_ROLE_CAMERA_DOWN),
        Input_GetKeyName(
            CM_KEYBOARD, g_Config.input.layout, INPUT_ROLE_CAMERA_FORWARD),
        Input_GetKeyName(
            CM_KEYBOARD, g_Config.input.layout, INPUT_ROLE_CAMERA_BACK),
        Input_GetKeyName(
            CM_KEYBOARD, g_Config.input.layout, INPUT_ROLE_CAMERA_LEFT),
        Input_GetKeyName(
            CM_KEYBOARD, g_Config.input.layout, INPUT_ROLE_CAMERA_RIGHT));

    const char *const rot_role = "\200 \203 \202 \201 : ";

    char z_roll_role[100];
    sprintf(z_roll_role, "%s: ", GS(PHOTO_MODE_ROLL_ROLE));

    char roll_role[100];
    sprintf(roll_role, "%s: ", GS(KEYMAP_ROLL));

    char fov_role[100];
    sprintf(fov_role, "%s: ", GS(PHOTO_MODE_FOV_ROLE));

    char reset_role[100];
    sprintf(reset_role, "%s: ", GS(KEYMAP_LOOK));

    char help_role[20];
    sprintf(
        help_role, "%s: ",
        Input_GetKeyName(
            CM_KEYBOARD, g_Config.input.layout, INPUT_ROLE_TOGGLE_UI));

    char action_role[100];
    sprintf(action_role, "%s: ", GS(KEYMAP_ACTION));

    char exit_role[100];
    sprintf(
        exit_role, "%s/%s: ",
        Input_GetKeyName(
            CM_KEYBOARD, g_Config.input.layout, INPUT_ROLE_TOGGLE_PHOTO_MODE),
        Input_GetKeyName(
            CM_KEYBOARD, g_Config.input.layout, INPUT_ROLE_OPTION));

    const char *const inputs[] = {
        move_role,  rot_role,  z_roll_role, roll_role, fov_role,
        reset_role, help_role, action_role, exit_role, NULL,
    };

    const char *const roles[] = {
        GS(PHOTO_MODE_MOVE_PROMPT),
        GS(PHOTO_MODE_ROTATE_PROMPT),
        GS(PHOTO_MODE_ROLL_PROMPT),
        GS(PHOTO_MODE_ROTATE90_PROMPT),
        GS(PHOTO_MODE_FOV_PROMPT),
        GS(PHOTO_MODE_RESET_PROMPT),
        GS(MISC_TOGGLE_HELP),
        GS(PHOTO_MODE_SNAP_PROMPT),
        GS(MISC_EXIT),
        NULL,
    };

    self->shown = true;
    self->label_count = 0;
    while (inputs[self->label_count] != NULL) {
        self->label_count++;
    }

    self->left_labels = Memory_Alloc(sizeof(UI_WIDGET *) * self->label_count);
    self->right_labels = Memory_Alloc(sizeof(UI_WIDGET *) * self->label_count);
    for (int32_t i = 0; i < self->label_count; i++) {
        self->left_labels[i] =
            UI_Label_Create(inputs[i], UI_LABEL_AUTO_SIZE, TEXT_HEIGHT_FIXED);
        UI_Stack_AddChild(self->left_stack, self->left_labels[i]);
        self->right_labels[i] =
            UI_Label_Create(roles[i], UI_LABEL_AUTO_SIZE, TEXT_HEIGHT_FIXED);
        UI_Stack_AddChild(self->right_stack, self->right_labels[i]);
    }

    self->window = UI_Window_Create(
        self->outer_stack, DIALOG_PADDING, DIALOG_PADDING, DIALOG_PADDING * 2,
        DIALOG_PADDING);

    M_SetPosition(self, WINDOW_MARGIN, WINDOW_MARGIN);
    return (UI_WIDGET *)self;
}
