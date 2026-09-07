#include <trx/core/json/util/file.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/log.h>
#include <trx/core/result.h>
#include <trx/core/subsystem.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gun/spec.h>
#include <trx/game/paths.h>

static RESULT M_ReadWeapon(JSON_READ_IO *io, const char *name);
static RESULT M_ReadWeapons(JSON_READ_IO *io);
static RESULT M_LoadFrom(const char *path);
static void M_Init(void);
static RESULT M_Load(void);

static RESULT M_ReadWeapon(JSON_READ_IO *const io, const char *const name)
{
    CATALOG_ID id = Catalog_KeyToID(CATALOG_WEAPONS, name, NO_CATALOG_ID);
    if (id == NO_CATALOG_ID) {
        MUST(Catalog_CreateKey(CATALOG_WEAPONS, name, &id));
    }
    FAIL_IF(
        !Gun_Registry_IsValidType((LARA_GUN_TYPE)id), "'%s' is no weapon",
        name);

    return Gun_Spec_Read(io, Gun_Registry_Get((LARA_GUN_TYPE)id));
}

static RESULT M_ReadWeapons(JSON_READ_IO *const io)
{
    const int32_t count = JSON_ReadIO_GetKeyCount(io);
    FAIL_IF(count < 0, "the file must hold a dictionary");
    for (int32_t i = 0; i < count; i++) {
        const char *const name = JSON_ReadIO_GetKeyAt(io, i);
        MUST(JSON_ReadIO_PushObject(io, name));
        const RESULT result = M_ReadWeapon(io, name);
        MUST(JSON_ReadIO_Pop(io));
        MUST(result, "'%s'", name);
    }
    return OK;
}

static RESULT M_LoadFrom(const char *const path)
{
    JSON_VALUE *root = nullptr;
    MUST(JSONFile_ReadRequired(path, &root));
    JSON_READ_IO *const io = JSON_ReadIO_Create(root, 0, path);
    const RESULT result = M_ReadWeapons(io);
    JSON_ReadIO_Destroy(io);
    JSON_ValueFree(root);
    return result;
}

static void M_Init(void)
{
    Gun_Registry_Seed();
}

static RESULT M_Load(void)
{
    const char *path = nullptr;
    RESULT result = GamePath_Resolve(
        GAME_DYNAMIC_PATH_COMMON_CONFIG, "weapons.json5", &path);
    if (IS_OK(result)) {
        result = M_LoadFrom(path);
    }
    return result;
}

REGISTER_SUBSYSTEM(.init = M_Init, .load = M_Load)
