#include "game/fmv.h"

#include "specific/s_fmv.h"

void FMVInit()
{
    S_FMV_Init();
}

int32_t WinPlayFMV(int32_t sequence, int32_t mode)
{
    return S_FMV_WinPlayFMV(sequence, mode);
}

int32_t S_PlayFMV(int32_t sequence, int32_t mode)
{
    return S_FMV_PlayFMV(sequence, mode);
}
