#pragma once

#include "../output/types.h"

typedef struct {
    struct {
        int32_t anim_count;
        int32_t change_count;
        int32_t range_count;
        int32_t command_count;
        int32_t bone_count;
        int32_t frame_count;
        int16_t *frames;
    } anims;

    struct {
        int32_t object_count;
        int32_t sprite_count;
        int32_t page_count;
        uint8_t *pages_24;
        RGBA_8888 *pages_32;
    } textures;

    struct {
        int32_t size;
        RGB_888 *data_24;
        RGB_888 *data_32;
    } palette;

    struct {
        int32_t info_count;
        int32_t offset_count;
        int32_t *offsets;
#if TR_VERSION == 1
        int32_t data_size;
        char *data;
#endif
    } samples;

    int32_t mesh_ptr_count;
} LEVEL_INFO;
