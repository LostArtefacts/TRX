#pragma once

// Load the game flow from a file.
void GF_LoadFromFile(const char *path);

// Load the game flow from a file.
// Returns false on I/O or parse/validation failure instead of exiting.
bool GF_TryLoadFromFile(const char *path);

// Load and validate path-backed gameflow references for a mod.
// Returns false if parsing fails or any required resolved path is missing.
bool GF_ValidateMod(const char *mod_name, const char *path);
