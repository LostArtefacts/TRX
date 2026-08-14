#pragma once

#include <trx/core/file.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    FILE_SEEK_SET,
    FILE_SEEK_CUR,
    FILE_SEEK_END,
} FILE_SEEK_MODE;

typedef enum {
    FILE_OPEN_READ,
    FILE_OPEN_READ_WRITE,
    FILE_OPEN_WRITE,
} FILE_OPEN_MODE;

// A handle to an open file, backed either by a file on disk or by a buffer in
// memory. Reads and seeks work the same way on both.
typedef struct TRX_FILE TRX_FILE;

// Open a file on disk. Returns nullptr if the file cannot be opened.
TRX_FILE *File_OpenPath(const char *path, FILE_OPEN_MODE mode);

// Open a file on disk and read all of it into memory. Only such a handle
// supports views and peeking. Returns nullptr if the file cannot be read.
TRX_FILE *File_OpenPathInMemory(const char *path);

// Open a buffer that is already in memory. The handle keeps its own copy of
// the data.
TRX_FILE *File_OpenBuffer(const char *data, size_t size);

// Open a view onto part of another handle. The bytes are not copied, so the
// parent handle must outlive the view.
TRX_FILE *File_OpenView(TRX_FILE *file, size_t offset, size_t size);

void File_Close(TRX_FILE *file);

// Return the path the handle was opened from, or nullptr for a handle opened
// from a buffer.
const char *File_GetPath(const TRX_FILE *file);

size_t File_Pos(const TRX_FILE *file);
size_t File_Size(const TRX_FILE *file);
size_t File_BytesLeft(const TRX_FILE *file);

void File_Seek(TRX_FILE *file, size_t pos, FILE_SEEK_MODE mode);
bool File_TrySeek(TRX_FILE *file, size_t pos, FILE_SEEK_MODE mode);
void File_Skip(TRX_FILE *file, int32_t bytes);
bool File_TrySkip(TRX_FILE *file, int32_t bytes);

// Controls what happens when a read reaches past the end of the file.
// By default, this is fatal because the plain reads below return values and
// have no way to report failure. With soft failure enabled, reads return zero
// and set an error flag that can be checked later, much like ferror. Once an
// error occurs, further reads do not advance the position.
void File_SetSoftFailure(TRX_FILE *file, bool soft_failure);
bool File_HasFailed(const TRX_FILE *file);
void File_ClearFailure(TRX_FILE *file);

void File_ReadData(TRX_FILE *file, void *data, size_t size);
bool File_TryReadData(TRX_FILE *file, void *data, size_t size);
void File_ReadItems(TRX_FILE *file, void *data, size_t count, size_t item_size);
bool File_TryReadItems(
    TRX_FILE *file, void *data, size_t count, size_t item_size);

int8_t File_ReadS8(TRX_FILE *file);
int16_t File_ReadS16(TRX_FILE *file);
int32_t File_ReadS32(TRX_FILE *file);
uint8_t File_ReadU8(TRX_FILE *file);
uint16_t File_ReadU16(TRX_FILE *file);
uint32_t File_ReadU32(TRX_FILE *file);
float File_ReadFloat(TRX_FILE *file);
double File_ReadDouble(TRX_FILE *file);

bool File_TryReadS8(TRX_FILE *file, int8_t *dst);
bool File_TryReadS16(TRX_FILE *file, int16_t *dst);
bool File_TryReadS32(TRX_FILE *file, int32_t *dst);
bool File_TryReadU8(TRX_FILE *file, uint8_t *dst);
bool File_TryReadU16(TRX_FILE *file, uint16_t *dst);
bool File_TryReadU32(TRX_FILE *file, uint32_t *dst);


// Return the bytes at the cursor for a caller that will read them directly.
// Only a handle with the whole file in memory can provide them; a streamed
// file returns nullptr.
const char *File_PeekBytes(const TRX_FILE *file, size_t *size);

void File_WriteData(TRX_FILE *file, const void *data, size_t size);
void File_WriteItems(
    TRX_FILE *file, const void *data, size_t count, size_t item_size);
void File_WriteS8(TRX_FILE *file, int8_t value);
void File_WriteS16(TRX_FILE *file, int16_t value);
void File_WriteS32(TRX_FILE *file, int32_t value);
void File_WriteU8(TRX_FILE *file, uint8_t value);
void File_WriteU16(TRX_FILE *file, uint16_t value);
void File_WriteU32(TRX_FILE *file, uint32_t value);
void File_WriteString(TRX_FILE *file, const char *fmt, ...);
