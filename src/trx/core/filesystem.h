#pragma once

#include <trx/core/result.h>

#include <stddef.h>
#include <stdint.h>

// Low-level filesystem module.
// Operations on paths and directories only; open file handles belong to
// core/file.h. Path policy, token expansion, and case normalization belong to
// game/paths.h.

// ============================================================================
// Path functions

// Return true when path points to an existing directory.
bool FS_DirExists(const char *path);

// Return true if path is absolute for current platform conventions.
bool FS_IsAbsolute(const char *path);

// Return true if path is not absolute.
bool FS_IsRelative(const char *path);

// Return true when path points to an existing file.
bool FS_Exists(const char *path);

// Return the process working directory (owning string), or nullptr.
char *FS_GetCurrentDirectory(void);

// Return parent directory component of path (owning string), or nullptr.
char *FS_GetParentDirectory(const char *path);

// Return the name of the file path points at, pointing into path itself, or
// nullptr when path is nullptr. Path with no directory in it is its own name.
const char *FS_GetBaseName(const char *path);

// Return the name of the file path points at, without its directory and
// without its extension (owning string), or nullptr when path is nullptr.
char *FS_GetStem(const char *path);

// Read the size and modification time of a path without opening it.
bool FS_GetMeta(const char *path, uint64_t *out_size, uint64_t *out_mtime);

// Reads a whole file into memory, reporting one that cannot be opened or that
// ends early. Caller frees the data with Memory_FreePointer().
RESULT FS_Load(const char *path, char **output_data, size_t *output_size);

// Deletes the file at path, reporting one that will not go.
RESULT FS_Delete(const char *path);

// ============================================================================
// Directory functions

// Creates one directory path component, reporting one that cannot be made. A
// directory that is already there is no fault.
RESULT FS_CreateDirectory(const char *path);
// Recursively ensure all parent directories for `path` exist, reporting the
// first that cannot be made.
RESULT FS_EnsureParentDirectories(const char *path);

typedef struct FS_DIR FS_DIR;

// Directory iteration. FS_OpenDirectory returns nullptr if the directory
// cannot be read. FS_ReadDirectory returns the name of the next entry, or
// nullptr once every entry has been returned. The name stays valid only until
// the next call on the same handle, and the entries include "." and "..".
//
//     FS_DIR *dir = FS_OpenDirectory(path);
//     if (dir != nullptr) {
//         const char *name;
//         while ((name = FS_ReadDirectory(dir)) != nullptr) {
//             ...
//         }
//         FS_CloseDirectory(dir);
//     }
FS_DIR *FS_OpenDirectory(const char *path);
const char *FS_ReadDirectory(FS_DIR *dir);
void FS_CloseDirectory(FS_DIR *dir);
