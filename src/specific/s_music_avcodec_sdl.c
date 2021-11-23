#include "specific/s_music.h"

#include "log.h"
#include "memory.h"
#include "specific/s_audio.h"

#include <stdio.h>

static int m_LoopedTrack = -1;
static int m_AudioStreamID = -1;
static float m_MusicVolume = 0.0f;

static void S_Music_StreamFinished(int sound_id, void *user_data);

static void S_Music_StreamFinished(int sound_id, void *user_data)
{
    // When a stream finishes, play the remembered background BGM.

    LOG_DEBUG("%d", sound_id);
    if (sound_id == m_AudioStreamID) {
        m_AudioStreamID = -1;
        if (m_LoopedTrack >= 0) {
            S_Music_PlayLooped(m_LoopedTrack);
        }
    }
}

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
    if (m_AudioStreamID < 0) {
        return false;
    }
    return S_Audio_PauseStreaming(m_AudioStreamID);
}

bool S_Music_Unpause()
{
    if (m_AudioStreamID < 0) {
        return false;
    }
    return S_Audio_UnpauseStreaming(m_AudioStreamID);
}

bool S_Music_Play(int16_t track)
{
    char file_path[64];
    sprintf(file_path, "music\\track%02d.flac", track);

    if (m_AudioStreamID >= 0) {
        // We're about to stop the currently playing track to play a new
        // foreground track. Chances are this is the looped background music.
        // Make sure not to execute the finish callback, otherwise it would
        // immediately try to resume the old BGM track which we want to stop.
        S_Audio_SetStreamFinishCallback(m_AudioStreamID, NULL, NULL);
        S_Audio_StopStreaming(m_AudioStreamID);
    }

    m_AudioStreamID = S_Audio_StartStreaming(file_path);

    if (m_AudioStreamID < 0) {
        LOG_ERROR("All music streams are busy");
        return false;
    }

    S_Audio_SetStreamVolume(m_AudioStreamID, m_MusicVolume);
    S_Audio_SetStreamFinishCallback(
        m_AudioStreamID, S_Music_StreamFinished, NULL);

    return true;
}

bool S_Music_PlayLooped(int16_t track)
{
    bool ret = S_Music_Play(track);
    m_LoopedTrack = track;
    if (ret) {
        ret &= S_Audio_SetStreamIsLooped(m_AudioStreamID, true);
    }
    return true;
}

bool S_Music_Stop()
{
    m_LoopedTrack = -1;
    return S_Audio_StopStreaming(m_AudioStreamID);
}
