#include "game/ui/widgets/controls_column.h"

#include "game/ui/widgets/controls_input_selector.h"

#include <libtrx/game/ui/widgets/stack.h>
#include <libtrx/memory.h>

typedef struct {
    UI_WIDGET_VTABLE vtable;
    UI_WIDGET *container;
    int32_t selector_count;
    UI_WIDGET **selectors;
} UI_CONTROLS_COLUMN;

static int32_t M_GetWidth(const UI_CONTROLS_COLUMN *self);
static int32_t M_GetHeight(const UI_CONTROLS_COLUMN *self);
static void M_SetPosition(UI_CONTROLS_COLUMN *self, int32_t x, int32_t y);
static void M_Control(UI_CONTROLS_COLUMN *self);
static void M_Draw(UI_CONTROLS_COLUMN *self);
static void M_Free(UI_CONTROLS_COLUMN *self);

static int32_t M_GetWidth(const UI_CONTROLS_COLUMN *const self)
{
    return self->container->get_width(self->container);
}

static int32_t M_GetHeight(const UI_CONTROLS_COLUMN *const self)
{
    return self->container->get_height(self->container);
}

static void M_SetPosition(
    UI_CONTROLS_COLUMN *const self, const int32_t x, const int32_t y)
{
    return self->container->set_position(self->container, x, y);
}

static void M_Control(UI_CONTROLS_COLUMN *const self)
{
    if (self->container->control != NULL) {
        self->container->control(self->container);
    }
}

static void M_Draw(UI_CONTROLS_COLUMN *const self)
{
    if (self->container->draw != NULL) {
        self->container->draw(self->container);
    }
}

static void M_Free(UI_CONTROLS_COLUMN *const self)
{
    for (int32_t i = 0; i < self->selector_count; i++) {
        self->selectors[i]->free(self->selectors[i]);
    }
    self->container->free(self->container);
    Memory_FreePointer(&self->selectors);
    Memory_Free(self);
}

UI_WIDGET *UI_ControlsColumn_Create(
    const int32_t column, UI_CONTROLS_CONTROLLER *const controller)
{
    UI_CONTROLS_COLUMN *const self = Memory_Alloc(sizeof(UI_CONTROLS_COLUMN));
    self->vtable = (UI_WIDGET_VTABLE) {
        .get_width = (UI_WIDGET_GET_WIDTH)M_GetWidth,
        .get_height = (UI_WIDGET_GET_HEIGHT)M_GetHeight,
        .set_position = (UI_WIDGET_SET_POSITION)M_SetPosition,
        .control = (UI_WIDGET_CONTROL)M_Control,
        .draw = (UI_WIDGET_DRAW)M_Draw,
        .free = (UI_WIDGET_FREE)M_Free,
    };

    self->selector_count = UI_ControlsController_GetInputRoleCount(column);
    self->container = UI_Stack_Create(
        UI_STACK_LAYOUT_VERTICAL, UI_STACK_AUTO_SIZE, UI_STACK_AUTO_SIZE);
    self->selectors = Memory_Alloc(sizeof(UI_WIDGET *) * self->selector_count);

    for (int32_t i = 0; i < self->selector_count; i++) {
        self->selectors[i] = UI_ControlsInputSelector_Create(
            UI_ControlsController_GetInputRole(column, i), controller);
        UI_Stack_AddChild(self->container, self->selectors[i]);
    }

    return (UI_WIDGET *)self;
}
