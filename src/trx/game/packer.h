#pragma once

#include <trx/game/output/types.h>

#include <stdint.h>

typedef struct {
    struct {
        int32_t page_count;
        RGBA_8888 *pages_32;
        uint8_t *pages_24;
        RGB_888 *palette_24;
    } source, level;
    int32_t object_count;
    int32_t sprite_count;
} PACKER_DATA;

// Attempts to pack the provided source pages into the level pages. Packing
// will begin on the last occupied page in the level and will continue until
// all textures have been successfully packed.
// Packing will fail if the area of any texture exceeds TEXTURE_PAGE_SIZE, or if
// all positioning attempts are exhausted and MAX_TEXTURE_PAGES has been
// reached.
bool Packer_Pack(PACKER_DATA *data);

// Returns the number of additional pages used in the packing process.
int32_t Packer_GetAddedPageCount(void);
