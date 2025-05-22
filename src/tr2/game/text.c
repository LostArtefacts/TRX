#include "game/text.h"

#include "decomp/decomp.h"
#include "game/clock.h"
#include "game/output.h"
#include "global/vars.h"

#include <libtrx/game/scaler.h>
#include <libtrx/utils.h>

static int32_t M_Scale(const int32_t value);

static int32_t M_Scale(const int32_t value)
{
    return Scaler_Calc(value, SCALER_TARGET_TEXT);
}

void Text_DrawText(TEXTSTRING *const text)
{
    if (text->glyphs == nullptr) {
        return;
    }

    const OBJECT *const obj = Object_Get(O_ALPHABET);
    if (!obj->loaded) {
        return;
    }

    int32_t box_w = 0;
    int32_t box_h = 0;
    const int32_t scale_h = M_Scale(text->scale.h);
    const int32_t scale_v = M_Scale(text->scale.v);

    int32_t x = (text->pos.x * M_Scale(TEXT_BASE_SCALE)) / TEXT_BASE_SCALE;
    int32_t y = (text->pos.y * M_Scale(TEXT_BASE_SCALE)) / TEXT_BASE_SCALE;
    int32_t z = text->pos.z;
    int32_t text_width =
        Text_GetWidth(text) * M_Scale(TEXT_BASE_SCALE) / TEXT_BASE_SCALE;

    const int32_t start_x = x;

    const GLYPH_INFO **glyph_ptr = text->glyphs;
    while (*glyph_ptr != nullptr) {
        const GLYPH_INFO *glyph = *glyph_ptr;
        if (glyph->role == GLYPH_NEWLINE) {
            y += TEXT_HEIGHT * M_Scale(text->scale.v) / TEXT_BASE_SCALE;
            x = start_x;
            goto loop_end;
        }

        if (glyph->role == GLYPH_SPACE) {
            x += text->word_spacing * scale_h / TEXT_BASE_SCALE;
            goto loop_end;
        }

        if (glyph->role == GLYPH_SECRET) {
            const int16_t sprite_idx =
                Object_Get(O_SECRET_1 + glyph->mesh_idx)->mesh_idx;
            const SPRITE_TEXTURE *const sprite =
                Output_GetSpriteTexture(sprite_idx);
            const float sprite_scale_h =
                text->scale.h / (sprite->x1 - sprite->x0);
            const float sprite_scale_v =
                text->scale.v / (sprite->y1 - sprite->y0);
            const float sprite_scale = MIN(sprite_scale_h, sprite_scale_v);
            Output_DrawScreenSprite(
                x + M_Scale(10), y, z, M_Scale(glyph->width * sprite_scale),
                M_Scale(glyph->width * sprite_scale), sprite_idx, SHADE_NEUTRAL,
                0);
            x += glyph->width * scale_h / TEXT_BASE_SCALE;
            goto loop_end;
        }

        if (glyph->role == GLYPH_COMPOUND) {
            const int32_t cx =
                x + (glyph->combine_with.offset_x * scale_h / TEXT_BASE_SCALE);
            const int32_t cy =
                y + (glyph->combine_with.offset_y * scale_h / TEXT_BASE_SCALE);
            if (glyph->combine_with.mesh_idx >= ABS(obj->mesh_count)) {
                goto loop_end;
            }

            Output_DrawScreenSprite(
                cx, cy, 0, scale_h, scale_v,
                obj->mesh_idx + glyph->combine_with.mesh_idx, SHADE_NEUTRAL, 0);
        }

        if (x >= 0 && x < g_PhdWinWidth && y >= 0 && y < g_PhdWinHeight) {
            if (glyph->mesh_idx >= ABS(obj->mesh_count)) {
                goto loop_end;
            }
            Output_DrawScreenSprite(
                x, y, z, scale_h, scale_v, obj->mesh_idx + glyph->mesh_idx,
                SHADE_NEUTRAL, 0);
        }

        if (glyph->role != GLYPH_COMBINING) {
            const int32_t spacing = text->letter_spacing + glyph->width;
            x += spacing * scale_h / TEXT_BASE_SCALE;
        }

    loop_end:
        glyph_ptr++;
    }
}

int32_t Text_GetMaxLineLength(void)
{
    return 640 / (TEXT_HEIGHT * 0.75);
}
