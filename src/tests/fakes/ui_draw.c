// Records scheduled UI draw calls as text, so layout tests can compare scenes
// without starting the renderer.

#include <fakes/ui_draw.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/vector.h>
#include <trx/game/ui/draw.h>

#include <stdio.h>
#include <string.h>

// Stores one formatted colour per draw argument.
typedef struct {
    char text[9];
} M_COLOR_TEXT;

static VECTOR *m_Lines = nullptr;

static void M_Record(const char *const line)
{
    if (m_Lines == nullptr) {
        m_Lines = Vector_Create(sizeof(char *));
    }
    char *const copy = Memory_DupStr(line);
    Vector_Add(m_Lines, &copy);
}

static M_COLOR_TEXT M_Color(const RGBA_8888 color)
{
    M_COLOR_TEXT out;
    snprintf(
        out.text, sizeof(out.text), "%02x%02x%02x%02x", color.r, color.g,
        color.b, color.a);
    return out;
}

#define M_COLOR(color) (M_Color(color).text)

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
    M_Record(String_FormatStatic(
        "sprite idx=%d x=%d y=%d z=%d scale_h=%d scale_v=%d", sprite_idx, sx,
        sy, z, scale_h, scale_v));
}

void UI_ScheduleDrawTextBackground(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy,
    const int32_t z, const int32_t w, const int32_t h,
    const TEXT_STYLE text_style)
{
    M_Record(String_FormatStatic(
        "text_background style=%d x=%d y=%d z=%d w=%d h=%d text_style=%d",
        (int32_t)ui_style, sx, sy, z, w, h, (int32_t)text_style));
}

void UI_ScheduleDrawTextOutline(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy,
    const int32_t z, const int32_t w, const int32_t h,
    const TEXT_STYLE text_style)
{
    M_Record(String_FormatStatic(
        "text_outline style=%d x=%d y=%d z=%d w=%d h=%d text_style=%d",
        (int32_t)ui_style, sx, sy, z, w, h, (int32_t)text_style));
}

void UI_ScheduleDrawScreenFlatQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 color)
{
    M_Record(String_FormatStatic(
        "quad x=%d y=%d z=%d w=%d h=%d color=%s", sx, sy, z, w, h,
        M_COLOR(color)));
}

void UI_ScheduleDrawScreenGradientQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br)
{
    M_Record(String_FormatStatic(
        "gradient x=%d y=%d z=%d w=%d h=%d tl=%s tr=%s bl=%s br=%s", sx, sy, z,
        w, h, M_COLOR(tl), M_COLOR(tr), M_COLOR(bl), M_COLOR(br)));
}

void UI_ScheduleDrawHorizontalLine(
    const UI_STYLE ui_style, const int32_t x0, const int32_t x1,
    const int32_t y, const int32_t z)
{
    M_Record(String_FormatStatic(
        "line style=%d x0=%d x1=%d y=%d z=%d", (int32_t)ui_style, x0, x1, y,
        z));
}

void UI_ScheduleDrawScreenCircle(
    const int32_t cx, const int32_t cy, const int32_t r_inner,
    const int32_t r_outer, const int32_t z, const RGBA_8888 color)
{
    M_Record(String_FormatStatic(
        "circle cx=%d cy=%d r_inner=%d r_outer=%d z=%d color=%s", cx, cy,
        r_inner, r_outer, z, M_COLOR(color)));
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
}
