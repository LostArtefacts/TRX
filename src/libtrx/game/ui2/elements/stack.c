#include "game/ui2/elements/stack.h"

#include "config.h"
#include "debug.h"
#include "game/ui2/helpers.h"
#include "utils.h"

#include <math.h>
#include <stdint.h>

typedef struct {
    UI2_STACK_SETTINGS settings;
} M_DATA;

static float M_CalcChildW(const UI2_NODE *node, const UI2_NODE *child);
static float M_CalcChildH(const UI2_NODE *node, const UI2_NODE *child);
static float M_CalcStartX(const UI2_NODE *node, const UI2_NODE *child);
static float M_CalcStartY(const UI2_NODE *node, const UI2_NODE *child);
static void M_Measure(UI2_NODE *node);
static void M_Layout(UI2_NODE *node, float x, float y, float w, float h);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = M_Measure,
    .layout = M_Layout,
    .draw = UI2_DrawWrapper,
};

static float M_CalcChildW(
    const UI2_NODE *const node, const UI2_NODE *const child)
{
    M_DATA *const data = node->data;
    if (data->settings.align.h == UI2_STACK_H_ALIGN_SPAN) {
        return MAX(child->measure_w, node->w);
    }
    return child->measure_w;
}

static float M_CalcChildH(
    const UI2_NODE *const node, const UI2_NODE *const child)
{
    M_DATA *const data = node->data;
    if (data->settings.align.v == UI2_STACK_V_ALIGN_SPAN) {
        return MAX(child->measure_h, node->h);
    }
    return child->measure_h;
}

static float M_CalcStartX(
    const UI2_NODE *const node, const UI2_NODE *const child)
{
    M_DATA *const data = node->data;
    switch (data->settings.align.h) {
    case UI2_STACK_H_ALIGN_SPAN:
    case UI2_STACK_H_ALIGN_LEFT:
        return node->x;
    case UI2_STACK_H_ALIGN_CENTER:
        return node->x + (node->w - child->measure_w) * 0.5f;
    case UI2_STACK_H_ALIGN_RIGHT:
        return node->x + node->w - child->measure_w;
    case UI2_STACK_H_ALIGN_DISTRIBUTE:
        ASSERT_FAIL();
    }
    return 0.0f;
}

static float M_CalcStartY(
    const UI2_NODE *const node, const UI2_NODE *const child)
{
    M_DATA *const data = node->data;
    switch (data->settings.align.v) {
    case UI2_STACK_V_ALIGN_SPAN:
    case UI2_STACK_V_ALIGN_TOP:
        return node->y;
    case UI2_STACK_V_ALIGN_CENTER:
        return node->y + (node->h - child->measure_h) * 0.5f;
    case UI2_STACK_V_ALIGN_BOTTOM:
        return node->y + node->h - child->measure_h;
    case UI2_STACK_V_ALIGN_DISTRIBUTE:
        ASSERT_FAIL();
    }
    return 0.0f;
}

static void M_Measure(UI2_NODE *const node)
{
    node->measure_w = 0.0f;
    node->measure_h = 0.0f;
    UI2_NODE *child = node->first_child;
    M_DATA *const data = node->data;
    while (child != nullptr) {
        if (data->settings.orientation == UI2_STACK_VERTICAL) {
            node->measure_w = MAX(node->measure_w, child->measure_w);
            node->measure_h += child->measure_h;
            if (child->next_sibling != nullptr) {
                node->measure_h += data->settings.spacing.v;
            }
        } else {
            node->measure_h = MAX(node->measure_h, child->measure_h);
            node->measure_w += child->measure_w;
            if (child->next_sibling != nullptr) {
                node->measure_w += data->settings.spacing.h;
            }
        }
        child = child->next_sibling;
    }
}

static void M_Layout(
    UI2_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI2_LayoutBasic(node, x, y, w, h);
    M_DATA *const data = node->data;

    // Count children and compute the total size they occupy on the main axis
    // including the base spacing from the settings.
    int32_t child_count = 0;
    float total_child_main_size = 0.0f;
    UI2_NODE *child = node->first_child;
    while (child != NULL) {
        switch (data->settings.orientation) {
        case UI2_STACK_HORIZONTAL:
            total_child_main_size += child->measure_w;
            break;
        case UI2_STACK_VERTICAL:
            total_child_main_size += child->measure_h;
            break;
        }
        child_count++;
        child = child->next_sibling;
    }

    // If there is at least one gap between children, compute the normal
    // (configured) total spacing on the main axis. If only 1 child or 0
    // children, there's no gap to distribute leftover space into.
    const int32_t gaps = (child_count > 1) ? (child_count - 1) : 0;
    float base_spacing = 0.0f;
    switch (data->settings.orientation) {
    case UI2_STACK_HORIZONTAL:
        base_spacing = data->settings.spacing.h * gaps;
        break;
    case UI2_STACK_VERTICAL:
        base_spacing = data->settings.spacing.v * gaps;
        break;
    }

    // The space that the children + base spacing absolutely need
    const float needed_size = total_child_main_size + base_spacing;

    // The leftover that we can distribute among the (child_count - 1) internal
    // gaps.
    float leftover = 0.0f;
    float extra_per_gap = 0.0f;
    switch (data->settings.orientation) {
    case UI2_STACK_HORIZONTAL:
        leftover = w - needed_size;
        break;
    case UI2_STACK_VERTICAL:
        leftover = h - needed_size;
        break;
    }

    if (gaps > 0 && leftover > 0.0f) {
        extra_per_gap = leftover / (float)gaps;
    }

    // Now we actually lay out the children
    float cx = x;
    float cy = y;
    child = node->first_child;
    while (child != nullptr) {
        const float cw = M_CalcChildW(node, child);
        const float ch = M_CalcChildH(node, child);

        switch (data->settings.orientation) {
        case UI2_STACK_HORIZONTAL:
            // For horizontal: vertical alignment is determined by M_CalcStartY
            cy = M_CalcStartY(node, child);

            // Lay out the child
            child->ops->layout(child, cx, cy, cw, ch);

            // Advance cx for the next child
            cx += cw;
            // Add normal spacing + any extra leftover that we are distributing
            if (child->next_sibling != nullptr) {
                cx += data->settings.spacing.h + extra_per_gap;
            }
            break;

        case UI2_STACK_VERTICAL:
            cx = M_CalcStartX(node, child);

            child->ops->layout(child, cx, cy, cw, ch);

            cy += ch;
            if (child->next_sibling != nullptr) {
                cy += data->settings.spacing.v + extra_per_gap;
            }
            break;
        }

        child = child->next_sibling;
    }
}

UI2_NODE *UI2_CreateStack(const UI2_STACK_SETTINGS settings)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, sizeof(M_DATA));
    if (node == nullptr) {
        return nullptr;
    }
    M_DATA *const data = node->data;
    data->settings = settings;
    return node;
}

void UI2_BeginStack(const UI2_STACK_ORIENTATION orientation)
{
    UI2_BeginStackEx((UI2_STACK_SETTINGS) {
        .orientation = orientation,
        .align = {
            .h = UI2_STACK_H_ALIGN_LEFT,
            .v = UI2_STACK_V_ALIGN_TOP,
        },
        .spacing = {
            .h = 0.0f,
            .v = 0.0f,
        },
    });
}

void UI2_BeginStackEx(const UI2_STACK_SETTINGS settings)
{
    UI2_NODE *const child = UI2_CreateStack(settings);
    UI2_AddChild(child);
    UI2_PushCurrent(child);
}

void UI2_EndStack(void)
{
    UI2_PopCurrent();
}
