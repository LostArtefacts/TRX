// The gym track manager, reduced to the flags the surface can see, plus the
// record table - which lives in the player's config, not in the level.

#include <fakes/assault.h>

#include <harness/fake_calls.h>

#include <trx/config/types.h>
#include <trx/config/vars.h>

static bool m_InGym;
static bool m_HasStats[GYM_TRACK_NUMBER_OF];
static bool m_Running[GYM_TRACK_NUMBER_OF];
static bool m_Visible[GYM_TRACK_NUMBER_OF];
static GYM_TRACK_TYPE m_ActiveTrack;
static int32_t m_Timings[GYM_TRACK_NUMBER_OF][FAKE_ASSAULT_TIMING_NUMBER_OF];

static GYM_TRACK_STATS m_AssaultStats = {};
static GYM_TRACK_STATS m_RacetrackStats = {};

static void M_Reset(void)
{
    g_ConfigStorage = (CONFIG) {};
    m_AssaultStats = (GYM_TRACK_STATS) {};
    m_RacetrackStats = (GYM_TRACK_STATS) {};
    m_InGym = true;
    m_ActiveTrack = GYM_TRACK_NONE;
    for (int32_t i = 0; i < GYM_TRACK_NUMBER_OF; i++) {
        for (int32_t j = 0; j < FAKE_ASSAULT_TIMING_NUMBER_OF; j++) {
            m_Timings[i][j] = 0;
        }
    }
    for (int32_t i = 0; i < GYM_TRACK_NUMBER_OF; i++) {
        m_HasStats[i] = true;
        m_Running[i] = false;
        m_Visible[i] = false;
    }
}

static int32_t M_GetTiming(
    const GYM_TRACK_TYPE track, const FAKE_ASSAULT_TIMING timing)
{
    if (track < 0 || track >= GYM_TRACK_NUMBER_OF) {
        return 0;
    }
    return m_Timings[track][timing];
}

bool Game_IsInGym(void)
{
    return m_InGym;
}

// Which game this is decides whether a track has a record table, not which
// level is up. Both tracks have one here; m_HasStats is what turns one off.
bool Gym_TrackManager_HasStats(const GYM_TRACK_TYPE track)
{
    return track >= 0 && track < GYM_TRACK_NUMBER_OF && m_HasStats[track];
}

GYM_TRACK_STATS *Gym_TrackManager_GetMutableStats(const GYM_TRACK_TYPE track)
{
    switch (track) {
    case GYM_TRACK_ASSAULT:
        return &m_AssaultStats;
    case GYM_TRACK_QUAD:
        return &m_RacetrackStats;
    default:
        return nullptr;
    }
}

const GYM_TRACK_STATS *Gym_TrackManager_GetStats(const GYM_TRACK_TYPE track)
{
    return Gym_TrackManager_GetMutableStats(track);
}

int32_t Gym_TrackManager_GetPenaltyFrames(const GYM_TRACK_TYPE track)
{
    return M_GetTiming(track, FAKE_ASSAULT_TIMING_PENALTY);
}

int32_t Gym_TrackManager_GetTargetPenaltyFrames(const GYM_TRACK_TYPE track)
{
    return M_GetTiming(track, FAKE_ASSAULT_TIMING_TARGET_PENALTY);
}

int32_t Gym_TrackManager_GetPenaltyDisplayTimer(const GYM_TRACK_TYPE track)
{
    return M_GetTiming(track, FAKE_ASSAULT_TIMING_PENALTY_TIMER);
}

int32_t Gym_TrackManager_GetLapTime(const GYM_TRACK_TYPE track)
{
    return M_GetTiming(track, FAKE_ASSAULT_TIMING_LAP_TIME);
}

int32_t Gym_TrackManager_GetLapTimeDisplayTimer(const GYM_TRACK_TYPE track)
{
    return M_GetTiming(track, FAKE_ASSAULT_TIMING_LAP_TIMER);
}

GYM_TRACK_TYPE Gym_TrackManager_GetActiveTrackType(void)
{
    return m_ActiveTrack;
}

bool Gym_TrackManager_IsTimerActive(const GYM_TRACK_TYPE track)
{
    return m_Running[track];
}

bool Gym_TrackManager_IsTimerDisplay(const GYM_TRACK_TYPE track)
{
    return m_Visible[track];
}

void Gym_TrackManager_Start(const GYM_TRACK_TYPE track)
{
    FAKE_RECORD("start", FV(track));
}

void Gym_TrackManager_Stop(const GYM_TRACK_TYPE track)
{
    FAKE_RECORD("stop", FV(track));
}

void Gym_TrackManager_Reset(const GYM_TRACK_TYPE track)
{
    FAKE_RECORD("reset", FV(track));
}

void Gym_TrackManager_Finish(const GYM_TRACK_TYPE track)
{
    FAKE_RECORD("finish", FV(track));
}

FAKE_ON_RESET(M_Reset)

void FakeAssault_SetHasStats(const GYM_TRACK_TYPE track, const bool has_stats)
{
    m_HasStats[track] = has_stats;
}

void FakeAssault_SetInGym(const bool in_gym)
{
    m_InGym = in_gym;
}

void FakeAssault_SetRunning(const GYM_TRACK_TYPE track, const bool running)
{
    m_Running[track] = running;
}

void FakeAssault_SetVisible(const GYM_TRACK_TYPE track, const bool visible)
{
    m_Visible[track] = visible;
}

void FakeAssault_SetTiming(
    const GYM_TRACK_TYPE track, const FAKE_ASSAULT_TIMING timing,
    const int32_t frames)
{
    if (track >= 0 && track < GYM_TRACK_NUMBER_OF) {
        m_Timings[track][timing] = frames;
    }
}

void FakeAssault_SetActiveTrack(const GYM_TRACK_TYPE track)
{
    m_ActiveTrack = track;
}
