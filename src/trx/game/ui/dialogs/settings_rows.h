#pragma once

// Which settings a tab holds, and in what order.
//
// Where a setting sits is the dialog's business and not the option's, so the
// arrangement lives here rather than beside the option. A tab is declared as an
// ordered list of names in a .def; the options themselves are made afresh
// whenever the game changes, and the arrangement is rebuilt with them.
//
// A script's option has no .def to be named in, so it says where it goes as it
// is declared - see UI_Settings_AddDeclaredRow.

#include <trx/config/option.h>
#include <trx/game/ui/dialogs/settings_handlers.h>

typedef enum {
    CONFIG_TAB_GAMEPLAY_GENERAL,
    CONFIG_TAB_GAMEPLAY_CONTROLS,
    CONFIG_TAB_GAMEPLAY_MODS,
    CONFIG_TAB_GAMEPLAY_FIXES,
    CONFIG_TAB_GRAPHIC_VISUALS,
    CONFIG_TAB_GRAPHIC_UI,
    CONFIG_TAB_GRAPHIC_UI_STATS,
    CONFIG_TAB_GRAPHIC_UI_BARS,
    CONFIG_TAB_GRAPHIC_RENDERING,
    CONFIG_TAB_SOUND_VOLUME,
    CONFIG_TAB_SOUND_MISC,
    CONFIG_TAB_COUNT,
} CONFIG_TAB;

// A row: the setting it shows, and what that setting does that data cannot say.
// The handler is resolved once with the row rather than looked up per read -
// laying a tab out asks whether a row is hidden once for every row above it,
// and finding a handler is a walk over all of them.
typedef struct {
    CONFIG_OPTION *option;
    const UI_SETTING_HANDLER *handler;
} UI_SETTINGS_ROW;

// How many rows a tab holds. A tab lists rows for every game and keeps the ones
// this game has an option for.
int32_t UI_Settings_GetRowCount(CONFIG_TAB tab);

// A tab's row, by position in the order the tab shows them.
const UI_SETTINGS_ROW *UI_Settings_GetRow(CONFIG_TAB tab, int32_t index);

// The tab a name names, which is the name of the .def its rows are written in.
// False where no tab answers to it.
bool UI_Settings_FindTab(const char *name, CONFIG_TAB *out);

// Adds a row for a setting no .def names, showing the option `key` names.
//
// The row sits directly before or after the row for the option an anchor
// names, and at the end of the tab where neither anchor is given or the option
// named is not one this tab shows. Several rows declared against one anchor
// read in the order they were declared in.
//
// The anchors are read here and not kept: a place is a number from this point
// on, so a row is never left naming one that has gone, and two rows cannot name
// each other.
void UI_Settings_AddDeclaredRow(
    CONFIG_TAB tab, const char *key, const char *before, const char *after);

// Drops every row a script asked for, leaving the ones the .defs name.
void UI_Settings_DropDeclaredRows(void);
