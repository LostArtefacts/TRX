#pragma once

#include "game/game_flow/types.h"

typedef GF_COMMAND (*GF_SEQUENCE_EVENT_HANDLER)(
    const GF_LEVEL *, const GF_SEQUENCE_EVENT *, GF_SEQUENCE_CONTEXT, void *);

GF_SEQUENCE_EVENT_HANDLER GF_GetSequenceEventHandler(
    GF_SEQUENCE_EVENT_TYPE event_type);
