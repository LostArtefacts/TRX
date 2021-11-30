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

PICTURE *Picture_CreateFromFile(const char *file_path)
{
    bool result = false;
    char *full_path = NULL;
    char *final_path = NULL;

    PICTURE *picture = Picture_Create();
    if (!picture) {
        goto cleanup;
    }

    File_GetFullPath(file_path, &full_path);
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
