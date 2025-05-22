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
    if (text->glyphs == nullptr) {
        return;
    }

    const OBJECT *const obj = Object_Get(O_ALPHABET);
    if (!obj->loaded) {
        return;
    }

    double x = text->pos.x;
    double y = text->pos.y;

    int32_t sx;
    int32_t sy;
    int32_t sh = Screen_GetRenderScale(text->scale.h, RSR_TEXT);
    int32_t sv = Screen_GetRenderScale(text->scale.v, RSR_TEXT);
    const int32_t start_x = x;

    const GLYPH_INFO **glyph_ptr = text->glyphs;
    while (*glyph_ptr != nullptr) {
        const GLYPH_INFO *glyph = *glyph_ptr;
        if (glyph->role == GLYPH_NEWLINE) {
            y += TEXT_HEIGHT * text->scale.v / TEXT_BASE_SCALE;
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
                obj->mesh_idx + glyph->combine_with.mesh_idx, SHADE_NEUTRAL, 0, 0);
        }

        if (glyph->mesh_idx >= ABS(obj->mesh_count)) {
            goto loop_end;
        }
        Output_DrawScreenSprite(
            sx, sy, 0, sh, sv, obj->mesh_idx + glyph->mesh_idx, SHADE_NEUTRAL, 0, 0);

        if (glyph->role != GLYPH_COMBINING) {
            x += (text->letter_spacing + glyph->width) * text->scale.h
                / TEXT_BASE_SCALE;
        }

    loop_end:
        glyph_ptr++;
    }
}

int32_t Text_GetMaxLineLength(void)
{
    return Screen_GetResWidthDownscaled(RSR_TEXT) / (TEXT_HEIGHT * 0.6);
}
