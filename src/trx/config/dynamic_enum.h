#pragma once

#include <stdint.h>

// A dynamic enum is a set of string values, discovered at runtime, keyed on an
// opaque token the caller owns - typically the address of the field that holds
// the current value. The registry stores the values and their display labels;
// the caller decides which tokens name a dynamic enum.
void DynamicEnum_ResetValues(const void *token);
bool DynamicEnum_AddValue(
    const void *token, const char *value, const char *label);
bool DynamicEnum_IsValidValue(const void *token, const char *value);
int32_t DynamicEnum_GetValueCount(const void *token);
const char *DynamicEnum_GetValueAt(const void *token, int32_t index);
const char *DynamicEnum_GetLabelAt(const void *token, int32_t index);
const char *DynamicEnum_GetLabelForValue(const void *token, const char *value);
bool DynamicEnum_CanCycle(const void *token, const char *current, int32_t dir);
const char *DynamicEnum_GetNext(
    const void *token, const char *current, int32_t dir);
