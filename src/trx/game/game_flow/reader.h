#pragma once

// Load the game flow from a file.
void GF_LoadFromFile(const char *path);

// Load the game flow from a string. Path is optional for error reporting.
// The game will exit in case of errors.
void GF_LoadFromString(const char *script_data, const char *path);
