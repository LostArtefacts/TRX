#include <trx/game/lara/skin/storage.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/enum_map.h>
#include <trx/game/catalog.h>
#include <trx/game/game_string.h>
#include <trx/game/lara.h>
#include <trx/game/shell.h>
#include <trx/json/util/file.h>
#include <trx/memory.h>
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

static bool M_ReadExtraMeshes(JSON_OBJECT *const root_obj)
{
    JSON_OBJECT *const extra_obj =
        JSON_ObjectGetObject(root_obj, "extra_meshes");
    if (extra_obj == nullptr) {
        return false;
    }

    for (JSON_OBJECT_ELEMENT *elem = extra_obj->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        const int32_t type = ENUM_MAP_GET(LARA_SKIN_EXTRA_MESH, name, -1);
        if (type < 0 || type >= NUM_EXTRA_MESHES) {
            Shell_ExitSystemFmt("unknown extra mesh type '%s'", name);
        }
        m_ExtraMeshOffsets[type] = JSON_ValueGetInt(elem->value, 0);
    }

    return true;
}

static bool M_LoadBraid(
    JSON_OBJECT *const outfit_obj, LARA_SKIN_OUTFIT *const outfit)
{
    JSON_OBJECT *const braid_obj = JSON_ObjectGetObject(outfit_obj, "braid");
    if (braid_obj != nullptr) {
        const char *const braid_mode_name =
            JSON_ObjectGetString(braid_obj, "mode", JSON_INVALID_STRING);
        if (braid_mode_name != JSON_INVALID_STRING) {
            const int32_t mode =
                ENUM_MAP_GET(LARA_SKIN_BRAID_MODE, braid_mode_name, -1);
            if (mode < 0 || mode >= NUM_BRAID_MODES) {
                Shell_ExitSystemFmt("unknown braid mode '%s'", braid_mode_name);
                return false;
            }
            outfit->braid.mode = mode;
        }

        outfit->braid.mesh_offset =
            JSON_ObjectGetInt(braid_obj, "mesh_offset", 0);
        outfit->braid.gold_offset =
            JSON_ObjectGetInt(braid_obj, "gold_offset", 0);

        JSON_OBJECT *const pos_obj =
            JSON_ObjectGetObject(braid_obj, "hair_pos");
        if (pos_obj != nullptr) {
            outfit->braid.hair_pos.x = JSON_ObjectGetInt(pos_obj, "x", 0);
            outfit->braid.hair_pos.y = JSON_ObjectGetInt(pos_obj, "y", 0);
            outfit->braid.hair_pos.z = JSON_ObjectGetInt(pos_obj, "z", 0);
        }
        outfit->braid.enabled = true;
    } else {
        outfit->braid.enabled = false;
    }

    return true;
}

static bool M_LoadGunMap(
    JSON_OBJECT *const outfit_obj, LARA_SKIN_OUTFIT *const outfit)
{
    const int32_t map_idx = JSON_ObjectGetInt(outfit_obj, "gun_map", -1);
    if (map_idx < 0 || map_idx >= m_GunMaps->count) {
        Shell_ExitSystemFmt("invalid gun map '%d'", map_idx);
        return false;
    }
    outfit->gun_map = (LARA_SKIN_GUN_MAP *)Vector_Get(m_GunMaps, map_idx);
    return true;
}

static bool M_LoadSFX(
    JSON_OBJECT *const outfit_obj, LARA_SKIN_OUTFIT *const outfit)
{
    const char *const feet_sample_name = JSON_ObjectGetString(
        outfit_obj, "footstep_sample_id", JSON_INVALID_STRING);
    if (feet_sample_name != JSON_INVALID_STRING) {
        CATALOG_ID feet_sample_id;
        if (!Catalog_NameToEnum(
                CATALOG_SAMPLES, feet_sample_name, &feet_sample_id)) {
            Shell_ExitSystemFmt("unknown sample id '%s'", feet_sample_name);
            return false;
        }
        outfit->footstep_sample_id = feet_sample_id;
    } else {
        outfit->footstep_sample_id = SFX_TRX_INVALID;
    }

    return true;
}

static bool M_LoadNoHolsters(
    JSON_OBJECT *const outfit_obj, LARA_SKIN_OUTFIT *const outfit)
{
    JSON_OBJECT *const holster_obj =
        JSON_ObjectGetObject(outfit_obj, "no_holster_offsets");
    if (holster_obj != nullptr) {
        outfit->no_holster_offsets.left =
            JSON_ObjectGetInt(holster_obj, "thigh_l", -1);
        outfit->no_holster_offsets.right =
            JSON_ObjectGetInt(holster_obj, "thigh_r", -1);
    } else {
        outfit->no_holster_offsets.left = -1;
        outfit->no_holster_offsets.right = -1;
    }
    return true;
}

static bool M_LoadExtras(
    JSON_OBJECT *const outfit_obj, LARA_SKIN_OUTFIT *const outfit)
{
    for (int32_t j = 0; j < LS_EXTRA_NUMBER_OF; j++) {
        outfit->extra_outfits[j] = LARA_SKIN_TYPE_DEFAULT;
    }

    JSON_OBJECT *const extra_obj =
        JSON_ObjectGetObject(outfit_obj, "extra_outfits");
    if (extra_obj != nullptr) {
        for (JSON_OBJECT_ELEMENT *elem = extra_obj->start; elem != nullptr;
             elem = elem->next) {
            const char *name = elem->name->string;
            const int32_t state = ENUM_MAP_GET(LARA_EXTRA_STATE, name, -1);
            if (state < 0 || state >= LS_EXTRA_NUMBER_OF) {
                Shell_ExitSystemFmt("unknown Lara extra state '%s'", name);
                return false;
            }

            name = JSON_ValueGetString(elem->value, JSON_INVALID_STRING);
            const LARA_SKIN_TYPE type = Lara_Skin_FindOutfitByName(name);
            if (type < 0 || type >= m_OutfitCount) {
                Shell_ExitSystemFmt("unknown outfit '%s'", name);
                return false;
            }
            outfit->extra_outfits[state] = type;
        }
    }

    return true;
}

static bool M_LoadOutfit(
    JSON_OBJECT *const outfit_obj, LARA_SKIN_OUTFIT *const outfit)
{
    const char *const mesh_obj_name =
        JSON_ObjectGetString(outfit_obj, "mesh_object", JSON_INVALID_STRING);
    if (mesh_obj_name == JSON_INVALID_STRING) {
        Shell_ExitSystemFmt("missing outfit mesh object");
        return false;
    }

    CATALOG_ID mesh_object_id;
    if (!Catalog_NameToEnum(CATALOG_OBJECTS, mesh_obj_name, &mesh_object_id)) {
        Shell_ExitSystemFmt("unknown outfit object_id '%s'", mesh_obj_name);
        return false;
    }
    outfit->obj_id = mesh_object_id;

    outfit->is_reflective =
        JSON_ObjectGetBool(outfit_obj, "is_reflective", false);
    outfit->combat_face_offset =
        JSON_ObjectGetInt(outfit_obj, "combat_face_offset", -1);

    if (!M_LoadBraid(outfit_obj, outfit)) {
        return false;
    }

    if (!M_LoadGunMap(outfit_obj, outfit)) {
        return false;
    }

    if (!M_LoadSFX(outfit_obj, outfit)) {
        return false;
    }

    if (!M_LoadNoHolsters(outfit_obj, outfit)) {
        return false;
    }

    if (!M_LoadExtras(outfit_obj, outfit)) {
        return false;
    }

    outfit->is_defined = true;
    return true;
}

static bool M_ReadOutfits(JSON_OBJECT *const root_obj)
{
    JSON_OBJECT *const outfits_map = JSON_ObjectGetObject(root_obj, "outfits");
    if (outfits_map == nullptr) {
        return false;
    }

    size_t outfit_count = 0;
    for (JSON_OBJECT_ELEMENT *elem = outfits_map->start; elem != nullptr;
         elem = elem->next) {
        outfit_count++;
    }

    if (outfit_count == 0) {
        Shell_ExitSystemFmt("missing outfits in configuration");
        return false;
    }

    m_Outfits = Memory_Alloc(sizeof(*m_Outfits) * outfit_count);
    m_OutfitCount = (int32_t)outfit_count;

    size_t idx = 0;
    for (JSON_OBJECT_ELEMENT *elem = outfits_map->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        JSON_OBJECT *const outfit_obj = JSON_ValueAsObject(elem->value);
        if (outfit_obj == nullptr) {
            Shell_ExitSystemFmt("invalid outfit '%s'", name);
            return false;
        }

        M_OUTFIT_ENTRY *const outfit = &m_Outfits[idx];
        outfit->name = Memory_DupStr(name);

        const char *const name_gs =
            JSON_ObjectGetString(outfit_obj, "name_gs", JSON_INVALID_STRING);
        if (name_gs == JSON_INVALID_STRING) {
            Shell_ExitSystemFmt("invalid '%s.name_gs'", name);
            return false;
        }
        outfit->name_gs = Memory_DupStr(name_gs);

        M_OUTFIT_LOOKUP *existing = nullptr;
        HASH_FIND_STR(m_OutfitLookup, outfit->name, existing);
        if (existing != nullptr) {
            Shell_ExitSystemFmt("duplicate outfit '%s'", name);
            return false;
        }

        M_OUTFIT_LOOKUP *const lookup = Memory_Alloc(sizeof(*lookup));
        lookup->name = outfit->name;
        lookup->index = (int32_t)idx;
        HASH_ADD_KEYPTR(
            hh, m_OutfitLookup, lookup->name, strlen(lookup->name), lookup);
        idx++;
    }

    idx = 0;
    for (JSON_OBJECT_ELEMENT *elem = outfits_map->start; elem != nullptr;
         elem = elem->next) {
        JSON_OBJECT *const outfit_obj = JSON_ValueAsObject(elem->value);
        ASSERT(outfit_obj != nullptr);
        if (!M_LoadOutfit(outfit_obj, &m_Outfits[idx].outfit)) {
            return false;
        }
        idx++;
    }

    return true;
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

    if (!M_ReadExtraMeshes(root)) {
        goto cleanup;
    }

    if (!M_ReadOutfits(root)) {
        goto cleanup;
    }

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
