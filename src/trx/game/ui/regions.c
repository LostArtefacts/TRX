#include <trx/game/ui/regions.h>

#include <trx/core/log.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/ui/elements/pad.h>
#include <trx/game/ui/elements/stack.h>
#include <trx/game/ui/helpers.h>

// Maximum reservations per scene.
#define M_MAX_SLOTS 64

#define M_PAD_X 20.0f
#define M_PAD_Y 14.0f
#define M_SPACING 3.0f

// Region anchor and horizontal alignment.
typedef struct {
    float x;
    float y;
    UI_STACK_H_ALIGN h_align;
} M_REGION_DESC;

typedef struct {
    UI_NODE *slots[UI_REGION_NUMBER_OF];
} M_DATA;

typedef struct {
    float w;
    float h;
} M_SLOT_DATA;

static const M_REGION_DESC m_Regions[UI_REGION_NUMBER_OF] = {
    [UI_REGION_TOP_LEFT] = { 0.0f, 0.0f, UI_STACK_H_ALIGN_LEFT },
    [UI_REGION_TOP_CENTER] = { 0.5f, 0.0f, UI_STACK_H_ALIGN_CENTER },
    [UI_REGION_TOP_RIGHT] = { 1.0f, 0.0f, UI_STACK_H_ALIGN_RIGHT },
    [UI_REGION_LEFT] = { 0.0f, 0.5f, UI_STACK_H_ALIGN_LEFT },
    [UI_REGION_CENTER] = { 0.5f, 0.5f, UI_STACK_H_ALIGN_CENTER },
    [UI_REGION_RIGHT] = { 1.0f, 0.5f, UI_STACK_H_ALIGN_RIGHT },
    [UI_REGION_BOTTOM_LEFT] = { 0.0f, 1.0f, UI_STACK_H_ALIGN_LEFT },
    [UI_REGION_BOTTOM_CENTER] = { 0.5f, 1.0f, UI_STACK_H_ALIGN_CENTER },
    [UI_REGION_BOTTOM_RIGHT] = { 1.0f, 1.0f, UI_STACK_H_ALIGN_RIGHT },
};

// Container and stacks for the current scene's regions.
static UI_NODE *m_Container;
static UI_NODE *m_Stacks[UI_REGION_NUMBER_OF];

// Reserved slots for the current scene.
static UI_NODE *m_Slots[M_MAX_SLOTS];
static int32_t m_SlotCount;

// Region currently being built. Regions do not nest.
static int32_t m_OpenRegion = -1;

// Caller node restored by UI_EndRegion.
static UI_NODE *m_Caller;

// Scene counter for the nodes above.
static uint32_t m_Generation;

// Middle box from the last laid-out scene.
static struct {
    float x;
    float y;
    float w;
    float h;
} m_CenterBox;

static float M_MeasuredHeight(const M_DATA *const data, const UI_REGION region)
{
    const UI_NODE *const slot = data->slots[region];
    return slot != nullptr ? slot->measure_h : 0.0f;
}

static float M_MeasuredWidth(const M_DATA *const data, const UI_REGION region)
{
    const UI_NODE *const slot = data->slots[region];
    return slot != nullptr ? slot->measure_w : 0.0f;
}

// The tallest region in a row sets that row's band height.
static float M_BandHeight(
    const M_DATA *const data, const UI_REGION left, const UI_REGION center,
    const UI_REGION right)
{
    return MAX(
        MAX(M_MeasuredHeight(data, left), M_MeasuredHeight(data, center)),
        M_MeasuredHeight(data, right));
}

static void M_Measure(UI_NODE *const node)
{
    node->measure_w = UI_GetCanvasWidth();
    node->measure_h = UI_GetCanvasHeight();
}

static float M_Offset(const float slack, const float ratio)
{
    return slack > 0.0f ? slack * ratio : 0.0f;
}

static void M_Layout(
    UI_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI_LayoutBasic(node, x, y, w, h);
    const M_DATA *const data = node->data;

    const float top = M_BandHeight(
        data, UI_REGION_TOP_LEFT, UI_REGION_TOP_CENTER, UI_REGION_TOP_RIGHT);
    const float bottom = M_BandHeight(
        data, UI_REGION_BOTTOM_LEFT, UI_REGION_BOTTOM_CENTER,
        UI_REGION_BOTTOM_RIGHT);
    const float left = M_MeasuredWidth(data, UI_REGION_LEFT);
    const float right = M_MeasuredWidth(data, UI_REGION_RIGHT);

    const float middle_y = y + top;
    const float middle_h = MAX(0.0f, h - top - bottom);
    m_CenterBox.x = x + left;
    m_CenterBox.y = middle_y;
    m_CenterBox.w = MAX(0.0f, w - left - right);
    m_CenterBox.h = middle_h;

    for (int32_t i = 0; i < UI_REGION_NUMBER_OF; i++) {
        UI_NODE *const slot = data->slots[i];
        if (slot == nullptr || slot->ops.layout == nullptr) {
            continue;
        }

        float box_x = x;
        float box_y = middle_y;
        float box_w = w;
        float box_h = middle_h;
        if (m_Regions[i].y == 0.0f) {
            box_y = y;
            box_h = top;
        } else if (m_Regions[i].y == 1.0f) {
            box_y = y + h - bottom;
            box_h = bottom;
        } else if (i == UI_REGION_CENTER) {
            box_x = m_CenterBox.x;
            box_w = m_CenterBox.w;
        }

        const float cw = slot->measure_w;
        const float ch = slot->measure_h;
        slot->ops.layout(
            slot, box_x + M_Offset(box_w - cw, m_Regions[i].x),
            box_y + M_Offset(box_h - ch, m_Regions[i].y), cw, ch);
    }
}

static void M_ForgetStaleNodes(void)
{
    const uint32_t generation = UI_GetSceneGeneration();
    if (generation == m_Generation) {
        return;
    }
    m_Generation = generation;
    m_Container = nullptr;
    for (int32_t i = 0; i < UI_REGION_NUMBER_OF; i++) {
        m_Stacks[i] = nullptr;
    }
    m_Caller = nullptr;
    m_OpenRegion = -1;
    for (int32_t i = 0; i < m_SlotCount; i++) {
        m_Slots[i] = nullptr;
    }
    m_SlotCount = 0;
}

// A reservation measures to its requested size and draws nothing.
static void M_MeasureSlot(UI_NODE *const node)
{
    const M_SLOT_DATA *const data = node->data;
    node->measure_w = data->w;
    node->measure_h = data->h;
}

static void M_EnsureContainer(void)
{
    if (m_Container != nullptr) {
        return;
    }
    UI_SetCurrent(UI_GetBuildRoot());
    m_Container = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_Measure,
            .layout = M_Layout,
            .draw = UI_DrawWrapper,
        },
        sizeof(M_DATA));
    m_Container->name = UI_NODE_NAME_REGIONS;
    UI_AddChild(m_Container);
}

void UI_BeginRegion(const UI_REGION region)
{
    ASSERT(region >= 0 && region < UI_REGION_NUMBER_OF);
    M_ForgetStaleNodes();
    ASSERT(m_Caller == nullptr);
    m_Caller = (UI_NODE *)UI_GetCurrent();
    m_OpenRegion = region;

    if (m_Stacks[region] != nullptr) {
        UI_SetCurrent(m_Stacks[region]);
        return;
    }

    // Attach regions to the shared container.
    M_EnsureContainer();
    UI_SetCurrent(m_Container);
    UI_BeginPad(M_PAD_X, M_PAD_Y);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = m_Regions[region].h_align },
        .spacing = { .v = M_SPACING },
    });
    m_Stacks[region] = (UI_NODE *)UI_GetCurrent();

    M_DATA *const data = m_Container->data;
    data->slots[region] = m_Container->last_child;
}

void UI_EndRegion(void)
{
    UI_SetCurrent(m_Caller);
    m_Caller = nullptr;
    m_OpenRegion = -1;
}

int32_t UI_Region_Reserve(const UI_REGION region, const float w, const float h)
{
    if (m_SlotCount >= M_MAX_SLOTS) {
        LOG_WARNING("no room left for a reservation");
        return -1;
    }

    // Join the open region when reserving from its draw event.
    const bool was_open = m_OpenRegion == (int32_t)region;
    if (!was_open) {
        if (m_OpenRegion != -1) {
            LOG_WARNING(
                "a reservation names a region that is not the open one");
            return -1;
        }
        UI_BeginRegion(region);
    }

    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_MeasureSlot,
            .layout = UI_LayoutBasic,
        },
        sizeof(M_SLOT_DATA));
    M_SLOT_DATA *const data = node->data;
    data->w = w;
    data->h = h;
    UI_AddChild(node);

    if (!was_open) {
        UI_EndRegion();
    }

    m_Slots[m_SlotCount] = node;
    return m_SlotCount++;
}

bool UI_Region_GetSlotBox(
    const int32_t slot, float *const x, float *const y, float *const w,
    float *const h)
{
    M_ForgetStaleNodes();
    if (slot < 0 || slot >= m_SlotCount || m_Slots[slot] == nullptr) {
        return false;
    }
    const UI_NODE *const node = m_Slots[slot];
    *x = node->x;
    *y = node->y;
    *w = node->w;
    *h = node->h;
    return true;
}

void UI_Region_GetCenterBox(
    float *const x, float *const y, float *const w, float *const h)
{
    // Before the first layout, the middle box is the full canvas.
    if (m_CenterBox.w <= 0.0f || m_CenterBox.h <= 0.0f) {
        *x = 0.0f;
        *y = 0.0f;
        *w = UI_GetCanvasWidth();
        *h = UI_GetCanvasHeight();
        return;
    }
    *x = m_CenterBox.x;
    *y = m_CenterBox.y;
    *w = m_CenterBox.w;
    *h = m_CenterBox.h;
}
