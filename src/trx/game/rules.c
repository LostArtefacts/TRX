#include <trx/game/rules.h>

#include <trx/version.h>

#include <string.h>

// The markers only shape the struct, so everything below expands them away.
#define X_GROUP_BEGIN(group_)
#define X_GROUP_END(group_)

#define X_RULE(type_, group_, field_, default_) .group_.field_ = default_,

RULES g_Rules = {
#include <trx/game/rules.def>
};

// What a reset puts back. Held apart from g_Rules so a rule whose number
// depends on the game can have its default written before any reset reads it.
static RULES m_Defaults = {
#include <trx/game/rules.def>
};

#undef X_RULE

// clang-format off
static const RULE m_Rules[] = {
#define X_RULE(type_, group_, field_, default_)                                \
    { .name = #group_ "." #field_,                                             \
      .type = Value_TypeOf(g_Rules.group_.field_),                             \
      .target = &g_Rules.group_.field_,                                        \
      .default_value = &m_Defaults.group_.field_ },
#include <trx/game/rules.def>
#undef X_RULE
    {}, // sentinel
};
// clang-format on

#undef X_GROUP_BEGIN
#undef X_GROUP_END

const RULE *Rules_GetMap(void)
{
    return m_Rules;
}

const RULE *Rules_GetByName(const char *const name)
{
    for (const RULE *rule = m_Rules; rule->name != nullptr; rule++) {
        if (strcmp(rule->name, name) == 0) {
            return rule;
        }
    }
    return nullptr;
}

void Rules_ResetOne(const RULE *const rule)
{
    Value_CopyPtr(rule->type, rule->target, rule->default_value);
}

// A rule can ship with a different number from game to game. Rules_Reset runs
// once a level is loaded, so the version has settled by the time this does.
static void M_ApplyGameDefaults(void)
{
    m_Defaults.corpse.fade_speed = g_TRVersion >= 4 ? 2 : 0;
}

void Rules_Reset(void)
{
    M_ApplyGameDefaults();
    for (const RULE *rule = m_Rules; rule->name != nullptr; rule++) {
        Rules_ResetOne(rule);
    }
}
