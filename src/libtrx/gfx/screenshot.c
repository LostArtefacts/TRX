#include "gfx/screenshot.h"

#include "engine/image.h"
#include "gfx/gl/utils.h"
#include "memory.h"

#include <assert.h>
#include <string.h>

bool GFX_Screenshot_CaptureToFile(const char *path)
{
    bool ret = false;

    GLint width;
    GLint height;
    GFX_Screenshot_CaptureToBuffer(
        NULL, &width, &height, 3, GL_RGB, GL_UNSIGNED_BYTE, true);

    IMAGE *image = Image_Create(width, height);
    assert(image);

    GFX_Screenshot_CaptureToBuffer(
        (uint8_t *)image->data, &width, &height, 3, GL_RGB, GL_UNSIGNED_BYTE,
        true);

    ret = Image_SaveToFile(image, path);

    if (image) {
        Image_Free(image);
    }
    return ret;
}

void GFX_Screenshot_CaptureToBuffer(
    uint8_t *out_buffer, GLint *out_width, GLint *out_height, GLint depth,
    GLenum format, GLenum type, bool vflip)
{
    assert(out_width);
    assert(out_height);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    GFX_GL_CheckError();

    GLint x = viewport[0];
    GLint y = viewport[1];
    *out_width = viewport[2];
    *out_height = viewport[3];

    if (!out_buffer) {
        return;
    }

    GLint pitch = *out_width * depth;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    GFX_GL_CheckError();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GFX_GL_CheckError();

    glReadBuffer(GL_BACK);
    GFX_GL_CheckError();
    glReadPixels(x, y, *out_width, *out_height, format, type, out_buffer);
    GFX_GL_CheckError();

    if (vflip) {
        uint8_t *scanline = Memory_Alloc(pitch);
        for (int y1 = 0, middle = *out_height / 2; y1 < middle; y1++) {
            int y2 = *out_height - 1 - y1;
            memcpy(scanline, &out_buffer[y1 * pitch], pitch);
            memcpy(&out_buffer[y1 * pitch], &out_buffer[y2 * pitch], pitch);
            memcpy(&out_buffer[y2 * pitch], scanline, pitch);
        }
        Memory_FreePointer(&scanline);
    }
}
