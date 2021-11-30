#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace glrage {
namespace ddraw {

class Blitter {
public:
    struct Rect {
        int32_t left, top, right, bottom;

        int32_t width()
        {
            return std::abs(right - left);
        }

        int32_t height()
        {
            return std::abs(bottom - top);
        }

        bool operator==(const Rect &r)
        {
            return left == r.left && top == r.top && right == r.right
                && bottom == r.bottom;
        }
    };

    struct Image {
        int32_t width;
        int32_t height;
        int32_t depth;
        std::vector<uint8_t> &buffer;

        uint8_t &operator()(int32_t x, int32_t y, int32_t z)
        {
            return buffer[(y * width + x) * depth + z];
        }

        bool operator==(const Image &i)
        {
            return width == i.width && height == i.height && depth == i.depth;
        }
    };

    static void blit(Image &srcImg, Rect &srcRect, Image dstImg, Rect &dstRect);

private:
    static const int32_t m_ratioBias = 16;
};

}
}
