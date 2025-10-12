#pragma once

#include "../inventory_ring/types.h"
#include "./types.h"

GF_COMMAND GF_EnterPhotoMode(void);
GF_COMMAND GF_PauseGame(void);
GF_COMMAND GF_ShowInventory(INVENTORY_MODE inv_mode);
bool GF_ShowInventoryKeys(OBJECT_ID receptacle_type_id);
GF_COMMAND GF_RunTitle(void);
GF_COMMAND GF_RunDemo(int32_t demo_num);
GF_COMMAND GF_RunCutscene(int32_t cutscene_num);
GF_COMMAND GF_RunGame(const GF_LEVEL *level, GF_SEQUENCE_CONTEXT seq_ctx);

GF_COMMAND GF_DoFrontendSequence(void);
GF_COMMAND GF_DoDemoSequence(int32_t demo_num);
GF_COMMAND GF_DoCutsceneSequence(int32_t cutscene_num);

extern GF_COMMAND GF_InterpretSequence(
    const GF_LEVEL *level, GF_SEQUENCE_CONTEXT seq_ctx, void *seq_ctx_arg);

// ----------------------------------------------------------------------------
// event handler stuff
// ----------------------------------------------------------------------------

#define DECLARE_GF_EVENT_HANDLER(name)                                         \
    GF_COMMAND name(                                                           \
        const GF_LEVEL *level, const GF_SEQUENCE_EVENT *event,                 \
        GF_SEQUENCE_CONTEXT seq_ctx, void *seq_ctx_arg)

void GF_SetSequenceEventHandler(
    GF_SEQUENCE_EVENT_TYPE event_type, GF_SEQUENCE_EVENT_HANDLER);
