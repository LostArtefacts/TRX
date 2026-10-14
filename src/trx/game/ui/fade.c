#include <trx/game/ui/fade.h>

#include <trx/core/memory.h>
#include <trx/game/output/overlay.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/draw.h>
#include <trx/game/viewport.h>

#include <string.h>

#define M_MAX_FADES 8
#define M_SLOT_BITS 8
#define M_SLOT_MASK ((1 << M_SLOT_BITS) - 1)
#define M_GENERATION_MASK 0x7FFFFF

typedef enum {
    M_KIND_FILL,
    M_KIND_SNAPSHOT,
    M_KIND_BACKDROP,
} M_KIND;

typedef struct {
    bool in_use;
    int32_t generation;
    int32_t order;
    M_KIND kind;
    UI_TWEEN opacity;
    RGBA_8888 color;
    int32_t snapshot_slot;
    BACKGROUND_TYPE backdrop;
    char *backdrop_path;
    bool is_moving;
    bool close_at_rest;
    bool holds_screen;
} M_FADE;

typedef struct {
    BACKGROUND_TYPE style;
    float progress;
    bool has_path;
    char path[];
} M_BACKDROP_ARGS;

static M_FADE m_Fades[M_MAX_FADES] = {};
static int32_t m_NextOrder = 0;

static int32_t M_MakeId(const int32_t slot)
{
    return (m_Fades[slot].generation << M_SLOT_BITS) | (slot + 1);
}

static M_FADE *M_Get(const int32_t id)
{
    const int32_t slot = (id & M_SLOT_MASK) - 1;
    if (slot < 0 || slot >= M_MAX_FADES) {
        return nullptr;
    }
    M_FADE *const fade = &m_Fades[slot];
    if (!fade->in_use || fade->generation != (id >> M_SLOT_BITS)) {
        return nullptr;
    }
    return fade;
}

static M_FADE *M_Push(const M_KIND kind, const float opacity)
{
    for (int32_t i = 0; i < M_MAX_FADES; i++) {
        M_FADE *const fade = &m_Fades[i];
        if (fade->in_use) {
            continue;
        }
        *fade = (M_FADE) {
            .in_use = true,
            .generation = (fade->generation + 1) & M_GENERATION_MASK,
            .order = m_NextOrder++,
            .kind = kind,
            .snapshot_slot = -1,
        };
        UI_Tween_Set(&fade->opacity, opacity);
        return fade;
    }
    return nullptr;
}

static void M_Close(M_FADE *const fade)
{
    if (fade->snapshot_slot >= 0) {
        Output_Overlay_FreeSnapshotSlot(fade->snapshot_slot);
    }
    Memory_FreePointer(&fade->backdrop_path);
    fade->in_use = false;
}

static void M_DrawBackdropOp(const void *const args)
{
    const M_BACKDROP_ARGS *const backdrop = args;
    Output_Overlay_DrawBackground(
        backdrop->style, backdrop->progress,
        backdrop->has_path ? backdrop->path : nullptr);
}

// A picture comes in from black, so the overlay takes how dark it is; every
// other style takes how far it has come in.
static void M_DrawBackdrop(const M_FADE *const fade)
{
    const float progress = UI_Tween_Get(&fade->opacity);
    const char *const path =
        fade->backdrop_path != nullptr ? fade->backdrop_path : "";
    const size_t path_size = strlen(path) + 1;
    const size_t size = sizeof(M_BACKDROP_ARGS) + path_size;
    M_BACKDROP_ARGS *const args = Memory_Alloc(size);
    args->style = fade->backdrop;
    args->progress = fade->backdrop == BK_IMAGE ? 1.0f - progress : progress;
    args->has_path = fade->backdrop_path != nullptr;
    memcpy(args->path, path, path_size);
    UI_ScheduleDrawOverlay(M_DrawBackdropOp, args, size);
    Memory_Free(args);
}

static void M_DrawFade(const M_FADE *const fade)
{
    if (fade->kind == M_KIND_BACKDROP) {
        M_DrawBackdrop(fade);
        return;
    }
    const float opacity = UI_Tween_Get(&fade->opacity);
    if (opacity <= 0.0f) {
        return;
    }
    if (fade->snapshot_slot >= 0) {
        UI_ScheduleDrawSnapshotSlot(fade->snapshot_slot, opacity);
        return;
    }
    RGBA_8888 color = fade->color;
    color.a = (uint8_t)(color.a * opacity + 0.5f);
    UI_ScheduleDrawScreenFlatQuad(
        0, 0, 0, Viewport_GetWidth(VIEWPORT_UI),
        Viewport_GetHeight(VIEWPORT_UI), color);
}

// Names the interface layer that a fade layer is drawn in.
static UI_LAYER M_GetLayer(const M_FADE *const fade)
{
    if (fade->kind == M_KIND_BACKDROP) {
        return UI_LAYER_BACKDROP;
    }
    return UI_LAYER_FADE;
}

// Draws the open fade layers of one interface layer, in push order.
static void M_DrawLayer(const UI_LAYER layer)
{
    UI_BeginLayer(layer);
    int32_t last_order = -1;
    while (true) {
        const M_FADE *next = nullptr;
        for (int32_t i = 0; i < M_MAX_FADES; i++) {
            const M_FADE *const fade = &m_Fades[i];
            if (fade->in_use && M_GetLayer(fade) == layer
                && fade->order > last_order
                && (next == nullptr || fade->order < next->order)) {
                next = fade;
            }
        }
        if (next == nullptr) {
            break;
        }
        M_DrawFade(next);
        last_order = next->order;
    }
    UI_EndLayer(layer);
}

int32_t UI_Fade_PushFill(const RGBA_8888 color, const float opacity)
{
    M_FADE *const fade = M_Push(M_KIND_FILL, opacity);
    if (fade == nullptr) {
        return UI_FADE_NONE;
    }
    fade->color = color;
    return M_MakeId(fade - m_Fades);
}

int32_t UI_Fade_PushSnapshot(const float opacity)
{
    const int32_t slot = Output_Overlay_CaptureSnapshotSlot();
    if (slot < 0) {
        return UI_FADE_NONE;
    }
    M_FADE *const fade = M_Push(M_KIND_SNAPSHOT, opacity);
    if (fade == nullptr) {
        Output_Overlay_FreeSnapshotSlot(slot);
        return UI_FADE_NONE;
    }
    fade->snapshot_slot = slot;
    return M_MakeId(fade - m_Fades);
}

int32_t UI_Fade_PushBackdrop(
    const BACKGROUND_TYPE style, const char *const path, const float opacity)
{
    M_FADE *const fade = M_Push(M_KIND_BACKDROP, opacity);
    if (fade == nullptr) {
        return UI_FADE_NONE;
    }
    // A picture draws over the whole screen, so only the other styles show
    // the game behind them.
    if (style != BK_IMAGE || path == nullptr) {
        Output_Overlay_CaptureGameSnapshot();
    }
    fade->backdrop = style;
    fade->backdrop_path = path != nullptr ? Memory_DupStr(path) : nullptr;
    return M_MakeId(fade - m_Fades);
}

void UI_Fade_HoldScreen(const int32_t id)
{
    M_FADE *const fade = M_Get(id);
    if (fade != nullptr) {
        fade->holds_screen = true;
    }
}

bool UI_Fade_IsHoldingScreen(void)
{
    for (int32_t i = 0; i < M_MAX_FADES; i++) {
        if (m_Fades[i].in_use && m_Fades[i].holds_screen) {
            return true;
        }
    }
    return false;
}

bool UI_Fade_IsOpen(const int32_t id)
{
    return M_Get(id) != nullptr;
}

float UI_Fade_GetOpacity(const int32_t id)
{
    const M_FADE *const fade = M_Get(id);
    return fade == nullptr ? 0.0f : UI_Tween_Get(&fade->opacity);
}

void UI_Fade_To(
    const int32_t id, const float target, const int32_t ticks,
    const UI_EASE ease, const bool close)
{
    M_FADE *const fade = M_Get(id);
    if (fade == nullptr) {
        return;
    }
    UI_Tween_To(&fade->opacity, target, ticks, 0, ease);
    fade->is_moving = true;
    fade->close_at_rest = close;
}

void UI_Fade_Close(const int32_t id)
{
    M_FADE *const fade = M_Get(id);
    if (fade != nullptr) {
        M_Close(fade);
    }
}

void UI_Fade_Draw(void)
{
    M_DrawLayer(UI_LAYER_BACKDROP);
    M_DrawLayer(UI_LAYER_FADE);
}

void UI_Fade_Tick(void (*const on_done)(int32_t id))
{
    for (int32_t i = 0; i < M_MAX_FADES; i++) {
        M_FADE *const fade = &m_Fades[i];
        if (!fade->in_use || !fade->is_moving
            || UI_Tween_IsMoving(&fade->opacity)) {
            continue;
        }
        fade->is_moving = false;
        const int32_t id = M_MakeId(i);
        if (fade->close_at_rest) {
            M_Close(fade);
        }
        if (on_done != nullptr) {
            on_done(id);
        }
    }
}
