#include <trx/game/lara/skin/storage.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/enum_map.h>
#include <trx/game/catalog.h>
#include <trx/game/game_string.h>
#include <trx/game/lara.h>
#include <trx/game/shell.h>
#include <trx/json/util/file.h>
#include <trx/json/util/read_io.h>
#include <trx/memory.h>
#include <trx/strings.h>
#include <trx/vector.h>

#include <string.h>
#include <uthash.h>

typedef struct {
    char *name;
    char *name_gs;
    LARA_SKIN_OUTFIT outfit;
} M_OUTFIT_ENTRY;

typedef struct M_OUTFIT_LOOKUP {
    char *name;
    int32_t index;
    UT_hash_handle hh;
} M_OUTFIT_LOOKUP;

static VECTOR *m_GunMaps = nullptr;
static M_OUTFIT_ENTRY *m_Outfits = nullptr;
static int32_t m_OutfitCount = 0;
static M_OUTFIT_LOOKUP *m_OutfitLookup = nullptr;
static int32_t m_ExtraMeshOffsets[NUM_EXTRA_MESHES] = {};

static void M_ExitWithJSONError(
    const char *const source_path, const JSON_READ_IO *const io)
{
    JSONFile_ExitWithReadIOError(
        io, String_FormatStatic("%s: outfits parse error", source_path));
}

static void M_SeedDynamicEnumValues(void)
{
    const CONFIG_OPTION *const option =
        Config_GetOption(&g_Config.visuals.lara_outfit);
    if (option == nullptr) {
        return;
    }

    Config_DynamicEnum_ResetValues(option);
    Config_DynamicEnum_AddValue(option, nullptr, GS_ID(LARA_OUTFIT_DEFAULT));
    for (int32_t i = 0; i < m_OutfitCount; i++) {
        Config_DynamicEnum_AddValue(
            option, m_Outfits[i].name, m_Outfits[i].name_gs);
    }
}

static void M_ResetOutfits(void)
{
    M_OUTFIT_LOOKUP *entry = nullptr;
    M_OUTFIT_LOOKUP *tmp = nullptr;
    HASH_ITER(hh, m_OutfitLookup, entry, tmp)
    {
        HASH_DEL(m_OutfitLookup, entry);
        Memory_FreePointer(&entry);
    }

    if (m_Outfits != nullptr) {
        for (int32_t i = 0; i < m_OutfitCount; i++) {
            Memory_FreePointer(&m_Outfits[i].name);
            Memory_FreePointer(&m_Outfits[i].name_gs);
        }
        Memory_FreePointer(&m_Outfits);
    }

    m_OutfitCount = 0;
    m_OutfitLookup = nullptr;
}

LARA_SKIN_TYPE Lara_Skin_FindOutfitByName(const char *const name)
{
    if (name == nullptr) {
        return LARA_SKIN_TYPE_DEFAULT;
    }

    M_OUTFIT_LOOKUP *entry = nullptr;
    HASH_FIND_STR(m_OutfitLookup, name, entry);
    if (entry == nullptr) {
        return -1;
    }

    return entry->index;
}

LARA_SKIN_TYPE Lara_Skin_GetDefaultType(void)
{
    return m_OutfitCount > 0 ? 0 : LARA_SKIN_TYPE_DEFAULT;
}

static bool M_ReadGunMaps(JSON_OBJECT *const root_obj)
{
    JSON_ARRAY *const maps = JSON_ObjectGetArray(root_obj, "gun_maps");
    if (maps == nullptr) {
        return false;
    }

    for (size_t i = 0; i < maps->length; ++i) {
        JSON_OBJECT *const map_obj = JSON_ArrayGetObject(maps, i);
        LARA_SKIN_GUN_MAP map = {};

        for (int32_t j = 0; j < NUM_WEAPONS; j++) {
            LARA_SKIN_MESH_MAP *const mesh_map = &map.mesh_offsets[j];
            memset(mesh_map, -1, sizeof(LARA_SKIN_MESH_MAP));
            const char *const gun_name =
                EnumMap_ToString(ENUM_MAP_NAME(LARA_GUN_TYPE), j);
            JSON_OBJECT *const gun_obj =
                JSON_ObjectGetObject(map_obj, gun_name);
            if (gun_obj == nullptr) {
                continue;
            }

            mesh_map->hand.right = JSON_ObjectGetInt(gun_obj, "hand_r", -1);
            mesh_map->hand.left = JSON_ObjectGetInt(gun_obj, "hand_l", -1);
            mesh_map->thigh.right = JSON_ObjectGetInt(gun_obj, "thigh_r", -1);
            mesh_map->thigh.left = JSON_ObjectGetInt(gun_obj, "thigh_l", -1);
            mesh_map->torso = JSON_ObjectGetInt(gun_obj, "torso", -1);
        }

        Vector_Add(m_GunMaps, &map);
    }

    return true;
}

static bool M_ReadExtraMeshes(JSON_READ_IO *const io)
{
    if (!JSON_OPTIONAL(JSON_PUSH(io, "extra_meshes"))) {
        return false;
    }

    JSON_OBJECT *const extra_obj = JSON_ReadIO_GetCurrentObject(io);
    if (extra_obj == nullptr) {
        JSON_ReadIO_SetError(io, "'extra_meshes' must be an object");
        JSON_MUST(JSON_POP(io));
        JSON_FAIL();
    }

    for (JSON_OBJECT_ELEMENT *elem = extra_obj->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        const int32_t type = ENUM_MAP_GET(LARA_SKIN_EXTRA_MESH, name, -1);
        if (type < 0 || type >= NUM_EXTRA_MESHES) {
            JSON_ReadIO_SetError(io, "unknown extra mesh type '%s'", name);
            JSON_MUST(JSON_POP(io));
            JSON_FAIL();
        }

        JSON_MUST(JSON_READ(io, name, &m_ExtraMeshOffsets[type]));
    }

    JSON_MUST(JSON_POP(io));
    JSON_FINISH();
}

static bool M_LoadBraid(JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    if (JSON_OPTIONAL(JSON_PUSH(io, "braid"))) {
        const char *braid_mode_name = nullptr;
        if (JSON_OPTIONAL(JSON_READ(io, "mode", &braid_mode_name))) {
            const int32_t mode =
                ENUM_MAP_GET(LARA_SKIN_BRAID_MODE, braid_mode_name, -1);
            if (mode < 0 || mode >= NUM_BRAID_MODES) {
                JSON_ReadIO_SetError(
                    io, "unknown braid mode '%s'", braid_mode_name);
                JSON_MUST(JSON_POP(io));
                JSON_FAIL();
            }
            outfit->braid.mode = mode;
        }

        JSON_READ_D(io, "mesh_offset", &outfit->braid.mesh_offset, 0);
        JSON_READ_D(io, "gold_offset", &outfit->braid.gold_offset, 0);
        JSON_READ_D(io, "hair_pos", &outfit->braid.hair_pos, (XYZ_32) {});
        outfit->braid.enabled = true;
        JSON_MUST(JSON_POP(io));
    } else {
        outfit->braid.enabled = false;
    }

    JSON_FINISH();
}

static bool M_LoadGunMap(JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    int32_t map_idx = -1;
    JSON_READ_D(io, "gun_map", &map_idx, -1);
    if (map_idx < 0 || map_idx >= m_GunMaps->count) {
        JSON_ReadIO_SetError(io, "invalid gun map '%d'", map_idx);
        JSON_FAIL();
    }
    outfit->gun_map = (LARA_SKIN_GUN_MAP *)Vector_Get(m_GunMaps, map_idx);
    JSON_FINISH();
}

static bool M_LoadSFX(JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    const char *feet_sample_name = nullptr;
    if (JSON_OPTIONAL(JSON_READ(io, "footstep_sample_id", &feet_sample_name))) {
        CATALOG_ID feet_sample_id;
        if (!Catalog_NameToEnum(
                CATALOG_SAMPLES, feet_sample_name, &feet_sample_id)) {
            JSON_ReadIO_SetError(
                io, "unknown sample id '%s'", feet_sample_name);
            JSON_FAIL();
        }
        outfit->footstep_sample_id = feet_sample_id;
    } else {
        outfit->footstep_sample_id = SFX_TRX_INVALID;
    }

    JSON_FINISH();
}

static bool M_LoadNoHolsters(
    JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    if (JSON_OPTIONAL(JSON_PUSH(io, "no_holster_offsets"))) {
        JSON_READ_D(io, "thigh_l", &outfit->no_holster_offsets.left, -1);
        JSON_READ_D(io, "thigh_r", &outfit->no_holster_offsets.right, -1);
        JSON_MUST(JSON_POP(io));
    } else {
        outfit->no_holster_offsets.left = -1;
        outfit->no_holster_offsets.right = -1;
    }
    JSON_FINISH();
}

static bool M_LoadExtras(JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    for (int32_t j = 0; j < LS_EXTRA_NUMBER_OF; j++) {
        outfit->extra_outfits[j] = LARA_SKIN_TYPE_DEFAULT;
    }

    if (JSON_OPTIONAL(JSON_PUSH(io, "extra_outfits"))) {
        JSON_OBJECT *const extra_obj = JSON_ReadIO_GetCurrentObject(io);
        if (extra_obj == nullptr) {
            JSON_ReadIO_SetError(io, "'extra_outfits' must be an object");
            JSON_MUST(JSON_POP(io));
            JSON_FAIL();
        }

        for (JSON_OBJECT_ELEMENT *elem = extra_obj->start; elem != nullptr;
             elem = elem->next) {
            const char *const state_name = elem->name->string;
            const int32_t state =
                ENUM_MAP_GET(LARA_EXTRA_STATE, state_name, -1);
            if (state < 0 || state >= LS_EXTRA_NUMBER_OF) {
                JSON_ReadIO_SetError(
                    io, "unknown Lara extra state '%s'", state_name);
                JSON_MUST(JSON_POP(io));
                JSON_FAIL();
            }

            const char *outfit_name = nullptr;
            JSON_MUST(JSON_READ(io, state_name, &outfit_name));
            const LARA_SKIN_TYPE type = Lara_Skin_FindOutfitByName(outfit_name);
            if (type < 0 || type >= m_OutfitCount) {
                JSON_ReadIO_SetError(io, "unknown outfit '%s'", outfit_name);
                JSON_MUST(JSON_POP(io));
                JSON_FAIL();
            }
            outfit->extra_outfits[state] = type;
        }
        JSON_MUST(JSON_POP(io));
    }

    JSON_FINISH();
}

static bool M_LoadOutfit(JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    const char *mesh_obj_name = nullptr;
    JSON_MUST(JSON_READ(io, "mesh_object", &mesh_obj_name));

    CATALOG_ID mesh_object_id;
    if (!Catalog_NameToEnum(CATALOG_OBJECTS, mesh_obj_name, &mesh_object_id)) {
        JSON_ReadIO_SetError(
            io, "unknown outfit object_id '%s'", mesh_obj_name);
        JSON_FAIL();
    }
    outfit->obj_id = mesh_object_id;

    JSON_READ_D(io, "is_reflective", &outfit->is_reflective, false);
    JSON_READ_D(io, "combat_face_offset", &outfit->combat_face_offset, -1);

    JSON_MUST(M_LoadBraid(io, outfit));
    JSON_MUST(M_LoadGunMap(io, outfit));
    JSON_MUST(M_LoadSFX(io, outfit));
    JSON_MUST(M_LoadNoHolsters(io, outfit));
    JSON_MUST(M_LoadExtras(io, outfit));

    outfit->is_defined = true;
    JSON_FINISH();
}

static bool M_ReadOutfits(JSON_READ_IO *const io)
{
    if (!JSON_ReadIO_HasKey(io, "outfits")) {
        return false;
    }
    JSON_MUST(JSON_PUSH(io, "outfits"));

    JSON_OBJECT *const outfits_map = JSON_ReadIO_GetCurrentObject(io);
    if (outfits_map == nullptr) {
        JSON_ReadIO_SetError(io, "'outfits' must be an object");
        JSON_FAIL();
    }

    size_t outfit_count = 0;
    for (JSON_OBJECT_ELEMENT *elem = outfits_map->start; elem != nullptr;
         elem = elem->next) {
        outfit_count++;
    }

    if (outfit_count == 0) {
        JSON_ReadIO_SetError(io, "missing outfits in configuration");
        JSON_MUST(JSON_POP(io));
        JSON_FAIL();
    }

    m_Outfits = Memory_Alloc(sizeof(*m_Outfits) * outfit_count);
    m_OutfitCount = (int32_t)outfit_count;

    size_t idx = 0;
    for (JSON_OBJECT_ELEMENT *elem = outfits_map->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        JSON_MUST(JSON_PUSH(io, name));

        M_OUTFIT_ENTRY *const outfit = &m_Outfits[idx];
        outfit->name = Memory_DupStr(name);

        const char *name_gs = nullptr;
        if (!JSON_READ(io, "name_gs", &name_gs)) {
            JSON_MUST(JSON_POP(io));
            JSON_FAIL();
        }
        outfit->name_gs = Memory_DupStr(name_gs);

        M_OUTFIT_LOOKUP *existing = nullptr;
        HASH_FIND_STR(m_OutfitLookup, outfit->name, existing);
        if (existing != nullptr) {
            JSON_ReadIO_SetError(io, "duplicate outfit '%s'", name);
            JSON_MUST(JSON_POP(io));
            JSON_FAIL();
        }

        M_OUTFIT_LOOKUP *const lookup = Memory_Alloc(sizeof(*lookup));
        lookup->name = outfit->name;
        lookup->index = (int32_t)idx;
        HASH_ADD_KEYPTR(
            hh, m_OutfitLookup, lookup->name, strlen(lookup->name), lookup);
        JSON_MUST(JSON_POP(io));
        idx++;
    }

    idx = 0;
    for (JSON_OBJECT_ELEMENT *elem = outfits_map->start; elem != nullptr;
         elem = elem->next) {
        JSON_MUST(JSON_PUSH(io, elem->name->string));
        if (!M_LoadOutfit(io, &m_Outfits[idx].outfit)) {
            JSON_MUST(JSON_POP(io));
            JSON_FAIL();
        }
        JSON_MUST(JSON_POP(io));
        idx++;
    }

    JSON_MUST(JSON_POP(io));
    JSON_FINISH();
}

void Lara_Skin_LoadFromFile(const char *const path)
{
    if (m_GunMaps != nullptr) {
        Vector_Free(m_GunMaps);
        m_GunMaps = nullptr;
    }
    m_GunMaps = Vector_Create(sizeof(LARA_SKIN_GUN_MAP));

    M_ResetOutfits();
    M_SeedDynamicEnumValues();
    memset(m_ExtraMeshOffsets, 0, sizeof(m_ExtraMeshOffsets));

    LOG_INFO("Reading outfit definitions from %s", path);
    JSON_VALUE *const doc = JSONFile_Read(path);
    JSON_OBJECT *const root = JSON_ValueAsObject(doc);
    if (root == nullptr) {
        goto cleanup;
    }

    if (!M_ReadGunMaps(root)) {
        goto cleanup;
    }

    JSON_READ_IO *const io = JSON_ReadIO_Create(doc, 0, path);

    if (!M_ReadExtraMeshes(io)) {
        const char *const error = JSON_ReadIO_GetError(io);
        if (error != nullptr && error[0] != '\0') {
            M_ExitWithJSONError(path, io);
        }
        JSON_ReadIO_Destroy(io);
        goto cleanup;
    }

    if (!M_ReadOutfits(io)) {
        const char *const error = JSON_ReadIO_GetError(io);
        if (error != nullptr && error[0] != '\0') {
            M_ExitWithJSONError(path, io);
        }
        JSON_ReadIO_Destroy(io);
        goto cleanup;
    }
    JSON_ReadIO_Destroy(io);

    M_SeedDynamicEnumValues();

cleanup:
    JSON_ValueFree(doc);
}

void Lara_Skin_Shutdown(void)
{
    if (m_GunMaps != nullptr) {
        Vector_Free(m_GunMaps);
        m_GunMaps = nullptr;
    }

    M_ResetOutfits();
    M_SeedDynamicEnumValues();
}

int32_t Lara_Skin_GetOutfitCount(void)
{
    return m_OutfitCount;
}

bool Lara_Skin_IsOutfitAvailable(const LARA_SKIN_TYPE skin_type)
{
    return skin_type >= 0 && skin_type < m_OutfitCount
        && m_Outfits[skin_type].outfit.is_defined;
}

const LARA_SKIN_OUTFIT *Lara_Skin_GetOutfit(const LARA_SKIN_TYPE skin_type)
{
    ASSERT(skin_type >= 0 && skin_type < m_OutfitCount);
    return &m_Outfits[skin_type].outfit;
}

const char *Lara_Skin_GetOutfitName(const LARA_SKIN_TYPE skin_type)
{
    if (skin_type < 0 || skin_type >= m_OutfitCount) {
        return nullptr;
    }
    return m_Outfits[skin_type].name;
}

int32_t Lara_Skin_GetExtraMeshOffset(const LARA_SKIN_EXTRA_MESH mesh)
{
    ASSERT(mesh >= 0 && mesh < NUM_EXTRA_MESHES);
    return m_ExtraMeshOffsets[mesh];
}
