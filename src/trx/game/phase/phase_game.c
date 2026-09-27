#include <trx/game/phase/phase_game.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/game/const.h>
#include <trx/game/fader.h>
#include <trx/game/game.h>
#include <trx/game/lua/events.h>
#include <trx/game/output.h>
#include <trx/game/output/overlay.h>
#include <trx/game/sound.h>

#define M_FADE_IN_DURATION (16.0f / (float)LOGIC_FPS)

typedef struct {
    const GF_LEVEL *level;
    GF_SEQUENCE_CONTEXT seq_ctx;
    FADER fade_in;
    struct {
        uint8_t reverb_type;
    } stashed_state;
} M_PRIV;

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (!Game_Start(p->level, p->seq_ctx)) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_EXIT_TO_TITLE },
        };
    }
    Game_SetIsPlaying(true);
    if (g_Config.visuals.enable_level_start_fade_effects) {
        Fader_InitTo(&p->fade_in, 1.0f, 0.0f, M_FADE_IN_DURATION);
    }
    return (PHASE_CONTROL) {
        .action = PHASE_ACTION_CONTINUE,
    };
}

static void M_End(PHASE *const phase)
{
    Game_End();
    Game_SetIsPlaying(false);
    Sound_SetReverbType(0);
}

static void M_Suspend(PHASE *const phase)
{
    Game_SetIsPlaying(false);
    M_PRIV *const p = phase->priv;
    p->stashed_state.reverb_type = Sound_GetReverbType();
    Sound_SetReverbType(0);
}

static void M_Resume(PHASE *const phase)
{
    Game_SetIsPlaying(true);
    M_PRIV *const p = phase->priv;
    Sound_SetReverbType(p->stashed_state.reverb_type);
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    LUA_FireEvent(LUA_EVENT_BEFORE_CONTROL);
    const GF_COMMAND gf_cmd = Game_Control(false);
    LUA_FireEvent(LUA_EVENT_AFTER_CONTROL);
    if (gf_cmd.action != GF_NOOP) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = gf_cmd,
        };
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Game_Draw(true);
    if (Fader_IsActive(&p->fade_in)) {
        Output_Overlay_DrawBlackRectangle(
            Fader_GetCurrentValue(&p->fade_in), true);
    }
}

PHASE *Phase_Game_Create(
    const GF_LEVEL *const level, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->level = level;
    p->seq_ctx = seq_ctx;
    phase->priv = p;
    phase->start = M_Start;
    phase->end = M_End;
    phase->suspend = M_Suspend;
    phase->resume = M_Resume;
    phase->control = M_Control;
    phase->draw = M_Draw;
    return phase;
}

void Phase_Game_Destroy(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Memory_Free(p);
    Memory_Free(phase);
}
