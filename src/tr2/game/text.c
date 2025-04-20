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

    int32_t box_w = 0;
    int32_t box_h = 0;
    const int32_t scale_h = M_Scale(text->scale.h);
    const int32_t scale_v = M_Scale(text->scale.v);

    if (text->flags.flash) {
        text->flash.count -= Clock_GetFrameAdvance();
        if (text->flash.count <= -text->flash.rate) {
            text->flash.count = text->flash.rate;
        } else if (text->flash.count < 0) {
            return;
        }
    }

    int32_t x = (text->pos.x * M_Scale(TEXT_BASE_SCALE)) / TEXT_BASE_SCALE;
    int32_t y = (text->pos.y * M_Scale(TEXT_BASE_SCALE)) / TEXT_BASE_SCALE;
    int32_t z = text->pos.z;
    int32_t text_width =
        Text_GetWidth(text) * M_Scale(TEXT_BASE_SCALE) / TEXT_BASE_SCALE;

    if (text->flags.centre_h) {
        x += (g_PhdWinWidth - text_width) / 2;
    } else if (text->flags.right) {
        x += g_PhdWinWidth - text_width;
    }

    if (text->flags.centre_v) {
        y += g_PhdWinHeight / 2;
    } else if (text->flags.bottom) {
        y += g_PhdWinHeight;
    }

    int32_t box_x = x
        + (text->background.offset.x * M_Scale(TEXT_BASE_SCALE))
            / TEXT_BASE_SCALE
        - ((2 * scale_h) / TEXT_BASE_SCALE);
    int32_t box_y = y
        + (text->background.offset.y * M_Scale(TEXT_BASE_SCALE))
            / TEXT_BASE_SCALE
        - ((4 * scale_v) / TEXT_BASE_SCALE)
        - ((11 * scale_v) / TEXT_BASE_SCALE);
    const int32_t start_x = x;

    const GLYPH_INFO **glyph_ptr = text->glyphs;
    while (*glyph_ptr != nullptr) {
        const GLYPH_INFO *glyph = *glyph_ptr;
        if (text->flags.multiline && glyph->role == GLYPH_NEWLINE) {
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

    if (text->flags.outline || text->flags.background) {
        if (text->background.size.x) {
            const int32_t background_width =
                (text->background.size.x * scale_h) / TEXT_BASE_SCALE;
            box_x += (text_width - background_width) / 2;
            box_w = background_width + 4;
        } else {
            box_w = text_width + 4;
        }

        const int32_t background_height =
            (text->background.size.y * scale_v) / TEXT_BASE_SCALE;
        box_h = text->background.size.y ? background_height
                                        : ((16 * scale_v) / TEXT_BASE_SCALE);
    }

    if (text->flags.background) {
        Output_DrawTextBackground(
            UI_STYLE_PC, box_x, box_y, box_w, box_h,
            z + text->background.offset.z, TS_REQUESTED);
    }

    if (text->flags.outline) {
        Output_DrawTextOutline(
            UI_STYLE_PC, box_x, box_y, box_w, box_h, z, TS_REQUESTED);
    }
}

int32_t Text_GetMaxLineLength(void)
{
    return 640 / (TEXT_HEIGHT * 0.75);
}
