#pragma once

#include <trx/config/option.h>
#include <trx/config/types.h>
#include <trx/core/json.h>

void Config_LoadFromJSON(JSON_OBJECT *root_obj);
void Config_DumpToJSON(JSON_OBJECT *root_obj);
void Config_Sanitize(void);

// What a write is. Setting an option up is nobody's business; a hold going on
// or coming off moves the live value without the player having chosen
// anything; the rest is the player's own doing and belongs in the file.
typedef enum {
    CONFIG_WRITE_SILENT,
    CONFIG_WRITE_TRANSIENT,
    CONFIG_WRITE_PERSIST,
} CONFIG_WRITE_KIND;

void Config_Option_Init(CONFIG_OPTION *option, const CONFIG_OPTION_DESC *desc);
void Config_Option_Free(CONFIG_OPTION *option);
void Config_Option_WriteAs(
    CONFIG_OPTION *option, const TRX_VALUE *value, CONFIG_WRITE_KIND kind);

// Drops every option there is, for a game being swapped for another.
void Config_DropAllOptions(void);

// Says an option moved, for the report Config_Update spends.
void Config_ReportChange(const CONFIG_OPTION *option, bool persist);
