#include <trx/game/music/common.h>

#include <trx/av/audio.h>
#include <trx/av/audio_decoder.h>
#include <trx/config.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/gym.h>
#include <trx/game/level.h>
#include <trx/game/music.h>
#include <trx/game/music/backend_cdaudio.h>
#include <trx/game/music/backend_cdaudio_wad.h>
#include <trx/game/music/backend_files.h>
#include <trx/game/paths.h>
#include <trx/game/rules.h>
#include <trx/game/shell/common.h>
#include <trx/version.h>

#include <string.h>

typedef struct {
    int32_t audio_stream_id;
    MUSIC_ID track_id;
    MUSIC_PLAY_MODE mode;
    bool active;
} M_MUSIC_STREAM;

static bool m_Initialised = false;
static MUSIC_TRACK_STATE m_TrackStates[MAX_MUSIC_TRACKS] = {};
static MUSIC_ID m_TrackCurrent = MX_INACTIVE;
static MUSIC_ID m_TrackDelayed = MX_INACTIVE;
static MUSIC_ID m_TrackLooped = MX_INACTIVE;
// Remember the last played track, whether normal or looped, to prevent
// immediately restarting it if Lara remains on the same trigger.
static MUSIC_ID m_TrackLastPlayed = MX_INACTIVE;
static MUSIC_ID m_TrackLastLooped = MX_INACTIVE;

// How long each track runs, in seconds, as its file says. Zero where nothing
// has asked yet, and a negative value where the answer was that nothing knows.
static double m_TrackDurations[MAX_MUSIC_TRACKS] = {};
static float m_MusicVolume = 0.0f;
static MUSIC_BACKEND *m_Backend = nullptr;
static M_MUSIC_STREAM m_MainStream = {
    .audio_stream_id = -1,
    .track_id = MX_INACTIVE,
    .mode = MPM_ONCE,
    .active = false,
};
static M_MUSIC_STREAM m_OverlayStreams[MUSIC_MAX_OVERLAY_TRACKS] = {};

static MUSIC_BACKEND *M_FindBackend(void)
{
    VECTOR *all_backends = Vector_Create(sizeof(MUSIC_BACKEND *));
    const char *const music_dir =
        GamePath_PeekResolve(GAME_DYNAMIC_PATH_MUSIC_DIR, nullptr);
    const char *const music_catalog_path = GamePath_PeekResolve(
        GAME_DYNAMIC_PATH_CATALOG, "catalog_music_files.csv");
    if (music_dir != nullptr || music_catalog_path != nullptr) {
        Vector_Add(
            all_backends,
            &(MUSIC_BACKEND *) {
                Music_Backend_Files_Factory(music_dir, music_catalog_path) });
    }

    if (g_TRVersion >= 2) {
        const char *const cdaudio_dat_path =
            GamePath_PeekResolve(GAME_DYNAMIC_PATH_CDAUDIO_FILE, "cdaudio.dat");
        const char *const cdaudio_wav_path =
            GamePath_PeekResolve(GAME_DYNAMIC_PATH_CDAUDIO_FILE, "cdaudio.wav");
        const char *const cdaudio_mp3_path =
            GamePath_PeekResolve(GAME_DYNAMIC_PATH_CDAUDIO_FILE, "cdaudio.mp3");

        if (cdaudio_dat_path != nullptr && cdaudio_wav_path != nullptr) {
            Vector_Add(
                all_backends,
                &(MUSIC_BACKEND *) { Music_Backend_CDAudio_Factory(
                    cdaudio_wav_path, cdaudio_dat_path) });
        }
        if (cdaudio_dat_path != nullptr && cdaudio_mp3_path != nullptr) {
            Vector_Add(
                all_backends,
                &(MUSIC_BACKEND *) { Music_Backend_CDAudio_Factory(
                    cdaudio_mp3_path, cdaudio_dat_path) });
        }
    }
    if (g_TRVersion >= 3) {
        const char *const cdaudio_wad_path =
            GamePath_PeekResolve(GAME_DYNAMIC_PATH_CDAUDIO_FILE, "cdaudio.wad");
        if (cdaudio_wad_path != nullptr) {
            Vector_Add(
                all_backends,
                &(MUSIC_BACKEND *) {
                    Music_Backend_CDAudioWad_Factory(cdaudio_wad_path) });
        }
    }

    MUSIC_BACKEND *backend = nullptr;
    for (int32_t i = 0; i < all_backends->count; i++) {
        MUSIC_BACKEND *const tmp_backend =
            *(MUSIC_BACKEND **)Vector_Get(all_backends, i);
        if (tmp_backend->init(tmp_backend)) {
            backend = tmp_backend;
            break;
        }
    }

    for (int32_t i = 0; i < all_backends->count; i++) {
        MUSIC_BACKEND *const tmp_backend =
            *(MUSIC_BACKEND **)Vector_Get(all_backends, i);
        if (tmp_backend != backend) {
            tmp_backend->shutdown(tmp_backend);
        }
    }
    Vector_Free(all_backends);

    return backend;
}

static void M_StreamReset(M_MUSIC_STREAM *const stream)
{
    stream->audio_stream_id = -1;
    stream->track_id = MX_INACTIVE;
    stream->mode = MPM_ONCE;
    stream->active = false;
}

static void M_StreamClose(M_MUSIC_STREAM *const stream)
{
    if (!stream->active || stream->audio_stream_id < 0) {
        M_StreamReset(stream);
        return;
    }

    // We are only interested in calling M_StreamFinished if a stream
    // finished by itself. In cases where we end the streams early by hand,
    // we clear the finish callback in order to avoid resuming the BGM playback
    // just after we stop it.
    SHOULD(Audio_Stream_SetFinishCallback(
        stream->audio_stream_id, nullptr, nullptr));
    SHOULD(Audio_Stream_Close(stream->audio_stream_id));
    M_StreamReset(stream);
}

static void M_StopMainStream(void)
{
    M_StreamClose(&m_MainStream);
}

static void M_StopOverlayStreams(void)
{
    for (int32_t i = 0; i < MUSIC_MAX_OVERLAY_TRACKS; i++) {
        M_StreamClose(&m_OverlayStreams[i]);
    }
}

static void M_ResetStreamState(void)
{
    M_StreamReset(&m_MainStream);
    for (int32_t i = 0; i < MUSIC_MAX_OVERLAY_TRACKS; i++) {
        M_StreamReset(&m_OverlayStreams[i]);
    }
}

static void M_StreamFinished(const int32_t stream_id, void *const user_data)
{
    M_MUSIC_STREAM *const stream = user_data;
    if (stream == nullptr) {
        return;
    }
    if (!stream->active || stream->audio_stream_id != stream_id) {
        return;
    }

    if (stream == &m_MainStream) {
        // When the main stream finishes, play the remembered BGM.
        m_TrackCurrent = MX_INACTIVE;
        M_StreamReset(stream);
        if (m_TrackLooped >= 0) {
            Music_Play_Direct(m_TrackLooped, MPM_LOOP);
        }
    } else {
        M_StreamReset(stream);
    }
}

static bool M_IsBrokenTrack(const MUSIC_ID track_id)
{
    if (track_id < 0) {
        return true;
    }
    if (g_TRVersion > 1) {
        return false;
    }
    const MUSIC_TRX_ID track = Music_FromGameID(track_id);
    return track == MX_UNUSED_0 || track == MX_UNUSED_1 || track == MX_UNUSED_2;
}

static bool M_IsAmbientTrack(const MUSIC_ID track_id)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->music_track == track_id) {
        return true;
    }

    const GF_AMBIENT_DATA *const ambient_data = &g_GameFlow.ambient_tracks;
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

static void M_SyncVolume(const M_MUSIC_STREAM *const stream)
{
    if (stream == nullptr || !stream->active || stream->audio_stream_id < 0) {
        return;
    }
    const float volume = stream->mode == MPM_OVERLAY
        ? g_Config.audio.music_volume * g_Config.audio.master_volume
        : m_MusicVolume;
    SHOULD(Audio_Stream_SetVolume(stream->audio_stream_id, volume));
}

static void M_SyncVolumes(void)
{
    M_SyncVolume(&m_MainStream);
    for (int32_t i = 0; i < MUSIC_MAX_OVERLAY_TRACKS; i++) {
        M_SyncVolume(&m_OverlayStreams[i]);
    }
}

static int32_t M_GetFreeOverlaySlot(void)
{
    for (int32_t i = 0; i < MUSIC_MAX_OVERLAY_TRACKS; i++) {
        if (!m_OverlayStreams[i].active) {
            return i;
        }
    }
    return -1;
}

// Returns the stream slot the overlay plays in - an overlay is slots 1.. - or
// -1 when it does not play.
static int32_t M_PlayOverlayTrack(
    const MUSIC_ID track_id, const double timestamp)
{
    if (Shell_GetArgs()->headless) {
        LOG_INFO("Not playing overlay track %d out loud", track_id);
        return -1;
    }
    if (m_Backend == nullptr) {
        LOG_WARNING(
            "Not playing overlay track %d because no backend is available",
            track_id);
        return -1;
    }

    const int32_t slot = M_GetFreeOverlaySlot();
    if (slot < 0) {
        LOG_WARNING(
            "Not playing overlay track %d because all %d overlay slots are in "
            "use",
            track_id, MUSIC_MAX_OVERLAY_TRACKS);
        return -1;
    }

    const int32_t stream_id = m_Backend->play(m_Backend, track_id);
    if (stream_id < 0) {
        LOG_ERROR("Failed to create overlay stream for track %d", track_id);
        return -1;
    }

    m_OverlayStreams[slot].audio_stream_id = stream_id;
    m_OverlayStreams[slot].track_id = track_id;
    m_OverlayStreams[slot].mode = MPM_OVERLAY;
    m_OverlayStreams[slot].active = true;
    M_SyncVolume(&m_OverlayStreams[slot]);
    SHOULD(Audio_Stream_SetIsLooped(stream_id, false));
    SHOULD(Audio_Stream_SetFinishCallback(
        stream_id, M_StreamFinished, &m_OverlayStreams[slot]));
    if (timestamp > 0.0) {
        SHOULD(Audio_Stream_SeekTimestamp(stream_id, timestamp));
    }
    SHOULD(Audio_Stream_Unpause(stream_id));
    return slot + 1;
}

// What every call that speaks to the music answers for first.
static RESULT M_CheckMainStream(void)
{
    FAIL_IF(
        !m_MainStream.active || m_MainStream.audio_stream_id < 0,
        "no music is playing");
    return OK;
}

static bool M_GetMainTrackState(MUSIC_STREAM_STATE *const state)
{
    if (!m_MainStream.active || state == nullptr) {
        return false;
    }

    state->track_id = MX_INACTIVE;
    state->mode = MPM_ONCE;
    state->timestamp = Audio_Stream_GetTimestamp(m_MainStream.audio_stream_id);

    if (m_TrackCurrent != MX_INACTIVE) {
        state->track_id = m_TrackCurrent;
        return true;
    }
    if (m_TrackLooped != MX_INACTIVE) {
        state->track_id = m_TrackLooped;
        state->mode = MPM_LOOP;
        return true;
    }
    return false;
}

static void M_SeekMainStream(const double timestamp)
{
    if (timestamp > 0.0) {
        SHOULD(Music_SeekTimestamp(timestamp));
    }
}

// Slot 0 is the main stream; slots 1.. are the overlays.
static M_MUSIC_STREAM *M_GetStreamBySlot(const int32_t slot)
{
    if (slot == 0) {
        return &m_MainStream;
    }
    if (slot >= 1 && slot <= MUSIC_MAX_OVERLAY_TRACKS) {
        return &m_OverlayStreams[slot - 1];
    }
    return nullptr;
}

static bool M_IsSpeechTrack(const MUSIC_ID track_id)
{
    switch (Music_FromGameID(track_id)) {
    case MX_BALDY_SPEECH:
    case MX_COWBOY_SPEECH:
    case MX_LARSON_SPEECH:
    case MX_NATLA_SPEECH:
    case MX_PIERRE_SPEECH:
    case MX_SKATEKID_SPEECH:
        return true;
    default:
        return false;
    }
}

static void M_Shutdown(void)
{
    m_Initialised = false;
    memset(m_TrackDurations, 0, sizeof(m_TrackDurations));
    M_StopMainStream();
    M_StopOverlayStreams();
    M_ResetStreamState();
    if (m_Backend != nullptr) {
        m_Backend->shutdown(m_Backend);
        m_Backend = nullptr;
    }
    IGNORE(Audio_Shutdown());
}

static void M_ApplyConfig(void)
{
    SHOULD(Music_Init());
    Music_SetVolume(g_Config.audio.music_volume);
}

// Returns the stream slot the track plays in - the main stream is slot 0, the
// overlays are slots 1.. - or -1 when the track does not play, which includes a
// track marked for later (delay) or a deferred ambient.
static int32_t M_Play(
    const MUSIC_ID track_id, const MUSIC_PLAY_MODE mode, const double timestamp)
{
    if (!m_Initialised) {
        return -1;
    }

    if (M_IsBrokenTrack(track_id)) {
        return -1;
    }

    if (mode == MPM_OVERLAY) {
        LOG_INFO("Playing overlay track %d", track_id);
        return M_PlayOverlayTrack(track_id, timestamp);
    }

    // Already on the main stream, so slot 0 carries it.
    if (track_id == m_TrackCurrent) {
        M_SeekMainStream(timestamp);
        return 0;
    }

    if (mode == MPM_NO_REPEAT && track_id == m_TrackLastPlayed) {
        return -1;
    }

    const bool is_looped = mode == MPM_LOOP || M_IsAmbientTrack(track_id);
    if (is_looped && track_id == m_TrackLastLooped && m_MainStream.active) {
        M_SeekMainStream(timestamp);
        return 0;
    }

    if (mode == MPM_DELAY) {
        m_TrackDelayed = track_id;
        return -1;
    }

    if (is_looped && m_TrackCurrent != MX_INACTIVE) {
        // OG TR3 behaviour: do not interrupt a regular track when the ambient
        // changes; remember the new ambient and restore it when the track ends.
        m_TrackDelayed = MX_INACTIVE;
        m_TrackLooped = track_id;
        m_TrackLastLooped = track_id;
        return -1;
    }

    bool played = false;
    M_StopMainStream();
    if (Shell_GetArgs()->headless) {
        LOG_INFO("Not playing track %d out loud", track_id);
    } else if (m_Backend == nullptr) {
        LOG_WARNING(
            "Not playing track %d because no backend is available", track_id);
    } else {
        LOG_INFO("Playing track %d, mode: %d", track_id, mode);
        const int32_t stream_id = m_Backend->play(m_Backend, track_id);
        if (stream_id < 0) {
            LOG_ERROR("Failed to create music stream for track %d", track_id);
        } else {
            m_MainStream.audio_stream_id = stream_id;
            m_MainStream.track_id = track_id;
            m_MainStream.mode = is_looped ? MPM_LOOP : MPM_ONCE;
            m_MainStream.active = true;
            M_SyncVolume(&m_MainStream);
            SHOULD(Audio_Stream_SetIsLooped(stream_id, is_looped));
            SHOULD(Audio_Stream_SetFinishCallback(
                stream_id, M_StreamFinished, &m_MainStream));
            if (timestamp > 0.0) {
                SHOULD(Audio_Stream_SeekTimestamp(stream_id, timestamp));
            }
            SHOULD(Audio_Stream_Unpause(stream_id));
            played = true;
        }
    }

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
    return played ? 0 : -1;
}

RESULT Music_Init(void)
{
    m_Initialised = true;
    memset(m_TrackDurations, 0, sizeof(m_TrackDurations));
    if (m_Backend != nullptr) {
        m_Backend->shutdown(m_Backend);
        m_Backend = nullptr;
    }
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
    M_ResetStreamState();
    // A run that draws nothing comes up with a backend all the same, so the
    // game still knows which track is playing and what a track resolves to.
    // What such a run has no use for is the audio device.
    if (Shell_GetArgs()->headless) {
        return OK;
    }
    return Audio_Init();
}

int32_t Music_Play_Direct(const MUSIC_ID track_id, const MUSIC_PLAY_MODE mode)
{
    return M_Play(track_id, mode, -1.0);
}

int32_t Music_Play_DirectAt(
    const MUSIC_ID track_id, const MUSIC_PLAY_MODE mode, const double timestamp)
{
    return M_Play(track_id, mode, timestamp);
}

int32_t Music_Play(const MUSIC_TRX_ID track, const MUSIC_PLAY_MODE mode)
{
    return Music_Play_Direct(Music_ToGameID(track), mode);
}

bool Music_IsTrackAvailable_Direct(const MUSIC_ID track)
{
    if (!m_Initialised || m_Backend == nullptr
        || m_Backend->is_track_available == nullptr) {
        return false;
    }
    return m_Backend->is_track_available(m_Backend, track);
}

int32_t Music_GetTrackLimit(void)
{
    if (!m_Initialised || m_Backend == nullptr
        || m_Backend->get_track_limit == nullptr) {
        return 0;
    }
    return m_Backend->get_track_limit(m_Backend);
}

char *Music_GetTrackPath(const MUSIC_ID track)
{
    if (!m_Initialised || m_Backend == nullptr
        || m_Backend->get_track_path == nullptr) {
        return nullptr;
    }
    return m_Backend->get_track_path(m_Backend, track);
}

double Music_GetTrackDuration(const MUSIC_ID track)
{
    if (track < 0 || track >= MAX_MUSIC_TRACKS) {
        return -1.0;
    }
    if (m_TrackDurations[track] != 0.0) {
        return m_TrackDurations[track];
    }

    double duration = -1.0;
    char *const path = Music_GetTrackPath(track);
    if (path != nullptr) {
        AUDIO_DECODER *decoder = nullptr;
        if (SHOULD(AudioDecoder_CreateFromPath(path, 2, &decoder))) {
            duration = AudioDecoder_GetDuration(decoder);
            AudioDecoder_Free(&decoder);
        }
        Memory_Free(path);
    }

    m_TrackDurations[track] = duration > 0.0 ? duration : -1.0;
    return m_TrackDurations[track];
}

void Music_Stop(void)
{
    m_TrackCurrent = MX_INACTIVE;
    m_TrackLastPlayed = MX_INACTIVE;
    m_TrackDelayed = MX_INACTIVE;
    m_TrackLooped = MX_INACTIVE;
    m_TrackLastLooped = MX_INACTIVE;
    M_StopMainStream();
    M_StopOverlayStreams();
    M_ResetStreamState();
}

void Music_StopTrack_Direct(const MUSIC_ID track)
{
    if (track != m_TrackCurrent || M_IsBrokenTrack(track)) {
        return;
    }

    M_StopMainStream();
    m_TrackCurrent = MX_INACTIVE;
    if (m_TrackLooped >= 0) {
        Music_Play_Direct(m_TrackLooped, MPM_LOOP);
    }
}

void Music_Pause(void)
{
    if (m_MainStream.active && m_MainStream.audio_stream_id >= 0) {
        SHOULD(Audio_Stream_Pause(m_MainStream.audio_stream_id));
    }
    for (int32_t i = 0; i < MUSIC_MAX_OVERLAY_TRACKS; i++) {
        if (m_OverlayStreams[i].active
            && m_OverlayStreams[i].audio_stream_id >= 0) {
            SHOULD(Audio_Stream_Pause(m_OverlayStreams[i].audio_stream_id));
        }
    }
}

void Music_Unpause(void)
{
    if (m_MainStream.active && m_MainStream.audio_stream_id >= 0) {
        SHOULD(Audio_Stream_Unpause(m_MainStream.audio_stream_id));
    }
    for (int32_t i = 0; i < MUSIC_MAX_OVERLAY_TRACKS; i++) {
        if (m_OverlayStreams[i].active
            && m_OverlayStreams[i].audio_stream_id >= 0) {
            SHOULD(Audio_Stream_Unpause(m_OverlayStreams[i].audio_stream_id));
        }
    }
}

double Music_GetTimestamp(void)
{
    if (!m_MainStream.active || m_MainStream.audio_stream_id < 0) {
        return -1.0;
    }
    return Audio_Stream_GetTimestamp(m_MainStream.audio_stream_id);
}

RESULT Music_SeekTimestamp(const double timestamp)
{
    MUST(M_CheckMainStream());
    return Audio_Stream_SeekTimestamp(m_MainStream.audio_stream_id, timestamp);
}

RESULT Music_SetSpeed(const double speed)
{
    MUST(M_CheckMainStream());
    return Audio_Stream_SetSpeed(m_MainStream.audio_stream_id, speed);
}

RESULT Music_SyncTimestamp(const double timestamp)
{
    MUST(M_CheckMainStream());
    return Audio_Stream_SyncTimestamp(m_MainStream.audio_stream_id, timestamp);
}

int32_t Music_GetStreamCount(void)
{
    int32_t count = 0;
    if (m_MainStream.active) {
        count++;
    }
    for (int32_t i = 0; i < MUSIC_MAX_OVERLAY_TRACKS; i++) {
        if (m_OverlayStreams[i].active) {
            count++;
        }
    }
    return count;
}

bool Music_GetStreamState(
    const int32_t index, MUSIC_STREAM_STATE *const out_state)
{
    if (index < 0 || out_state == nullptr) {
        return false;
    }

    int32_t stream_index = 0;
    if (m_MainStream.active) {
        if (stream_index == index) {
            return M_GetMainTrackState(out_state);
        }
        stream_index++;
    }

    for (int32_t i = 0; i < MUSIC_MAX_OVERLAY_TRACKS; i++) {
        if (!m_OverlayStreams[i].active) {
            continue;
        }

        if (stream_index == index) {
            out_state->track_id = m_OverlayStreams[i].track_id;
            out_state->mode = m_OverlayStreams[i].mode;
            out_state->timestamp =
                Audio_Stream_GetTimestamp(m_OverlayStreams[i].audio_stream_id);
            return true;
        }
        stream_index++;
    }

    return false;
}

int32_t Music_GetStreamSlotCount(void)
{
    return 1 + MUSIC_MAX_OVERLAY_TRACKS;
}

bool Music_GetStreamSlotState(
    const int32_t slot, MUSIC_STREAM_STATE *const out_state)
{
    if (out_state == nullptr) {
        return false;
    }
    if (slot == 0) {
        return M_GetMainTrackState(out_state);
    }
    const M_MUSIC_STREAM *const stream = M_GetStreamBySlot(slot);
    if (stream == nullptr || !stream->active) {
        return false;
    }
    out_state->track_id = stream->track_id;
    out_state->mode = stream->mode;
    out_state->timestamp = Audio_Stream_GetTimestamp(stream->audio_stream_id);
    return true;
}

void Music_StopStream(const int32_t slot)
{
    if (slot == 0) {
        // Stopping the main one-shot lets the deferred ambient loop resume;
        // when the ambient loop is what plays, there is nothing to resume, so
        // it ends.
        const bool had_current = m_TrackCurrent != MX_INACTIVE;
        const MUSIC_ID looped = m_TrackLooped;
        M_StopMainStream();
        m_TrackCurrent = MX_INACTIVE;
        if (had_current && looped >= 0) {
            Music_Play_Direct(looped, MPM_LOOP);
        } else {
            m_TrackLooped = MX_INACTIVE;
        }
        return;
    }
    M_MUSIC_STREAM *const stream = M_GetStreamBySlot(slot);
    if (stream != nullptr) {
        M_StreamClose(stream);
    }
}

void Music_PauseStream(const int32_t slot)
{
    const M_MUSIC_STREAM *const stream = M_GetStreamBySlot(slot);
    if (stream != nullptr && stream->active && stream->audio_stream_id >= 0) {
        SHOULD(Audio_Stream_Pause(stream->audio_stream_id));
    }
}

void Music_UnpauseStream(const int32_t slot)
{
    const M_MUSIC_STREAM *const stream = M_GetStreamBySlot(slot);
    if (stream != nullptr && stream->active && stream->audio_stream_id >= 0) {
        SHOULD(Audio_Stream_Unpause(stream->audio_stream_id));
    }
}

RESULT Music_SeekStream(const int32_t slot, const double timestamp)
{
    const M_MUSIC_STREAM *const stream = M_GetStreamBySlot(slot);
    FAIL_IF(
        stream == nullptr || !stream->active || stream->audio_stream_id < 0,
        "slot %d holds no music", slot);
    return Audio_Stream_SeekTimestamp(stream->audio_stream_id, timestamp);
}

RESULT Music_SeekTrackTimestamp(
    const MUSIC_ID track_id, const MUSIC_PLAY_MODE mode, const double timestamp)
{
    if (mode == MPM_OVERLAY) {
        for (int32_t i = MUSIC_MAX_OVERLAY_TRACKS - 1; i >= 0; i--) {
            if (!m_OverlayStreams[i].active
                || m_OverlayStreams[i].track_id != track_id) {
                continue;
            }
            return Audio_Stream_SeekTimestamp(
                m_OverlayStreams[i].audio_stream_id, timestamp);
        }
        return FAIL("track %d is not playing as an overlay", track_id);
    }

    MUSIC_STREAM_STATE state = {};
    FAIL_IF(!M_GetMainTrackState(&state), "no music is playing");
    FAIL_IF(
        state.track_id != track_id || state.mode != mode,
        "track %d is not what the music is playing", track_id);
    return Audio_Stream_SeekTimestamp(m_MainStream.audio_stream_id, timestamp);
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
        M_SyncVolumes();
    }
}

void Music_ResetTrackStates(void)
{
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        m_TrackStates[i] = (MUSIC_TRACK_STATE) {};
    }
}

MUSIC_TRACK_STATE *Music_GetTrackState(const MUSIC_ID track_id)
{
    return &m_TrackStates[track_id];
}

void Music_Trigger(MUSIC_ID track_id, const MUSIC_TRIGGER *const trigger)
{
    // An antitrigger aimed at track 0 silences the track that is playing.
    if (track_id == (MUSIC_ID)0 && trigger->kind == MUSIC_TRIGGER_ANTI) {
        Music_Stop();
        return;
    }

    if (track_id <= Music_ToGameID(MX_UNUSED_1) || track_id >= MAX_MUSIC_TRACKS
        || (Game_IsInGym() && !Gym_CanPlayMusicTrack(&track_id))) {
        return;
    }

    if (M_IsAmbientTrack(track_id)) {
        Music_Play_Direct(track_id, MPM_LOOP);
        return;
    }

    MUSIC_PLAY_MODE play_mode = MPM_NO_REPEAT;
    if (g_Config.audio.fix_speeches_killing_music
        && M_IsSpeechTrack(track_id)) {
        play_mode = MPM_OVERLAY;
    }

    MUSIC_TRACK_STATE *const track = &m_TrackStates[track_id];

    if (g_TRVersion == 1) {
        if (track->is_one_shot) {
            return;
        }

        if (trigger->kind == MUSIC_TRIGGER_SWITCH) {
            track->mask ^= trigger->mask;
        } else if (trigger->kind == MUSIC_TRIGGER_ANTI) {
            track->mask &= ~trigger->mask;
        } else {
            track->mask |= trigger->mask;
        }

        if (track->mask == TRIGGER_MASK_ALL) {
            if (trigger->one_shot) {
                track->is_one_shot = true;
            }
            Music_Play_Direct(track_id, play_mode);
        } else {
            Music_StopTrack_Direct(track_id);
        }
        return;
    }

    if (g_TRVersion == 2) {
        if ((track->mask & trigger->mask) != 0) {
            return;
        }

        if (trigger->one_shot) {
            track->mask |= trigger->mask;
        }

        if (trigger->timer == 0) {
            Music_Play_Direct(track_id, play_mode);
            return;
        }

        if (track_id != Music_GetDelayedTrack()) {
            Music_Play_Direct(track_id, MPM_DELAY);
            track->delay = LOGIC_FPS * trigger->timer;
            return;
        }

        if (track->delay == 0) {
            return;
        }

        track->delay--;
        if (track->delay == 0) {
            Music_Play_Direct(track_id, play_mode);
        }

        return;
    }

    {
        if (!Game_IsInGym()) {
            // TR3+ used one-shot as an extra bit together with the other five
            // usual trigger bits. This is used to allow triggering the same
            // track multiple times in a level, but keeping one-shot to mean per
            // unique trigger setup.
            uint8_t trigger_mask = trigger->mask;
            if (trigger->one_shot) {
                trigger_mask |= 1 << 6;
            }

            uint8_t track_mask = track->mask;
            if (track->is_one_shot) {
                track_mask |= 1 << 6;
            }

            if ((track_mask & trigger_mask) == trigger_mask) {
                return;
            }

            track->mask |= trigger->mask;
            track->is_one_shot |= trigger->one_shot;
        }

        Music_Play_Direct(track_id, play_mode);
    }
}

REGISTER_SUBSYSTEM(.apply_config = M_ApplyConfig, .shutdown = M_Shutdown)
