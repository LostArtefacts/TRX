#pragma once

#include <trx/game/input/common.h>

#include <stdbool.h>
#include <stdint.h>

// Generic view of a backend-specific binding for combo operations.
// Backends create these from their concrete binding types.
typedef struct {
    int32_t key_count;
    const void *keys;
    int32_t key_stride;
} INPUT_COMBO_BINDING;

typedef bool (*INPUT_COMBO_KEYS_EQUAL)(const void *a, const void *b);
typedef INPUT_COMBO_BINDING (*INPUT_COMBO_GET_BINDING)(
    INPUT_LAYOUT layout, INPUT_ROLE role, int32_t slot);

static inline const void *Input_ComboKeyAt(
    const INPUT_COMBO_BINDING *b, int32_t idx)
{
    return (const char *)b->keys + idx * b->key_stride;
}

// Check if sub's keys are a proper subset of super's keys.
bool Input_ComboIsProperSubset(
    INPUT_COMBO_BINDING sub, INPUT_COMBO_BINDING super,
    INPUT_COMBO_KEYS_EQUAL keys_equal);

// Check if any binding in the layout is a longer combo that shares the
// same starter key (keys[0]) as bind.
bool Input_ComboHasLonger(
    INPUT_LAYOUT layout, INPUT_ROLE skip_role, INPUT_COMBO_BINDING bind,
    INPUT_COMBO_GET_BINDING get_binding, INPUT_COMBO_KEYS_EQUAL keys_equal);

// Check if a key is the first key of any multi-key combo.
bool Input_ComboIsStarter(
    INPUT_LAYOUT layout, const void *key, INPUT_COMBO_GET_BINDING get_binding,
    INPUT_COMBO_KEYS_EQUAL keys_equal);

// Find the role with a single-key binding matching the given key,
// excluding immediate, sustained, and non-rebindable roles (which are
// never deferred). Returns (INPUT_ROLE)-1 if none found.
INPUT_ROLE Input_ComboFindDeferrableRole(
    INPUT_LAYOUT layout, const void *key, INPUT_COMBO_GET_BINDING get_binding,
    INPUT_COMBO_KEYS_EQUAL keys_equal);

// Check if a key (as a single-key binding) is bound to a rebindable
// immediate role.
bool Input_ComboIsKeyImmediate(
    INPUT_LAYOUT layout, const void *key, INPUT_COMBO_GET_BINDING get_binding,
    INPUT_COMBO_KEYS_EQUAL keys_equal);

// Check if a combo starts with an immediate role's key.
bool Input_ComboStartsWithImmediate(
    INPUT_LAYOUT layout, INPUT_COMBO_BINDING bind,
    INPUT_COMBO_GET_BINDING get_binding, INPUT_COMBO_KEYS_EQUAL keys_equal);

// Check if a combo starts with a non-capturing sustained role's key and
// contains an immediate role's key. Such combos are invalid because the
// immediate action fires before the combo can form.
bool Input_ComboSustainedHasImmediate(
    INPUT_LAYOUT layout, INPUT_COMBO_BINDING bind,
    INPUT_COMBO_GET_BINDING get_binding, INPUT_COMBO_KEYS_EQUAL keys_equal);

// Run combo-specific conflict checks on all roles. Flags bindings that
// start with an immediate key or are invalid sustained+immediate combos.
// Must be called after the normal conflict helper has run.
void Input_ComboCheckConflicts(
    INPUT_LAYOUT layout, INPUT_COMBO_GET_BINDING get_binding,
    INPUT_COMBO_KEYS_EQUAL keys_equal, bool conflicts[INPUT_ROLE_NUMBER_OF]);
