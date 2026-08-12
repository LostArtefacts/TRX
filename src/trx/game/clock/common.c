#include <trx/game/clock/common.h>

#include <trx/config.h>
#include <trx/core/subsystem.h>
#include <trx/game/clock/const.h>
#include <trx/game/clock/timer.h>
#include <trx/game/clock/turbo.h>
#include <trx/game/replay/test_recorder.h>
#include <trx/game/replay/test_replay.h>
#include <trx/game/shell/common.h>

#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

static bool m_Disabled = false;
static Uint64 m_LastCounter = 0;
static Uint64 m_InitCounter = 0;
static Uint64 m_Frequency = 0;
static double m_Accumulator = 0.0;
static struct {
    double real_time_at_last_change;
    double sim_time_at_last_change;
    double sim_speed;
} m_Priv;

// A clock that counts frames rather than seconds
static bool m_IsFixedFPS = false;
static double m_FixedFrameTime = 0.0;
static double m_FixedOffset = 0.0;
static double m_FixedAnchor = 0.0;

static double M_GetHighPrecisionCounter(void)
{
    if (m_IsFixedFPS) {
        return m_FixedAnchor + m_FixedOffset;
    }
    return (SDL_GetPerformanceCounter() - m_InitCounter) / (double)m_Frequency;
}

// Holds until the given number of frames is due by the real clock.
static void M_WaitForFrame(const double frames)
{
    const Uint64 now = SDL_GetPerformanceCounter();
    if (m_LastCounter != 0) {
        const double frame_ticks = frames * m_Frequency / Clock_GetCurrentFPS();
        const double elapsed = (double)(now - m_LastCounter);
        if (elapsed < frame_ticks) {
            SDL_Delay(
                (Uint32)(((frame_ticks - elapsed) / m_Frequency) * 1000.0));
        }
    }
    m_LastCounter = SDL_GetPerformanceCounter();
}

static void M_Init(void)
{
    m_Frequency = SDL_GetPerformanceFrequency();
    m_InitCounter = SDL_GetPerformanceCounter();
}

static void M_ApplyConfig(void)
{
    Clock_SetSimSpeed(Clock_GetSpeedMultiplier());

    // Frames rather than seconds, so a fader lasts the same however fast they
    // come. Asked of the replay, not of the arguments: the session swaps to
    // the ones the recording carries, which hold no path.
    if (!TestReplay_IsOpened() && !TestRecorder_IsOpened()) {
        return;
    }
    const SHELL_ARGS *const args = Shell_GetArgs();
    Clock_EnableFixedFPS(
        args->headless_fps > 0 ? args->headless_fps : Clock_GetCurrentFPS());
    if (args->headless) {
        Clock_DisableWait();
    }
}

void Clock_DisableWait(void)
{
    m_Disabled = true;
}

void Clock_EnableWait(void)
{
    m_Disabled = false;
}

void Clock_EnableFixedFPS(const int32_t fps)
{
    if (fps <= 0) {
        if (m_IsFixedFPS) {
            // The real counter picks up where the frame count left off, so a
            // timer spanning the change does not see the clock jump.
            const double frame_now = m_FixedAnchor + m_FixedOffset;
            m_InitCounter = SDL_GetPerformanceCounter()
                - (Uint64)(frame_now * (double)m_Frequency);
        }
        m_IsFixedFPS = false;
        return;
    }

    m_FixedAnchor = M_GetHighPrecisionCounter();
    m_FixedOffset = 0.0;
    m_FixedFrameTime = 1.0 / (double)fps;
    m_IsFixedFPS = true;
}

size_t Clock_GetDateTime(char *const buffer, const size_t size)
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);

    return snprintf(
        buffer, size, "%04d%02d%02d_%02d%02d%02d", tptr->tm_year + 1900,
        tptr->tm_mon + 1, tptr->tm_mday, tptr->tm_hour, tptr->tm_min,
        tptr->tm_sec);
}

int32_t Clock_GetCurrentFPS(void)
{
    return g_Config.rendering.fps;
}

int32_t Clock_GetFrameAdvance(void)
{
    return Clock_GetCurrentFPS() == 30 ? 2 : 1;
}

void Clock_SyncTick(void)
{
    m_LastCounter = SDL_GetPerformanceCounter();
    m_Accumulator = 0.0;
}

int32_t Clock_WaitTick(void)
{
    if (m_IsFixedFPS) {
        // Above 1x the extra speed comes from more frames per drawn one,
        // below it from waiting longer for the single frame. Waiting alone
        // cannot outrun the cost of drawing.
        const double speed = Clock_GetSpeedMultiplier();
        const int32_t frames = speed > 1.0 ? (int32_t)speed : 1;
        m_FixedOffset += frames * m_FixedFrameTime;
        if (!m_Disabled) {
            M_WaitForFrame(frames / speed);
        }
        return frames;
    }
    if (m_Disabled) {
        return 1;
    }
    const Uint64 current_counter = SDL_GetPerformanceCounter();

    // If this is the first call, just initialize and return a frame.
    if (m_LastCounter == 0) {
        m_LastCounter = current_counter;
        return 1;
    }

    const int32_t fps = Clock_GetCurrentFPS();
    const double speed_multiplier = Clock_GetSpeedMultiplier();

    // The duration of one frame in performance counter units
    const double frame_ticks = m_Frequency / (fps * speed_multiplier);

    // Calculate elapsed ticks since last call
    const double elapsed_ticks = (double)(current_counter - m_LastCounter);

    // Add the elapsed ticks to the accumulator
    m_Accumulator += elapsed_ticks;

    // Determine how many frames we can "release" from the accumulator
    int32_t frames = (int32_t)(m_Accumulator / frame_ticks);

    if (frames < 1) {
        // Not enough accumulated time for even one frame

        // Calculate how long we should wait (in ms) to hit the frame boundary
        double needed = frame_ticks - m_Accumulator;
        double delay_ms = (needed / m_Frequency) * 1000.0;

        if (delay_ms > 0) {
            SDL_Delay((Uint32)delay_ms);
        }

        // After waiting, measure again to be accurate
        const Uint64 after_delay_counter = SDL_GetPerformanceCounter();
        const double after_delay_elapsed =
            (double)(after_delay_counter - current_counter);
        m_Accumulator += after_delay_elapsed;

        // Now, we should have at least one frame available
        frames = (int32_t)(m_Accumulator / frame_ticks);
        if (frames < 1) {
            // To avoid a possible floating-point corner case, ensure at least
            // one frame
            frames = 1;
        }
    }

    // Consume the frames from the m_Accumulator
    m_Accumulator -= frames * frame_ticks;

    // Update the last counter to the current performance counter
    m_LastCounter = SDL_GetPerformanceCounter();

    return frames;
}

double Clock_GetRealTime(void)
{
    return M_GetHighPrecisionCounter();
}

double Clock_GetSimTime(void)
{
    const double real_now = M_GetHighPrecisionCounter();
    const double real_delta = real_now - m_Priv.real_time_at_last_change;
    return m_Priv.sim_time_at_last_change + real_delta * m_Priv.sim_speed;
}

void Clock_SetSimSpeed(const double new_speed)
{
    // First, figure out how much sim time has passed so far
    const double prev_sim_time = Clock_GetSimTime();
    // Then re-anchor the reference point
    m_Priv.real_time_at_last_change = M_GetHighPrecisionCounter();
    m_Priv.sim_time_at_last_change = prev_sim_time;
    m_Priv.sim_speed = new_speed;
}

REGISTER_SUBSYSTEM(.init = M_Init, .apply_config = M_ApplyConfig)
