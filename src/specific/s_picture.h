#ifndef T1M_SPECIFIC_S_PICTURE_H
#define T1M_SPECIFIC_S_PICTURE_H

#include "global/types.h"

#include <stdbool.h>

bool S_Picture_LoadFromFile(PICTURE *target_pic, const char *path);
bool S_Picture_SaveToFile(const PICTURE *pic, const char *path);

bool S_Picture_ScaleLetterbox(
    PICTURE *target_pic, const PICTURE *source_pic, int target_width,
    int target_height);
bool S_Picture_ScaleCrop(
    PICTURE *target_pic, const PICTURE *source_pic, int target_width,
    int target_height);
bool S_Picture_ScaleStretch(
    PICTURE *target_pic, const PICTURE *source_pic, int target_width,
    int target_height);

#endif
