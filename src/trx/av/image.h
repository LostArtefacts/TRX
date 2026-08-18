#pragma once

#include <trx/core/result.h>

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} IMAGE_PIXEL;

typedef struct {
    int32_t width;
    int32_t height;
    IMAGE_PIXEL *data;
} IMAGE;

typedef enum {
    IMAGE_FIT_STRETCH,
    IMAGE_FIT_CROP,
    IMAGE_FIT_LETTERBOX,
    IMAGE_FIT_SMART,
} IMAGE_FIT_MODE;

IMAGE *Image_Create(int width, int height);

// Reads an image from disk, reporting a file that does not exist and one the
// reader cannot decode. Caller frees it with Image_Free().
RESULT Image_CreateFromFile(const char *path, IMAGE **out_image);

// As Image_CreateFromFile, but scales the image to the given size during the
// read.
RESULT Image_CreateFromFileInto(
    const char *path, int32_t target_width, int32_t target_height,
    IMAGE_FIT_MODE fit_mode, IMAGE **out_image);

void Image_Free(IMAGE *image);

bool Image_GetFileInfo(const char *path, int32_t *width, int32_t *height);

// Writes an image to disk, reporting a path it cannot write and a format with
// no encoder.
RESULT Image_SaveToFile(const IMAGE *image, const char *path);

IMAGE *Image_Scale(
    const IMAGE *source_image, size_t target_width, size_t target_height,
    IMAGE_FIT_MODE fit_mode);
