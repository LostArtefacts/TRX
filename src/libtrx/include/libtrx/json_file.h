#pragma once

#include "json.h"

typedef struct {
    bool exit_on_error;
} JSON_FILE_OPTIONS;

// Read and parse a JSON5 file. Missing files will return nullptr.
// @param path  Path to read.
// @return      The root JSON_VALUE, or nullptr on I/O/parse failure. Caller
//              must free the result with JSON_ValueFree().
JSON_VALUE *JSONFile_Read(const char *path);

// Like JSONFile_Read(), except with additional options.
JSON_VALUE *JSONFile_ReadEx(const char *path, JSON_FILE_OPTIONS options);

// Write a JSON_VALUE to disk (pretty-printed), overwriting only if changed.
// @param path  Path to read.
// @param root  Value to write to the file.
// @return      Returns true if the file was written; false on error or no-op.
bool JSONFile_Write(const char *path, JSON_VALUE *root);
