#pragma once

#include "../event_manager.h"
#include "../json.h"
#include "./option.h"

#include <stdbool.h>
#include <stdint.h>

void Config_Init(void);
void Config_Shutdown(void);

bool Config_Read(void);
bool Config_Write(void);

int32_t Config_SubscribeChanges(EVENT_LISTENER listener, void *user_data);
void Config_UnsubscribeChanges(int32_t listener_id);

extern const char *Config_GetPath(void);

extern void Config_Sanitize(void);
extern void Config_ApplyChanges(void);

extern const CONFIG_OPTION *Config_GetOptionMap(void);

extern void Config_LoadFromJSON(JSON_OBJECT *root_obj);
extern void Config_DumpToJSON(JSON_OBJECT *root_obj);
