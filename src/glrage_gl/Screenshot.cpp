#include "glrage_gl/Screenshot.hpp"

#include "glrage_util/ErrorUtils.hpp"
#include "glrage_util/StringUtils.hpp"

extern "C" {
#include "memory.h"
#include "game/picture.h"
}

#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace glrage {
namespace gl {

bool Screenshot::capture(const std::string &path)
{
    bool ret = false;

    GLint width;
    GLint height;
    std::vector<uint8_t> buffer;

    Screenshot::capture(
        buffer, width, height, 3, GL_RGB, GL_UNSIGNED_BYTE, true);

    PICTURE *pic = Picture_Create();
    if (!pic) {
        goto cleanup;
    }
    pic->width = width;
    pic->height = height;
    pic->data = static_cast<RGB888 *>(Memory_Alloc(width * height * 3));
    std::copy(
        buffer.begin(), buffer.end(), reinterpret_cast<uint8_t *>(pic->data));

    ret = Picture_SaveToFile(pic, path.c_str());

cleanup:
    if (pic) {
        Picture_Free(pic);
    }
    return ret;
}

void Screenshot::capture(
    std::vector<uint8_t> &buffer, GLint &width, GLint &height, GLint depth,
    GLenum format, GLenum type, bool vflip)
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

}
}
