#include "game/sound.h"

#include "engine/audio.h"
#include "game/game_buf.h"

static int32_t m_SourceCount = 0;
static OBJECT_VECTOR *m_Sources = nullptr;

void Sound_InitialiseSources(const int32_t num_sources)
{
    m_SourceCount = num_sources;
    m_Sources = num_sources == 0
        ? nullptr
        : GameBuf_Alloc(
              num_sources * sizeof(OBJECT_VECTOR), GBUF_SOUND_SOURCES);
}

int32_t Sound_GetSourceCount(void)
{
    return m_SourceCount;
}

OBJECT_VECTOR *Sound_GetSource(const int32_t source_idx)
{
    if (m_Sources == nullptr) {
        return nullptr;
    }
    return &m_Sources[source_idx];
}

void Sound_PauseAll(void)
{
    Audio_Sample_PauseAll();
}

void Sound_UnpauseAll(void)
{
    Audio_Sample_UnpauseAll();
}
