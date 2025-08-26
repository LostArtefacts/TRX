#include "json_file.h"

#include "filesystem.h"
#include "game/shell.h"
#include "json.h"
#include "log.h"
#include "memory.h"
#include "strings.h"

#include <string.h>

#define M_PARSE_FLAGS JSON_PARSE_FLAGS_ALLOW_JSON5

JSON_VALUE *JSONFile_Read(const char *path)
{
    return JSONFile_ReadEx(path, (JSON_FILE_OPTIONS) {});
}

JSON_VALUE *JSONFile_ReadEx(const char *path, const JSON_FILE_OPTIONS options)
{
    char *file_data = nullptr;
    if (!File_Load(path, &file_data, nullptr)) {
        return nullptr;
    }

    JSON_PARSE_RESULT pr;
    JSON_VALUE *const value = JSON_ParseEx(
        file_data, strlen(file_data), M_PARSE_FLAGS, nullptr, nullptr, &pr);
    if (value == nullptr) {
        const char *const error_msg = String_FormatStatic(
            "parse error in '%s': %s (line %d, char %d)", path,
            JSON_GetErrorDescription(pr.error), pr.error_line_no,
            pr.error_row_no);
        LOG_ERROR("%s", error_msg);
        if (options.exit_on_error) {
            Shell_ExitSystemFmt("%s", error_msg);
        }
    }
    Memory_FreePointer(&file_data);
    return value;
}

bool JSONFile_Write(const char *path, JSON_VALUE *const value)
{
    char *old_data = nullptr;
    File_Load(path, &old_data, nullptr);

    size_t out_len;
    char *out_data = JSON_WritePretty(value, "  ", "\n", &out_len);

    bool updated = false;
    if (old_data == nullptr || strcmp(old_data, out_data) != 0) {
        MYFILE *const fp = File_Open(path, FILE_OPEN_WRITE);
        if (fp == nullptr) {
            LOG_ERROR("unable to open '%s' for writing", path);
        } else {
            LOG_DEBUG("saving JSON to %s", path);
            File_WriteData(fp, out_data, out_len - 1); // w/o \0
            File_Close(fp);
            updated = true;
        }
    }

    Memory_FreePointer(&old_data);
    Memory_FreePointer(&out_data);
    return updated;
}
