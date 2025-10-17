#include "game/music/common.h"

#include "config.h"
#include "engine/audio.h"
#include "game/level.h"
#include "game/music.h"
#include "game/music/backend_cdaudio.h"
#include "game/music/backend_files.h"
#include "game/sound.h"
#include "log.h"

static bool m_Initialised = false;
static uint16_t m_MusicTrackFlags[MAX_MUSIC_TRACKS] = {};
static MUSIC_ID m_TrackCurrent = MX_INACTIVE;
static MUSIC_ID m_TrackDelayed = MX_INACTIVE;
static MUSIC_ID m_TrackLooped = MX_INACTIVE;
// Remember the last played track, whether normal or looped, to prevent
// immediately restarting it if Lara remains on the same trigger.
static MUSIC_ID m_TrackLastPlayed = MX_INACTIVE;
static MUSIC_ID m_TrackLastLooped = MX_INACTIVE;

static float m_MusicVolume = 0.0f;
static int32_t m_AudioStreamID = -1;
static const MUSIC_BACKEND *m_Backend = nullptr;

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
            m_TrackLastLooped = MX_INACTIVE;
            Music_Play_Direct(m_TrackLooped, MPM_LOOPED);
        }
    }
}

static bool M_IsBrokenTrack(const MUSIC_ID track_id)
{
    if (track_id < 0) {
        return true;
    }
    if (TR_VERSION > 1) {
        return false;
    }
    const MUSIC_TRX_ID track = Music_FromGameID(track_id);
    return track == MX_UNUSED_0 || track == MX_UNUSED_1 || track == MX_UNUSED_2;
}

static bool M_IsAmbientTrack(const MUSIC_ID track_id)
{
    const GF_AMBIENT_DATA *const ambient_data = Level_GetAmbientData();
    if (ambient_data == nullptr) {
        return false;
    }

    for (int32_t i = 0; i < ambient_data->count; i++) {
        if (ambient_data->ids[i] == track_id) {
            return true;
        }
    }

    return false;
}

static void M_SyncVolume(const int32_t audio_stream_id)
{
    if (audio_stream_id < 0) {
        return;
    }
    Audio_Stream_SetVolume(audio_stream_id, m_MusicVolume);
}

bool Music_Init(void)
{
    m_Initialised = true;
    m_Backend = M_FindBackend();
    if (m_Backend == nullptr) {
        LOG_ERROR("No music backend is available");
        goto finish;
    }

    LOG_INFO("Chosen music backend: %s", m_Backend->describe(m_Backend));
    Music_SetVolume(g_Config.audio.music_volume);

finish:
    m_TrackCurrent = MX_INACTIVE;
    m_TrackLastPlayed = MX_INACTIVE;
    m_TrackDelayed = MX_INACTIVE;
    m_TrackLooped = MX_INACTIVE;
    m_TrackLastLooped = MX_INACTIVE;
    return Audio_Init();
}

void Music_Shutdown(void)
{
    m_Initialised = false;
    M_StopActiveStream();
    Audio_Shutdown();
}

bool Music_Play_Direct(const MUSIC_ID track_id, const MUSIC_PLAY_MODE mode)
{
    if (!m_Initialised) {
        return false;
    }

    if (M_IsBrokenTrack(track_id)) {
        return false;
    }

    if (mode != MPM_ALWAYS && track_id == m_TrackCurrent) {
        return true;
    }

    if (mode == MPM_TRACKED && track_id == m_TrackLastPlayed) {
        return true;
    }

    const bool is_looped = mode == MPM_LOOPED || M_IsAmbientTrack(track_id);
    if (is_looped && track_id == m_TrackLastLooped) {
        return true;
    }

    if (mode == MPM_DELAYED) {
        m_TrackDelayed = track_id;
        return true;
    }

#if TR_VERSION == 1
    const MUSIC_TRX_ID track = Music_FromGameID(track_id);
    // TODO: utilise secondary audio stream to allow playing high fidelity
    // versions of these sounds.
    if (g_Config.audio.fix_secrets_killing_music && track == MX_SECRET
        && Sound_IsAvailable(SFX_SECRET)) {
        return Sound_Effect(SFX_SECRET, nullptr, SPM_ALWAYS);
    }

    if (g_Config.audio.fix_speeches_killing_music) {
        SAMPLE_TRX_ID sample_id = SFX_INVALID;
        switch (track) {
        case MX_BALDY_SPEECH:
            sample_id = SFX_BALDY_SPEECH;
            break;
        case MX_COWBOY_SPEECH:
            sample_id = SFX_COWBOY_SPEECH;
            break;
        case MX_LARSON_SPEECH:
            sample_id = SFX_LARSON_SPEECH;
            break;
        case MX_NATLA_SPEECH:
            sample_id = SFX_NATLA_SPEECH;
            break;
        case MX_PIERRE_SPEECH:
            sample_id = SFX_PIERRE_SPEECH;
            break;
        case MX_SKATEKID_SPEECH:
            sample_id = SFX_SKATEKID_SPEECH;
            break;
        default:
            break;
        }
        if (Sound_IsAvailable(sample_id)) {
            return Sound_Effect(sample_id, nullptr, SPM_ALWAYS);
        }
    }
#endif

    M_StopActiveStream();

    if (m_Backend == nullptr) {
        LOG_WARNING(
            "Not playing track %d because no backend is available", track_id);
        goto finish;
    }

    LOG_INFO("Playing track %d, mode: %d", track_id, mode);

    m_AudioStreamID = m_Backend->play(m_Backend, track_id);
    if (m_AudioStreamID < 0) {
        LOG_ERROR("Failed to create music stream for track %d", track_id);
        goto finish;
    }

    M_SyncVolume(m_AudioStreamID);
    Audio_Stream_SetIsLooped(m_AudioStreamID, is_looped);
    Audio_Stream_SetFinishCallback(m_AudioStreamID, M_StreamFinished, nullptr);

finish:
    m_TrackDelayed = MX_INACTIVE;
    if (is_looped) {
        // Reset the regular track outside of M_StreamFinished so that
        // Music_GetCurrentPlayingTrack returns the looped track; otherwise, the
        // stopped track could be stored in the savegame despite being inactive.
        m_TrackCurrent = MX_INACTIVE;
        m_TrackLooped = track_id;
        m_TrackLastLooped = track_id;
    } else {
        m_TrackCurrent = track_id;
        m_TrackLastPlayed = track_id;
    }
    return true;
}

bool Music_Play(const MUSIC_TRX_ID track, const MUSIC_PLAY_MODE mode)
{
    return Music_Play_Direct(Music_ToGameID(track), mode);
}

void Music_Stop(void)
{
    m_TrackCurrent = MX_INACTIVE;
    m_TrackLastPlayed = MX_INACTIVE;
    m_TrackDelayed = MX_INACTIVE;
    m_TrackLooped = MX_INACTIVE;
    m_TrackLastLooped = MX_INACTIVE;
    M_StopActiveStream();
}

void Music_StopTrack_Direct(const MUSIC_ID track)
{
    if (track != m_TrackCurrent || M_IsBrokenTrack(track)) {
        return;
    }

    M_StopActiveStream();
    m_TrackCurrent = MX_INACTIVE;

    if (m_TrackLooped >= 0) {
        Music_Play_Direct(m_TrackLooped, MPM_LOOPED);
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

bool Music_SyncTimestamp(const double timestamp)
{
    if (m_AudioStreamID < 0) {
        return false;
    }
    return Audio_Stream_SyncTimestamp(m_AudioStreamID, timestamp);
}

MUSIC_ID Music_GetDelayedTrack(void)
{
    return m_TrackDelayed;
}

MUSIC_ID Music_GetCurrentPlayingTrack(void)
{
    return m_TrackCurrent == MX_INACTIVE ? m_TrackLooped : m_TrackCurrent;
}

MUSIC_ID Music_GetCurrentLoopedTrack(void)
{
    return m_TrackLooped;
}

void Music_SetVolume(float volume)
{
    volume *= g_Config.audio.master_volume;
    if (volume != m_MusicVolume) {
        m_MusicVolume = volume;
        M_SyncVolume(m_AudioStreamID);
    }
}

void Music_ResetTrackFlags(void)
{
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        m_MusicTrackFlags[i] = 0;
    }
}

uint16_t Music_GetTrackFlags(const MUSIC_ID track_id)
{
    return m_MusicTrackFlags[track_id];
}

void Music_SetTrackFlags(const MUSIC_ID track_id, const uint16_t flags)
{
    m_MusicTrackFlags[track_id] = flags;
}

MUSIC_ID Music_ConvertLegacyTrack(const MUSIC_ID track_id)
{
#if TR_VERSION == 1
    return track_id;
#else
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
#endif
}
