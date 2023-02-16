#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct INJECTION_INFO {
    int32_t texture_page_count;
    int32_t texture_count;
    int32_t sprite_info_count;
    int32_t sprite_count;
    int32_t mesh_count;
    int32_t mesh_ptr_count;
    int32_t anim_change_count;
    int32_t anim_range_count;
    int32_t anim_cmd_count;
    int32_t anim_bone_count;
    int32_t anim_frame_count;
    int32_t anim_count;
    int32_t object_count;
    int32_t sfx_count;
    int32_t sfx_data_size;
    int32_t sample_count;
    int32_t mesh_edit_count;
    int32_t texture_overwrite_count;
} INJECTION_INFO;

bool Inject_Init(
    int injection_count, char *filenames[], INJECTION_INFO *aggregate);
bool Inject_AllInjections(LEVEL_INFO *level_info);
