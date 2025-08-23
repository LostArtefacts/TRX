#include "game/fader.h"

#include "config.h"
#include "game/clock.h"
#include "game/output.h"
#include "game/shell.h"
#include "utils.h"

void Fader_InitEx(FADER *const fader, FADER_ARGS args)
{
    if (args.initial == FADER_ANY) {
        args.initial = Fader_GetCurrentValue(fader);

        // Reduce duration proportionally to how close the initial value is to
        // the target.
        double ratio = ABS(args.target - args.initial) / 128.0;
        CLAMP(ratio, 0.0, 1.0);
        args.duration *= ratio;
        if (ratio < 1.0) {
            args.debuff = 0;
        }
    }

    fader->target_drawn = false;
    fader->args = args;
    ClockTimer_Sync(&fader->timer);
}

void Fader_Init(
    FADER *fader, const int32_t initial, const int32_t target,
    const double duration)
{
    Fader_InitEx(
        fader,
        (FADER_ARGS) {
            .initial = initial,
            .target = target,
            .duration = duration,
            .debuff = target == FADER_BLACK ? 3.0 / (double)LOGIC_FPS : 0,
        });
}

int32_t Fader_GetCurrentValue(const FADER *const fader)
{
    if (!g_Config.visuals.enable_fade_effects || fader->args.duration == 0.0) {
        if (fader->args.target == FADER_SEMI_BLACK
            || fader->args.target == FADER_ALMOST_BLACK) {
            return fader->args.target;
        }
        return FADER_TRANSPARENT;
    }
    const double elapsed_time = ClockTimer_PeekElapsed(&fader->timer);
    const double target_time = fader->args.duration;
    double ratio = elapsed_time / target_time;
    CLAMP(ratio, 0.0, 1.0);
    return fader->args.initial
        + (fader->args.target - fader->args.initial) * ratio;
}

bool Fader_IsActive(const FADER *const fader)
{
    if (!g_Config.visuals.enable_fade_effects) {
        return false;
    }
    if (fader->args.duration <= 0.0) {
        return false;
    }
    return !fader->target_drawn;
}

void Fader_Draw(FADER *const fader)
{
    const int32_t current = Fader_GetCurrentValue(fader);
    fader->target_drawn |= current == fader->args.target;
    if (current != 0) {
        Output_DrawBlackRectangle(current);
    }
}
