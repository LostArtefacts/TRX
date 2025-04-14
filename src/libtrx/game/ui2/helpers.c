#include "game/ui2/helpers.h"

#include "utils.h"

void UI2_MeasureWrapper(UI2_NODE *const node)
{
    node->measure_w = 0.0f;
    node->measure_h = 0.0f;
    const UI2_NODE *child = node->first_child;
    while (child != nullptr) {
        node->measure_w = MAX(node->measure_w, child->measure_w);
        node->measure_h = MAX(node->measure_h, child->measure_h);
        child = child->next_sibling;
    }
}

void UI2_LayoutBasic(
    UI2_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    node->x = x;
    node->y = y;
    node->w = w;
    node->h = h;
}

void UI2_LayoutWrapper(
    UI2_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI2_LayoutBasic(node, x, y, w, h);
    UI2_NODE *child = node->first_child;
    while (child != nullptr) {
        if (child->ops->layout != nullptr) {
            child->ops->layout(child, x, y, w, h);
        }
        child = child->next_sibling;
    }
}

void UI2_DrawWrapper(const UI2_NODE *const node)
{
    if (node->measure_w <= 0.0f || node->measure_h <= 0.0f) {
        return;
    }
    UI2_NODE *child = node->first_child;
    while (child != nullptr) {
        if (child->ops->draw != nullptr) {
            child->ops->draw(child);
        }
        child = child->next_sibling;
    }
}
