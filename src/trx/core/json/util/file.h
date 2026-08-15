#pragma once

#include <trx/core/json.h>
#include <trx/core/result.h>

typedef struct JSON_READ_IO JSON_READ_IO;

// Reads and parses a JSON5 file the caller can do without. A file that is not
// there reads as nothing and is no fault; one that does not parse is reported,
// naming the file, the line and the column. Caller frees the value with
// JSON_ValueFree().
RESULT JSONFile_Read(const char *path, JSON_VALUE **out_value);

// Reads and parses a JSON5 file that has to be there, reporting its absence as
// well as anything wrong with what it holds.
RESULT JSONFile_ReadRequired(const char *path, JSON_VALUE **out_value);

// Write a JSON_VALUE to disk (pretty-printed), overwriting only if changed.
// @param path  Path to read.
// @param root  Value to write to the file.
// @return      Returns true if the file was written; false on error or no-op.
// Writes a JSON_VALUE to disk, pretty-printed, leaving the file alone where
// it already holds what would be written. Reports a file it could not open.
RESULT JSONFile_Write(const char *path, JSON_VALUE *root);
