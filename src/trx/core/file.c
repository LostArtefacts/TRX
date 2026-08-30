#include <trx/core/file.h>

#include <trx/core/filesystem.h>
#include <trx/core/filesystem/priv.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings/common.h>
#include <trx/core/utils.h>
#include <trx/debug.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#if defined(_WIN32)
    #include <io.h>
#else
    #include <unistd.h>
#endif

#define M_DISK_WINDOW 65536

#define M_DEFINE_READ(name_, type_)                                            \
    type_ name_(TRX_FILE *const file)                                          \
    {                                                                          \
        type_ result;                                                          \
        File_ReadData(file, &result, sizeof(result));                          \
        return result;                                                         \
    }

#define M_DEFINE_TRY_READ(name_, type_)                                        \
    bool name_(TRX_FILE *const file, type_ *const dst)                         \
    {                                                                          \
        return File_TryReadData(file, dst, sizeof(type_));                     \
    }

#define M_DEFINE_WRITE(name_, type_)                                           \
    void name_(TRX_FILE *const file, const type_ value)                        \
    {                                                                          \
        File_WriteData(file, &value, sizeof(value));                           \
    }

typedef struct {
    size_t (*fill)(void *state, size_t offset, const char **out_data);
    bool (*write)(void *state, size_t offset, const void *src, size_t size);
    bool (*set_size)(void *state, size_t size);
    void (*close)(void *state);
} M_STRATEGY;

typedef struct {
    char *data;
    size_t size;
    bool owns_data;
} M_BUFFER_STATE;

typedef struct {
    FILE *fp;
    size_t os_pos;
    char *window;
} M_DISK_STATE;

struct TRX_FILE {
    const M_STRATEGY *strategy;
    void *state;
    char *path;
    size_t pos;
    size_t size;
    size_t base;
    bool contiguous;
    bool soft_failure;
    bool failed;

    const char *window;
    size_t window_start;
    size_t window_size;
};

static size_t M_BufferFill(void *state, size_t offset, const char **out_data);
static void M_BufferClose(void *state);
static size_t M_DiskFill(void *state, size_t offset, const char **out_data);
static bool M_DiskWrite(
    void *state, size_t offset, const void *src, size_t size);
static bool M_DiskSetSize(void *state, size_t size);
static void M_DiskClose(void *state);
static void M_Fail(TRX_FILE *file, const char *message);
static void M_FailPastEnd(TRX_FILE *file, const char *what);
static bool M_EnsureWindow(TRX_FILE *file);
static int32_t M_CheckCount(TRX_FILE *file, int32_t count);

static const M_STRATEGY m_BufferStrategy = {
    .fill = M_BufferFill,
    .write = nullptr,
    .close = M_BufferClose,
};

static const M_STRATEGY m_DiskStrategy = {
    .fill = M_DiskFill,
    .write = M_DiskWrite,
    .set_size = M_DiskSetSize,
    .close = M_DiskClose,
};

static size_t M_BufferFill(
    void *const state, const size_t offset, const char **const out_data)
{
    M_BUFFER_STATE *const buffer = state;
    if (offset >= buffer->size) {
        *out_data = nullptr;
        return 0;
    }
    *out_data = buffer->data + offset;
    return buffer->size - offset;
}

static void M_BufferClose(void *const state)
{
    M_BUFFER_STATE *const buffer = state;
    if (buffer->owns_data) {
        Memory_FreePointer(&buffer->data);
    }
    Memory_Free(buffer);
}

static size_t M_DiskFill(
    void *const state, const size_t offset, const char **const out_data)
{
    M_DISK_STATE *const disk = state;
    if (disk->os_pos != offset) {
        if (fseek(disk->fp, (long)offset, SEEK_SET) != 0) {
            *out_data = nullptr;
            return 0;
        }
        disk->os_pos = offset;
    }
    const size_t read = fread(disk->window, 1, M_DISK_WINDOW, disk->fp);
    disk->os_pos += read;
    *out_data = disk->window;
    return read;
}

static bool M_DiskWrite(
    void *const state, const size_t offset, const void *const src,
    const size_t size)
{
    M_DISK_STATE *const disk = state;
    if (disk->os_pos != offset) {
        if (fseek(disk->fp, (long)offset, SEEK_SET) != 0) {
            return false;
        }
        disk->os_pos = offset;
    }
    if (fwrite(src, 1, size, disk->fp) != size) {
        return false;
    }
    disk->os_pos += size;
    return true;
}

static bool M_DiskSetSize(void *const state, const size_t size)
{
    M_DISK_STATE *const disk = state;
    fflush(disk->fp);
#if defined(_WIN32)
    const bool ok = _chsize_s(_fileno(disk->fp), (int64_t)size) == 0;
#else
    const bool ok = ftruncate(fileno(disk->fp), (off_t)size) == 0;
#endif
    disk->os_pos = 0;
    fseek(disk->fp, 0, SEEK_SET);
    return ok;
}

static void M_DiskClose(void *const state)
{
    M_DISK_STATE *const disk = state;
    fclose(disk->fp);
    Memory_Free(disk->window);
    Memory_Free(disk);
}

static void M_Fail(TRX_FILE *const file, const char *const message)
{
    if (!file->soft_failure) {
        ASSERT_FAIL_FMT("%s", message);
    }
    if (!file->failed) {
        LOG_ERROR("%s", message);
        file->failed = true;
    }
}

static void M_FailPastEnd(TRX_FILE *const file, const char *const what)
{
    M_Fail(
        file,
        String_FormatStatic(
            "%s past the end of %s", what,
            file->path != nullptr ? file->path : "the file"));
}

static bool M_EnsureWindow(TRX_FILE *const file)
{
    if (file->pos >= file->window_start
        && file->pos < file->window_start + file->window_size) {
        return true;
    }
    const char *data = nullptr;
    const size_t filled =
        file->strategy->fill(file->state, file->base + file->pos, &data);
    file->window = data;
    file->window_start = file->pos;
    file->window_size = MIN(filled, file->size - file->pos);
    return file->window_size > 0;
}

static int32_t M_CheckCount(TRX_FILE *const file, const int32_t count)
{
    if (count < 0 || (size_t)count > File_BytesLeft(file)) {
        M_Fail(
            file,
            String_FormatStatic(
                "%d records do not fit in the %zu bytes left of %s", count,
                File_BytesLeft(file),
                file->path != nullptr ? file->path : "the file"));
        return 0;
    }
    return count;
}

RESULT File_OpenPath(
    const char *const path, const FILE_OPEN_MODE mode,
    TRX_FILE **const out_file)
{
    *out_file = nullptr;
    const char *fopen_mode = nullptr;
    switch (mode) {
    case FILE_OPEN_WRITE:
        fopen_mode = "wb";
        break;
    case FILE_OPEN_READ:
        fopen_mode = "rb";
        break;
    case FILE_OPEN_READ_WRITE:
        fopen_mode = "r+b";
        break;
    }
    ASSERT(fopen_mode != nullptr);

    FILE *const fp = FS_PlatformFopen(path, fopen_mode);
    FAIL_IF(
        fp == nullptr, "%s: the file could not be opened: %s", path,
        strerror(errno));

    fseek(fp, 0, SEEK_END);
    const long end = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    M_DISK_STATE *const disk = Memory_Alloc(sizeof(M_DISK_STATE));
    disk->fp = fp;
    disk->os_pos = 0;
    disk->window = Memory_Alloc(M_DISK_WINDOW);

    TRX_FILE *const file = Memory_Alloc(sizeof(TRX_FILE));
    file->strategy = &m_DiskStrategy;
    file->state = disk;
    file->path = Memory_DupStr(path);
    file->size = end < 0 ? 0 : (size_t)end;
    *out_file = file;
    return OK;
}

RESULT File_OpenPathInMemory(const char *const path, TRX_FILE **const out_file)
{
    *out_file = nullptr;
    char *data = nullptr;
    size_t size = 0;
    MUST(FS_Load(path, &data, &size));
    TRX_FILE *const file = File_OpenBuffer(data, size);
    file->path = Memory_DupStr(path);
    Memory_FreePointer(&data);
    *out_file = file;
    return OK;
}

TRX_FILE *File_OpenBuffer(const char *const data, const size_t size)
{
    M_BUFFER_STATE *const buffer = Memory_Alloc(sizeof(M_BUFFER_STATE));
    buffer->data = size > 0 ? Memory_Dup(data, size) : nullptr;
    buffer->size = size;
    buffer->owns_data = true;

    TRX_FILE *const file = Memory_Alloc(sizeof(TRX_FILE));
    file->strategy = &m_BufferStrategy;
    file->state = buffer;
    file->size = size;
    file->contiguous = true;
    return file;
}

TRX_FILE *File_OpenView(
    TRX_FILE *const file, const size_t offset, const size_t size)
{
    ASSERT(file->contiguous);
    ASSERT(offset + size <= file->size);

    M_BUFFER_STATE *const parent = file->state;
    M_BUFFER_STATE *const buffer = Memory_Alloc(sizeof(M_BUFFER_STATE));
    buffer->data = parent->data;
    buffer->size = file->base + offset + size;
    buffer->owns_data = false;

    TRX_FILE *const view = Memory_Alloc(sizeof(TRX_FILE));
    view->strategy = &m_BufferStrategy;
    view->state = buffer;
    view->path = file->path != nullptr ? Memory_DupStr(file->path) : nullptr;
    view->base = file->base + offset;
    view->size = size;
    view->contiguous = true;
    view->soft_failure = file->soft_failure;
    return view;
}

void File_Close(TRX_FILE *const file)
{
    if (file == nullptr) {
        return;
    }
    file->strategy->close(file->state);
    Memory_FreePointer(&file->path);
    Memory_Free(file);
}

const char *File_GetPath(const TRX_FILE *const file)
{
    return file->path;
}

size_t File_Pos(const TRX_FILE *const file)
{
    return file->pos;
}

size_t File_Size(const TRX_FILE *const file)
{
    return file->size;
}

size_t File_BytesLeft(const TRX_FILE *const file)
{
    return file->size - file->pos;
}

void File_SetSoftFailure(TRX_FILE *const file, const bool soft_failure)
{
    file->soft_failure = soft_failure;
}

bool File_HasFailed(const TRX_FILE *const file)
{
    return file->failed;
}

void File_ClearFailure(TRX_FILE *const file)
{
    file->failed = false;
}

bool File_TrySeek(
    TRX_FILE *const file, const size_t pos, const FILE_SEEK_MODE mode)
{
    size_t target;
    switch (mode) {
    case FILE_SEEK_SET:
        target = pos;
        break;
    case FILE_SEEK_CUR:
        target = file->pos + pos;
        break;
    case FILE_SEEK_END:
        target = file->size + pos;
        break;
    default:
        return false;
    }
    if (target > file->size) {
        return false;
    }
    file->pos = target;
    return true;
}

void File_Seek(
    TRX_FILE *const file, const size_t pos, const FILE_SEEK_MODE mode)
{
    if (!File_TrySeek(file, pos, mode)) {
        M_FailPastEnd(file, "seek");
    }
}

bool File_TrySkip(TRX_FILE *const file, const int32_t bytes)
{
    if (bytes < 0 && (size_t)(-(int64_t)bytes) > file->pos) {
        return false;
    }
    return File_TrySeek(file, (size_t)(int64_t)bytes, FILE_SEEK_CUR);
}

void File_Skip(TRX_FILE *const file, const int32_t bytes)
{
    if (!File_TrySkip(file, bytes)) {
        M_FailPastEnd(file, "skip");
    }
}

bool File_TryReadData(TRX_FILE *const file, void *const data, size_t size)
{
    if (size == 0) {
        return true;
    }
    ASSERT(data != nullptr);
    if (size > File_BytesLeft(file)) {
        return false;
    }

    char *dst = data;
    while (size > 0) {
        if (!M_EnsureWindow(file)) {
            return false;
        }
        const size_t offset = file->pos - file->window_start;
        const size_t take = MIN(file->window_size - offset, size);
        memcpy(dst, file->window + offset, take);
        dst += take;
        size -= take;
        file->pos += take;
    }
    return true;
}

void File_ReadData(TRX_FILE *const file, void *const data, const size_t size)
{
    if (size == 0) {
        return;
    }
    if (file->failed) {
        memset(data, 0, size);
        return;
    }
    if (!File_TryReadData(file, data, size)) {
        M_FailPastEnd(file, "read");
        memset(data, 0, size);
    }
}

void File_ReadItems(
    TRX_FILE *const file, void *const data, const size_t count,
    const size_t item_size)
{
    File_ReadData(file, data, count * item_size);
}

bool File_TryReadItems(
    TRX_FILE *const file, void *const data, const size_t count,
    const size_t item_size)
{
    return File_TryReadData(file, data, count * item_size);
}

const char *File_PeekBytes(const TRX_FILE *const file, size_t *const size)
{
    if (!file->contiguous) {
        *size = 0;
        return nullptr;
    }
    const M_BUFFER_STATE *const buffer = file->state;
    *size = File_BytesLeft(file);
    return buffer->data + file->base + file->pos;
}

int32_t File_ReadCountS16(TRX_FILE *const file)
{
    return M_CheckCount(file, File_ReadS16(file));
}

int32_t File_ReadCountS32(TRX_FILE *const file)
{
    return M_CheckCount(file, File_ReadS32(file));
}

RESULT File_SetSize(TRX_FILE *const file, const size_t size)
{
    FAIL_IF(
        file->strategy->set_size == nullptr, "%s cannot be resized",
        file->path != nullptr ? file->path : "a buffer");
    FAIL_IF(
        !file->strategy->set_size(file->state, file->base + size),
        "%s could not be resized to %zu bytes",
        file->path != nullptr ? file->path : "a buffer", size);
    file->size = size;
    file->pos = MIN(file->pos, size);
    file->window_size = 0;
    return OK;
}

void File_WriteData(
    TRX_FILE *const file, const void *const data, const size_t size)
{
    if (file->strategy->write == nullptr) {
        ASSERT_FAIL_FMT(
            "%s cannot be written to",
            file->path != nullptr ? file->path : "a buffer");
    }
    if (!file->strategy->write(
            file->state, file->base + file->pos, data, size)) {
        M_Fail(file, "the write did not go through");
        return;
    }
    file->pos += size;
    file->size = MAX(file->size, file->pos);
    file->window_size = 0;
}

void File_WriteItems(
    TRX_FILE *const file, const void *const data, const size_t count,
    const size_t item_size)
{
    File_WriteData(file, data, count * item_size);
}

void File_WriteString(TRX_FILE *const file, const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    const char *const text = String_FormatStaticV(fmt, va);
    va_end(va);
    File_WriteData(file, text, strlen(text));
}

M_DEFINE_READ(File_ReadS8, int8_t)
M_DEFINE_READ(File_ReadS16, int16_t)
M_DEFINE_READ(File_ReadS32, int32_t)
M_DEFINE_READ(File_ReadU8, uint8_t)
M_DEFINE_READ(File_ReadU16, uint16_t)
M_DEFINE_READ(File_ReadU32, uint32_t)
M_DEFINE_READ(File_ReadFloat, float)
M_DEFINE_READ(File_ReadDouble, double)

M_DEFINE_TRY_READ(File_TryReadS8, int8_t)
M_DEFINE_TRY_READ(File_TryReadS16, int16_t)
M_DEFINE_TRY_READ(File_TryReadS32, int32_t)
M_DEFINE_TRY_READ(File_TryReadU8, uint8_t)
M_DEFINE_TRY_READ(File_TryReadU16, uint16_t)
M_DEFINE_TRY_READ(File_TryReadU32, uint32_t)

M_DEFINE_WRITE(File_WriteS8, int8_t)
M_DEFINE_WRITE(File_WriteS16, int16_t)
M_DEFINE_WRITE(File_WriteS32, int32_t)
M_DEFINE_WRITE(File_WriteU8, uint8_t)
M_DEFINE_WRITE(File_WriteU16, uint16_t)
M_DEFINE_WRITE(File_WriteU32, uint32_t)
