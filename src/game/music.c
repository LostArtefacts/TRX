#include "game/music.h"

#include "config.h"
#include "game/gameflow.h"
#include "game/sound.h"
#include "global/vars.h"
#include "log.h"
#include "specific/s_audio.h"

#include <stdio.h>

static float m_MusicVolume = 0.0f;
static int m_AudioStreamID = -1;
static int16_t m_Track = 0;
static int16_t m_TrackLooped = -1;

static void Music_StopActiveStream(void);
static void Music_StreamFinished(int stream_id, void *user_data);

static void Music_StopActiveStream(void)
{
    if (m_AudioStreamID < 0) {
        return;
    }

    // We are only interested in calling Music_StreamFinished if a stream
    // finished by itself. In cases where we end the streams early by hand,
    // we clear the finish callback in order to avoid resuming the BGM playback
    // just after we stop it.
    S_Audio_StreamSoundSetFinishCallback(m_AudioStreamID, NULL, NULL);
    S_Audio_StreamSoundClose(m_AudioStreamID);
}

static void Music_StreamFinished(int stream_id, void *user_data)
{
    // When a stream finishes, play the remembered background BGM.

    if (stream_id == m_AudioStreamID) {
        m_AudioStreamID = -1;
        if (m_Track == MX_SECRET) {
            m_Track = 0;
        }
        if (m_TrackLooped >= 0) {
            Music_PlayLooped(m_TrackLooped);
        }
    }
}

bool Music_Init(void)
{
    return S_Audio_Init();
}

void Music_Shutdown(void)
{
    S_Audio_Shutdown();
}

bool Music_Play(int16_t track)
{
    if (track == Music_CurrentTrack()) {
        return false;
    }

    if (track <= MX_UNUSED_1) {
        return false;
    }

    if (g_Config.fix_secrets_killing_music && track == MX_SECRET) {
        return Sound_Effect(SFX_SECRET, NULL, SPM_ALWAYS);
    }

    if (track == MX_CAVES_AMBIENT) {
        return false;
    }

    char file_path[64];
    sprintf(file_path, "music/track%02d.flac", track);

    Music_StopActiveStream();
    m_AudioStreamID = S_Audio_StreamSoundCreateFromFile(file_path);

    if (m_AudioStreamID < 0) {
        LOG_ERROR("All music streams are busy");
        return false;
    }

    m_Track = track;

    S_Audio_StreamSoundSetVolume(m_AudioStreamID, m_MusicVolume);
    S_Audio_StreamSoundSetFinishCallback(
        m_AudioStreamID, Music_StreamFinished, NULL);

    return true;
}

bool Music_PlayLooped(int16_t track)
{
    if (track == Music_CurrentTrack()) {
        return false;
    }

    if (g_CurrentLevel == g_GameFlow.title_level_num
        && !g_Config.enable_music_in_menu) {
        return false;
    }

    char file_path[64];
    sprintf(file_path, "music/track%02d.flac", track);

    Music_StopActiveStream();
    m_AudioStreamID = S_Audio_StreamSoundCreateFromFile(file_path);

    if (m_AudioStreamID < 0) {
        LOG_ERROR("All music streams are busy");
        return false;
    }

    S_Audio_StreamSoundSetVolume(m_AudioStreamID, m_MusicVolume);
    S_Audio_StreamSoundSetFinishCallback(
        m_AudioStreamID, Music_StreamFinished, NULL);
    S_Audio_StreamSoundSetIsLooped(m_AudioStreamID, true);

    m_TrackLooped = track;

    return true;
}

void Music_Stop(void)
{
    m_Track = 0;
    m_TrackLooped = -1;
    Music_StopActiveStream();
}

void Music_SetVolume(int16_t volume)
{
    m_MusicVolume = volume ? (25 * volume + 5) / 255.0f : 0.0f;
    if (m_AudioStreamID >= 0) {
        S_Audio_StreamSoundSetVolume(m_AudioStreamID, m_MusicVolume);
    }
}

void Music_Pause(void)
{
    if (m_AudioStreamID < 0) {
        return;
    }
    S_Audio_StreamSoundPause(m_AudioStreamID);
}

void Music_Unpause(void)
{
    if (m_AudioStreamID < 0) {
        return;
    }
    S_Audio_StreamSoundUnpause(m_AudioStreamID);
}

int16_t Music_CurrentTrack(void)
{
    return m_Track;
}
