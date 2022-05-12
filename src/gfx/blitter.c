#include "gfx/blitter.h"

#include "util.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

static const int32_t m_RatioBias = 16;

static int32_t GFX_BlitterRect_GetWidth(const GFX_BlitterRect *rect);
static int32_t GFX_BlitterRect_GetHeight(const GFX_BlitterRect *rect);
static bool GFX_BlitterRect_Equals(
    const GFX_BlitterRect *rect, const GFX_BlitterRect *other);

static int32_t GFX_BlitterRect_GetWidth(const GFX_BlitterRect *rect)
{
    assert(rect);
    return ABS(rect->right - rect->left);
}

static int32_t GFX_BlitterRect_GetHeight(const GFX_BlitterRect *rect)
{
    assert(rect);
    return ABS(rect->bottom - rect->top);
}

static bool GFX_BlitterRect_Equals(
    const GFX_BlitterRect *rect, const GFX_BlitterRect *other)
{
    assert(rect);
    assert(other);
    return rect->left == other->left && rect->top == other->top
        && rect->right == other->right && rect->bottom == other->bottom;
}

void GFX_Blit(
    const GFX_BlitterImage *src_img, const GFX_BlitterRect *src_rect,
    GFX_BlitterImage *dst_img, const GFX_BlitterRect *dst_rect)
{
    assert(src_img);
    assert(src_rect);
    assert(dst_img);
    assert(dst_rect);

    // do fast direct copy if possible
    if (src_img->width == dst_img->width && src_img->height == dst_img->height
        && src_img->depth == dst_img->depth
        && GFX_BlitterRect_Equals(src_rect, dst_rect)) {
        memcpy(
            dst_img->buffer, src_img->buffer,
            src_img->width * src_img->height * src_img->depth);
        dst_img->buffer = src_img->buffer;
        return;
    }

    int32_t src_rect_width = GFX_BlitterRect_GetWidth(src_rect);
    int32_t src_rect_height = GFX_BlitterRect_GetHeight(src_rect);
    int32_t dst_rect_width = GFX_BlitterRect_GetWidth(dst_rect);
    int32_t dst_rect_height = GFX_BlitterRect_GetHeight(dst_rect);

    int32_t x_ratio = ((src_rect_width << m_RatioBias) / dst_rect_width) + 1;
    int32_t y_ratio = ((src_rect_height << m_RatioBias) / dst_rect_height) + 1;

    bool x1flip = dst_rect->left > dst_rect->right;
    bool x2flip = src_rect->left > src_rect->right;
    bool y1flip = dst_rect->top > dst_rect->bottom;
    bool y2flip = src_rect->top > src_rect->bottom;

    for (int32_t y = 0; y < dst_rect_height; y++) {
        int32_t y1 = y;
        if (y1flip) {
            y1 = dst_rect->top - y1 - 1;
        } else {
            y1 += dst_rect->top;
        }

        int32_t y2 = (y * y_ratio) >> m_RatioBias;
        if (y2flip) {
            y2 = src_rect->top - y2 - 1;
        } else {
            y2 += src_rect->top;
        }

        for (int32_t x = 0; x < dst_rect_width; x++) {
            int32_t x1 = x;
            if (x1flip) {
                x1 = dst_rect->left - x1 - 1;
            } else {
                x1 += dst_rect->left;
            }

            int32_t x2 = (x * x_ratio) >> m_RatioBias;
            if (x2flip) {
                x2 = src_rect->left - x2 - 1;
            } else {
                x2 += src_rect->left;
            }

            for (int32_t n = 0; n < dst_img->depth; n++) {
                dst_img
                    ->buffer[(y1 * dst_img->width + x1) * dst_img->depth + n] =
                    src_img->buffer
                        [(y2 * src_img->width + x2) * src_img->depth + n];
            }
        }
    }
}
