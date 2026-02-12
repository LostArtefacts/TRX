#include <trx/json/util/file.h>

#include <trx/filesystem.h>
#include <trx/game/shell.h>
#include <trx/json.h>
#include <trx/log.h>
#include <trx/memory.h>
#include <trx/strings.h>

#include <string.h>

#define M_PARSE_FLAGS                                                          \
    (JSON_PARSE_FLAGS_ALLOW_JSON5 | JSON_PARSE_FLAGS_ALLOW_LOCATION_INFORMATION)

static const char *M_FormatErrorMessage(
    const char *const source_path, const JSON_PARSE_RESULT *const pr,
    const bool multiline)
{
    const char *const separator = multiline ? "\n" : " ";
    return String_FormatStatic(
        "Error parsing '%s' (line %d, col %d):%s%s", source_path,
        pr->error_line_no, pr->error_row_no, separator,
        JSON_GetErrorDescription(pr->error));
}

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
        const char *const log_message = M_FormatErrorMessage(path, &pr, false);
        const char *const dialog_message =
            M_FormatErrorMessage(path, &pr, true);
        if (options.exit_on_error) {
            Shell_ExitSystemEx(log_message, dialog_message);
        } else {
            LOG_ERROR("%s", log_message);
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
