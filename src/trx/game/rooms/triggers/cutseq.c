#include <trx/game/cutseq.h>
#include <trx/game/rooms.h>

// TR4 TO_CUTSCENE trigger: starts an in-game cutscene from cutseq.pak.
static void M_Handle(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    CutSeq_HandleTrigger((int16_t)(intptr_t)cmd->parameter);
}

REGISTER_TRIGGER_HANDLER(TO_CUTSCENE, M_Handle)
