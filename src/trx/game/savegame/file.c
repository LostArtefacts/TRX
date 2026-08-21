#include <trx/game/savegame/file.h>

#include <trx/core/bson.h>
#include <trx/core/file.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/lua/store.h>
#include <trx/game/savegame.h>
#include <trx/game/sparks/manager.h>
#include <trx/version.h>

#include <string.h>
#include <zconf.h>
#include <zlib.h>

#define M_MAGIC_TR1X MKTAG('T', '1', 'M', 'B') // TOOD: remove me after TRX 1.5
#define M_MAGIC_TR2X MKTAG('T', '2', 'X', 'B') // TOOD: remove me after TRX 1.5
#define M_MAGIC_TRX MKTAG('T', 'R', 'X', 'S')

static RESULT M_ParseFromBuffer(
    const char *const buffer, int32_t *const version_out,
    JSON_VALUE **const out_root)
{
    *out_root = nullptr;

    const SAVEGAME_BSON_HEADER *const header = (SAVEGAME_BSON_HEADER *)buffer;
    if (header->magic != M_MAGIC_TR1X && header->magic != M_MAGIC_TR2X
        && header->magic != M_MAGIC_TRX) {
        return FAIL("the file is not a savegame");
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
        Memory_FreePointer(&uncompressed);
        return FAIL(
            "the savegame data would not decompress (error %d)", error_code);
    }

    JSON_VALUE *root = nullptr;
    const RESULT result = BSON_Parse(uncompressed, uncompressed_size, &root);
    Memory_FreePointer(&uncompressed);
    MUST(result, "the savegame data does not read as BSON");
    *out_root = root;
    return OK;
}

static RESULT M_ReadRaw(
    TRX_FILE *const fp, int32_t *const version_out, JSON_VALUE **const out_root)
{
    File_SetSoftFailure(fp, true);
    const size_t buffer_size = File_Size(fp);
    AUTO_FREE char *buffer = Memory_Alloc(buffer_size);
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, buffer_size);
    if (File_HasFailed(fp)) {
        return FAIL("%s: the saved game ends early", File_GetPath(fp));
    }

    const RESULT result = M_ParseFromBuffer(buffer, version_out, out_root);
    return Result_Prefix(result, "%s", File_GetPath(fp));
}

static RESULT M_SaveRaw(
    TRX_FILE *const fp, const JSON_VALUE *const root, const int32_t level_num,
    const bool is_quick)
{
    size_t uncompressed_size = 0;
    char *uncompressed = nullptr;
    MUST(BSON_Write(root, (void **)&uncompressed, &uncompressed_size));

    uLongf compressed_size = compressBound(uncompressed_size);
    char *compressed = Memory_Alloc(compressed_size);
    const int32_t result = compress(
        (Bytef *)compressed, &compressed_size, (const Bytef *)uncompressed,
        (uLongf)uncompressed_size);
    if (result != Z_OK) {
        Memory_FreePointer(&uncompressed);
        Memory_FreePointer(&compressed);
        return FAIL("the savegame data would not compress");
    }

    const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
    const JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    const SAVEGAME_BSON_HEADER header = {
        .magic = M_MAGIC_TRX,
        .initial_version = Savegame_GetInitialVersion(),
        .version = SG_CURRENT_VERSION,
        .compressed_size = compressed_size,
        .uncompressed_size = uncompressed_size,
    };
    const SAVEGAME_BSON_EXTENDED_HEADER extra_header = {
        .flags = Game_GetBonusFlag() | (is_quick ? SAVEGAME_EXT_FLAG_QUICK : 0),
        .counter = JSON_ObjectGetInt(
            root_obj, "save_counter", SG_Manager_GetCounter()),
        .level_num = level->num,
        .title_size = level->title != nullptr ? strlen(level->title) : 0,
    };

    File_WriteData(fp, &header, sizeof(header));
    File_WriteData(fp, compressed, compressed_size);
    File_WriteData(fp, &extra_header, sizeof(extra_header));
    File_WriteData(
        fp, level->title, level->title != nullptr ? strlen(level->title) : 0);

    Memory_FreePointer(&uncompressed);
    Memory_FreePointer(&compressed);
    return OK;
}

static RESULT M_Load(JSON_READ_IO *const io)
{
    MUST(SG_File_LoadResumeInfoList(io));
    MUST(SG_File_LoadMisc(io));
    MUST(SG_File_LoadInventory(io));
    MUST(SG_File_LoadFlipmaps(io));
    MUST(SG_File_LoadCameras(io));
    MUST(SG_File_LoadItems(io));
    MUST(SG_File_LoadEffects(io));
    MUST(SG_File_LoadFX(io));
    MUST(Sparks_Load(io));
    MUST(SG_File_LoadFlares(io));
    MUST(SG_File_LoadMusic(io));
    MUST(SG_File_LoadLara(io));
    MUST(SG_File_LoadRules(io));
    MUST(LUA_Store_Load(io));
    return OK;
}

const char *SG_File_GetSaveFilePattern(void)
{
    return g_GameFlow.savegame_file_fmt;
}

const char *SG_File_GetQuickSaveFilePattern(void)
{
    const char *const pattern = SG_File_GetSaveFilePattern();
    const char *const placeholder = strchr(pattern, '%');
    if (placeholder == nullptr) {
        return String_FormatStatic("%s_q", pattern);
    }
    const int32_t prefix_size = placeholder - pattern;
    return String_FormatStatic("%.*sq%s", prefix_size, pattern, placeholder);
}

RESULT SG_File_LoadFromFile(TRX_FILE *const fp)
{
    int32_t sg_version = -1;
    JSON_VALUE *root = nullptr;
    MUST(M_ReadRaw(fp, &sg_version, &root));
    JSON_READ_IO *const io =
        JSON_ReadIO_Create(root, sg_version, File_GetPath(fp));
    const RESULT result = M_Load(io);
    JSON_ReadIO_Destroy(io);
    JSON_ValueFree(root);
    return result;
}

RESULT SG_File_SaveToFile(TRX_FILE *const fp, SAVEGAME_INFO *const info)
{
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    JSON_WRITE_IO *const io = JSON_WriteIO_Create();

    SG_File_DumpResumeInfoList(io);
    SG_File_DumpInventory(io);
    SG_File_DumpFlipmaps(io);
    SG_File_DumpCameras(io);
    SG_File_DumpItems(io);
    SG_File_DumpEffects(io);
    SG_File_DumpFX(io);
    Sparks_Save(io);
    SG_File_DumpLara(io);
    SG_File_DumpMusic(io);
    SG_File_DumpFlares(io);
    SG_File_DumpRules(io);
    SG_File_DumpMisc(io);
    LUA_Store_Dump(io);

    const RESULT result = M_SaveRaw(
        fp, JSON_WriteIO_GetRoot(io), current_level->num,
        info != nullptr && info->is_quick);
    JSON_WriteIO_Destroy(io);
    return result;
}

RESULT SG_File_FillInfo(TRX_FILE *const fp, SAVEGAME_INFO *const info)
{
    *info = (SAVEGAME_INFO) {};

    SAVEGAME_BSON_HEADER header = {};
    File_Seek(fp, 0, FILE_SEEK_SET);
    FAIL_IF(
        !File_TryReadData(fp, &header, sizeof(SAVEGAME_BSON_HEADER)),
        "%s: the savegame header could not be read", File_GetPath(fp));
    FAIL_IF(
        header.magic != M_MAGIC_TR1X && header.magic != M_MAGIC_TR2X
            && header.magic != M_MAGIC_TRX,
        "%s: the file is not a savegame", File_GetPath(fp));
    FAIL_IF(
        header.version < SG_MIN_SUPPORTED_VERSION,
        "%s: the savegame is version %d, older than the %d TRX reads",
        File_GetPath(fp), header.version, SG_MIN_SUPPORTED_VERSION);
    info->initial_version = header.initial_version;
    info->features.restart = header.initial_version >= SG_VERSION_LEGACY;
    info->features.select_level = header.initial_version >= SG_VERSION_1;

    // recover the slot information from the end of the file
    File_Skip(fp, header.compressed_size);
    SAVEGAME_BSON_EXTENDED_HEADER extra_header;
    if (File_TryReadData(fp, &extra_header, sizeof(extra_header))) {
        info->counter = extra_header.counter;
        info->level_num = extra_header.level_num;
        info->is_quick = (extra_header.flags & SAVEGAME_EXT_FLAG_QUICK) != 0;
        info->level_title = Memory_Alloc(extra_header.title_size + 1);
        File_ReadData(fp, info->level_title, extra_header.title_size);
        return OK;
    }

    // recover the slot information from the savegame structures
    File_Seek(fp, 0, FILE_SEEK_SET);
    JSON_VALUE *root = nullptr;
    MUST(M_ReadRaw(fp, nullptr, &root));
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    RESULT result = OK;
    if (root_obj == nullptr) {
        result =
            FAIL("%s: the savegame holds no root object", File_GetPath(fp));
    } else {
        info->counter = JSON_ObjectGetInt(root_obj, "save_counter", -1);
        info->level_num = JSON_ObjectGetInt(root_obj, "level_num", -1);
        const char *level_title =
            JSON_ObjectGetString(root_obj, "level_title", nullptr);
        if (level_title != nullptr) {
            info->level_title = Memory_DupStr(level_title);
        }
        if (info->level_num == -1) {
            result = FAIL(
                "%s: the savegame names no level number", File_GetPath(fp));
        }
    }
    JSON_ValueFree(root);

    return result;
}

RESULT SG_File_LoadOnlyResumeInfo(TRX_FILE *const fp)
{
    int32_t sg_version = -1;
    JSON_VALUE *root = nullptr;
    MUST(M_ReadRaw(fp, &sg_version, &root));
    JSON_READ_IO *const io =
        JSON_ReadIO_Create(root, sg_version, File_GetPath(fp));
    const RESULT result = SG_File_LoadResumeInfoList(io);
    JSON_ReadIO_Destroy(io);
    JSON_ValueFree(root);
    return result;
}

RESULT SG_File_UpdateDeathCounters(
    TRX_FILE *const fp, int32_t level_num, const int32_t death_count,
    const bool is_quick)
{
    JSON_VALUE *root = nullptr;
    MUST(M_ReadRaw(fp, nullptr, &root));

    RESULT result = OK;
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    if (root_obj == nullptr) {
        result =
            FAIL("%s: the savegame holds no root object", File_GetPath(fp));
        goto cleanup;
    }

    JSON_OBJECT *const misc_obj = JSON_ObjectGetObject(root_obj, "misc");
    if (misc_obj == nullptr) {
        result = FAIL("%s: the savegame holds no 'misc'", File_GetPath(fp));
        goto cleanup;
    }
    JSON_ObjectEvictKey(misc_obj, "death_count");
    JSON_ObjectAppendInt(misc_obj, "death_count", death_count);

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    int32_t resume_idx = -1;
    for (int32_t i = 0; i < level_table->count; i++) {
        if (level_table->levels[i].num == level_num) {
            resume_idx = i;
            break;
        }
    }

    JSON_ARRAY *const resume_arr = JSON_ObjectGetArray(root_obj, "resume_info");
    if (resume_arr != nullptr && resume_idx != -1) {
        JSON_OBJECT *const resume_obj =
            JSON_ArrayGetObject(resume_arr, resume_idx);
        if (resume_obj != nullptr) {
            JSON_ObjectEvictKey(resume_obj, "death_count");
            JSON_ObjectAppendInt(resume_obj, "death_count", death_count);
        }
    }

    File_Seek(fp, 0, FILE_SEEK_SET);
    result = M_SaveRaw(fp, root, level_num, is_quick);

cleanup:
    JSON_ValueFree(root);
    return result;
}
