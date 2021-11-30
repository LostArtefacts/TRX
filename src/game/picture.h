#ifndef T1M_GAME_PICTURE_H
#define T1M_GAME_PICTURE_H

#include "global/types.h"

#include <stdint.h>

PICTURE *Picture_Create();
PICTURE *Picture_CreateFromFile(const char *file_path);
void Picture_Free(PICTURE *picture);

bool Picture_Scale(
    PICTURE *target_pic, const PICTURE *source_pic, size_t target_width,
    size_t target_height);

#endif
