#include "game/picture.h"

#include "filesystem.h"
#include "memory.h"
#include "specific/s_picture.h"

#include <assert.h>

static const char *m_Extensions[] = {
    ".png", ".jpg", ".jpeg", ".pcx", NULL,
};

PICTURE *Picture_Create()
{
    PICTURE *picture = Memory_Alloc(sizeof(picture));
    picture->width = 0;
    picture->height = 0;
    picture->data = NULL;
    return picture;
}

PICTURE *Picture_CreateFromFile(const char *path)
{
    bool result = false;
    char *full_path = NULL;
    char *final_path = NULL;

    PICTURE *picture = Picture_Create();
    if (!picture) {
        goto cleanup;
    }

    File_GetFullPath(path, &full_path);
    File_GuessExtension(full_path, &final_path, m_Extensions);

    result = S_Picture_LoadFromFile(picture, final_path);

cleanup:
    if (!result && picture) {
        Memory_Free(picture);
        picture = NULL;
    }

    if (final_path) {
        Memory_Free(final_path);
        final_path = NULL;
    }

    if (full_path) {
        Memory_Free(full_path);
        full_path = NULL;
    }

    return picture;
}

bool Picture_SaveToFile(const PICTURE *pic, const char *path)
{
    assert(pic);
    assert(path);

    char *full_path = NULL;
    File_GetFullPath(path, &full_path);

    bool ret = S_Picture_SaveToFile(pic, full_path);

    if (full_path) {
        Memory_Free(full_path);
        full_path = NULL;
    }

    return ret;
}

bool Picture_ScaleLetterbox(
    PICTURE *target_pic, const PICTURE *source_pic, size_t target_width,
    size_t target_height)
{
    assert(target_pic);
    assert(source_pic);
    return S_Picture_ScaleLetterbox(
        target_pic, source_pic, target_width, target_height);
}

bool Picture_ScaleCrop(
    PICTURE *target_pic, const PICTURE *source_pic, size_t target_width,
    size_t target_height)
{
    assert(target_pic);
    assert(source_pic);
    return S_Picture_ScaleCrop(
        target_pic, source_pic, target_width, target_height);
}

bool Picture_ScaleSmart(
    PICTURE *target_pic, const PICTURE *source_pic, size_t target_width,
    size_t target_height)
{
    assert(target_pic);
    assert(source_pic);
    const float source_ratio = source_pic->width / (float)source_pic->height;
    const float target_ratio = target_width / (float)target_height;

    // if the difference between aspect ratios is under 10%, just stretch it
    const float ar_diff =
        (source_ratio > target_ratio ? source_ratio / target_ratio
                                     : target_ratio / source_ratio)
        - 1.0f;
    if (ar_diff <= 0.1f) {
        return S_Picture_ScaleStretch(
            target_pic, source_pic, target_width, target_height);
    }

    // if the viewport is too wide, center the image
    if (source_ratio <= target_ratio) {
        return S_Picture_ScaleLetterbox(
            target_pic, source_pic, target_width, target_height);
    }

    // if the image is too wide, crop the image
    return S_Picture_ScaleCrop(
        target_pic, source_pic, target_width, target_height);
}

void Picture_Free(PICTURE *picture)
{
    if (picture) {
        if (picture->data) {
            Memory_Free(picture->data);
        }
        Memory_Free(picture);
    }
}
