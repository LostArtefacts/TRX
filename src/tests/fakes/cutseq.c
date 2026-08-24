#include <fakes/cutseq.h>

#include <trx/game/cutseq/playback.h>

static bool m_IsPlaying = false;

void FakeCutSeq_SetPlaying(const bool playing)
{
    m_IsPlaying = playing;
}

bool CutSeq_IsAvailable(void)
{
    return true;
}

int32_t CutSeq_GetCount(void)
{
    return 0;
}

bool CutSeq_IsPlaying(void)
{
    return m_IsPlaying;
}

int32_t CutSeq_GetCurrent(void)
{
    return -1;
}

int32_t CutSeq_GetFrame(void)
{
    return 0;
}

void CutSeq_Request(const int32_t num, const bool fade_out)
{
}

bool CutSeq_IsPlayed(const int32_t num)
{
    return false;
}

void CutSeq_SetPlayed(const int32_t num, const bool played)
{
}

void CutSeq_SetPlayedMask(const uint64_t mask)
{
}

int32_t CutSeq_GetActorCount(void)
{
    return 0;
}

void CutSeq_SetActorVisible(const int32_t actor, const bool visible)
{
}

void CutSeq_SetActorNodeMesh(
    const int32_t actor, const int32_t node, const OBJECT_ID object_id,
    const int32_t mesh_idx)
{
}

void CutSeq_SetLaraReturn(const XYZ_32 pos, const int16_t rot)
{
}

void CutSeq_SetLaraShadowBounds(const BOUNDS_16 bounds)
{
}

int32_t CutSeq_GetFOV(void)
{
    return 0;
}

void CutSeq_SetFOV(const int32_t fov)
{
}

float CutSeq_GetLetterbox(void)
{
    return 0.0f;
}

void CutSeq_SetLetterbox(const float ratio)
{
}
