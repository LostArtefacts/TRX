#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *path;
    const uint8_t *data;
    size_t size;
} LUA_EMBEDDED_SCRIPT;

// Declares part of the trx.* API, and is required by name.
extern const LUA_EMBEDDED_SCRIPT g_LUA_EmbeddedModules[];

// Declares nothing, and is run for its effect once the API is sealed.
extern const LUA_EMBEDDED_SCRIPT g_LUA_EmbeddedRuntimeScripts[];
