#include "game/ui/widgets/spacer.h"

#include "memory.h"

typedef struct {
    UI_WIDGET_VTABLE vtable;
    int32_t width;
    int32_t height;
} UI_SPACER;

static int32_t M_GetWidth(const UI_SPACER *self);
static int32_t M_GetHeight(const UI_SPACER *self);
static void M_SetPosition(UI_SPACER *self, int32_t x, int32_t y);
static void M_Control(UI_SPACER *self);
static void M_Draw(UI_SPACER *self);
static void M_Free(UI_SPACER *self);

static int32_t M_GetWidth(const UI_SPACER *const self)
{
    return self->width;
}

static int32_t M_GetHeight(const UI_SPACER *const self)
{
    return self->height;
}

static void M_SetPosition(
    UI_SPACER *const self, const int32_t x, const int32_t y)
{
}

static void M_Free(UI_SPACER *const self)
{
    Memory_Free(self);
}

UI_WIDGET *UI_Spacer_Create(const int32_t width, const int32_t height)
{
    UI_SPACER *const self = Memory_Alloc(sizeof(UI_SPACER));
    self->vtable = (UI_WIDGET_VTABLE) {
        .control = NULL,
        .draw = NULL,
        .get_width = (UI_WIDGET_GET_WIDTH)M_GetWidth,
        .get_height = (UI_WIDGET_GET_HEIGHT)M_GetHeight,
        .set_position = (UI_WIDGET_SET_POSITION)M_SetPosition,
        .free = (UI_WIDGET_FREE)M_Free,
    };
    self->width = width;
    self->height = height;
    return (UI_WIDGET *)self;
}

void UI_Spacer_SetSize(
    UI_WIDGET *const widget, const int32_t width, const int32_t height)
{
    UI_SPACER *const self = (UI_SPACER *)widget;
    self->width = width;
    self->height = height;
}
