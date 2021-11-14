#include "Screenshot.hpp"

#include <glrage_util/ErrorUtils.hpp>
#include <glrage_util/StringUtils.hpp>

#include <cstdint>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

namespace glrage {
namespace gl {

// heavily simplified Targa header struct for raw BGR(A) data
struct TGAHeader
{
    uint8_t blank1[2];
    uint8_t format;
    uint8_t blank2[9];
    uint16_t width;
    uint16_t height;
    uint8_t depth;
    uint8_t blank3;
};

void Screenshot::capture(const std::string& path)
{
    // open screenshot file
    std::ofstream file(path, std::ofstream::binary);
    if (!file.good()) {
        throw std::runtime_error("Can't open screenshot file '" +
                                 path + "': " +
                                 ErrorUtils::getSystemErrorString());
    }

    // copy framebuffer to local buffer
    GLint width;
    GLint height;
    GLint depth = 3;

    std::vector<uint8_t> buffer;
    capture(buffer, width, height, depth, GL_BGR, GL_UNSIGNED_BYTE, false);

    // create Targa header
    TGAHeader tgaHeader = {{0, 0}, 0, {0,0,0,0,0,0,0,0,0}, 0, 0, 0, 0};
    tgaHeader.format = 2;
    tgaHeader.width = width;
    tgaHeader.height = height;
    tgaHeader.depth = depth * 8;

    file.write(reinterpret_cast<char*>(&tgaHeader), sizeof(TGAHeader));
    file.write(reinterpret_cast<char*>(&buffer[0]), buffer.size());
    file.close();
}

void Screenshot::capture(std::vector<uint8_t>& buffer, GLint& width,
    GLint& height, GLint depth, GLenum format, GLenum type, bool vflip)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLint x = viewport[0];
    GLint y = viewport[1];
    width = viewport[2];
    height = viewport[3];

    GLint pitch = width * depth;
    buffer.resize(pitch * height);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(x, y, width, height, format, type, &buffer[0]);

    if (vflip) {
        for (GLint i = 0, middle = height / 2; i < middle; i++) {
            auto first1 = std::next(buffer.begin(), i * pitch);
            auto last1 = std::next(buffer.begin(), (i + 1) * pitch);
            auto first2 = std::prev(buffer.end(), (i + 1) * pitch);
            std::swap_ranges(first1, last1, first2);
        }
    }
}

} // namespace gl
} // namespace glrage
