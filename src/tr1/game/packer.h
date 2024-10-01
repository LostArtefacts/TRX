#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t level_page_count;
    int32_t source_page_count;
    int32_t object_count;
    int32_t sprite_count;
    RGBA_8888 *source_pages;
    RGBA_8888 *level_pages;
} PACKER_DATA;

// Attempts to pack the provided source pages into the level pages. Packing
// will begin on the last occupied page in the level and will continue until
// all textures have been successfully packed.
// Packing will fail if the area of any texture exceeds PAGE_SIZE, or if all
// positioning attempts are exhausted and MAX_TEXTPAGES has been reached.
bool Packer_Pack(PACKER_DATA *data);

// Returns the number of additional pages used in the packing process.
int32_t Packer_GetAddedPageCount(void);
