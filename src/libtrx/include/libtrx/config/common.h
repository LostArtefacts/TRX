#pragma once

#include "../event_manager.h"
#include "../json.h"
#include "./option.h"

#include <stdint.h>

void Config_Init(void);
void Config_Shutdown(void);

bool Config_Read(void);
bool Config_Write(void);

const CONFIG_OPTION *Config_GetOptionMap(void);

int32_t Config_SubscribeChanges(EVENT_LISTENER listener, void *user_data);
void Config_UnsubscribeChanges(int32_t listener_id);

// Returns true if a given setting (a pointer into a g_Config member)
// was forced by the game flow file.
bool Config_IsOptionEnforced(const void *target);
