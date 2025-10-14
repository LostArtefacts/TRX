#include "game/phase/phase_inventory.h"

#include "config.h"
#include "debug.h"
#include "game/game_flow.h"
#include "game/inventory_ring.h"
#include "game/music.h"
#include "game/output.h"
#include "game/overlay.h"
#include "memory.h"

typedef struct {
    INVENTORY_MODE mode;
    INV_RING *ring;
} M_PRIV;

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;

    const GF_LEVEL *const level = GF_GetTitleLevel();
    if (p->mode == INV_TITLE_MODE && g_Config.audio.enable_music_in_menu
        && level->music_track >= 0) {
        Music_Play_Direct(level->music_track, MPM_LOOPED);
    }

    p->ring = InvRing_Open(p->mode);
    if (p->ring == nullptr) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    ASSERT(p->ring != nullptr);
    const GF_COMMAND gf_cmd = InvRing_Control(p->ring);
    return (PHASE_CONTROL) {
        .action = p->ring->motion.status == RNG_DONE ? PHASE_ACTION_END
                                                     : PHASE_ACTION_CONTINUE,
        .gf_cmd = gf_cmd,
    };
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->mode == INV_TITLE_MODE) {
        Music_Stop();
    }
    if (p->ring != nullptr) {
        InvRing_Close(p->ring);
        p->ring = nullptr;
    }
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    ASSERT(p->ring != nullptr);
    Output_DrawBackground();
    InvRing_Draw(p->ring);
}

PHASE *Phase_Inventory_Create(const INVENTORY_MODE mode)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->mode = mode;
    phase->priv = p;
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    return phase;
}

void Phase_Inventory_Destroy(PHASE *const phase)
{
    Memory_Free(phase->priv);
    Memory_Free(phase);
}
