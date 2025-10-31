#include "game/savegame/bson.h"

#include "bson.h"
#include "game/savegame.h"
#include "log.h"
#include "memory.h"
#include "utils.h"

#include <zconf.h>
#include <zlib.h>

#define M_MAGIC_TR1X MKTAG('T', '1', 'M', 'B')
#define M_MAGIC_TR2X MKTAG('T', '2', 'X', 'B')
#define M_MAGIC_TRX MKTAG('T', 'R', 'X', 'S')

static JSON_VALUE *M_ParseFromBuffer(
    const char *const buffer, int32_t *const version_out)
{
    const SAVEGAME_BSON_HEADER *const header = (SAVEGAME_BSON_HEADER *)buffer;
    if (header->magic != M_MAGIC_TR1X && header->magic != M_MAGIC_TR2X
        && header->magic != M_MAGIC_TRX) {
        LOG_ERROR("Invalid savegame magic");
        return nullptr;
    }

    if (version_out != nullptr) {
        *version_out = header->version;
    }

    const char *const compressed = buffer + sizeof(SAVEGAME_BSON_HEADER);
    char *uncompressed = Memory_Alloc(header->uncompressed_size);

    uLongf uncompressed_size = header->uncompressed_size;
    const int32_t error_code = uncompress(
        (Bytef *)uncompressed, &uncompressed_size, (const Bytef *)compressed,
        (uLongf)header->compressed_size);
    if (error_code != Z_OK) {
        LOG_ERROR("Failed to decompress the data (error %d)", error_code);
        Memory_FreePointer(&uncompressed);
        return nullptr;
    }

    JSON_VALUE *const root = BSON_Parse(uncompressed, uncompressed_size);
    Memory_FreePointer(&uncompressed);
    return root;
}

bool Savegame_BSON_FillInfo(MYFILE *const fp, SAVEGAME_INFO *const info)
{
    SAVEGAME_BSON_HEADER header;
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, &header, sizeof(SAVEGAME_BSON_HEADER));
    info->initial_version = header.initial_version;
    info->features.restart = header.initial_version >= VERSION_LEGACY;
    info->features.select_level = header.initial_version >= VERSION_1;

    // recover the slot information from the end of the file
    File_Skip(fp, header.compressed_size);
    SAVEGAME_BSON_EXTENDED_HEADER extra_header;
    if (File_ReadData(fp, &extra_header, sizeof(extra_header))) {
        info->counter = extra_header.counter;
        info->level_num = extra_header.level_num;
        info->level_title = Memory_Alloc(extra_header.title_size + 1);
        File_ReadData(fp, info->level_title, extra_header.title_size);
        return true;
    }

    // recover the slot information from the bson structures
    bool result = false;
    File_Seek(fp, 0, FILE_SEEK_SET);
    JSON_VALUE *root = Savegame_BSON_ReadRaw(fp, nullptr);
    JSON_OBJECT *root_obj = JSON_ValueAsObject(root);
    if (root_obj != nullptr) {
        info->counter = JSON_ObjectGetInt(root_obj, "save_counter", -1);
        info->level_num = JSON_ObjectGetInt(root_obj, "level_num", -1);
        const char *level_title =
            JSON_ObjectGetString(root_obj, "level_title", nullptr);
        if (level_title != nullptr) {
            info->level_title = Memory_DupStr(level_title);
        }
        result = info->level_num != -1;
    }
    JSON_ValueFree(root);

    return result;
}

JSON_VALUE *Savegame_BSON_ReadRaw(MYFILE *const fp, int32_t *const version_out)
{
    const size_t buffer_size = File_Size(fp);
    char *buffer = Memory_Alloc(buffer_size);
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, buffer_size);

    JSON_VALUE *const result = M_ParseFromBuffer(buffer, version_out);
    Memory_FreePointer(&buffer);
    return result;
}

bool Savegame_BSON_LoadFromFile(MYFILE *const fp)
{
    bool result = false;

    int32_t version = -1;
    JSON_VALUE *const root = Savegame_BSON_ReadRaw(fp, &version);
    SAVEGAME_BSON_READ_CONTEXT *const ctx = Savegame_BSON_StartRead(root);

    if (!Savegame_BSON_LoadMisc(ctx)) {
        goto cleanup;
    }
    if (!Savegame_BSON_LoadResumeInfoList(ctx, version)) {
        goto cleanup;
    }
    if (!Savegame_BSON_LoadInventory(ctx)) {
        goto cleanup;
    }
    if (!Savegame_BSON_LoadFlipmaps(ctx)) {
        goto cleanup;
    }
    if (!Savegame_BSON_LoadCameras(ctx)) {
        goto cleanup;
    }
    if (!Savegame_BSON_LoadItems(ctx, version)) {
        goto cleanup;
    }
    if (!Savegame_BSON_LoadEffects(ctx)) {
        goto cleanup;
    }
    if (!Savegame_BSON_LoadFlares(ctx)) {
        goto cleanup;
    }
    if (!Savegame_BSON_LoadMusic(ctx, version)) {
        goto cleanup;
    }
    if (!Savegame_BSON_LoadLara(ctx, version)) {
        goto cleanup;
    }

    result = true;

cleanup:
    Savegame_BSON_FinishRead(ctx, result);
    JSON_ValueFree(root);
    return result;
}

bool Savegame_BSON_LoadOnlyResumeInfo(MYFILE *const fp)
{
    int32_t version;
    JSON_VALUE *const root = Savegame_BSON_ReadRaw(fp, &version);
    SAVEGAME_BSON_READ_CONTEXT *const ctx = Savegame_BSON_StartRead(root);
    const bool result = Savegame_BSON_LoadResumeInfoList(ctx, version);
    Savegame_BSON_FinishRead(ctx, result);
    JSON_ValueFree(root);
    return result;
}
