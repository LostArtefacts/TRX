#ifndef T1M_GAME_PICTURE_H
#define T1M_GAME_PICTURE_H

#include "global/types.h"

#include <stdint.h>

PICTURE *Picture_Create();
PICTURE *Picture_CreateFromFile(const char *path);
void Picture_Free(PICTURE *picture);

bool Picture_SaveToFile(const PICTURE *pic, const char *path);

bool Picture_ScaleFit(
    PICTURE *target_pic, const PICTURE *source_pic, size_t target_width,
    size_t target_height);
bool Picture_ScaleCover(
    PICTURE *target_pic, const PICTURE *source_pic, size_t target_width,
    size_t target_height);
bool Picture_ScaleSmart(
    PICTURE *target_pic, const PICTURE *source_pic, size_t target_width,
    size_t target_height);

#endif
