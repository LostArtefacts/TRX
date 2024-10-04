#include "game/text.h"

#include "game/console/common.h"
#include "game/output.h"
#include "game/overlay.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <assert.h>
#include <string.h>

#define TEXT_MAX_STRINGS 128
#define CHAR_SECRET_1 0x7Fu
#define CHAR_SECRET_2 0x80u
#define CHAR_SECRET_3 0x81u
#define IS_CHAR_LEGAL(c) ((c) <= CHAR_SECRET_3 && ((c) <= 18u || (c) >= 32u))
#define IS_CHAR_SECRET(c) ((c) >= CHAR_SECRET_1 && (c) <= CHAR_SECRET_3)
#define IS_CHAR_DIACRITIC(c)                                                   \
    ((c) == '(' || (c) == ')' || (c) == '$' || (c) == '~')
#define IS_CHAR_SPACE(c) ((c) == 32)
#define IS_CHAR_DIGIT(c) ((c) <= 0xAu)

// TODO: replace textstring == NULL checks with assertions

static TEXTSTRING m_TextStrings[TEXT_MAX_STRINGS] = { 0 };

void __cdecl Text_Init(void)
{
    Overlay_DisplayModeInfo(NULL);
    for (int32_t i = 0; i < TEXT_MAX_STRINGS; i++) {
        TEXTSTRING *const text = &m_TextStrings[i];
        m_TextStrings[i].flags.all = 0;
    }
}

void Text_Shutdown(void)
{
    for (int32_t i = 0; i < TEXT_MAX_STRINGS; i++) {
        TEXTSTRING *const text = &m_TextStrings[i];
        Memory_FreePointer(&text->content);
    }
}

TEXTSTRING *__cdecl Text_Create(
    const int32_t x, const int32_t y, const int32_t z,
    const char *const content)
{
    if (content == NULL) {
        return NULL;
    }

    int32_t free_idx = -1;
    for (int32_t i = 0; i < TEXT_MAX_STRINGS; i++) {
        TEXTSTRING *const text = &m_TextStrings[i];
        if (!text->flags.active) {
            free_idx = i;
            break;
        }
    }

    if (free_idx == -1) {
        return NULL;
    }

    TEXTSTRING *const text = &m_TextStrings[free_idx];
    text->content = Memory_DupStr(content);
    text->scale.h = PHD_ONE;
    text->scale.v = PHD_ONE;
    text->pos.x = (x * Text_GetScaleH(PHD_ONE)) / PHD_ONE;
    text->pos.y = (y * Text_GetScaleV(PHD_ONE)) / PHD_ONE;
    text->pos.z = z;

    text->letter_spacing = 1;
    text->word_spacing = 6;

    text->text_flags = 0;
    text->outl_flags = 0;
    text->bgnd_flags = 0;
    text->bgnd_size.x = 0;
    text->bgnd_size.y = 0;
    text->bgnd_off.x = 0;
    text->bgnd_off.y = 0;
    text->bgnd_off.z = 0;
    text->flags.all = 0;
    text->flags.active = 1;

    return text;
}

void __cdecl Text_ChangeText(TEXTSTRING *const text, const char *const content)
{
    if (text == NULL) {
        return;
    }
    assert(content != NULL);
    if (text->flags.active) {
        Memory_FreePointer(&text->content);
        text->content = Memory_DupStr(content);
    }
}

void __cdecl Text_SetPos(TEXTSTRING *const text, int16_t x, int16_t y)
{
    if (text == NULL) {
        return;
    }
    text->pos.x = (x * Text_GetScaleH(PHD_ONE)) / PHD_ONE;
    text->pos.y = (y * Text_GetScaleV(PHD_ONE)) / PHD_ONE;
}

void __cdecl Text_SetScale(
    TEXTSTRING *const text, const int32_t scale_h, const int32_t scale_v)
{
    if (text == NULL) {
        return;
    }
    text->scale.h = scale_h;
    text->scale.v = scale_v;
}

void __cdecl Text_Flash(
    TEXTSTRING *const text, const int16_t enable, const int16_t rate)
{
    if (text == NULL) {
        return;
    }
    if (enable) {
        text->flags.flash = 1;
        text->flash.rate = rate;
        text->flash.count = rate;
    } else {
        text->flags.flash = 0;
    }
}

void __cdecl Text_AddBackground(
    TEXTSTRING *const text, const int16_t x_size, const int16_t y_size,
    const int16_t x_off, const int16_t y_off, const int16_t z_off,
    const INV_COLOR color, const uint16_t *const gour_ptr, const uint16_t flags)
{
    if (text == NULL) {
        return;
    }
    uint32_t scale_h = Text_GetScaleH(text->scale.h);
    uint32_t scale_v = Text_GetScaleV(text->scale.v);
    text->flags.background = 1;
    text->bgnd_size.x = (scale_h * x_size) / PHD_ONE;
    text->bgnd_size.y = (scale_v * y_size) / PHD_ONE;
    text->bgnd_off.x = (scale_h * x_off) / PHD_ONE;
    text->bgnd_off.y = (scale_v * y_off) / PHD_ONE;
    text->bgnd_off.z = z_off;
    text->bgnd_color = color;
    text->bgnd_gour = gour_ptr;
    text->bgnd_flags = flags;
}

void __cdecl Text_RemoveBackground(TEXTSTRING *const text)
{
    if (text == NULL) {
        return;
    }
    text->flags.background = 0;
}

void __cdecl Text_AddOutline(
    TEXTSTRING *const text, const int16_t enable, const INV_COLOR color,
    const uint16_t *const gour_ptr, const uint16_t flags)
{
    if (text == NULL) {
        return;
    }
    text->flags.outline = 1;
    text->outl_gour = gour_ptr;
    text->outl_color = color;
    text->outl_flags = flags;
}

void __cdecl Text_RemoveOutline(TEXTSTRING *const text)
{
    if (text == NULL) {
        return;
    }
    text->flags.outline = 0;
}

void __cdecl Text_CentreH(TEXTSTRING *const text, const int16_t enable)
{
    if (text == NULL) {
        return;
    }
    text->flags.centre_h = enable;
}

void __cdecl Text_CentreV(TEXTSTRING *const text, const int16_t enable)
{
    if (text == NULL) {
        return;
    }
    text->flags.centre_v = enable;
}

void __cdecl Text_AlignRight(TEXTSTRING *const text, const int16_t enable)
{
    if (text == NULL) {
        return;
    }
    text->flags.right = enable;
}

void __cdecl Text_AlignBottom(TEXTSTRING *const text, const int16_t enable)
{
    if (text == NULL) {
        return;
    }
    text->flags.bottom = enable;
}

void __cdecl Text_SetMultiline(TEXTSTRING *text, bool enable)
{
    if (text == NULL) {
        return;
    }
    text->flags.multiline = enable;
}

int32_t __cdecl Text_Remove(TEXTSTRING *const text)
{
    if (text == NULL) {
        return false;
    }
    if (!text->flags.active) {
        return false;
    }
    text->flags.active = false;
    return true;
}

int32_t __cdecl Text_GetWidth(TEXTSTRING *const text)
{
    if (text == NULL) {
        return 0;
    }

    const uint32_t scale_h = Text_GetScaleH(text->scale.h);
    const char *content = text->content;
    int32_t width = 0;

    while (1) {
        uint8_t c = *content++;
        if (!c) {
            break;
        }

        if (!IS_CHAR_LEGAL(c) || IS_CHAR_DIACRITIC(c)) {
            continue;
        }

        int32_t spacing;
        if (IS_CHAR_SPACE(c)) {
            spacing = text->word_spacing;
        } else if (IS_CHAR_SECRET(c)) {
            spacing = 16;
        } else {
            int16_t sprite_num;
            if (IS_CHAR_DIGIT(c)) {
                sprite_num = c + 81;
            } else {
                sprite_num = g_TextASCIIMap[c];
            }

            // TODO: OG bug - this should check c, not sprite_num
            if (sprite_num >= '0' && sprite_num <= '9') {
                spacing = 12;
            } else {
                // TODO: OG bug - wrong letter spacing calculation
                spacing = g_TextSpacing[sprite_num] + text->letter_spacing;
            }
        }

        width += spacing * scale_h / PHD_ONE;
    }

    // TODO: OG bug - wrong letter spacing calculation; pointless ~1
    return ((int16_t)width - text->letter_spacing) & ~1;
}

int32_t Text_GetHeight(const TEXTSTRING *const text)
{
    if (text == NULL) {
        return 0;
    }
    int32_t height = TEXT_HEIGHT;
    char *content = text->content;
    for (char letter = *content; letter != '\0'; letter = *content++) {
        if (text->flags.multiline && letter == '\n') {
            height += TEXT_HEIGHT;
        }
    }
    return height * Text_GetScaleV(text->scale.v) / PHD_ONE;
}

void __cdecl Text_Draw(void)
{
    // TODO: move me outta here!
    Console_Draw();
    for (int32_t i = 0; i < TEXT_MAX_STRINGS; i++) {
        TEXTSTRING *const text = &m_TextStrings[i];
        if (text->flags.active) {
            Text_DrawText(text);
        }
    }
}

void __cdecl Text_DrawBorder(
    const int32_t x, const int32_t y, const int32_t z, const int32_t width,
    const int32_t height)
{
    const int32_t mesh_idx = g_Objects[O_TEXT_BOX].mesh_idx;

    const int32_t offset = 4;
    const int32_t x0 = x + offset;
    const int32_t y0 = y + offset;
    const int32_t x1 = x0 + width - offset * 2;
    const int32_t y1 = y0 + height - offset * 2;
    const int32_t scale_h = PHD_ONE;
    const int32_t scale_v = PHD_ONE;

    Output_DrawScreenSprite2D(
        x0, y0, z, scale_h, scale_v, mesh_idx + 0, 0x1000, 0);
    Output_DrawScreenSprite2D(
        x1, y0, z, scale_h, scale_v, mesh_idx + 1, 0x1000, 0);
    Output_DrawScreenSprite2D(
        x1, y1, z, scale_h, scale_v, mesh_idx + 2, 0x1000, 0);
    Output_DrawScreenSprite2D(
        x0, y1, z, scale_h, scale_v, mesh_idx + 3, 0x1000, 0);

    int32_t w = (width - offset * 2) * PHD_ONE / 8;
    int32_t h = (height - offset * 2) * PHD_ONE / 8;

    Output_DrawScreenSprite2D(x0, y0, z, w, scale_v, mesh_idx + 4, 0x1000, 0);
    Output_DrawScreenSprite2D(x1, y0, z, scale_h, h, mesh_idx + 5, 0x1000, 0);
    Output_DrawScreenSprite2D(x0, y1, z, w, scale_v, mesh_idx + 6, 0x1000, 0);
    Output_DrawScreenSprite2D(x0, y0, z, scale_h, h, mesh_idx + 7, 0x1000, 0);
}

void __cdecl Text_DrawText(TEXTSTRING *const text)
{
    int32_t box_w = 0;
    int32_t box_h = 0;
    const int32_t scale_h = Text_GetScaleH(text->scale.h);
    const int32_t scale_v = Text_GetScaleV(text->scale.v);

    if (text->flags.flash) {
        text->flash.count -= g_Camera.num_frames;
        if (text->flash.count <= -text->flash.rate) {
            text->flash.count = text->flash.rate;
        } else if (text->flash.count < 0) {
            return;
        }
    }

    int32_t x = text->pos.x;
    int32_t y = text->pos.y;
    int32_t z = text->pos.z;
    int32_t text_width = Text_GetWidth(text);

    if (text->flags.centre_h) {
        x += (GetRenderWidth() - text_width) / 2;
    } else if (text->flags.right) {
        x += GetRenderWidth() - text_width;
    }

    if (text->flags.centre_v) {
        y += GetRenderHeight() / 2;
    } else if (text->flags.bottom) {
        y += GetRenderHeight();
    }

    int32_t box_x = x + text->bgnd_off.x - ((2 * scale_h) / PHD_ONE);
    int32_t box_y = y + text->bgnd_off.y - ((4 * scale_v) / PHD_ONE)
        - ((11 * scale_v) / PHD_ONE);
    const int32_t start_x = x;

    const char *content = text->content;
    while (1) {
        const uint8_t c = *content++;
        if (!c) {
            break;
        }

        if (text->flags.multiline && c == '\n') {
            y += TEXT_HEIGHT * Text_GetScaleV(text->scale.v) / PHD_ONE;
            x = start_x;
            continue;
        }

        if (!IS_CHAR_LEGAL(c)) {
            continue;
        }

        if (IS_CHAR_SPACE(c)) {
            const int32_t spacing = text->word_spacing;
            x += spacing * scale_h / PHD_ONE;
        } else if (IS_CHAR_SECRET(c)) {
            Output_DrawPickup(
                x + 10, y, 7144,
                g_Objects[O_SECRET_1 + c - CHAR_SECRET_1].mesh_idx, 4096);
            const int32_t spacing = 16;
            x += spacing * scale_h / PHD_ONE;
        } else {
            int32_t sprite_num;
            if (IS_CHAR_DIGIT(c)) {
                sprite_num = c + 81;
            } else if (c <= 0x12) {
                sprite_num = c + 91;
            } else {
                sprite_num = g_TextASCIIMap[c];
            }

            if (c >= '0' && c <= '9') {
                const int32_t spacing = (12 - g_TextSpacing[sprite_num]) / 2;
                x += spacing * scale_h / PHD_ONE;
            }

            if (x >= 0 && x < GetRenderWidth() && y >= 0
                && y < GetRenderHeight()) {
                Output_DrawScreenSprite2D(
                    x, y, z, scale_h, scale_v,
                    g_Objects[O_ALPHABET].mesh_idx + sprite_num, 4096,
                    text->text_flags);
            }

            if (IS_CHAR_DIACRITIC(c)) {
                continue;
            }

            if (c >= '0' && c <= '9') {
                const int32_t x_off = (12 - g_TextSpacing[sprite_num]) / 2;
                x += (12 - x_off) * scale_h / PHD_ONE;
            } else {
                const int32_t spacing =
                    g_TextSpacing[sprite_num] + text->letter_spacing;
                x += spacing * scale_h / PHD_ONE;
            }
        }
    }

    if (text->flags.outline || text->flags.background) {
        if (text->bgnd_size.x) {
            box_x += (text_width - text->bgnd_size.x) / 2;
            box_w = text->bgnd_size.x + 4;
        } else {
            box_w = text_width + 4;
        }

        box_h =
            text->bgnd_size.y ? text->bgnd_size.y : ((16 * scale_v) / PHD_ONE);
    }

    if (text->flags.background) {
        S_DrawScreenFBox(
            box_x, box_y, text->bgnd_off.z + z + 2, box_w, box_h,
            text->bgnd_color, (const GOURAUD_FILL *)text->bgnd_gour,
            text->bgnd_flags);
    }

    if (text->flags.outline) {
        Text_DrawBorder(box_x, box_y, z, box_w, box_h);
    }
}

uint32_t __cdecl Text_GetScaleH(const uint32_t value)
{
    const int32_t render_width = GetRenderWidth();
    const int32_t render_scale = MAX(render_width, 640) * PHD_ONE / 640;
    return (value / PHD_HALF) * (render_scale / PHD_HALF);
}

uint32_t __cdecl Text_GetScaleV(const uint32_t value)
{
    const int32_t render_height = GetRenderHeight();
    const int32_t render_scale = MAX(render_height, 480) * PHD_ONE / 480;
    return (value / PHD_HALF) * (render_scale / PHD_HALF);
}

int32_t Text_GetMaxLineLength(void)
{
    return 640 / (TEXT_HEIGHT * 0.75);
}
