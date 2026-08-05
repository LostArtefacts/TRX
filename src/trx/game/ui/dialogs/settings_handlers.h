#pragma once

// What a settings row does that cannot be said in data.
//
// Most of what a row needs - what the setting is, what it accepts, what it is
// called - the option itself knows. What is left is behaviour: greying a row
// while the setting it depends on is off, filtering an enum value the level has
// no object for, poking the mixer as a volume moves. None of that is
// expressible as a value, so it stays C.
//
// A handler names the option it belongs to rather than being pointed at by a
// row, so it sits in the file that owns the behaviour, and a row is left saying
// nothing but which setting it shows.

#include <trx/config/option.h>
#include <trx/core/utils.h>

typedef struct {
    // The option this handler is for, by the name the option answers to.
    const char *key;

    // Handed to every callback below. What a handler written in C needs is
    // already in the option; this is for one that is not - a Lua binding
    // registers one handler and carries the reference to the script's own
    // function here.
    void *user_data;

    const char *(*format_value)(const CONFIG_OPTION *option, void *user_data);
    bool (*can_change_value)(
        const CONFIG_OPTION *option, int32_t dir, void *user_data);
    bool (*request_change_value)(
        CONFIG_OPTION *option, int32_t dir, void *user_data);
    bool (*is_available)(const CONFIG_OPTION *option, void *user_data);
    bool (*is_visible)(const CONFIG_OPTION *option, void *user_data);
    bool (*is_enum_value_available)(
        const CONFIG_OPTION *option, int32_t value, void *user_data);

    // How far one press moves a numeric setting, normally and while fine
    // adjustment is held. Zero means one step. This is the dialog's business
    // and not the option's: outside the dialog nothing steps a setting, and
    // what the setting accepts is the option's bounds.
    int32_t delta_slow;
    int32_t delta_fast;

    // The values an enum row cycles through, terminated by -1. Null - which is
    // nearly always - means the order the enum was defined in, which is the
    // order a player reads them in. Only an enum the menu wants shown in some
    // other order needs to say so.
    const int32_t *enum_order;
} UI_SETTING_HANDLER;

void UI_Settings_AddHandler(const UI_SETTING_HANDLER *handler);

// Drops a handler, by the pointer it was added with. A handler a file
// registers never comes here; one a script owns has to, before the script goes,
// or its key stays claimed and registering it again trips AddHandler.
void UI_Settings_RemoveHandler(const UI_SETTING_HANDLER *handler);

// Bumped whenever a handler is added. Anything that resolves an option into a
// handler and keeps the answer has to notice that a later registration - a
// script's - may have changed it.
int32_t UI_Settings_GetHandlerGeneration(void);

// The handler for an option, never null: a row with no handler gets one that
// answers nothing, so a caller reads a field rather than checking twice.
const UI_SETTING_HANDLER *UI_Settings_GetHandler(const CONFIG_OPTION *option);

// Registers a handler as the file is linked, so nothing has to drive a list and
// the order modules initialize in does not matter.
#define REGISTER_UI_SETTING_HANDLER(...)                                       \
    __attribute__((__constructor__)) static void CONCAT(                       \
        M_RegisterUISettingHandler_, __LINE__)(void)                           \
    {                                                                          \
        static const UI_SETTING_HANDLER m_Handler = { __VA_ARGS__ };           \
        UI_Settings_AddHandler(&m_Handler);                                    \
    }
