#pragma once

#include <trx/game/input/common.h>

// Only the backends themselves look inside an event, and this header is what
// anything asking a backend a question has to include - so keep the type
// opaque here rather than pulling SDL2 in with it.
typedef union SDL_Event SDL_Event;

typedef struct {
    void (*init)(void);
    void (*shutdown)(void);
    void (*discover)(void);
    bool (*custom_update)(INPUT_STATE *result, INPUT_LAYOUT layout);
    void (*process_event)(const SDL_Event *event);
    bool (*is_pressed)(INPUT_LAYOUT layout, INPUT_ROLE role);
    bool (*is_role_conflicted)(INPUT_LAYOUT layout, INPUT_ROLE role);
    const char *(*get_name)(INPUT_LAYOUT layout, INPUT_ROLE role, int32_t slot);
    void (*unassign_role)(INPUT_LAYOUT layout, INPUT_ROLE role, int32_t slot);
    bool (*assign_from_json_object)(
        INPUT_LAYOUT layout, INPUT_ROLE role, int32_t slot,
        const JSON_OBJECT *bind_obj);
    bool (*assign_to_json_object)(
        INPUT_LAYOUT layout, INPUT_ROLE role, int32_t slot,
        JSON_OBJECT *bind_obj);
    void (*reset_layout)(INPUT_LAYOUT layout);
    bool (*read_and_assign)(INPUT_LAYOUT layout, INPUT_ROLE role, int32_t slot);
    void (*resolve_combos)(INPUT_LAYOUT layout, INPUT_STATE *result);
} INPUT_BACKEND_IMPL;

const INPUT_BACKEND_IMPL *Input_GetBackendImpl(INPUT_BACKEND backend);
