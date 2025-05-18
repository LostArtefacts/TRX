#include "game/phase/phase_inventory.h"

#include "debug.h"
#include "game/inventory_ring.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/text.h"
#include "memory.h"

typedef struct {
    INVENTORY_MODE mode;
    INV_RING *ring;
} M_PRIV;

static PHASE_CONTROL M_Start(PHASE *phase);
static void M_End(PHASE *phase);
static PHASE_CONTROL M_Control(PHASE *phase, int32_t num_frames);
static void M_Draw(PHASE *phase);

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    p->ring = InvRing_Open(p->mode);
    if (p->ring == nullptr) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static PHASE_CONTROL M_Control(PHASE *const phase, int32_t num_frames)
{
    M_PRIV *const p = phase->priv;
    ASSERT(p->ring != nullptr);
    const GF_COMMAND gf_cmd = InvRing_Control(p->ring, num_frames);
    return (PHASE_CONTROL) {
        .action = p->ring->motion.status == RNG_DONE ? PHASE_ACTION_END
                                                     : PHASE_ACTION_CONTINUE,
        .gf_cmd = gf_cmd,
    };
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
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
    Text_Draw();
    Output_DrawPolyList();
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
