#include "game/fmv.h"

#include "game/music.h"
#include "game/sound.h"
#include "shared/filesystem.h"
#include "shared/memory.h"
#include "specific/s_fmv.h"

#include <stddef.h>

static const char *m_Extensions[] = {
    ".mp4", ".mkv", ".mpeg", ".avi", ".webm", ".rpl", NULL,
};

bool FMV_Init(void)
{
    return S_FMV_Init();
}

bool FMV_Play(const char *path)
{
    Music_Stop();
    Sound_StopAllSamples();

    char *final_path = File_GuessExtension(path, m_Extensions);
    bool ret = S_FMV_Play(final_path);
    Memory_FreePointer(&final_path);
    return ret;
}
