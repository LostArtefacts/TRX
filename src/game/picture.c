#include "game/picture.h"

#include "filesystem.h"
#include "memory.h"
#include "specific/s_picture.h"

#include <assert.h>

static const char *m_Extensions[] = {
    ".png", ".jpg", ".jpeg", ".pcx", NULL,
};

PICTURE *Picture_Create(int width, int height)
{
    PICTURE *picture = Memory_Alloc(sizeof(PICTURE));
    picture->width = width;
    picture->height = height;
    picture->data = Memory_Alloc(width * height * sizeof(RGB888));
    return picture;
}

PICTURE *Picture_CreateFromFile(const char *path)
{
    char *full_path = File_GetFullPath(path);
    char *final_path = File_GuessExtension(full_path, m_Extensions);

    PICTURE *picture = S_Picture_CreateFromFile(final_path);
    Memory_FreePointer(&final_path);
    Memory_FreePointer(&full_path);
    return picture;
}

bool Picture_SaveToFile(const PICTURE *pic, const char *path)
{
    assert(pic);
    assert(path);

    char *full_path = File_GetFullPath(path);

    bool ret = S_Picture_SaveToFile(pic, full_path);

    Memory_FreePointer(&full_path);
    return ret;
}

PICTURE *Picture_ScaleLetterbox(
    const PICTURE *source_pic, size_t target_width, size_t target_height)
{
    assert(source_pic);
    return S_Picture_ScaleLetterbox(source_pic, target_width, target_height);
}

PICTURE *Picture_ScaleCrop(
    const PICTURE *source_pic, size_t target_width, size_t target_height)
{
    assert(source_pic);
    return S_Picture_ScaleCrop(source_pic, target_width, target_height);
}

PICTURE *Picture_ScaleSmart(
    const PICTURE *source_pic, size_t target_width, size_t target_height)
{
    assert(source_pic);
    const float source_ratio = source_pic->width / (float)source_pic->height;
    const float target_ratio = target_width / (float)target_height;

    // if the difference between aspect ratios is under 10%, just stretch it
    const float ar_diff =
        (source_ratio > target_ratio ? source_ratio / target_ratio
                                     : target_ratio / source_ratio)
        - 1.0f;
    if (ar_diff <= 0.1f) {
        return S_Picture_ScaleStretch(source_pic, target_width, target_height);
    }

    // if the viewport is too wide, center the image
    if (source_ratio <= target_ratio) {
        return S_Picture_ScaleLetterbox(
            source_pic, target_width, target_height);
    }

    // if the image is too wide, crop the image
    return S_Picture_ScaleCrop(source_pic, target_width, target_height);
}

void Picture_Free(PICTURE *picture)
{
    if (picture) {
        Memory_FreePointer(&picture->data);
    }
    Memory_FreePointer(&picture);
}
