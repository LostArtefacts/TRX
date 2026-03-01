#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include "vendor/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "vendor/stb_image_resize2.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb_image_write.h"

#include <trx/av/image.h>

#include <trx/debug.h>
#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>

#include "vendor/webp/decode.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The native image.c uses M_GetBlit + libswscale for fit modes.
// We replicate the blit logic here and use stb_image_resize2 instead.

typedef struct {
    struct {
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
    } src, dst;
} IMAGE_BLIT;

static IMAGE_BLIT M_GetBlit(
    const int32_t source_width, const int32_t source_height,
    const int32_t target_width, const int32_t target_height,
    IMAGE_FIT_MODE fit_mode)
{
    const float source_ratio = source_width / (float)source_height;
    const float target_ratio = target_width / (float)target_height;

    if (fit_mode == IMAGE_FIT_SMART) {
        const float ar_diff =
            (source_ratio > target_ratio ? source_ratio / target_ratio
                                         : target_ratio / source_ratio)
            - 1.0f;
        if (ar_diff <= 0.1f) {
            fit_mode = IMAGE_FIT_STRETCH;
        } else if (source_ratio <= target_ratio) {
            fit_mode = IMAGE_FIT_LETTERBOX;
        } else {
            fit_mode = IMAGE_FIT_CROP;
        }
    }

    IMAGE_BLIT blit;

    switch (fit_mode) {
    case IMAGE_FIT_STRETCH:
        blit.src.width = source_width;
        blit.src.height = source_height;
        blit.src.x = 0;
        blit.src.y = 0;

        blit.dst.width = target_width;
        blit.dst.height = target_height;
        blit.dst.x = 0;
        blit.dst.y = 0;
        break;

    case IMAGE_FIT_CROP:
        blit.src.width = source_ratio < target_ratio
            ? source_width
            : source_height * target_ratio;
        blit.src.height = source_ratio < target_ratio
            ? source_width / target_ratio
            : source_height;
        blit.src.x = (source_width - blit.src.width) / 2;
        blit.src.y = (source_height - blit.src.height) / 2;

        blit.dst.width = target_width;
        blit.dst.height = target_height;
        blit.dst.x = 0;
        blit.dst.y = 0;
        break;

    case IMAGE_FIT_LETTERBOX:
        blit.src.width = source_width;
        blit.src.height = source_height;
        blit.src.x = 0;
        blit.src.y = 0;

        blit.dst.width = (source_ratio > target_ratio)
            ? target_width
            : target_height * source_ratio;
        blit.dst.height = (source_ratio > target_ratio)
            ? target_width / source_ratio
            : target_height;
        blit.dst.x = (target_width - blit.dst.width) / 2;
        blit.dst.y = (target_height - blit.dst.height) / 2;
        break;

    default:
        ASSERT_FAIL();
        break;
    }
    return blit;
}

// Read an entire file into a malloc'd buffer.  Returns nullptr on failure.
static unsigned char *M_ReadFileRaw(const char *full_path, size_t *out_size)
{
    FILE *f = fopen(full_path, "rb");
    if (f == nullptr) {
        return nullptr;
    }
    fseek(f, 0, SEEK_END);
    const long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0) {
        fclose(f);
        return nullptr;
    }
    unsigned char *buf = malloc((size_t)len);
    if (buf == nullptr) {
        fclose(f);
        return nullptr;
    }
    const size_t read = fread(buf, 1, (size_t)len, f);
    fclose(f);
    if (read != (size_t)len) {
        free(buf);
        return nullptr;
    }
    *out_size = (size_t)len;
    return buf;
}

// Try to decode a file as WebP.  Returns malloc'd RGB24 data or nullptr.
static unsigned char *M_LoadWebP(const char *full_path, int *w, int *h)
{
    size_t file_size = 0;
    unsigned char *file_data = M_ReadFileRaw(full_path, &file_size);
    if (file_data == nullptr) {
        return nullptr;
    }

    int webp_w, webp_h;
    if (!WebPGetInfo(file_data, file_size, &webp_w, &webp_h)) {
        free(file_data);
        return nullptr;
    }

    // Decode to RGB24 — WebPDecodeRGB allocates the output buffer via malloc.
    unsigned char *pixels =
        WebPDecodeRGB(file_data, file_size, &webp_w, &webp_h);
    free(file_data);

    if (pixels == nullptr) {
        return nullptr;
    }

    *w = webp_w;
    *h = webp_h;
    return pixels;
}

// Load an image file, always returning 3-channel RGB to match IMAGE_PIXEL
// layout.  Tries stb_image first (PNG/JPEG/BMP/etc.), then falls back to
// libwebp for WebP files.
// Returns pixel data (caller must free() it), and populates w/h.
static unsigned char *M_LoadFile(const char *path, int *w, int *h)
{
    // On Emscripten, MEMFS paths work as-is; no resolution needed.
    char *full_path = Memory_DupStr(path);
    if (full_path == nullptr) {
        LOG_ERROR("Cannot resolve image path: %s", path);
        return nullptr;
    }

    // Try stb_image first (handles PNG, JPEG, BMP, GIF, TGA, PSD, HDR, PIC).
    int channels;
    unsigned char *pixels = stbi_load(full_path, w, h, &channels, 3);
    if (pixels != nullptr) {
        Memory_FreePointer(&full_path);
        return pixels;
    }

    // stb_image failed — try libwebp (handles WebP lossy + lossless).
    pixels = M_LoadWebP(full_path, w, h);
    Memory_FreePointer(&full_path);

    if (pixels == nullptr) {
        LOG_ERROR("Failed to load image %s", path);
    }
    return pixels;
}

IMAGE *Image_Create(const int width, const int height)
{
    IMAGE *image = Memory_Alloc(sizeof(IMAGE));
    image->width = width;
    image->height = height;
    image->data = Memory_Alloc(width * height * sizeof(IMAGE_PIXEL));
    memset(image->data, 0, width * height * sizeof(IMAGE_PIXEL));
    return image;
}

IMAGE *Image_CreateFromFile(const char *const path)
{
    ASSERT(path != nullptr);

    int w, h;
    unsigned char *pixels = M_LoadFile(path, &w, &h);
    if (pixels == nullptr) {
        return nullptr;
    }

    IMAGE *image = Image_Create(w, h);
    memcpy(image->data, pixels, w * h * sizeof(IMAGE_PIXEL));
    free(pixels);
    return image;
}

IMAGE *Image_CreateFromFileInto(
    const char *const path, const int32_t target_width,
    const int32_t target_height, const IMAGE_FIT_MODE fit_mode)
{
    ASSERT(path != nullptr);

    int w, h;
    unsigned char *pixels = M_LoadFile(path, &w, &h);
    if (pixels == nullptr) {
        return nullptr;
    }

    IMAGE_BLIT blit = M_GetBlit(w, h, target_width, target_height, fit_mode);

    // Crop the source region if needed.
    const unsigned char *src_pixels =
        pixels + (blit.src.y * w + blit.src.x) * 3;
    const int src_stride = w * 3;

    IMAGE *const target_image = Image_Create(target_width, target_height);

    unsigned char *dst_pixels = (unsigned char *)target_image->data
        + (blit.dst.y * target_width + blit.dst.x) * 3;
    const int dst_stride = target_width * 3;

    stbir_resize_uint8_linear(
        src_pixels, blit.src.width, blit.src.height, src_stride, dst_pixels,
        blit.dst.width, blit.dst.height, dst_stride, STBIR_RGB);

    free(pixels);
    return target_image;
}

bool Image_GetFileInfo(const char *path, int32_t *width, int32_t *height)
{
    // On Emscripten, MEMFS paths work as-is; no resolution needed.
    char *full_path = Memory_DupStr(path);
    if (full_path == nullptr) {
        if (width) {
            *width = 0;
        }
        if (height) {
            *height = 0;
        }
        return false;
    }

    // Try stb_image first.
    int w, h, channels;
    int ok = stbi_info(full_path, &w, &h, &channels);

    // Fall back to libwebp for WebP files.
    if (!ok) {
        size_t file_size = 0;
        unsigned char *file_data = M_ReadFileRaw(full_path, &file_size);
        if (file_data != nullptr) {
            ok = WebPGetInfo(file_data, file_size, &w, &h);
            free(file_data);
        }
    }

    Memory_FreePointer(&full_path);

    if (!ok) {
        if (width) {
            *width = 0;
        }
        if (height) {
            *height = 0;
        }
        return false;
    }

    if (width) {
        *width = w;
    }
    if (height) {
        *height = h;
    }
    return true;
}

bool Image_SaveToFile(const IMAGE *const image, const char *const path)
{
    ASSERT(image != nullptr);
    ASSERT(path != nullptr);

    // On Emscripten, MEMFS paths work as-is; no resolution needed.
    char *full_path = Memory_DupStr(path);
    if (full_path == nullptr) {
        full_path = Memory_DupStr(path);
    }

    int result = 0;
    if (strstr(path, ".png")) {
        result = stbi_write_png(
            full_path, image->width, image->height, 3, image->data,
            image->width * 3);
    } else if (strstr(path, ".jpg") || strstr(path, ".jpeg")) {
        result = stbi_write_jpg(
            full_path, image->width, image->height, 3, image->data, 90);
    } else {
        LOG_ERROR("Cannot determine image format for path '%s'", path);
    }

    Memory_FreePointer(&full_path);

    if (!result) {
        LOG_ERROR("Failed to save image: %s", path);
        return false;
    }
    return true;
}

IMAGE *Image_Scale(
    const IMAGE *const source_image, size_t target_width, size_t target_height,
    IMAGE_FIT_MODE fit_mode)
{
    ASSERT(source_image != nullptr);
    ASSERT(source_image->data != nullptr);
    ASSERT(target_width > 0);
    ASSERT(target_height > 0);

    IMAGE_BLIT blit = M_GetBlit(
        source_image->width, source_image->height, target_width, target_height,
        fit_mode);

    IMAGE *const target_image = Image_Create(target_width, target_height);

    const unsigned char *src_pixels = (const unsigned char *)source_image->data
        + (blit.src.y * source_image->width + blit.src.x) * 3;
    const int src_stride = source_image->width * 3;

    unsigned char *dst_pixels = (unsigned char *)target_image->data
        + (blit.dst.y * (int32_t)target_width + blit.dst.x) * 3;
    const int dst_stride = (int32_t)target_width * 3;

    stbir_resize_uint8_linear(
        src_pixels, blit.src.width, blit.src.height, src_stride, dst_pixels,
        blit.dst.width, blit.dst.height, dst_stride, STBIR_RGB);

    return target_image;
}

void Image_Free(IMAGE *image)
{
    if (image) {
        Memory_FreePointer(&image->data);
    }
    Memory_FreePointer(&image);
}
