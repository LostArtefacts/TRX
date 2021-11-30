#ifndef T1M_SPECIFIC_S_PICTURE_H
#define T1M_SPECIFIC_S_PICTURE_H

#include "global/types.h"

#include <stdbool.h>

bool S_Picture_LoadFromFile(PICTURE *target_pic, const char *file_path);
bool S_Picture_Scale(
    PICTURE *target_pic, const PICTURE *source_pic, int target_width,
    int target_height);

#endif
