#include <trx/game/lara/skin/storage.h>

#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/core/dynamic_enum.h>
#include <trx/core/enum_map.h>
#include <trx/core/json/util/file.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/shell.h>

#include <string.h>
#include <uthash.h>

// The color the shipped golden models are textured in, to the texel.
#define M_DEFAULT_GOLD_COLOR ((RGB_888) { 0xFF, 0xEE, 0x8B })

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
        Config_FindOptionByMirror(&g_Config.visuals.lara_outfit);
    if (option == nullptr) {
        return;
    }
    const void *const token = Config_Option_GetEnumKey(option);
    DynamicEnum_ResetValues(token);
    DynamicEnum_AddValue(
        token, nullptr, GS_ID("dynamic/enums/lara_outfit/default"));
    for (int32_t i = 0; i < m_OutfitCount; i++) {
        if (!m_Outfits[i].outfit.is_selectable) {
            continue;
        }
        DynamicEnum_AddValue(token, m_Outfits[i].name, m_Outfits[i].name_gs);
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

static RESULT M_ReadGunMaps(JSON_READ_IO *const io)
{
    MUST(JSON_PUSH(io, "gun_maps"));

    const int32_t map_count = JSON_ARRAY_LEN(io);
    if (map_count < 0) {
        return JSON_ReadIO_Fail(io, "a list was expected");
    }

    for (int32_t i = 0; i < map_count; ++i) {
        MUST(JSON_PUSH_INDEX(io, i));
        if (JSON_ReadIO_GetCurrentObject(io) == nullptr) {
            MUST(JSON_POP(io));
            return JSON_ReadIO_Fail(io, "gun map %d must be an object", i);
        }
        LARA_SKIN_GUN_MAP map = {};
        for (int32_t j = 0; j < NUM_WEAPONS; j++) {
            memset(&map.mesh_offsets[j], -1, sizeof(LARA_SKIN_MESH_MAP));
        }

        JSON_OBJECT *const map_obj = JSON_ReadIO_GetCurrentObject(io);
        for (JSON_OBJECT_ELEMENT *elem = map_obj->start; elem != nullptr;
             elem = elem->next) {
            const char *const name = elem->name->string;
            const int32_t type = ENUM_MAP_GET(LARA_GUN_TYPE, name, -1);
            if (type < 0 || type >= NUM_WEAPONS) {
                return JSON_ReadIO_Fail(
                    io, "gun map %d names an unknown weapon '%s'", i, name);
            }

            LARA_SKIN_MESH_MAP *const mesh_map = &map.mesh_offsets[type];
            MUST(JSON_PUSH(io, name));
            MUST(JSON_READ_OPT(io, "hand_r", &mesh_map->hand.right));
            MUST(JSON_READ_OPT(io, "hand_l", &mesh_map->hand.left));
            MUST(JSON_READ_OPT(io, "thigh_r", &mesh_map->thigh.right));
            MUST(JSON_READ_OPT(io, "thigh_l", &mesh_map->thigh.left));
            MUST(JSON_READ_OPT(io, "torso", &mesh_map->torso));
            MUST(JSON_POP(io));
        }

        Vector_Add(m_GunMaps, &map);
        MUST(JSON_POP(io));
    }

    MUST(JSON_POP(io));
    return OK;
}

static RESULT M_ReadExtraMeshes(JSON_READ_IO *const io)
{
    if (!JSON_ReadIO_HasKey(io, "extra_meshes")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "extra_meshes"));

    JSON_OBJECT *const extra_obj = JSON_ReadIO_GetCurrentObject(io);
    if (extra_obj == nullptr) {
        return JSON_ReadIO_Fail(io, "'extra_meshes' must be an object");
    }

    for (JSON_OBJECT_ELEMENT *elem = extra_obj->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        const int32_t type = ENUM_MAP_GET(LARA_SKIN_EXTRA_MESH, name, -1);
        if (type < 0 || type >= NUM_EXTRA_MESHES) {
            MUST(JSON_POP(io));
            return JSON_ReadIO_Fail(io, "unknown extra mesh type '%s'", name);
        }

        MUST(JSON_READ(io, name, &m_ExtraMeshOffsets[type]));
    }

    MUST(JSON_POP(io));
    return OK;
}

static RESULT M_LoadBraidHeadSeam(
    JSON_READ_IO *const io, LARA_SKIN_BRAID_HEAD_SEAM *const seam)
{
    if (!JSON_ReadIO_HasKey(io, "head_seam")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "head_seam"));

    const int32_t pairs = MIN(JSON_ARRAY_LEN(io), SEAM_MAX_VERTEX_PAIRS);
    for (int32_t i = 0; i < pairs; i++) {
        MUST(JSON_PUSH_INDEX(io, i));
        int32_t seg_vertex = 0;
        int32_t head_vertex = 0;
        MUST(JSON_READ_A(io, 0, &seg_vertex));
        MUST(JSON_READ_A(io, 1, &head_vertex));
        seam->pairs[seam->count].vertex_a = seg_vertex;
        seam->pairs[seam->count].vertex_b = head_vertex;
        seam->count++;
        MUST(JSON_POP(io));
    }

    MUST(JSON_POP(io));
    return OK;
}

static RESULT M_LoadBraid(
    JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    outfit->braid.count = 0;
    outfit->braid.auto_enabled = false;

    if (!JSON_ReadIO_HasKey(io, "braid")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "braid"));

    const int32_t count = JSON_ARRAY_LEN(io);
    if (count == 0) {
        return OK;
    }

    outfit->braid.count = MIN(count, Lara_Hair_GetBraidCount());
    for (int32_t i = 0; i < outfit->braid.count; ++i) {
        MUST(JSON_PUSH_INDEX(io, i));

        const char *braid_mode_name = nullptr;
        MUST(JSON_READ_OPT(io, "mode", &braid_mode_name));
        if (braid_mode_name != nullptr) {
            const int32_t mode =
                ENUM_MAP_GET(LARA_SKIN_BRAID_MODE, braid_mode_name, -1);
            if (mode < 0 || mode >= NUM_BRAID_MODES) {
                MUST(JSON_POP(io));
                return JSON_ReadIO_Fail(
                    io, "unknown braid mode '%s'", braid_mode_name);
            }
            outfit->braid.mode = mode;
        }

        MUST(
            JSON_READ_D(io, "auto_enabled", &outfit->braid.auto_enabled, true));
        MUST(JSON_READ_D(
            io, "mesh_offset", &outfit->braid.setup[i].mesh_offset, 0));
        MUST(JSON_READ_D(
            io, "position", &outfit->braid.setup[i].position, (XYZ_32) {}));
        MUST(M_LoadBraidHeadSeam(io, &outfit->braid.setup[i].head_seam));

        MUST(JSON_POP(io));
    }

    MUST(JSON_POP(io));

    return OK;
}

static RESULT M_LoadGunMap(
    JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    int32_t map_idx = -1;
    MUST(JSON_READ_D(io, "gun_map", &map_idx, -1));
    if (map_idx < 0 || map_idx >= m_GunMaps->count) {
        MUST(JSON_POP(io));
        return JSON_ReadIO_Fail(io, "invalid gun map '%d'", map_idx);
    }
    outfit->gun_map = (LARA_SKIN_GUN_MAP *)Vector_Get(m_GunMaps, map_idx);
    return OK;
}

static RESULT M_LoadNoHolsters(
    JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    if (JSON_ReadIO_HasKey(io, "no_holster_offsets")) {
        MUST(JSON_PUSH(io, "no_holster_offsets"));
        MUST(JSON_READ_D(io, "thigh_l", &outfit->no_holster_offsets.left, -1));
        MUST(JSON_READ_D(io, "thigh_r", &outfit->no_holster_offsets.right, -1));
        MUST(JSON_POP(io));
    } else {
        outfit->no_holster_offsets.left = -1;
        outfit->no_holster_offsets.right = -1;
    }
    return OK;
}

static RESULT M_LoadExtras(
    JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    for (int32_t j = 0; j < LS_EXTRA_NUMBER_OF; j++) {
        outfit->extra_outfits[j] = LARA_SKIN_TYPE_DEFAULT;
    }

    if (JSON_ReadIO_HasKey(io, "extra_outfits")) {
        MUST(JSON_PUSH(io, "extra_outfits"));
        JSON_OBJECT *const extra_obj = JSON_ReadIO_GetCurrentObject(io);
        if (extra_obj == nullptr) {
            return JSON_ReadIO_Fail(io, "'extra_outfits' must be an object");
        }

        for (JSON_OBJECT_ELEMENT *elem = extra_obj->start; elem != nullptr;
             elem = elem->next) {
            const char *const state_name = elem->name->string;
            const int32_t state =
                ENUM_MAP_GET(LARA_EXTRA_STATE, state_name, -1);
            if (state < 0 || state >= LS_EXTRA_NUMBER_OF) {
                MUST(JSON_POP(io));
                return JSON_ReadIO_Fail(
                    io, "unknown Lara extra state '%s'", state_name);
            }

            const char *outfit_name = nullptr;
            MUST(JSON_READ(io, state_name, &outfit_name));
            const LARA_SKIN_TYPE type = Lara_Skin_FindOutfitByName(outfit_name);
            if (type < 0 || type >= m_OutfitCount) {
                MUST(JSON_POP(io));
                return JSON_ReadIO_Fail(io, "unknown outfit '%s'", outfit_name);
            }
            outfit->extra_outfits[state] = type;
        }
        MUST(JSON_POP(io));
    }

    if (JSON_ReadIO_HasKey(io, "extra_mesh_positions")) {
        MUST(JSON_PUSH(io, "extra_mesh_positions"));
        JSON_OBJECT *const extra_obj = JSON_ReadIO_GetCurrentObject(io);
        if (extra_obj == nullptr) {
            MUST(JSON_POP(io));
            return JSON_ReadIO_Fail(
                io, "'extra_mesh_positions' must be an object");
        }

        for (JSON_OBJECT_ELEMENT *elem = extra_obj->start; elem != nullptr;
             elem = elem->next) {
            const char *const name = elem->name->string;
            const int32_t type = ENUM_MAP_GET(LARA_SKIN_EXTRA_MESH, name, -1);
            if (type < 0 || type >= NUM_EXTRA_MESHES) {
                MUST(JSON_POP(io));
                return JSON_ReadIO_Fail(
                    io, "unknown extra mesh type '%s'", name);
            }

            MUST(JSON_READ(io, name, &outfit->extra_mesh_positions[type]));
        }
        MUST(JSON_POP(io));
    }

    return OK;
}

static RESULT M_LoadObjectID(
    JSON_READ_IO *const io, const char *const key, OBJECT_ID *const out_obj_id)
{
    *out_obj_id = NO_OBJECT;

    const char *obj_name = nullptr;
    MUST(JSON_READ(io, key, &obj_name));

    const CATALOG_ID object_id =
        Catalog_FromKey(CATALOG_OBJECTS, obj_name, NO_OBJECT);
    if (object_id == NO_OBJECT) {
        MUST(JSON_POP(io));
        return JSON_ReadIO_Fail(io, "unknown outfit object_id '%s'", obj_name);
    }
    *out_obj_id = object_id;

    return OK;
}

static RESULT M_LoadObjectIDOr(
    JSON_READ_IO *const io, const char *const key, OBJECT_ID *const out_obj_id,
    const OBJECT_ID fallback)
{
    if (!JSON_ReadIO_HasKey(io, key)) {
        *out_obj_id = fallback;
        return OK;
    }
    return M_LoadObjectID(io, key, out_obj_id);
}

static RESULT M_LoadOutfit(
    JSON_READ_IO *const io, LARA_SKIN_OUTFIT *const outfit)
{
    MUST(M_LoadObjectID(io, "mesh_object", &outfit->mesh_obj_id));
    // An outfit that names no joints object keeps the mesh object's.
    IGNORE(M_LoadObjectID(io, "joints_object", &outfit->joints_obj_id));
    MUST(M_LoadObjectIDOr(
        io, "extra_object", &outfit->extra_obj_id, O_LARA_SKIN_SWAP_EXTRA));
    MUST(M_LoadObjectIDOr(
        io, "guns_object", &outfit->guns_obj_id, O_LARA_SKIN_SWAP_GUNS));
    MUST(M_LoadObjectIDOr(
        io, "legs_object", &outfit->legs_obj_id, O_LARA_SKIN_SWAP_LEGS));

    MUST(JSON_READ_D(
        io, "gold_color", &outfit->gold_color, M_DEFAULT_GOLD_COLOR));
    MUST(JSON_READ_D(io, "is_selectable", &outfit->is_selectable, true));
    MUST(
        JSON_READ_D(io, "combat_face_offset", &outfit->combat_face_offset, -1));
    MUST(
        JSON_READ_D(io, "speech_face_offset", &outfit->speech_face_offset, -1));
    MUST(JSON_READ_D(
        io, "supports_sunglasses", &outfit->supports_sunglasses, true));
    MUST(JSON_READ_D(io, "is_barefoot", &outfit->is_barefoot, false));

    MUST(M_LoadBraid(io, outfit));
    MUST(M_LoadGunMap(io, outfit));
    MUST(M_LoadNoHolsters(io, outfit));
    MUST(M_LoadExtras(io, outfit));

    outfit->is_defined = true;
    return OK;
}

static RESULT M_ReadOutfits(JSON_READ_IO *const io)
{
    MUST(JSON_PUSH(io, "outfits"));

    JSON_OBJECT *const outfits_map = JSON_ReadIO_GetCurrentObject(io);
    if (outfits_map == nullptr) {
        return JSON_ReadIO_Fail(io, "'outfits' must be an object");
    }

    size_t outfit_count = 0;
    for (JSON_OBJECT_ELEMENT *elem = outfits_map->start; elem != nullptr;
         elem = elem->next) {
        outfit_count++;
    }

    if (outfit_count == 0) {
        return JSON_ReadIO_Fail(io, "missing outfits in configuration");
    }

    m_Outfits = Memory_Alloc(sizeof(*m_Outfits) * outfit_count);
    m_OutfitCount = (int32_t)outfit_count;

    size_t idx = 0;
    for (JSON_OBJECT_ELEMENT *elem = outfits_map->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        MUST(JSON_PUSH(io, name));

        M_OUTFIT_ENTRY *const outfit = &m_Outfits[idx];
        outfit->name = Memory_DupStr(name);

        const char *name_gs = nullptr;
        MUST(JSON_READ_OPT(io, "name_gs", &name_gs));
        outfit->name_gs = Memory_DupStr(name_gs);

        M_OUTFIT_LOOKUP *existing = nullptr;
        HASH_FIND_STR(m_OutfitLookup, outfit->name, existing);
        if (existing != nullptr) {
            MUST(JSON_POP(io));
            return JSON_ReadIO_Fail(io, "duplicate outfit '%s'", name);
        }

        M_OUTFIT_LOOKUP *const lookup = Memory_Alloc(sizeof(*lookup));
        lookup->name = outfit->name;
        lookup->index = (int32_t)idx;
        HASH_ADD_KEYPTR(
            hh, m_OutfitLookup, lookup->name, strlen(lookup->name), lookup);
        MUST(JSON_POP(io));
        idx++;
    }

    idx = 0;
    for (JSON_OBJECT_ELEMENT *elem = outfits_map->start; elem != nullptr;
         elem = elem->next) {
        MUST(JSON_PUSH(io, elem->name->string));
        const RESULT loaded = M_LoadOutfit(io, &m_Outfits[idx].outfit);
        if (!IS_OK(loaded)) {
            MUST(JSON_POP(io));
            return loaded;
        }
        MUST(JSON_POP(io));
        idx++;
    }

    MUST(JSON_POP(io));
    return OK;
}

static RESULT M_LoadFile(JSON_READ_IO *const io)
{
    MUST(M_ReadGunMaps(io));
    MUST(M_ReadExtraMeshes(io));
    MUST(M_ReadOutfits(io));
    return OK;
}

static void M_Shutdown(void)
{
    if (m_GunMaps != nullptr) {
        Vector_Free(m_GunMaps);
        m_GunMaps = nullptr;
    }

    M_ResetOutfits();
}

static RESULT M_LoadFrom(const char *const source_path)
{
    if (m_GunMaps != nullptr) {
        Vector_Free(m_GunMaps);
        m_GunMaps = nullptr;
    }
    m_GunMaps = Vector_Create(sizeof(LARA_SKIN_GUN_MAP));

    M_ResetOutfits();
    M_SeedDynamicEnumValues();
    memset(m_ExtraMeshOffsets, 0, sizeof(m_ExtraMeshOffsets));

    LOG_INFO("Reading outfit definitions from %s", source_path);
    JSON_VALUE *doc = nullptr;
    MUST(JSONFile_ReadRequired(source_path, &doc));

    JSON_READ_IO *const io = JSON_ReadIO_Create(doc, 0, source_path);
    RESULT result = M_LoadFile(io);
    if (IS_OK(result)) {
        M_SeedDynamicEnumValues();
    } else {
    }

    JSON_ReadIO_Destroy(io);
    JSON_ValueFree(doc);
    return result;
}

static RESULT M_Load(void)
{
    const char *source_path = nullptr;
    RESULT result = GamePath_Resolve(
        GAME_DYNAMIC_PATH_COMMON_CONFIG, "outfits.json5", &source_path);
    if (IS_OK(result)) {
        result = M_LoadFrom(source_path);
    }
    return result;
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
    for (int32_t i = 0; i < m_OutfitCount; i++) {
        if (Lara_Skin_IsOutfitAvailable(i)) {
            return i;
        }
    }
    return m_OutfitCount > 0 ? 0 : LARA_SKIN_TYPE_DEFAULT;
}

int32_t Lara_Skin_GetOutfitCount(void)
{
    return m_OutfitCount;
}

bool Lara_Skin_IsOutfitDefined(const LARA_SKIN_TYPE skin_type)
{
    return skin_type >= 0 && skin_type < m_OutfitCount
        && m_Outfits[skin_type].outfit.is_defined;
}

// An outfit the level has no meshes for is not one she can wear. Levels built
// before an outfit was added carry every slot but that one, and an install can
// be missing the injection that holds them all.
bool Lara_Skin_IsOutfitAvailable(const LARA_SKIN_TYPE skin_type)
{
    if (!Lara_Skin_IsOutfitDefined(skin_type)) {
        return false;
    }

    const OBJECT *const skin_obj =
        Object_Get(m_Outfits[skin_type].outfit.mesh_obj_id);
    return skin_obj->loaded && skin_obj->mesh_count == LM_NUMBER_OF;
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

REGISTER_SUBSYSTEM(.load = M_Load, .shutdown = M_Shutdown)
