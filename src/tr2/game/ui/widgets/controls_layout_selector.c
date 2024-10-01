#include "game/ui/widgets/controls_layout_selector.h"

#include <libtrx/game/ui/widgets/label.h>
#include <libtrx/memory.h>

typedef struct {
    UI_WIDGET_VTABLE vtable;
    UI_WIDGET *label;
    UI_CONTROLS_CONTROLLER *controller;
} UI_CONTROLS_LAYOUT_SELECTOR;

static int32_t M_GetWidth(const UI_CONTROLS_LAYOUT_SELECTOR *self);
static int32_t M_GetHeight(const UI_CONTROLS_LAYOUT_SELECTOR *self);
static void M_SetPosition(
    UI_CONTROLS_LAYOUT_SELECTOR *self, int32_t x, int32_t y);
static void M_Control(UI_CONTROLS_LAYOUT_SELECTOR *self);
static void M_Draw(UI_CONTROLS_LAYOUT_SELECTOR *self);
static void M_Free(UI_CONTROLS_LAYOUT_SELECTOR *self);

static int32_t M_GetWidth(const UI_CONTROLS_LAYOUT_SELECTOR *const self)
{
    return self->label->get_width(self->label);
}

static int32_t M_GetHeight(const UI_CONTROLS_LAYOUT_SELECTOR *const self)
{
    return self->label->get_height(self->label);
}

static void M_SetPosition(
    UI_CONTROLS_LAYOUT_SELECTOR *const self, const int32_t x, const int32_t y)
{
    self->label->set_position(self->label, x, y);
}

static void M_Control(UI_CONTROLS_LAYOUT_SELECTOR *const self)
{
    if (self->controller->state == UI_CONTROLS_STATE_NAVIGATE_LAYOUT) {
        UI_Label_AddFrame(self->label);
        UI_Label_ChangeText(
            self->label, Input_GetLayoutName(self->controller->active_layout));
    } else {
        UI_Label_RemoveFrame(self->label);
    }
    if (self->label->control != NULL) {
        self->label->control(self->label);
    }
}

static void M_Draw(UI_CONTROLS_LAYOUT_SELECTOR *const self)
{
    if (self->label->draw != NULL) {
        self->label->draw(self->label);
    }
}

static void M_Free(UI_CONTROLS_LAYOUT_SELECTOR *const self)
{
    self->label->free(self->label);
    Memory_Free(self);
}

UI_WIDGET *UI_ControlsLayoutSelector_Create(
    UI_CONTROLS_CONTROLLER *const controller)
{
    UI_CONTROLS_LAYOUT_SELECTOR *self =
        Memory_Alloc(sizeof(UI_CONTROLS_LAYOUT_SELECTOR));
    self->vtable = (UI_WIDGET_VTABLE) {
        .get_width = (UI_WIDGET_GET_WIDTH)M_GetWidth,
        .get_height = (UI_WIDGET_GET_HEIGHT)M_GetHeight,
        .set_position = (UI_WIDGET_SET_POSITION)M_SetPosition,
        .control = (UI_WIDGET_CONTROL)M_Control,
        .draw = (UI_WIDGET_DRAW)M_Draw,
        .free = (UI_WIDGET_FREE)M_Free,
    };

    self->controller = controller;
    self->label = UI_Label_Create(
        Input_GetLayoutName(self->controller->active_layout),
        UI_LABEL_AUTO_SIZE, 25);
    UI_Label_AddFrame(self->label);
    return (UI_WIDGET *)self;
}
