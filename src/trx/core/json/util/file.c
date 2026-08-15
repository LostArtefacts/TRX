#include <trx/core/json/util/file.h>

#include <trx/core/file.h>
#include <trx/core/filesystem.h>
#include <trx/core/json.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/shell.h>

#include <string.h>

#define M_PARSE_FLAGS                                                          \
    (JSON_PARSE_FLAGS_ALLOW_JSON5 | JSON_PARSE_FLAGS_ALLOW_LOCATION_INFORMATION)

RESULT JSONFile_Read(const char *const path, JSON_VALUE **const out_value)
{
    *out_value = nullptr;

    // A file that is not there is not a fault; the caller says whether it
    // needed one. One that is there and will not read is another matter.
    if (!FS_Exists(path)) {
        return OK;
    }

    char *file_data = nullptr;
    MUST(FS_Load(path, &file_data, nullptr));

    JSON_PARSE_RESULT pr;
    JSON_VALUE *const value = JSON_ParseEx(
        file_data, strlen(file_data), M_PARSE_FLAGS, nullptr, nullptr, &pr);
    Memory_FreePointer(&file_data);
    if (value == nullptr) {
        return FAIL(
            "%s (line %d, col %d): %s", path, pr.error_line_no, pr.error_row_no,
            JSON_GetErrorDescription(pr.error));
    }

    *out_value = value;
    return OK;
}

RESULT JSONFile_ReadRequired(
    const char *const path, JSON_VALUE **const out_value)
{
    MUST(JSONFile_Read(path, out_value));
    FAIL_IF(*out_value == nullptr, "%s: the file is not there", path);
    return OK;
}

RESULT JSONFile_Write(const char *path, JSON_VALUE *const value)
{
    RESULT result = OK;
    char *old_data = nullptr;
    IGNORE(FS_Load(path, &old_data, nullptr));

    size_t out_len;
    char *out_data = JSON_WritePretty(value, "  ", "\n", &out_len);

    if (old_data == nullptr || strcmp(old_data, out_data) != 0) {
        TRX_FILE *const fp = File_OpenPath(path, FILE_OPEN_WRITE);
        if (fp == nullptr) {
            result = FAIL("%s: the file could not be opened for writing", path);
        } else {
            LOG_DEBUG("saving JSON to %s", path);
            File_WriteData(fp, out_data, out_len - 1); // w/o \0
            File_Close(fp);
        }
    }

    Memory_FreePointer(&old_data);
    Memory_FreePointer(&out_data);
    return result;
}
