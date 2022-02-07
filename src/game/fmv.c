#include "game/fmv.h"

#include "filesystem.h"
#include "global/const.h"
#include "memory.h"
#include "specific/s_fmv.h"

static const char *m_Extensions[] = {
    ".mp4", ".mkv", "mpeg", ".avi", ".webm", ".rpl", NULL,
};

bool FMV_Init()
{
    return S_FMV_Init();
}

bool FMV_Play(const char *file_path)
{
    bool ret = false;
    char *full_path = File_GetFullPath(file_path);
    char *final_path = NULL;

    File_GuessExtension(full_path, &final_path, m_Extensions);

    ret = S_FMV_Play(final_path);

    Memory_FreePointer(&final_path);
    Memory_FreePointer(&full_path);
    return ret;
}
