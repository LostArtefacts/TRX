#pragma once

#include "global/types.h"

#include <stdbool.h>

PICTURE *S_Picture_CreateFromFile(const char *path);
bool S_Picture_SaveToFile(const PICTURE *pic, const char *path);

PICTURE *S_Picture_ScaleLetterbox(
    const PICTURE *source_pic, int target_width, int target_height);
PICTURE *S_Picture_ScaleCrop(
    const PICTURE *source_pic, int target_width, int target_height);
PICTURE *S_Picture_ScaleStretch(
    const PICTURE *source_pic, int target_width, int target_height);
