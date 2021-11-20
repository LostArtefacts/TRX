#include "game/fmv.h"

#include "specific/s_fmv.h"

bool FMV_Init()
{
    return S_FMV_Init();
}

void FMV_Play(const char *file_path)
{
    S_FMV_Play(file_path);
}
