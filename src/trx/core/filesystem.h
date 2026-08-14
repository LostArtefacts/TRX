#pragma once

#include <trx/core/file.h>

#include <stddef.h>
#include <stdint.h>

// Low-level filesystem module.
// Intentionally dumb wrappers over file/dir primitives. No path policy,
// no token expansion, and no case-normalization logic belongs here.

typedef TRX_FILE MYFILE;

// ============================================================================
// Path functions

// Return true when path points to an existing directory.
bool File_DirExists(const char *path);

// Return true if path is absolute for current platform conventions.
bool File_IsAbsolute(const char *path);

// Return true if path is not absolute.
bool File_IsRelative(const char *path);

// Return true when path points to an existing file.
bool File_Exists(const char *path);

// Return the process working directory (owning string), or nullptr.
char *File_GetCurrentDirectory(void);

// Return parent directory component of path (owning string), or nullptr.
char *File_GetParentDirectory(const char *path);

// Return the name of the file path points at, pointing into path itself, or
// nullptr when path is nullptr. Path with no directory in it is its own name.
const char *File_GetBaseName(const char *path);

// Return the name of the file path points at, without its directory and
// without its extension (owning string), or nullptr when path is nullptr.
char *File_GetStem(const char *path);

// Read size and modification time for path without opening it.
bool File_GetMeta(const char *path, uint64_t *out_size, uint64_t *out_mtime);

// Read the whole file at path into a newly allocated, zero-terminated buffer.
// Caller must free it with Memory_FreePointer().
bool File_Load(const char *path, char **output_data, size_t *output_size);

// Delete the file at path. Returns false if it could not be deleted.
bool File_Delete(const char *path);

// ============================================================================
// Directory functions

// Create one directory path component.
void File_CreateDirectory(const char *path);
// Recursively ensure all parent directories for `path` exist.
void File_EnsureParentDirectories(const char *path);

// Directory iteration API.
void *File_OpenDirectory(const char *path);
const char *File_ReadDirectory(void *dir);
void File_CloseDirectory(void *dir);
