#pragma once

#include <stdint.h>

// A dynamic enum is a set of string values, discovered at runtime, keyed on an
// opaque token the caller owns - typically the address of the field that holds
// the current value. The registry stores the values and their display labels;
// the caller decides which tokens name a dynamic enum.
void DynamicEnum_ResetValues(const void *token);
bool DynamicEnum_AddValue(
    const void *token, const char *value, const char *label);

// Whether a value is one the caller can be offered right now. A value the
// registry knows but has turned off stays valid to hold and keeps its label -
// what the caller already chose is not taken away from them - and is passed
// over when cycling. Values start out on.
void DynamicEnum_SetValueEnabled(
    const void *token, const char *value, bool enabled);
bool DynamicEnum_IsValueEnabled(const void *token, const char *value);
bool DynamicEnum_IsValidValue(const void *token, const char *value);
int32_t DynamicEnum_GetValueCount(const void *token);
const char *DynamicEnum_GetValueAt(const void *token, int32_t index);
const char *DynamicEnum_GetLabelAt(const void *token, int32_t index);
const char *DynamicEnum_GetLabelForValue(const void *token, const char *value);
bool DynamicEnum_CanCycle(const void *token, const char *current, int32_t dir);
const char *DynamicEnum_GetNext(
    const void *token, const char *current, int32_t dir);
