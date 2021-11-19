#ifndef T1M_GAME_PICTURE_H
#define T1M_GAME_PICTURE_H

#include "global/types.h"

#include <stdint.h>

PICTURE *Picture_Create(const char *file_path);
void Picture_Free(PICTURE *picture);

#endif
