#include <trx/game/ui/regions.h>

#include <trx/core/log.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/scaler.h>

// Maximum reservations per scene.
#define M_MAX_SLOTS 64

#define M_PAD_X 20.0f
#define M_PAD_Y 14.0f
#define M_SPACING 3.0f

// Where a region sits on the screen and where its slots sit across it, each
// as a fraction from the left or the top.
typedef struct {
    float x;
    float y;
} M_REGION_DESC;

typedef struct {
    UI_REGION region;
    float x;
    float y;
    float w;
    float h;
} M_SLOT;

typedef struct {
    float x;
    float y;
    float w;
    float h;
} M_BOX;

static const M_REGION_DESC m_Regions[UI_REGION_NUMBER_OF] = {
    [UI_REGION_TOP_LEFT] = { 0.0f, 0.0f },
    [UI_REGION_TOP_CENTER] = { 0.5f, 0.0f },
    [UI_REGION_TOP_RIGHT] = { 1.0f, 0.0f },
    [UI_REGION_LEFT] = { 0.0f, 0.5f },
    [UI_REGION_CENTER] = { 0.5f, 0.5f },
    [UI_REGION_RIGHT] = { 1.0f, 0.5f },
    [UI_REGION_BOTTOM_LEFT] = { 0.0f, 1.0f },
    [UI_REGION_BOTTOM_CENTER] = { 0.5f, 1.0f },
    [UI_REGION_BOTTOM_RIGHT] = { 1.0f, 1.0f },
};

static M_SLOT m_Slots[M_MAX_SLOTS];
static int32_t m_SlotCount;

// Each region as the last layout placed it, with its padding.
static M_BOX m_Boxes[UI_REGION_NUMBER_OF];

// The middle box from the last layout, and what the regions were laid out in.
static M_BOX m_CenterBox;
static M_BOX m_Area;

// The size of a region's slots stacked with their padding, in canvas units.
static void M_MeasureRegion(
    const UI_REGION region, float *const out_w, float *const out_h)
{
    const float scale = UI_Scaler_GetTextScale();
    float w = 0.0f;
    float h = 0.0f;
    int32_t count = 0;
    for (int32_t i = 0; i < m_SlotCount; i++) {
        if (m_Slots[i].region != region) {
            continue;
        }
        w = MAX(w, m_Slots[i].w);
        h += m_Slots[i].h;
        count++;
    }
    if (count > 1) {
        h += (count - 1) * M_SPACING * scale;
    }
    *out_w = w + 2.0f * M_PAD_X * scale;
    *out_h = h + 2.0f * M_PAD_Y * scale;
}

// Stacks the slots of a region down its box, each set across it as the region
// sits on the screen.
static void M_LayoutRegion(const UI_REGION region, const M_BOX box)
{
    const float scale = UI_Scaler_GetTextScale();
    const float inner_x = box.x + M_PAD_X * scale;
    const float inner_w = box.w - 2.0f * M_PAD_X * scale;
    float y = box.y + M_PAD_Y * scale;
    for (int32_t i = 0; i < m_SlotCount; i++) {
        M_SLOT *const slot = &m_Slots[i];
        if (slot->region != region) {
            continue;
        }
        slot->x = inner_x + (inner_w - slot->w) * m_Regions[region].x;
        slot->y = y;
        y += slot->h + M_SPACING * scale;
    }
}

static float M_Offset(const float slack, const float ratio)
{
    return slack > 0.0f ? slack * ratio : 0.0f;
}

// The height of a row of regions: that of its tallest region.
static float M_BandHeight(
    const float *const heights, const UI_REGION left, const UI_REGION center,
    const UI_REGION right)
{
    return MAX(MAX(heights[left], heights[center]), heights[right]);
}

// The height a row of regions takes from a box of the given width, centered:
// that of the tallest region the box would reach.
static float M_BandHeightFor(
    const float width, const UI_REGION left, const UI_REGION center,
    const UI_REGION right)
{
    const float middle = m_CenterBox.x + m_CenterBox.w * 0.5f;
    const float x0 = middle - width * 0.5f;
    const float x1 = middle + width * 0.5f;
    const UI_REGION regions[] = { left, center, right };
    float result = 0.0f;
    for (int32_t i = 0; i < 3; i++) {
        const M_BOX *const box = &m_Boxes[regions[i]];
        if (box->h > 0.0f && box->x < x1 && box->x + box->w > x0) {
            result = MAX(result, box->h);
        }
    }
    return result;
}

void UI_Region_Clear(void)
{
    m_SlotCount = 0;
}

int32_t UI_Region_Reserve(const UI_REGION region, const float w, const float h)
{
    ASSERT(region >= 0 && region < UI_REGION_NUMBER_OF);
    if (m_SlotCount >= M_MAX_SLOTS) {
        LOG_WARNING("no room left for a reservation");
        return -1;
    }
    m_Slots[m_SlotCount] = (M_SLOT) { .region = region, .w = w, .h = h };
    return m_SlotCount++;
}

bool UI_Region_GetSlotBox(
    const int32_t slot, float *const x, float *const y, float *const w,
    float *const h)
{
    if (slot < 0 || slot >= m_SlotCount) {
        return false;
    }
    *x = m_Slots[slot].x;
    *y = m_Slots[slot].y;
    *w = m_Slots[slot].w;
    *h = m_Slots[slot].h;
    return true;
}

void UI_Region_Layout(void)
{
    float widths[UI_REGION_NUMBER_OF];
    float heights[UI_REGION_NUMBER_OF];
    for (int32_t i = 0; i < UI_REGION_NUMBER_OF; i++) {
        M_MeasureRegion(i, &widths[i], &heights[i]);
    }

    const float y = UI_GetWorkingCanvasTop();
    const float w = UI_GetCanvasWidth();
    const float h = UI_GetWorkingCanvasBottom() - y;
    const float top = M_BandHeight(
        heights, UI_REGION_TOP_LEFT, UI_REGION_TOP_CENTER, UI_REGION_TOP_RIGHT);
    const float bottom = M_BandHeight(
        heights, UI_REGION_BOTTOM_LEFT, UI_REGION_BOTTOM_CENTER,
        UI_REGION_BOTTOM_RIGHT);
    const float left = widths[UI_REGION_LEFT];
    const float right = widths[UI_REGION_RIGHT];

    m_Area = (M_BOX) { .x = 0.0f, .y = y, .w = w, .h = h };
    m_CenterBox = (M_BOX) {
        .x = left,
        .y = y + top,
        .w = MAX(0.0f, w - left - right),
        .h = MAX(0.0f, h - top - bottom),
    };

    for (int32_t i = 0; i < UI_REGION_NUMBER_OF; i++) {
        M_BOX band = {
            .x = 0.0f, .y = m_CenterBox.y, .w = w, .h = m_CenterBox.h
        };
        if (m_Regions[i].y == 0.0f) {
            band.y = y;
            band.h = top;
        } else if (m_Regions[i].y == 1.0f) {
            band.y = y + h - bottom;
            band.h = bottom;
        } else if (i == UI_REGION_CENTER) {
            band.x = m_CenterBox.x;
            band.w = m_CenterBox.w;
        }
        m_Boxes[i] = (M_BOX) {
            .x = band.x + M_Offset(band.w - widths[i], m_Regions[i].x),
            .y = band.y + M_Offset(band.h - heights[i], m_Regions[i].y),
            .w = widths[i],
            .h = heights[i],
        };
        M_LayoutRegion(i, m_Boxes[i]);
    }
}

void UI_Region_GetCenterBox(
    float *const x, float *const y, float *const w, float *const h)
{
    // Before the first layout, the middle box is the canvas between the bars.
    if (m_CenterBox.w <= 0.0f || m_CenterBox.h <= 0.0f) {
        *x = 0.0f;
        *y = UI_GetWorkingCanvasTop();
        *w = UI_GetCanvasWidth();
        *h = UI_GetWorkingCanvasBottom() - *y;
        return;
    }
    *x = m_CenterBox.x;
    *y = m_CenterBox.y;
    *w = m_CenterBox.w;
    *h = m_CenterBox.h;
}

void UI_Region_GetCenterBoxFor(
    const float width, float *const x, float *const y, float *const w,
    float *const h)
{
    UI_Region_GetCenterBox(x, y, w, h);
    if (width < 0.0f || m_CenterBox.w <= 0.0f || m_CenterBox.h <= 0.0f) {
        return;
    }
    const float top = M_BandHeightFor(
        width, UI_REGION_TOP_LEFT, UI_REGION_TOP_CENTER, UI_REGION_TOP_RIGHT);
    const float bottom = M_BandHeightFor(
        width, UI_REGION_BOTTOM_LEFT, UI_REGION_BOTTOM_CENTER,
        UI_REGION_BOTTOM_RIGHT);
    *y = m_Area.y + top;
    *h = MAX(0.0f, m_Area.h - top - bottom);
}
