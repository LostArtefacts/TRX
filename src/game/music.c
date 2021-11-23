#include "game/music.h"

#include "config.h"
#include "log.h"
#include "game/sound.h"
#include "global/vars.h"
#include "specific/s_audio.h"

#include <stdio.h>

static float m_MusicVolume = 0.0f;
static int m_AudioStreamID = -1;
static int16_t m_Track = 0;
static int16_t m_TrackLooped = -1;

static void Music_StreamFinished(int sound_id, void *user_data);

static void Music_StreamFinished(int sound_id, void *user_data)
{
    // When a stream finishes, play the remembered background BGM.

    LOG_DEBUG("%d", sound_id);
    if (sound_id == m_AudioStreamID) {
        m_AudioStreamID = -1;
        if (m_TrackLooped >= 0) {
            Music_PlayLooped(m_TrackLooped);
        }
    }
}

bool Music_Init()
{
    return S_Audio_Init();
}

bool Music_Play(int16_t track)
{
    if (track == Music_CurrentTrack()) {
        return false;
    }

    if (CurrentLevel == GF.title_level_num && T1MConfig.disable_music_in_menu) {
        return false;
    }

    if (track <= 1) {
        return false;
    }

    if (T1MConfig.fix_secrets_killing_music && track == 13) {
        return Sound_Effect(SFX_SECRET, NULL, SPM_ALWAYS);
    }

    if (track == 0) {
        Music_Stop();
        return false;
    }

    if (track == 5) {
        return false;
    }

    m_Track = track;

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
        m_AudioStreamID, Music_StreamFinished, NULL);

    return true;
}

void Music_PlayLooped(int16_t track)
{
    bool ret = Music_Play(track);
    m_TrackLooped = track;
    if (ret) {
        S_Audio_SetStreamIsLooped(m_AudioStreamID, true);
    }
}

bool Music_Stop()
{
    m_Track = 0;
    m_TrackLooped = -1;
    return S_Audio_StopStreaming(m_AudioStreamID);
}

void Music_SetVolume(int16_t volume)
{
    m_MusicVolume = volume ? (25 * volume + 5) / 255.0f : 0.0f;
}

void Music_Pause()
{
    if (m_AudioStreamID < 0) {
        return;
    }
    S_Audio_PauseStreaming(m_AudioStreamID);
}

void Music_Unpause()
{
    if (m_AudioStreamID < 0) {
        return;
    }
    S_Audio_UnpauseStreaming(m_AudioStreamID);
}

int16_t Music_CurrentTrack()
{
    return m_Track;
}
