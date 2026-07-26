#pragma once

#include <trx/core/value.h>
#include <trx/game/const.h>

#include <stdint.h>

// The numbers the engine plays by: gameplay values that belong to no single
// object, so they have no place among an object's properties. Levels get them
// back at their defaults, so a script that changes one does not leak into the
// next level.
//
// A rule is one row in rules.def. Engine code reads it off g_Rules by name;
// the console and Lua reach it through the map below.

// clang-format off
typedef struct {
#define X_GROUP_BEGIN(group_) struct {
#define X_RULE(type_, group_, field_, default_) type_ field_;
#define X_GROUP_END(group_) } group_;
#include <trx/game/rules.def>
#undef X_GROUP_BEGIN
#undef X_RULE
#undef X_GROUP_END
} RULES;
// clang-format on

extern RULES g_Rules;

typedef struct {
    const char *name; // the row's group and field, dotted
    TRX_VALUE_TYPE type;
    void *target;
    const void *default_value;
} RULE;

// Terminated by a rule with a null name.
const RULE *Rules_GetMap(void);

const RULE *Rules_GetByName(const char *name);

void Rules_ResetOne(const RULE *rule);
void Rules_Reset(void);
