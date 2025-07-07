#pragma once

extern void GF_PreSequenceHook(GF_SEQUENCE_CONTEXT seq_ctx, void *seq_ctx_arg);
extern GF_SEQUENCE_CONTEXT GF_SwitchSequenceContext(
    const GF_SEQUENCE_EVENT *event, GF_SEQUENCE_CONTEXT seq_ctx);

GF_SEQUENCE_EVENT_HANDLER GF_GetSequenceEventHandler(
    GF_SEQUENCE_EVENT_TYPE event_type);
