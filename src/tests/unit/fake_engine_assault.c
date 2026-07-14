// The gym track manager, reduced to the flags the surface can see, plus the
// record table - which lives in the player's config, not in the level.

#include "fake_engine_assault.h"

#include <trx/config/types.h>
#include <trx/config/vars.h>

#include <stdbool.h>

CONFIG g_Config;
FAKE_ASSAULT_CALLS g_FakeAssaultCalls;

static bool m_InGym;
static bool m_HasStats[GYM_TRACK_NUMBER_OF];
static bool m_Running[GYM_TRACK_NUMBER_OF];
static bool m_Visible[GYM_TRACK_NUMBER_OF];
static GYM_TRACK_TYPE m_ActiveTrack;

bool Game_IsInGym(void)
{
    return m_InGym;
}

bool Config_Update(void)
{
    g_FakeAssaultCalls.config_writes++;
    return true;
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
        return &g_Config.profile.assault_stats;
    case GYM_TRACK_QUAD:
        return &g_Config.profile.racetrack_stats;
    default:
        return nullptr;
    }
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
    g_FakeAssaultCalls.start++;
    g_FakeAssaultCalls.last_track = track;
}

void Gym_TrackManager_Stop(const GYM_TRACK_TYPE track)
{
    g_FakeAssaultCalls.stop++;
    g_FakeAssaultCalls.last_track = track;
}

void Gym_TrackManager_Reset(const GYM_TRACK_TYPE track)
{
    g_FakeAssaultCalls.reset++;
    g_FakeAssaultCalls.last_track = track;
}

void Gym_TrackManager_Finish(const GYM_TRACK_TYPE track)
{
    g_FakeAssaultCalls.finish++;
    g_FakeAssaultCalls.last_track = track;
}

void FakeAssault_Reset(void)
{
    g_Config = (CONFIG) {};
    g_FakeAssaultCalls = (FAKE_ASSAULT_CALLS) {};
    m_InGym = true;
    m_ActiveTrack = GYM_TRACK_NONE;
    for (int32_t i = 0; i < GYM_TRACK_NUMBER_OF; i++) {
        m_HasStats[i] = true;
        m_Running[i] = false;
        m_Visible[i] = false;
    }
}

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

void FakeAssault_SetActiveTrack(const GYM_TRACK_TYPE track)
{
    m_ActiveTrack = track;
}
