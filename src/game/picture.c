#include "game/picture.h"

#include "memory.h"
#include "specific/s_picture.h"

#include <assert.h>

PICTURE *Picture_Create()
{
    PICTURE *picture = Memory_Alloc(sizeof(picture));
    picture->width = 0;
    picture->height = 0;
    picture->data = NULL;
    return picture;
}

PICTURE *Picture_CreateFromFile(const char *file_path)
{
    PICTURE *picture = Picture_Create();
    if (!picture) {
        return NULL;
    }
    if (!S_Picture_LoadFromFile(picture, file_path)) {
        Memory_Free(picture);
        return NULL;
    }
    return picture;
}

bool Picture_Scale(
    PICTURE *target_pic, const PICTURE *source_pic, size_t target_width,
    size_t target_height)
{
    assert(target_pic);
    assert(source_pic);
    return S_Picture_Scale(target_pic, source_pic, target_width, target_height);
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
