#include "game/anims/common.h"

#include "game/game_buf.h"

static int32_t m_AnimCount = 0;
static ANIM *m_Anims = nullptr;
static ANIM_CHANGE *m_Changes = nullptr;
static ANIM_RANGE *m_Ranges = nullptr;
static ANIM_BONE *m_Bones = nullptr;
static ANIM m_NullAnim = {};

void Anim_InitialiseAnims(const int32_t num_anims)
{
    m_AnimCount = num_anims;
    m_Anims = GameBuf_Alloc(sizeof(ANIM) * num_anims, GBUF_ANIMS);
}

void Anim_InitialiseChanges(const int32_t num_changes)
{
    m_Changes =
        GameBuf_Alloc(sizeof(ANIM_CHANGE) * num_changes, GBUF_ANIM_CHANGES);
}

void Anim_InitialiseRanges(const int32_t num_ranges)
{
    m_Ranges = GameBuf_Alloc(sizeof(ANIM_RANGE) * num_ranges, GBUF_ANIM_RANGES);
}

void Anim_InitialiseBones(const int32_t num_bones)
{
    m_Bones = GameBuf_Alloc(sizeof(ANIM_BONE) * num_bones, GBUF_ANIM_BONES);
}

int32_t Anim_GetTotalCount(void)
{
    return m_AnimCount;
}

ANIM *Anim_GetAnim(const int32_t anim_idx)
{
    return anim_idx == NO_ANIM ? &m_NullAnim : &m_Anims[anim_idx];
}

ANIM_CHANGE *Anim_GetChange(const int32_t change_idx)
{
    return &m_Changes[change_idx];
}

ANIM_RANGE *Anim_GetRange(const int32_t range_idx)
{
    return &m_Ranges[range_idx];
}

ANIM_BONE *Anim_GetBone(const int32_t bone_idx)
{
    return &m_Bones[bone_idx];
}

bool Anim_TestAbsFrameEqual(const int16_t abs_frame, const int16_t frame)
{
    return abs_frame == frame;
}

bool Anim_TestAbsFrameRange(
    const int16_t abs_frame, const int16_t start, const int16_t end)
{
    return abs_frame >= start && abs_frame <= end;
}
