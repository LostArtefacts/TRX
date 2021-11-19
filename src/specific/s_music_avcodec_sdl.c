#include "specific/s_music.h"

#include "log.h"
#include "memory.h"
#include "specific/s_audio.h"

#include <stdio.h>

static int m_MusicAudioStreamID = -1;
static float m_MusicVolume = 0.0f;

bool S_Music_Init()
{
    return S_Audio_Init();
}

bool S_Music_SetVolume(int16_t volume)
{
    m_MusicVolume = volume / 255.0f;
    return true;
}

bool S_Music_Pause()
{
    return S_Audio_PauseStreaming(m_MusicAudioStreamID);
}

bool S_Music_Unpause()
{
    return S_Audio_UnpauseStreaming(m_MusicAudioStreamID);
}

bool S_Music_Play(int16_t track)
{
    char file_path[64];
    sprintf(file_path, "music\\track%02d.flac", track);

    m_MusicAudioStreamID = S_Audio_StartStreaming(file_path);

    if (m_MusicAudioStreamID < 0) {
        LOG_ERROR("All music streams are busy");
        return false;
    }

    S_Audio_SetStreamVolume(m_MusicAudioStreamID, m_MusicVolume);

    return true;
}

bool S_Music_Stop()
{
    return S_Audio_StopStreaming(m_MusicAudioStreamID);
}
