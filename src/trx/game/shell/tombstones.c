#include <trx/game/shell/tombstones.h>

#include <trx/core/filesystem.h>
#include <trx/core/hash.h>
#include <trx/core/json.h>
#include <trx/core/json/util/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/result.h>
#include <trx/game/paths.h>

#include <stdlib.h>

#define M_TOMBSTONES_PATH "cfg/tombstones.json5"

static RESULT M_HashFile(const char *const path, uint64_t *const out_hash)
{
    AUTO_FREE char *data = nullptr;
    size_t size = 0;
    MUST(FS_Load(path, &data, &size));
    *out_hash = Hash_FNV1a64_Update(Hash_FNV1a64_Init(), data, size);
    return OK;
}

static bool M_IsShipped(const JSON_ARRAY *const hashes, const uint64_t hash)
{
    for (const JSON_ARRAY_ELEMENT *elem = hashes->start; elem != nullptr;
         elem = elem->next) {
        const char *const hex = JSON_ValueGetString(elem->value, nullptr);
        if (hex != nullptr && strtoull(hex, nullptr, 16) == hash) {
            return true;
        }
    }
    return false;
}

static void M_Sweep(const char *const rel_path, const JSON_ARRAY *const hashes)
{
    AUTO_FREE char *joined = GamePath_Join(GAME_PATH_TRX_DIR, rel_path);
    AUTO_FREE char *path = GamePath_ResolveCase(joined);
    if (path == nullptr) {
        return;
    }

    uint64_t hash;
    if (!SHOULD(M_HashFile(path, &hash), "The obsolete file stays in place")) {
        return;
    }
    if (!M_IsShipped(hashes, hash)) {
        LOG_WARNING(
            "%s is no longer part of TRX and has local changes, so it stays "
            "in place. Remove it if TRX misbehaves.",
            path);
        return;
    }
    if (SHOULD(FS_Delete(path), "The obsolete file stays in place")) {
        LOG_INFO("Removed obsolete file: %s", path);
    }
}

void Shell_SweepTombstones(void)
{
    AUTO_FREE char *manifest_path =
        GamePath_Join(GAME_PATH_TRX_DIR, M_TOMBSTONES_PATH);
    if (!FS_Exists(manifest_path)) {
        return;
    }

    JSON_VALUE *root = nullptr;
    if (!SHOULD(
            JSONFile_Read(manifest_path, &root),
            "Obsolete files stay in place")) {
        return;
    }

    const JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    if (root_obj != nullptr) {
        for (const JSON_OBJECT_ELEMENT *elem = root_obj->start; elem != nullptr;
             elem = elem->next) {
            const JSON_ARRAY *const hashes = JSON_ValueAsArray(elem->value);
            if (hashes != nullptr) {
                M_Sweep(elem->name->string, hashes);
            }
        }
    }
    JSON_ValueFree(root);
}
