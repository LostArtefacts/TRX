#include "game/music.h"

#include "config.h"
#include "filesystem.h"
#include "game/gameflow.h"
#include "game/sound.h"
#include "global/vars.h"
#include "log.h"
#include "memory.h"
#include "specific/s_audio.h"

#include <stdio.h>

static const char *m_Extensions[] = { ".flac", ".ogg", ".mp3", ".wav", NULL };

static float m_MusicVolume = 0.0f;
static int m_AudioStreamID = -1;
static MUSIC_TRACK_ID m_TrackCurrent = MX_INACTIVE;
static MUSIC_TRACK_ID m_TrackLastPlayed = MX_INACTIVE;
static MUSIC_TRACK_ID m_TrackLooped = MX_INACTIVE;

static bool Music_IsBrokenTrack(MUSIC_TRACK_ID track);
static void Music_StopActiveStream(void);
static void Music_StreamFinished(int stream_id, void *user_data);
static char *Music_GetTrackFileName(MUSIC_TRACK_ID track);

static bool Music_IsBrokenTrack(MUSIC_TRACK_ID track)
{
    return track == MX_UNUSED_0 || track == MX_UNUSED_1 || track == MX_UNUSED_2;
}

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

static char *Music_GetTrackFileName(MUSIC_TRACK_ID track)
{
    char file_path[64];
    sprintf(file_path, "music/track%02d.flac", track);
    return File_GuessExtension(file_path, m_Extensions);
}

static void Music_StreamFinished(int stream_id, void *user_data)
{
    // When a stream finishes, play the remembered background BGM.

    if (stream_id == m_AudioStreamID) {
        m_AudioStreamID = -1;
        m_TrackCurrent = MX_INACTIVE;
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

bool Music_Play(MUSIC_TRACK_ID track)
{
    if (track == m_TrackCurrent || track == m_TrackLastPlayed
        || Music_IsBrokenTrack(track)) {
        return false;
    }

    if (g_Config.fix_secrets_killing_music && track == MX_SECRET) {
        return Sound_Effect(SFX_SECRET, NULL, SPM_ALWAYS);
    }

    if (g_Config.fix_speeches_killing_music && track >= MX_BALDY_SPEECH
        && track <= MX_SKATEKID_SPEECH) {
        return Sound_Effect(
            SFX_BALDY_SPEECH + track - MX_BALDY_SPEECH, NULL, SPM_ALWAYS);
    }

    Music_StopActiveStream();

    char *file_path = Music_GetTrackFileName(track);
    m_AudioStreamID = S_Audio_StreamSoundCreateFromFile(file_path);
    Memory_FreePointer(&file_path);

    if (m_AudioStreamID < 0) {
        LOG_ERROR("All music streams are busy");
        return false;
    }

    m_TrackCurrent = track;
    if (track != MX_SECRET) {
        m_TrackLastPlayed = track;
    }

    S_Audio_StreamSoundSetVolume(m_AudioStreamID, m_MusicVolume);
    S_Audio_StreamSoundSetFinishCallback(
        m_AudioStreamID, Music_StreamFinished, NULL);

    return true;
}

bool Music_PlayLooped(MUSIC_TRACK_ID track)
{
    if (track == m_TrackCurrent || track == m_TrackLastPlayed
        || Music_IsBrokenTrack(track)) {
        return false;
    }

    if (g_CurrentLevel == g_GameFlow.title_level_num
        && !g_Config.enable_music_in_menu) {
        return false;
    }

    Music_StopActiveStream();

    char *file_path = Music_GetTrackFileName(track);
    m_AudioStreamID = S_Audio_StreamSoundCreateFromFile(file_path);
    Memory_FreePointer(&file_path);

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
    m_TrackCurrent = MX_INACTIVE;
    m_TrackLastPlayed = MX_INACTIVE;
    m_TrackLooped = MX_INACTIVE;
    Music_StopActiveStream();
}

void Music_StopTrack(MUSIC_TRACK_ID track)
{
    if (track != m_TrackCurrent || Music_IsBrokenTrack(track)) {
        return;
    }

    Music_StopActiveStream();
    m_TrackCurrent = MX_INACTIVE;

    if (m_TrackLooped >= 0) {
        Music_PlayLooped(m_TrackLooped);
    }
}

int16_t Music_GetVolume(void)
{
    return m_MusicVolume * Music_GetMaxVolume();
}

void Music_SetVolume(int16_t volume)
{
    m_MusicVolume = volume ? (25 * volume + 5) / 255.0f : 0.0f;
    if (m_AudioStreamID >= 0) {
        S_Audio_StreamSoundSetVolume(m_AudioStreamID, m_MusicVolume);
    }
}

int16_t Music_GetMinVolume(void)
{
    return 0;
}

int16_t Music_GetMaxVolume(void)
{
    return 10;
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

MUSIC_TRACK_ID Music_GetCurrentTrack(void)
{
    return m_TrackCurrent;
}

MUSIC_TRACK_ID Music_GetLastPlayedTrack(void)
{
    return m_TrackLastPlayed;
}

MUSIC_TRACK_ID Music_GetCurrentLoopedTrack(void)
{
    return m_TrackLooped;
}

MUSIC_TRACK_ID Music_GetCurrentPlayingTrack(void)
{
    return m_TrackCurrent == MX_INACTIVE ? m_TrackLooped : m_TrackCurrent;
}

double Music_GetDuration(void)
{
    if (m_AudioStreamID < 0) {
        return -1.0;
    }
    return S_Audio_StreamGetDuration(m_AudioStreamID);
}

double Music_GetTimestamp(void)
{
    if (m_AudioStreamID < 0) {
        return -1.0;
    }
    return S_Audio_StreamGetTimestamp(m_AudioStreamID);
}

bool Music_SeekTimestamp(double timestamp)
{
    if (m_AudioStreamID < 0) {
        return false;
    }
    return S_Audio_StreamSeekTimestamp(m_AudioStreamID, timestamp);
}
