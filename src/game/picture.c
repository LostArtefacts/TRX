#include "game/picture.h"

#include "memory.h"
#include "specific/s_picture.h"

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

void Picture_Free(PICTURE *picture)
{
    if (picture) {
        if (picture->data) {
            Memory_Free(picture->data);
        }
        Memory_Free(picture);
    }
}
