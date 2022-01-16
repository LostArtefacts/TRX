#pragma once

#include "global/types.h"

#include <stdint.h>

PICTURE *Picture_Create(int width, int height);
PICTURE *Picture_CreateFromFile(const char *path);
void Picture_Free(PICTURE *picture);

bool Picture_SaveToFile(const PICTURE *pic, const char *path);

PICTURE *Picture_ScaleFit(
    const PICTURE *source_pic, size_t target_width, size_t target_height);
PICTURE *Picture_ScaleCover(
    const PICTURE *source_pic, size_t target_width, size_t target_height);
PICTURE *Picture_ScaleSmart(
    const PICTURE *source_pic, size_t target_width, size_t target_height);
