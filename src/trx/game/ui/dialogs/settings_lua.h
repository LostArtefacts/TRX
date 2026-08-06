#pragma once

// Drops every settings row a script asked for, along with the handler each one
// registered. A game's rows go when the game does, before the next game's
// script runs; see settings_lua.c.
void UI_SettingsLua_DropDeclaredRows(void);
