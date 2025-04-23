#include "game/text.h"

#include "game/clock.h"
#include "game/output.h"
#include "game/screen.h"
#include "global/vars.h"

#include <libtrx/config.h>

#define TEXT_BOX_OFFSET_X 2
#define TEXT_BOX_OFFSET_Y1 0
#define TEXT_BOX_OFFSET_Y2 3

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

    int32_t bxpos = text->background.offset.x + x - TEXT_BOX_OFFSET_X;
    int32_t bypos =
        text->background.offset.y + y + TEXT_BOX_OFFSET_Y1 - TEXT_HEIGHT_FIXED;

    int32_t sx;
    int32_t sy;
    int32_t sh = Screen_GetRenderScale(text->scale.h, RSR_TEXT);
    int32_t sv = Screen_GetRenderScale(text->scale.v, RSR_TEXT);
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
            bwidth = text->background.size.x + TEXT_BOX_OFFSET_X * 2;
        } else {
            bwidth = text_width + TEXT_BOX_OFFSET_X * 2;
        }
        if (text->background.size.y) {
            bheight = text->background.size.y;
        } else {
            bheight = TEXT_HEIGHT_FIXED + TEXT_BOX_OFFSET_Y2;
        }
    }

    if (text->flags.background) {
        sx = Screen_GetRenderScale(bxpos, RSR_TEXT);
        sy = Screen_GetRenderScale(bypos, RSR_TEXT);
        sh = Screen_GetRenderScale(bwidth, RSR_TEXT);
        sv = Screen_GetRenderScale(bheight, RSR_TEXT);
        Output_DrawTextBackground(
            g_Config.ui.menu_style, sx, sy, sh, sv, 0, text->background.style);
    }

    if (text->flags.outline) {
        sx = Screen_GetRenderScale(bxpos, RSR_TEXT);
        sy = Screen_GetRenderScale(bypos, RSR_TEXT);
        sh = Screen_GetRenderScale(bwidth, RSR_TEXT);
        sv = Screen_GetRenderScale(bheight, RSR_TEXT);
        Output_DrawTextOutline(
            g_Config.ui.menu_style, sx, sy, sh, sv, 0, text->outline.style);
    }
}

int32_t Text_GetMaxLineLength(void)
{
    return Screen_GetResWidthDownscaled(RSR_TEXT) / (TEXT_HEIGHT_FIXED * 0.6);
}
