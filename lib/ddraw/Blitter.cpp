#include "Blitter.hpp"

#include <algorithm>

namespace glrage {
namespace ddraw {

void Blitter::blit(Image& srcImg, Rect& srcRect, Image dstImg, Rect& dstRect)
{
    // do fast direct copy if possible
    if (srcImg == dstImg && srcRect == dstRect) {
        dstImg.buffer = srcImg.buffer;
        return;
    }

    int32_t srcRectWidth = srcRect.width();
    int32_t srcRectHeight = srcRect.height();

    int32_t dstRectWidth = dstRect.width();
    int32_t dstRectHeight = dstRect.height();

    int32_t xRatio = ((srcRectWidth << m_ratioBias) / dstRectWidth) + 1;
    int32_t yRatio = ((srcRectHeight << m_ratioBias) / dstRectHeight) + 1;

    bool x1Flip = dstRect.left > dstRect.right;
    bool x2Flip = srcRect.left > srcRect.right;

    bool y1Flip = dstRect.top > dstRect.bottom;
    bool y2Flip = srcRect.top > srcRect.bottom;

    for (int32_t y = 0; y < dstRectHeight; y++) {
        int32_t y1 = y;
        int32_t y2 = (y * yRatio) >> m_ratioBias;

        if (y1Flip) {
            y1 = dstRect.top - y1 - 1;
        } else {
            y1 += dstRect.top;
        }

        if (y2Flip) {
            y2 = srcRect.top - y2 - 1;
        } else {
            y2 += srcRect.top;
        }

        for (int32_t x = 0; x < dstRectWidth; x++) {
            int32_t x1 = x;
            int32_t x2 = (x * xRatio) >> m_ratioBias;

            if (x1Flip) {
                x1 = dstRect.left - x1 - 1;
            } else {
                x1 += dstRect.left;
            }

            if (x2Flip) {
                x2 = srcRect.left - x2 - 1;
            } else {
                x2 += srcRect.left;
            }

            for (int32_t n = 0; n < dstImg.depth; n++) {
                dstImg(x1, y1, n) = srcImg(x2, y2, n);
            }
        }
    }
}

} // namespace ddraw
} // namespace glrage
