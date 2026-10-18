// Records scheduled UI draw calls as text, so layout tests can compare scenes
// without starting the renderer.

#include <fakes/ui_draw.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/vector.h>
#include <trx/game/output/overlay.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/keys.h>

#include <stdio.h>
#include <string.h>

// Stores one formatted colour per draw argument.
typedef struct {
    char text[9];
} M_COLOR_TEXT;

// The system clipboard, reduced to a string a test reads back by reading the
// clipboard again. The real one talks to the window system.
static char m_Clipboard[1024] = "";

static VECTOR *m_Lines = nullptr;
static VECTOR *m_Layers = nullptr;
static double m_InterpolationRate = 0.0;

static M_COLOR_TEXT M_Color(const RGBA_8888 color)
{
    M_COLOR_TEXT out;
    snprintf(
        out.text, sizeof(out.text), "%02x%02x%02x%02x", color.r, color.g,
        color.b, color.a);
    return out;
}

#define M_COLOR(color) (M_Color(color).text)

static M_COLOR_TEXT M_ColorF(const RGBA_F color)
{
    return M_Color((RGBA_8888) {
        .r = (uint8_t)(color.r * 255.0f + 0.5f),
        .g = (uint8_t)(color.g * 255.0f + 0.5f),
        .b = (uint8_t)(color.b * 255.0f + 0.5f),
        .a = (uint8_t)(color.a * 255.0f + 0.5f),
    });
}

#define M_COLOR_F(color) (M_ColorF(color).text)

void FakeUIDraw_Record(const char *const line)
{
    if (m_Lines == nullptr) {
        m_Lines = Vector_Create(sizeof(char *));
        m_Layers = Vector_Create(sizeof(UI_LAYER));
    }
    char *const copy = Memory_DupStr(line);
    Vector_Add(m_Lines, &copy);
    const UI_LAYER layer = UI_GetLayer();
    Vector_Add(m_Layers, &layer);
}

UI_LAYER FakeUIDraw_GetLayer(const int32_t index)
{
    return *(const UI_LAYER *)Vector_Get(m_Layers, index);
}

const char *FakeUIDraw_GetLayerName(const int32_t index)
{
    static const char *const names[UI_LAYER_NUMBER_OF] = {
        [UI_LAYER_BACKDROP] = "backdrop",   [UI_LAYER_UNDER] = "under",
        [UI_LAYER_INTERFACE] = "interface", [UI_LAYER_FADE] = "fade",
        [UI_LAYER_CONSOLE] = "console",     [UI_LAYER_EXIT] = "exit",
    };
    return names[FakeUIDraw_GetLayer(index)];
}

void UI_InitDraw(void)
{
}

void UI_ShutdownDraw(void)
{
    FakeUIDraw_Forget();
}

void UI_ClearDraw(void)
{
    FakeUIDraw_Forget();
}

void UI_ScheduleDrawScreenSprite(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t scale_h,
    const int32_t scale_v, const int32_t sprite_idx, const RGBA_F colors[4])
{
    FakeUIDraw_Record(String_FormatStatic(
        "sprite idx=%d x=%d y=%d z=%d scale_h=%d scale_v=%d tl=%s tr=%s bl=%s "
        "br=%s",
        sprite_idx, sx, sy, z, scale_h, scale_v, M_COLOR_F(colors[0]),
        M_COLOR_F(colors[1]), M_COLOR_F(colors[2]), M_COLOR_F(colors[3])));
}

void UI_ScheduleDrawScreenSpriteRect(
    const int32_t sprite_idx, const int32_t x0, const int32_t y0,
    const int32_t x1, const int32_t y1, const int32_t z, const RGBA_F color)
{
    FakeUIDraw_Record(String_FormatStatic(
        "sprite_rect idx=%d x0=%d y0=%d x1=%d y1=%d z=%d color=%s", sprite_idx,
        x0, y0, x1, y1, z, M_COLOR_F(color)));
}

void UI_ScheduleDrawScreenLine(
    const float x0, const float y0, const float x1, const float y1,
    const float width, const int32_t z, const RGBA_8888 color)
{
    FakeUIDraw_Record(String_FormatStatic(
        "segment x0=%.1f y0=%.1f x1=%.1f y1=%.1f width=%.1f z=%d color=%s", x0,
        y0, x1, y1, width, z, M_COLOR(color)));
}

void UI_ScheduleDrawImage(
    const char *const path, const int32_t x0, const int32_t y0,
    const int32_t x1, const int32_t y1, const float opacity)
{
    FakeUIDraw_Record(String_FormatStatic(
        "image path=%s x0=%d y0=%d x1=%d y1=%d opacity=%.2f", path, x0, y0, x1,
        y1, opacity));
}

void UI_ScheduleDrawTextBackground(
    const UI_STYLE ui_style, const UI_MENU_COLORS *const colors,
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const TEXT_STYLE text_style)
{
    FakeUIDraw_Record(String_FormatStatic(
        "text_background style=%d x=%d y=%d z=%d w=%d h=%d text_style=%d",
        (int32_t)ui_style, sx, sy, z, w, h, (int32_t)text_style));
}

void UI_ScheduleDrawTextOutline(
    const UI_STYLE ui_style, const UI_MENU_COLORS *const colors,
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const TEXT_STYLE text_style)
{
    FakeUIDraw_Record(String_FormatStatic(
        "text_outline style=%d x=%d y=%d z=%d w=%d h=%d text_style=%d",
        (int32_t)ui_style, sx, sy, z, w, h, (int32_t)text_style));
}

void UI_ScheduleDrawScreenBlack(const float opacity)
{
    FakeUIDraw_Record(String_FormatStatic("black opacity=%.3f", opacity));
}

void UI_ScheduleDrawScreenFlatQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 color)
{
    FakeUIDraw_Record(String_FormatStatic(
        "quad x=%d y=%d z=%d w=%d h=%d color=%s", sx, sy, z, w, h,
        M_COLOR(color)));
}

void UI_ScheduleDrawScreenGradientQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br)
{
    FakeUIDraw_Record(String_FormatStatic(
        "gradient x=%d y=%d z=%d w=%d h=%d tl=%s tr=%s bl=%s br=%s", sx, sy, z,
        w, h, M_COLOR(tl), M_COLOR(tr), M_COLOR(bl), M_COLOR(br)));
}

void UI_ScheduleDrawHorizontalLine(
    const UI_STYLE ui_style, const UI_MENU_COLORS *const colors,
    const int32_t x0, const int32_t x1, const int32_t y, const int32_t z)
{
    FakeUIDraw_Record(String_FormatStatic(
        "line style=%d x0=%d x1=%d y=%d z=%d", (int32_t)ui_style, x0, x1, y,
        z));
}

void UI_ScheduleDrawScreenCircle(
    const int32_t cx, const int32_t cy, const int32_t r_inner,
    const int32_t r_outer, const int32_t z, const RGBA_8888 color)
{
    FakeUIDraw_Record(String_FormatStatic(
        "circle cx=%d cy=%d r_inner=%d r_outer=%d z=%d color=%s", cx, cy,
        r_inner, r_outer, z, M_COLOR(color)));
}

// How far a drawn frame sits between two ticks. A test sets it to see what the
// blend gives part way through.
void FakeUIDraw_SetInterpolationRate(const double rate)
{
    m_InterpolationRate = rate;
}

double Interpolation_GetUIRate(void)
{
    return m_InterpolationRate;
}

void UI_ScheduleDrawModel(
    const OBJECT_ID object_id, const uint32_t mesh_mask, const int32_t sx,
    const int32_t sy, const int32_t w, const int32_t h, const XYZ_16 rot,
    const bool held, const int16_t tilt, const UI_MODEL_LIGHT *const light)
{
    FakeUIDraw_Record(String_FormatStatic(
        "model obj=%d x=%d y=%d w=%d h=%d rot_x=%d rot_y=%d rot_z=%d mask=%u "
        "lit=%u held=%d",
        (int32_t)object_id, sx, sy, w, h, rot.x, rot.y, rot.z, mesh_mask,
        light != nullptr ? light->lit_mask : 0, held));
}

void UI_ScheduleDrawModelInWorld(
    const OBJECT_ID object_id, const uint32_t mesh_mask,
    const UI_MODEL_WORLD *const world, const UI_MODEL_LIGHT *const light)
{
    // Each step as M for a move, T for a turn and S for a scale, and each bone
    // turn as the bone and the axes it turns about.
    char kinds[UI_MODEL_STEPS + 1] = "";
    for (int32_t i = 0; i < world->step_count; i++) {
        kinds[i] = world->steps[i].kind == UI_MODEL_STEP_TURN ? 'T'
            : world->steps[i].kind == UI_MODEL_STEP_SCALE     ? 'S'
                                                              : 'M';
    }
    char bones[UI_MODEL_BONES * 24] = "";
    for (int32_t i = 0; i < world->bone_count; i++) {
        char part[24];
        snprintf(
            part, sizeof(part), "%s%d:%d,%d,%d", i > 0 ? ";" : "",
            world->bones[i].bone,
            world->bones[i].axes.x ? world->bones[i].rot.x : 0,
            world->bones[i].axes.y ? world->bones[i].rot.y : 0,
            world->bones[i].axes.z ? world->bones[i].rot.z : 0);
        strcat(bones, part);
    }
    FakeUIDraw_Record(String_FormatStatic(
        "world_model obj=%d mask=%u lit=%u lights=%d steps=%d fov=%d kinds=%s "
        "frame=%d next=%d blend=%.2f bones=%s pivot=%d",
        (int32_t)object_id, mesh_mask, light != nullptr ? light->lit_mask : 0,
        light != nullptr ? light->light_count : 0, world->step_count,
        world->fov, kinds, world->frame, world->next_frame, world->blend, bones,
        world->at_pivot ? 1 : 0));
}

void UI_ScheduleDrawDarken(const float opacity)
{
    FakeUIDraw_Record(String_FormatStatic("darken opacity=%.3f", opacity));
}

void UI_ScheduleDrawSnapshotSlot(const int32_t slot, const float opacity)
{
    FakeUIDraw_Record(
        String_FormatStatic("snapshot slot=%d opacity=%.3f", slot, opacity));
}

void UI_ScheduleDrawOverlay(
    void (*const draw)(const void *args), const void *const args,
    const size_t args_size)
{
    draw(args);
}

void UI_ScheduleDrawBackground(
    const char *const path, const bool mono, const float dim)
{
    FakeUIDraw_Record(String_FormatStatic(
        "background path=%s mono=%d dim=%.2f", path != nullptr ? path : "-",
        mono, dim));
}

void UI_ScheduleDrawBackdrop(
    const BACKGROUND_TYPE style, const float progress, const char *const path)
{
    FakeUIDraw_Record(String_FormatStatic(
        "backdrop style=%d progress=%.2f path=%s", (int32_t)style, progress,
        path != nullptr ? path : "-"));
}

void UI_Draw(void)
{
}

int32_t FakeUIDraw_GetCount(void)
{
    return m_Lines == nullptr ? 0 : m_Lines->count;
}

const char *FakeUIDraw_GetLine(const int32_t index)
{
    if (m_Lines == nullptr || index < 0 || index >= m_Lines->count) {
        return nullptr;
    }
    return *(const char **)Vector_Get(m_Lines, index);
}

char *FakeUIDraw_Describe(void)
{
    const int32_t count = FakeUIDraw_GetCount();
    size_t length = 0;
    for (int32_t i = 0; i < count; i++) {
        length += strlen(FakeUIDraw_GetLine(i)) + 1;
    }

    char *const out = Memory_Alloc(length + 1);
    char *cursor = out;
    for (int32_t i = 0; i < count; i++) {
        const char *const line = FakeUIDraw_GetLine(i);
        const size_t line_length = strlen(line);
        memcpy(cursor, line, line_length);
        cursor += line_length;
        *cursor++ = '\n';
    }
    *cursor = '\0';
    return out;
}

void FakeUIDraw_Forget(void)
{
    if (m_Lines == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_Lines->count; i++) {
        char **const line = Vector_Get(m_Lines, i);
        Memory_FreePointer(line);
    }
    Vector_Free(m_Lines);
    m_Lines = nullptr;
    Vector_Free(m_Layers);
    m_Layers = nullptr;
}

const char *UI_GetClipboardText(void)
{
    return m_Clipboard;
}

RESULT UI_SetClipboardText(const char *const text)
{
    strncpy(m_Clipboard, text, sizeof m_Clipboard - 1);
    m_Clipboard[sizeof m_Clipboard - 1] = '\0';
    return OK;
}

// The interface lays itself out between the cinematic bars. A test without the
// overlay fake has none; the overlay fake says where they are.
__attribute__((weak)) float Output_Overlay_GetLetterbox(void)
{
    return 0.0f;
}

// A backdrop records its style and how far it has come in, as the inline
// overlay draw that a backdrop fade layer schedules runs at once here.
__attribute__((weak)) void Output_Overlay_DrawBackground(
    const BACKGROUND_TYPE style, const float opacity,
    const char *const image_path)
{
    FakeUIDraw_Record(String_FormatStatic(
        "backdrop style=%d opacity=%.3f path=%s", style, opacity,
        image_path != nullptr ? image_path : "-"));
}
