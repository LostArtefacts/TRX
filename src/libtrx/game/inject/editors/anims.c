#include "game/inject.h"
#include "game/objects/common.h"

#include <string.h>

static void M_FrameEdits(const INJECTION *injection, int32_t data_count);

static void M_FrameEdits(
    const INJECTION *const injection, const int32_t data_count)
{
    LEVEL_INFO *const level_info = Level_GetInfo();
    for (int32_t i = 0; i < data_count; i++) {
        const GAME_OBJECT_ID obj_id = VFile_ReadS32(injection->fp);
        const int32_t anim_idx = VFile_ReadS32(injection->fp);
        const int32_t packed_rot = VFile_ReadS32(injection->fp);

        const OBJECT *const obj = Object_Get(obj_id);
        if (!obj->loaded) {
            continue;
        }

        const ANIM *const anim = Object_GetAnim(obj, anim_idx);
        int16_t *data_ptr =
            &level_info->anims.frames[anim->frame_ofs / sizeof(int16_t)];
        data_ptr += 10;
        memcpy(data_ptr, &packed_rot, sizeof(int32_t));
    }
}

REGISTER_INJECT_EDITOR(IDT_FRAME_EDITS, M_FrameEdits)
