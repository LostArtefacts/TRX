#include "game/ui2/elements/label.h"

#include "config.h"
#include "game/text.h"
#include "game/ui2/helpers.h"

#include <string.h>

typedef struct {
    UI2_LABEL_SETTINGS settings;
    char *text;
} M_DATA;

static void M_Measure(UI2_NODE *node);
static void M_Draw(const UI2_NODE *node);

static UI2_LABEL_SETTINGS m_DefaultSettings = { .scale = 1.0f };
static const UI2_WIDGET_OPS m_Ops = {
    .measure = M_Measure,
    .layout = UI2_LayoutBasic,
    .draw = M_Draw,
};

static TEXTSTRING *M_CreateText(
    const float x, const float y, const char *text,
    const UI2_LABEL_SETTINGS settings);

static TEXTSTRING *M_CreateText(
    const float x, const float y, const char *text,
    const UI2_LABEL_SETTINGS settings)
{
    TEXTSTRING *const textstring = Text_Create(x, y, text);
    Text_SetPos(
        textstring, x / g_Config.ui.text_scale,
        y / g_Config.ui.text_scale + TEXT_HEIGHT_FIXED - 1);
    Text_SetMultiline(textstring, true);
    Text_SetScale(
        textstring, settings.scale * TEXT_BASE_SCALE,
        settings.scale * TEXT_BASE_SCALE);
    return textstring;
}

static void M_Measure(UI2_NODE *const node)
{
    M_DATA *const data = node->data;
    UI2_Label_MeasureEx(
        data->text, &node->measure_w, &node->measure_h, data->settings);
}

static void M_Draw(const UI2_NODE *const node)
{
    M_DATA *const data = node->data;
    TEXTSTRING *const textstring =
        M_CreateText(node->x, node->y, data->text, data->settings);
    if (data->settings.z != 0) {
        textstring->pos.z = data->settings.z;
    }
    Text_DrawText(textstring);
    Text_Remove(textstring);
    UI2_DrawWrapper(node);
}

void UI2_Label(const char *const text)
{
    UI2_LabelEx(text, m_DefaultSettings);
}

void UI2_LabelEx(const char *const text, const UI2_LABEL_SETTINGS settings)
{
    UI2_NODE *const node =
        UI2_AllocNode(&m_Ops, sizeof(M_DATA) + strlen(text) + 1);
    M_DATA *const data = node->data;
    data->settings = settings;
    data->text = (char *)node->data + sizeof(M_DATA);
    strcpy(data->text, text);
    UI2_AddChild(node);
}

void UI2_Label_Measure(
    const char *const text, float *const out_w, float *const out_h)
{
    UI2_Label_MeasureEx(text, out_w, out_h, m_DefaultSettings);
}

void UI2_Label_MeasureEx(
    const char *const text, float *const out_w, float *const out_h,
    const UI2_LABEL_SETTINGS settings)
{
    TEXTSTRING *const textstring = M_CreateText(0, 0, text, settings);
    if (out_w != nullptr) {
        *out_w = Text_GetWidth(textstring) * g_Config.ui.text_scale;
    }
    if (out_h != nullptr) {
        *out_h = Text_GetHeight(textstring) * g_Config.ui.text_scale;
    }
    Text_Remove(textstring);
}
