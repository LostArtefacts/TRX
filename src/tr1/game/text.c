#include "game/text.h"

#include "game/clock.h"
#include "game/output.h"
#include "game/screen.h"
#include "global/vars.h"

#include <libtrx/config.h>

#define TEXT_BOX_OFFSET 2

static RGBA_8888 m_MenuColorMap[MC_NUMBER_OF] = {
    { 70, 30, 107, 230 }, // MC_PURPLE_C
    { 70, 30, 107, 0 }, // MC_PURPLE_E
    { 91, 46, 9, 255 }, // MC_BROWN_C
    { 91, 46, 9, 0 }, // MC_BROWN_E
    { 197, 197, 197, 255 }, // MC_GREY_C
    { 45, 45, 45, 255 }, // MC_GREY_E
    { 96, 96, 96, 255 }, // MC_GREY_TL
    { 32, 32, 32, 255 }, // MC_GREY_TR
    { 63, 63, 63, 255 }, // MC_GREY_BL
    { 0, 0, 0, 255 }, // MC_GREY_BR
    { 0, 0, 0, 255 }, // MC_BLACK
    { 232, 192, 112, 255 }, // MC_GOLD_LIGHT
    { 140, 112, 56, 255 }, // MC_GOLD_DARK
};

typedef struct {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} QUAD_INFO;

static void M_DrawTextBackground(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h,
    TEXT_STYLE text_style);
static void M_DrawTextOutline(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h,
    TEXT_STYLE text_style);

static void M_DrawTextBackground(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy, int32_t w,
    int32_t h, const TEXT_STYLE text_style)
{
    if (ui_style == UI_STYLE_PC) {
        Output_DrawScreenFBox(sx, sy, w, h);
        return;
    }

    // Make sure height and width divisible by 2.
    w = 2 * ((w + 1) / 2);
    h = 2 * ((h + 1) / 2);
    Output_DrawScreenFBox(sx - 1, sy - 1, w + 1, h + 1);

    QUAD_INFO gradient_quads[4] = { { sx, sy, w / 2, h / 2 },
                                    { sx + w, sy, -w / 2, h / 2 },
                                    { sx, sy + h, w / 2, -h / 2 },
                                    { sx + w, sy + h, -w / 2, -h / 2 } };

    if (text_style == TS_HEADING) {
        for (int i = 0; i < 4; i++) {
            Output_DrawScreenGradientQuad(
                gradient_quads[i].x, gradient_quads[i].y, gradient_quads[i].w,
                gradient_quads[i].h, Text_GetMenuColor(MC_BROWN_E),
                Text_GetMenuColor(MC_BROWN_E), Text_GetMenuColor(MC_BROWN_E),
                Text_GetMenuColor(MC_BROWN_C));
        }
    } else if (text_style == TS_REQUESTED) {
        for (int i = 0; i < 4; i++) {
            Output_DrawScreenGradientQuad(
                gradient_quads[i].x, gradient_quads[i].y, gradient_quads[i].w,
                gradient_quads[i].h, Text_GetMenuColor(MC_PURPLE_E),
                Text_GetMenuColor(MC_PURPLE_E), Text_GetMenuColor(MC_PURPLE_E),
                Text_GetMenuColor(MC_PURPLE_C));
        }
    }
}

static void M_DrawTextOutline(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy, int32_t w,
    int32_t h, const TEXT_STYLE text_style)
{
    if (ui_style == UI_STYLE_PC) {
        Output_DrawScreenBox(
            sx, sy, w, h, Text_GetMenuColor(MC_GOLD_DARK),
            Text_GetMenuColor(MC_GOLD_LIGHT), TEXT_OUTLINE_THICKNESS);
        return;
    }

    if (text_style == TS_HEADING) {
        Output_DrawGradientScreenBox(
            sx, sy, w, h, Text_GetMenuColor(MC_BLACK),
            Text_GetMenuColor(MC_BLACK), Text_GetMenuColor(MC_BLACK),
            Text_GetMenuColor(MC_BLACK), TEXT_OUTLINE_THICKNESS);
    } else if (text_style == TS_BACKGROUND) {
        Output_DrawGradientScreenBox(
            sx, sy, w, h, Text_GetMenuColor(MC_GREY_TL),
            Text_GetMenuColor(MC_GREY_TR), Text_GetMenuColor(MC_GREY_BL),
            Text_GetMenuColor(MC_GREY_BR), TEXT_OUTLINE_THICKNESS);
    } else if (text_style == TS_REQUESTED) {
        // Make sure height and width divisible by 2.
        w = 2 * ((w + 1) / 2);
        h = 2 * ((h + 1) / 2);
        Output_DrawCentreGradientScreenBox(
            sx, sy, w, h, Text_GetMenuColor(MC_GREY_E),
            Text_GetMenuColor(MC_GREY_C), TEXT_OUTLINE_THICKNESS);
    }
}

RGBA_8888 Text_GetMenuColor(MENU_COLOR color)
{
    return m_MenuColorMap[color];
}

void Text_DrawText(TEXTSTRING *const text)
{
    if (text->flags.drawn) {
        return;
    }
    text->flags.drawn = 1;

    if (text->flags.hide || text->glyphs == nullptr) {
        return;
    }

    const OBJECT *const obj = Object_Get(O_ALPHABET);
    if (!obj->loaded) {
        return;
    }

    if (text->flags.flash) {
        text->flash.count -= Clock_GetFrameAdvance();
        if (text->flash.count <= -text->flash.rate) {
            text->flash.count = text->flash.rate;
        } else if (text->flash.count < 0) {
            return;
        }
    }

    double x = text->pos.x;
    double y = text->pos.y;
    int32_t text_width = Text_GetWidth(text);

    if (text->flags.centre_h) {
        x += (Screen_GetResWidthDownscaled(RSR_TEXT) - text_width) / 2;
    } else if (text->flags.right) {
        x += (Screen_GetResWidthDownscaled(RSR_TEXT) - text_width);
    }

    if (text->flags.centre_v) {
        y += Screen_GetResHeightDownscaled(RSR_TEXT) / 2.0;
    } else if (text->flags.bottom) {
        y += Screen_GetResHeightDownscaled(RSR_TEXT);
    }

    int32_t bxpos = text->background.offset.x + x - TEXT_BOX_OFFSET;
    int32_t bypos =
        text->background.offset.y + y - TEXT_BOX_OFFSET * 2 - TEXT_HEIGHT;

    int32_t sx;
    int32_t sy;
    int32_t sh;
    int32_t sv;
    const int32_t start_x = x;

    const GLYPH_INFO **glyph_ptr = text->glyphs;
    while (*glyph_ptr != nullptr) {
        const GLYPH_INFO *glyph = *glyph_ptr;
        if (text->flags.multiline && glyph->role == GLYPH_NEWLINE) {
            y += TEXT_HEIGHT_FIXED * text->scale.v / TEXT_BASE_SCALE;
            x = start_x;
            goto loop_end;
        }
        if (glyph->role == GLYPH_SPACE) {
            x += text->word_spacing * text->scale.h / TEXT_BASE_SCALE;
            goto loop_end;
        }

        sx = Screen_GetRenderScale(x, RSR_TEXT);
        sy = Screen_GetRenderScale(y, RSR_TEXT);
        sh = Screen_GetRenderScale(text->scale.h, RSR_TEXT);
        sv = Screen_GetRenderScale(text->scale.v, RSR_TEXT);

        if (glyph->role == GLYPH_COMPOUND) {
            const int32_t csx = sx
                + Screen_GetRenderScale(glyph->combine_with.offset_x, RSR_TEXT);
            const int32_t csy = sy
                + Screen_GetRenderScale(glyph->combine_with.offset_y, RSR_TEXT);
            if (glyph->combine_with.mesh_idx >= ABS(obj->mesh_count)) {
                goto loop_end;
            }

            Output_DrawScreenSprite(
                csx, csy, 0, sh, sv,
                obj->mesh_idx + glyph->combine_with.mesh_idx, 16 << 8, 0, 0);
        }

        if (glyph->mesh_idx >= ABS(obj->mesh_count)) {
            goto loop_end;
        }
        Output_DrawScreenSprite(
            sx, sy, 0, sh, sv, obj->mesh_idx + glyph->mesh_idx, 16 << 8, 0, 0);

        if (glyph->role != GLYPH_COMBINING) {
            x += (text->letter_spacing + glyph->width) * text->scale.h
                / TEXT_BASE_SCALE;
        }
    loop_end:
        glyph_ptr++;
    }

    int32_t bwidth = 0;
    int32_t bheight = 0;
    if (text->flags.background || text->flags.outline) {
        if (text->background.size.x) {
            bxpos += text_width / 2;
            bxpos -= text->background.size.x / 2;
            bwidth = text->background.size.x + TEXT_BOX_OFFSET * 2;
        } else {
            bwidth = text_width + TEXT_BOX_OFFSET * 2;
        }
        if (text->background.size.y) {
            bheight = text->background.size.y;
        } else {
            bheight = TEXT_HEIGHT + 7;
        }
    }

    if (text->flags.background) {
        sx = Screen_GetRenderScale(bxpos, RSR_TEXT);
        sy = Screen_GetRenderScale(bypos, RSR_TEXT);
        sh = Screen_GetRenderScale(bwidth, RSR_TEXT);
        sv = Screen_GetRenderScale(bheight, RSR_TEXT);

        M_DrawTextBackground(
            g_Config.ui.menu_style, sx, sy, sh, sv, text->background.style);
    }

    if (text->flags.outline) {
        sx = Screen_GetRenderScale(bxpos, RSR_TEXT);
        sy = Screen_GetRenderScale(bypos, RSR_TEXT);
        sh = Screen_GetRenderScale(bwidth, RSR_TEXT);
        sv = Screen_GetRenderScale(bheight, RSR_TEXT);

        M_DrawTextOutline(
            g_Config.ui.menu_style, sx, sy, sh, sv, text->outline.style);
    }
}

int32_t Text_GetMaxLineLength(void)
{
    return Screen_GetResWidthDownscaled(RSR_TEXT) / (TEXT_HEIGHT * 0.75);
}
