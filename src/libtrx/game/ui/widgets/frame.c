#include "game/ui/widgets/frame.h"

#include "game/text.h"
#include "memory.h"

typedef struct {
    UI_WIDGET_VTABLE vtable;
    UI_WIDGET *root;
    int32_t margin;
    int32_t padding;
    bool is_frame_visible;
    TEXTSTRING *frame;
} UI_FRAME;

static int32_t M_GetWidth(const UI_FRAME *self);
static int32_t M_GetHeight(const UI_FRAME *self);
static void M_SetPosition(UI_FRAME *self, int32_t x, int32_t y);
static void M_Draw(UI_FRAME *self);
static void M_Free(UI_FRAME *self);

static int32_t M_GetWidth(const UI_FRAME *const self)
{
    if (self->vtable.is_hidden) {
        return 0;
    }
    return self->root->get_width(self->root) + self->margin * 2
        + self->padding * 2;
}

static int32_t M_GetHeight(const UI_FRAME *const self)
{
    if (self->vtable.is_hidden) {
        return 0;
    }
    return self->root->get_height(self->root) + self->margin * 2
        + self->padding * 2;
}

static void M_SetPosition(
    UI_FRAME *const self, const int32_t x, const int32_t y)
{
    const int32_t w = M_GetWidth(self);
    const int32_t h = M_GetHeight(self);
    self->root->set_position(
        self->root, x + self->margin + self->padding,
        y + self->margin + self->padding);
    Text_SetPos(
        self->frame, x + self->margin, y + self->margin + TEXT_HEIGHT_FIXED);
    self->frame->pos.z = 24;
    Text_AddBackground(
        self->frame, w - self->margin * 2, h - self->margin * 2,
        (w - self->margin * 2) / 2, 0, TS_REQUESTED);
    Text_AddOutline(self->frame, TS_REQUESTED);
}

static void M_Draw(UI_FRAME *const self)
{
    if (self->vtable.is_hidden) {
        return;
    }
    self->root->draw(self->root);
    if (self->frame != nullptr && self->is_frame_visible) {
        Text_DrawText(self->frame);
    }
}

static void M_Free(UI_FRAME *const self)
{
    Text_Remove(self->frame);
    Memory_Free(self);
}

UI_WIDGET *UI_Frame_Create(
    UI_WIDGET *const root, const int32_t margin, const int32_t padding)
{
    UI_FRAME *const self = Memory_Alloc(sizeof(UI_FRAME));
    self->vtable = (UI_WIDGET_VTABLE) {
        .control = nullptr,
        .draw = (UI_WIDGET_DRAW)M_Draw,
        .get_width = (UI_WIDGET_GET_WIDTH)M_GetWidth,
        .get_height = (UI_WIDGET_GET_HEIGHT)M_GetHeight,
        .set_position = (UI_WIDGET_SET_POSITION)M_SetPosition,
        .free = (UI_WIDGET_FREE)M_Free,
    };
    self->root = root;
    self->margin = margin;
    self->padding = padding;
    self->is_frame_visible = true;
    self->frame = Text_Create(0, 0, "");
    self->frame->pos.z = -400;
    self->frame->flags.manual_draw = 1;
    return (UI_WIDGET *)self;
}

void UI_Frame_SetFrameVisible(
    UI_WIDGET *const widget, const bool is_frame_visible)
{
    UI_FRAME *const self = (UI_FRAME *)widget;
    self->is_frame_visible = is_frame_visible;
}
