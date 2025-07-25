#pragma once

#include "./control.h"

typedef struct PHASE PHASE;

typedef PHASE_CONTROL (*PHASE_START_FUNC)(PHASE *phase);
typedef void (*PHASE_END_FUNC)(PHASE *phase);
typedef void (*PHASE_SUSPEND_FUNC)(PHASE *phase);
typedef void (*PHASE_RESUME_FUNC)(PHASE *phase);
typedef PHASE_CONTROL (*PHASE_CONTROL_FUNC)(PHASE *phase);
typedef void (*PHASE_DRAW_FUNC)(PHASE *phase);

typedef struct PHASE {
    PHASE_START_FUNC start;
    PHASE_END_FUNC end;
    PHASE_SUSPEND_FUNC suspend;
    PHASE_RESUME_FUNC resume;
    PHASE_CONTROL_FUNC control;
    PHASE_DRAW_FUNC draw;
    void *priv;
} PHASE;
