#include "game/music.h"

#include "game/game_flow.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/engine/audio.h>
#include <libtrx/filesystem.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <stdio.h>

static const char *m_Extensions[] = {
    ".flac", ".ogg", ".mp3", ".wav", nullptr,
};

static bool m_Muted = false;
static int16_t m_Volume = 0;
static int m_AudioStreamID = -1;
static MUSIC_TRACK_ID m_TrackCurrent = MX_INACTIVE;
static MUSIC_TRACK_ID m_TrackLastPlayed = MX_INACTIVE;
static MUSIC_TRACK_ID m_TrackDelayed = MX_INACTIVE;
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
    Audio_Stream_SetFinishCallback(m_AudioStreamID, nullptr, nullptr);
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
            Music_Play(m_TrackLooped, MPM_LOOPED);
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

bool Music_Play(const MUSIC_TRACK_ID track_id, const MUSIC_PLAY_MODE mode)
{
    if (M_IsBrokenTrack(track_id)) {
        return false;
    }

    if (mode != MPM_ALWAYS && track_id == m_TrackCurrent) {
        return false;
    }

    if (mode == MPM_TRACKED && track_id == m_TrackLastPlayed) {
        return false;
    }

    if (mode == MPM_DELAYED) {
        m_TrackDelayed = track_id;
        return false;
    }

    if (g_Config.audio.fix_secrets_killing_music && track_id == MX_SECRET
        && Sound_IsAvailable(SFX_SECRET)) {
        return Sound_Effect(SFX_SECRET, nullptr, SPM_ALWAYS);
    }

    if (g_Config.audio.fix_speeches_killing_music && track_id >= MX_BALDY_SPEECH
        && track_id <= MX_SKATEKID_SPEECH) {
        const SOUND_EFFECT_ID speech_id =
            SFX_BALDY_SPEECH + track_id - MX_BALDY_SPEECH;
        if (Sound_IsAvailable(speech_id)) {
            return Sound_Effect(speech_id, nullptr, SPM_ALWAYS);
        }
    }

    M_StopActiveStream();

    char *file_path = M_GetTrackFileName(track_id);
    m_AudioStreamID = Audio_Stream_CreateFromFile(file_path);
    Memory_FreePointer(&file_path);

    if (m_AudioStreamID < 0) {
        LOG_ERROR("All music streams are busy");
        return false;
    }

    M_SyncVolume(m_AudioStreamID);
    Audio_Stream_SetIsLooped(m_AudioStreamID, mode == MPM_LOOPED);
    Audio_Stream_SetFinishCallback(m_AudioStreamID, M_StreamFinished, nullptr);

    m_TrackDelayed = MX_INACTIVE;
    if (mode == MPM_LOOPED) {
        m_TrackLooped = track_id;
    } else {
        m_TrackCurrent = track_id;
        m_TrackLastPlayed = track_id;
    }

    return true;
}

void Music_Stop(void)
{
    m_TrackCurrent = MX_INACTIVE;
    m_TrackLastPlayed = MX_INACTIVE;
    m_TrackDelayed = MX_INACTIVE;
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
        Music_Play(m_TrackLooped, MPM_LOOPED);
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

MUSIC_TRACK_ID Music_GetCurrentPlayingTrack(void)
{
    return m_TrackCurrent == MX_INACTIVE ? m_TrackLooped : m_TrackCurrent;
}

MUSIC_TRACK_ID Music_GetCurrentLoopedTrack(void)
{
    return m_TrackLooped;
}

MUSIC_TRACK_ID Music_GetLastPlayedTrack(void)
{
    return m_TrackLastPlayed;
}

MUSIC_TRACK_ID Music_GetDelayedTrack(void)
{
    return m_TrackDelayed;
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
