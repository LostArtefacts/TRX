#include "game/music/common.h"

#include "config.h"
#include "engine/audio.h"
#include "game/music.h"
#include "game/music/backend_cdaudio.h"
#include "game/music/backend_files.h"
#include "game/sound.h"
#include "log.h"

static uint16_t m_MusicTrackFlags[MAX_MUSIC_TRACKS] = {};
static MUSIC_TRACK_ID m_TrackCurrent = MX_INACTIVE;
static MUSIC_TRACK_ID m_TrackLastPlayed = MX_INACTIVE;
static MUSIC_TRACK_ID m_TrackDelayed = MX_INACTIVE;
static MUSIC_TRACK_ID m_TrackLooped = MX_INACTIVE;

static bool m_Muted = false;
static int16_t m_MusicVolume = 0;
static int32_t m_AudioStreamID = -1;
static const MUSIC_BACKEND *m_Backend = nullptr;

static const MUSIC_BACKEND *M_FindBackend(void);
static void M_StopActiveStream(void);
static void M_StreamFinished(int32_t stream_id, void *user_data);
static bool M_IsBrokenTrack(MUSIC_TRACK_ID track);
static int32_t M_GetRealTrack(int32_t track_id);
static void M_SyncVolume(int32_t audio_stream_id);

static const MUSIC_BACKEND *M_FindBackend(void)
{
    MUSIC_BACKEND *all_backends[] = {
        Music_Backend_Files_Factory("music"),
#if TR_VERSION == 2
        Music_Backend_CDAudio_Factory("audio/cdaudio.wav"),
        Music_Backend_CDAudio_Factory("audio/cdaudio.mp3"),
#endif
        nullptr,
    };

    const MUSIC_BACKEND *result = nullptr;
    for (MUSIC_BACKEND **backend_ptr = all_backends; *backend_ptr != nullptr;
         backend_ptr++) {
        if ((*backend_ptr)->init(*backend_ptr)) {
            result = *backend_ptr;
            break;
        }
    }

    for (MUSIC_BACKEND **backend_ptr = all_backends; *backend_ptr != nullptr;
         backend_ptr++) {
        if (*backend_ptr != result) {
            (*backend_ptr)->shutdown(*backend_ptr);
        }
    }

    return result;
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

static void M_StreamFinished(const int32_t stream_id, void *const user_data)
{
    // When a stream finishes, play the remembered BGM.
    if (stream_id == m_AudioStreamID) {
        m_TrackCurrent = MX_INACTIVE;
        m_AudioStreamID = -1;
        if (m_TrackLooped >= 0) {
            Music_Play(m_TrackLooped, MPM_LOOPED);
        }
    }
}

static bool M_IsBrokenTrack(const MUSIC_TRACK_ID track)
{
#if TR_VERSION == 1
    return track == MX_UNUSED_0 || track == MX_UNUSED_1 || track == MX_UNUSED_2;
#else
    return false;
#endif
}

static int32_t M_GetRealTrack(const int32_t track_id)
{
#if TR_VERSION == 2
    const int8_t skipped_track_ids[] = { 2, 19, 20, 26, -1 };
    int32_t idx = 0;
    int32_t ret_track_id = 2;

    for (int32_t i = 2; i < track_id; i++) {
        if ((skipped_track_ids[idx] >= 0) && (i == skipped_track_ids[idx])) {
            idx++;
        } else {
            ret_track_id++;
        }
    }
    return ret_track_id;
#else
    return track_id;
#endif
}

static void M_SyncVolume(const int32_t audio_stream_id)
{
    if (audio_stream_id < 0) {
        return;
    }

    const float multiplier = m_MusicVolume != 0 ? m_MusicVolume / 10.0f : 0.0f;
    Audio_Stream_SetVolume(audio_stream_id, m_Muted ? 0 : multiplier);
}

bool Music_Init(void)
{
    bool result = false;

    m_Backend = M_FindBackend();
    if (m_Backend == nullptr) {
        LOG_ERROR("No music backend is available");
        goto finish;
    }

    LOG_INFO("Chosen music backend: %s", m_Backend->describe(m_Backend));
    result = true;
    Music_SetVolume(g_Config.audio.music_volume);

finish:
    m_TrackCurrent = MX_INACTIVE;
    m_TrackLastPlayed = MX_INACTIVE;
    m_TrackDelayed = MX_INACTIVE;
    m_TrackLooped = MX_INACTIVE;
    return result && Audio_Init();
}

void Music_Shutdown(void)
{
    M_StopActiveStream();
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

#if TR_VERSION == 1
    // TODO: utilise secondary audio stream to allow playing high fidelity
    // versions of these sounds.
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
#endif

    M_StopActiveStream();

    if (m_Backend == nullptr) {
        LOG_WARNING(
            "Not playing track %d because no backend is available", track_id);
        goto finish;
    }

    const int32_t real_track_id = M_GetRealTrack(track_id);
    LOG_INFO(
        "Playing track %d (real: %d), mode: %d", track_id, real_track_id, mode);

    m_AudioStreamID = m_Backend->play(m_Backend, real_track_id);
    if (m_AudioStreamID < 0) {
        LOG_ERROR("Failed to create music stream for track %d", track_id);
        goto finish;
    }

    M_SyncVolume(m_AudioStreamID);
    Audio_Stream_SetIsLooped(m_AudioStreamID, mode == MPM_LOOPED);
    Audio_Stream_SetFinishCallback(m_AudioStreamID, M_StreamFinished, nullptr);

finish:
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

void Music_StopTrack(const MUSIC_TRACK_ID track)
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

double Music_GetTimestamp(void)
{
    if (m_AudioStreamID < 0) {
        return -1.0;
    }
    return Audio_Stream_GetTimestamp(m_AudioStreamID);
}

bool Music_SeekTimestamp(const double timestamp)
{
    if (m_AudioStreamID < 0) {
        return false;
    }
    return Audio_Stream_SeekTimestamp(m_AudioStreamID, timestamp);
}

MUSIC_TRACK_ID Music_GetDelayedTrack(void)
{
    return m_TrackDelayed;
}

MUSIC_TRACK_ID Music_GetCurrentPlayingTrack(void)
{
    return m_TrackCurrent == MX_INACTIVE ? m_TrackLooped : m_TrackCurrent;
}

MUSIC_TRACK_ID Music_GetCurrentLoopedTrack(void)
{
    return m_TrackLooped;
}

int32_t Music_GetMinVolume(void)
{
    return 0;
}

int32_t Music_GetMaxVolume(void)
{
    return 10;
}

void Music_SetVolume(const int32_t volume)
{
    if (volume != m_MusicVolume) {
        m_MusicVolume = volume;
        M_SyncVolume(m_AudioStreamID);
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

void Music_ResetTrackFlags(void)
{
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        m_MusicTrackFlags[i] = 0;
    }
}

uint16_t Music_GetTrackFlags(const int32_t track_idx)
{
    return m_MusicTrackFlags[track_idx];
}

void Music_SetTrackFlags(const int32_t track, const uint16_t flags)
{
    m_MusicTrackFlags[track] = flags;
}
