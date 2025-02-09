#include "game/game_flow/sequencer.h"

#include "debug.h"
#include "enum_map.h"
#include "game/game_flow/sequencer_priv.h"

#define M_MAX_QUEUE_SIZE 10

static GF_COMMAND M_RunEvent(
    const GF_LEVEL *level, const GF_SEQUENCE_EVENT *event,
    GF_SEQUENCE_CONTEXT seq_ctx, void *seq_ctx_arg);
static bool M_PostponeEvent(const GF_SEQUENCE_EVENT *event);
static void M_ResetQueue(GF_EVENT_QUEUE_TYPE queue_type);

typedef struct {
    int32_t count;
    const GF_SEQUENCE_EVENT *events[M_MAX_QUEUE_SIZE];
} M_EVENT_QUEUE;

static M_EVENT_QUEUE m_EventQueues[GF_EVENT_QUEUE_NUMBER_OF] = {};

static GF_COMMAND M_RunEvent(
    const GF_LEVEL *const level, const GF_SEQUENCE_EVENT *const event,
    const GF_SEQUENCE_CONTEXT seq_ctx, void *const seq_ctx_arg)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    LOG_DEBUG(
        "event type=%s(%d) data=0x%x",
        ENUM_MAP_TO_STRING(GF_SEQUENCE_EVENT_TYPE, event->type), event->type,
        event->data);

    const GF_SEQUENCE_EVENT_HANDLER event_handler =
        GF_GetSequenceEventHandler(event->type);
    if (event_handler == nullptr) {
        return gf_cmd;
    }

    gf_cmd = event_handler(level, event, seq_ctx, seq_ctx_arg);
    LOG_DEBUG(
        "event type=%s(%d) data=0x%x finished, result: action=%s, "
        "param=%d",
        ENUM_MAP_TO_STRING(GF_SEQUENCE_EVENT_TYPE, event->type), event->type,
        event->data, ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action),
        gf_cmd.param);
    return gf_cmd;
}

static bool M_PostponeEvent(const GF_SEQUENCE_EVENT *const event)
{
    const GF_EVENT_QUEUE_TYPE queue_type =
        GF_ShouldDeferSequenceEvent(event->type);
    if (queue_type == GF_EVENT_QUEUE_NONE) {
        return false;
    }
    M_EVENT_QUEUE *const queue = &m_EventQueues[queue_type];
    queue->events[queue->count] = event;
    ASSERT(queue->count + 1 < M_MAX_QUEUE_SIZE);
    queue->count++;
    return true;
}

static void M_ResetQueue(const GF_EVENT_QUEUE_TYPE queue_type)
{
    M_EVENT_QUEUE *const queue = &m_EventQueues[queue_type];
    queue->count = 0;
}

GF_COMMAND GF_InterpretSequence(
    const GF_LEVEL *const level, GF_SEQUENCE_CONTEXT seq_ctx,
    void *const seq_ctx_arg)
{
    ASSERT(level != nullptr);
    LOG_DEBUG(
        "running sequence for level=%d type=%d seq_ctx=%d", level->num,
        level->type, seq_ctx);

    GF_PreSequenceHook();

    GF_COMMAND gf_cmd = { .action = GF_EXIT_TO_TITLE };

    const GF_SEQUENCE *const sequence = &level->sequence;
    for (int32_t i = 0; i < sequence->length; i++) {
        const GF_SEQUENCE_EVENT *const event = &sequence->events[i];
        if (GF_ShouldSkipSequenceEvent(level, event)) {
            continue;
        }

        if (M_PostponeEvent(event)) {
            continue;
        }

        // Handle the event
        gf_cmd = M_RunEvent(level, event, seq_ctx, seq_ctx_arg);
        if (gf_cmd.action != GF_NOOP) {
            return gf_cmd;
        }

        // Update sequence context if necessary
        seq_ctx = GF_SwitchSequenceContext(event, seq_ctx);
    }

    LOG_DEBUG(
        "sequence finished: action=%s param=%d",
        ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action), gf_cmd.param);
    return gf_cmd;
}

GF_COMMAND GF_RunSequencerQueue(
    const GF_EVENT_QUEUE_TYPE queue_type, const GF_LEVEL *const level,
    GF_SEQUENCE_CONTEXT seq_ctx, void *const seq_ctx_arg)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    M_EVENT_QUEUE *const queue = &m_EventQueues[queue_type];
    for (int32_t i = 0; i < queue->count; i++) {
        const GF_SEQUENCE_EVENT *const event = queue->events[i];
        gf_cmd = M_RunEvent(level, event, seq_ctx, seq_ctx_arg);
        if (gf_cmd.action != GF_NOOP) {
            return gf_cmd;
        }
    }
    queue->count = 0;
    return gf_cmd;
}
