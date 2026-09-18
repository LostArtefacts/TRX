#pragma once

#include <trx/core/result.h>
#include <trx/game/game_flow/types.h>

typedef RESULT (*GF_SEQUENCE_EVENT_HANDLER)(
    const GF_LEVEL *, const GF_SEQUENCE *, int32_t event_id,
    GF_SEQUENCE_CONTEXT, void *, GF_COMMAND *out_cmd);

GF_SEQUENCE_EVENT_HANDLER GF_GetSequenceEventHandler(
    GF_SEQUENCE_EVENT_TYPE event_type);

// Shows the pending level view after the level is ready.
void GF_ShowPendingLoadingCamera(void);

// Shows the title level view from its sequence.
void GF_ShowTitleLoadingCamera(const GF_LEVEL *level);
