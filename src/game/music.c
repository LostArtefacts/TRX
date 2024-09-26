#include "game/music.h"

#include "config.h"
#include "game/gameflow.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/engine/audio.h>
#include <libtrx/filesystem.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <stdio.h>

static const char *m_Extensions[] = { ".flac", ".ogg", ".mp3", ".wav", NULL };

static bool m_Muted = false;
static int16_t m_Volume = 0;
static int m_AudioStreamID = -1;
static MUSIC_TRACK_ID m_TrackCurrent = MX_INACTIVE;
static MUSIC_TRACK_ID m_TrackLastPlayed = MX_INACTIVE;
static MUSIC_TRACK_ID m_TrackLooped = MX_INACTIVE;

static void M_SyncVolume(const int32_t audio_stream_id);
static bool M_IsBrokenTrack(MUSIC_TRACK_ID track);
static void M_StopActiveStream(void);
static void M_StreamFinished(int stream_id, void *user_data);
static char *M_GetTrackFileName(MUSIC_TRACK_ID track);

static void M_SyncVolume(const int32_t audio_stream_id)
{
    if (audio_stream_id < 0) {
        return;
    }
    const float multiplier = m_Volume ? (25 * m_Volume + 5) / 255.0f : 0.0f;
    Audio_Stream_SetVolume(audio_stream_id, m_Muted ? 0 : multiplier);
}

static bool M_IsBrokenTrack(MUSIC_TRACK_ID track)
{
    return track == MX_UNUSED_0 || track == MX_UNUSED_1 || track == MX_UNUSED_2;
}

static void M_StopActiveStream(void)
{
    if (m_AudioStreamID < 0) {
        return;
    }

    // We are only interested in calling M_StreamFinished if a stream
    // finished by itself. In cases where we end the streams early by hand,
    // we clear the finish callback in order to avoid resuming the BGM playback
    // just after we stop it.
    Audio_Stream_SetFinishCallback(m_AudioStreamID, NULL, NULL);
    Audio_Stream_Close(m_AudioStreamID);
}

static char *M_GetTrackFileName(MUSIC_TRACK_ID track)
{
    char file_path[64];
    sprintf(file_path, "music/track%02d.flac", track);
    return File_GuessExtension(file_path, m_Extensions);
}

static void M_StreamFinished(int stream_id, void *user_data)
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
    return Audio_Init();
}

void Music_Shutdown(void)
{
    Audio_Shutdown();
}

bool Music_Play(MUSIC_TRACK_ID track)
{
    if (track == m_TrackCurrent || track == m_TrackLastPlayed
        || M_IsBrokenTrack(track)) {
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

    M_StopActiveStream();

    char *file_path = M_GetTrackFileName(track);
    m_AudioStreamID = Audio_Stream_CreateFromFile(file_path);
    Memory_FreePointer(&file_path);

    if (m_AudioStreamID < 0) {
        LOG_ERROR("All music streams are busy");
        return false;
    }

    m_TrackCurrent = track;
    if (track != MX_SECRET) {
        m_TrackLastPlayed = track;
    }

    M_SyncVolume(m_AudioStreamID);
    Audio_Stream_SetFinishCallback(m_AudioStreamID, M_StreamFinished, NULL);

    return true;
}

bool Music_PlayLooped(MUSIC_TRACK_ID track)
{
    if (track == m_TrackCurrent || track == m_TrackLastPlayed
        || M_IsBrokenTrack(track)) {
        return false;
    }

    M_StopActiveStream();

    char *file_path = M_GetTrackFileName(track);
    m_AudioStreamID = Audio_Stream_CreateFromFile(file_path);
    Memory_FreePointer(&file_path);

    if (m_AudioStreamID < 0) {
        LOG_ERROR("All music streams are busy");
        return false;
    }

    M_SyncVolume(m_AudioStreamID);
    Audio_Stream_SetFinishCallback(m_AudioStreamID, M_StreamFinished, NULL);
    Audio_Stream_SetIsLooped(m_AudioStreamID, true);

    m_TrackLooped = track;

    return true;
}

void Music_Stop(void)
{
    m_TrackCurrent = MX_INACTIVE;
    m_TrackLastPlayed = MX_INACTIVE;
    m_TrackLooped = MX_INACTIVE;
    M_StopActiveStream();
}

void Music_StopTrack(MUSIC_TRACK_ID track)
{
    if (track != m_TrackCurrent || M_IsBrokenTrack(track)) {
        return;
    }

    M_StopActiveStream();
    m_TrackCurrent = MX_INACTIVE;

    if (m_TrackLooped >= 0) {
        Music_PlayLooped(m_TrackLooped);
    }
}

void Music_Mute(void)
{
    m_Muted = true;
    M_SyncVolume(m_AudioStreamID);
}

void Music_Unmute(void)
{
    m_Muted = false;
    M_SyncVolume(m_AudioStreamID);
}

int16_t Music_GetVolume(void)
{
    return m_Volume;
}

void Music_SetVolume(int16_t volume)
{
    if (volume != m_Volume) {
        m_Volume = volume;
        M_SyncVolume(m_AudioStreamID);
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
    Audio_Stream_Pause(m_AudioStreamID);
}

void Music_Unpause(void)
{
    if (m_AudioStreamID < 0) {
        return;
    }
    Audio_Stream_Unpause(m_AudioStreamID);
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
    return Audio_Stream_GetDuration(m_AudioStreamID);
}

double Music_GetTimestamp(void)
{
    if (m_AudioStreamID < 0) {
        return -1.0;
    }
    return Audio_Stream_GetTimestamp(m_AudioStreamID);
}

bool Music_SeekTimestamp(double timestamp)
{
    if (m_AudioStreamID < 0) {
        return false;
    }
    return Audio_Stream_SeekTimestamp(m_AudioStreamID, timestamp);
}
