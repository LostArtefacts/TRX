#include "game/music.h"

#include "specific/music.h"

int32_t Music_Init()
{
    return S_Music_Init();
}

void Music_PlayLooped()
{
    S_Music_PlayLooped();
}

int32_t Music_Play(int16_t track)
{
    return S_Music_Play(track);
}

int32_t Music_Stop()
{
    return S_Music_Stop();
}

void Music_Loop()
{
    S_Music_Loop();
}

void Music_AdjustVolume(int16_t volume)
{
    S_Music_AdjustVolume(volume);
}

void Music_Pause()
{
    S_Music_Pause();
}

void Music_Unpause()
{
    S_Music_Unpause();
}
