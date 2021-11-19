#include "game/picture.h"

#include "memory.h"
#include "specific/s_picture.h"

PICTURE *Picture_Create(const char *file_path)
{
    PICTURE *picture = Memory_Alloc(sizeof(picture));
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
