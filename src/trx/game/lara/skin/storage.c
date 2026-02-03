#include <trx/game/lara/skin/storage.h>

#include <trx/debug.h>
#include <trx/enum_map.h>
#include <trx/game/catalog.h>
#include <trx/game/lara.h>
#include <trx/game/shell.h>
#include <trx/json_file.h>
#include <trx/vector.h>

static VECTOR *m_GunMaps = nullptr;
static VECTOR *m_Braids = nullptr;
static LARA_SKIN_OUTFIT m_Outfits[NUM_LARA_SKINS] = {};
static int32_t m_ExtraMeshOffsets[NUM_EXTRA_MESHES] = {};

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
            const char *const gun_name =
                EnumMap_ToString(ENUM_MAP_NAME(LARA_GUN_TYPE), j);
            JSON_OBJECT *const gun_obj =
                JSON_ObjectGetObject(map_obj, gun_name);
            if (gun_obj == nullptr) {
                mesh_map->hand.left = -1;
                mesh_map->hand.right = -1;
                mesh_map->thigh.left = -1;
                mesh_map->thigh.right = -1;
            } else {
                mesh_map->hand.right = JSON_ObjectGetInt(gun_obj, "hand_r", -1);
                mesh_map->hand.left = JSON_ObjectGetInt(gun_obj, "hand_l", -1);
                mesh_map->thigh.right =
                    JSON_ObjectGetInt(gun_obj, "thigh_r", -1);
                mesh_map->thigh.left =
                    JSON_ObjectGetInt(gun_obj, "thigh_l", -1);
            }
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

    outfit->mesh_offset = JSON_ObjectGetInt(outfit_obj, "mesh_offset", 1);

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

    int32_t tmp_i = JSON_ObjectGetInt(outfit_obj, "gun_map", -1);
    if (tmp_i < 0 || tmp_i >= m_GunMaps->count) {
        Shell_ExitSystemFmt("invalid gun map '%d'", tmp_i);
        return false;
    }
    outfit->gun_map = (LARA_SKIN_GUN_MAP *)Vector_Get(m_GunMaps, tmp_i);

    outfit->is_reflective =
        JSON_ObjectGetBool(outfit_obj, "is_reflective", false);

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

    outfit->combat_face_offset =
        JSON_ObjectGetInt(outfit_obj, "combat_face_offset", -1);

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
    }

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
            const int32_t type = ENUM_MAP_GET(LARA_SKIN_TYPE, name, -1);
            if (type < 0 || type >= NUM_LARA_SKINS) {
                Shell_ExitSystemFmt("unknown skin type '%s'", name);
                return false;
            }
            outfit->extra_outfits[state] = type;
        }
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

    for (JSON_OBJECT_ELEMENT *elem = outfits_map->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        const int32_t type = ENUM_MAP_GET(LARA_SKIN_TYPE, name, -1);
        if (type == LARA_SKIN_TYPE_DEFAULT) {
            Shell_ExitSystemFmt("default skin cannot be defined");
            return false;
        } else if (type < 0 || type >= NUM_LARA_SKINS) {
            Shell_ExitSystemFmt("unknown skin type '%s'", name);
            return false;
        }

        JSON_OBJECT *const outfit_obj = JSON_ValueAsObject(elem->value);
        LARA_SKIN_OUTFIT *const outfit = &m_Outfits[type];
        if (!M_LoadOutfit(outfit_obj, outfit)) {
            return false;
        }
    }

    return true;
}

void Lara_Skin_LoadFromFile(const char *const path)
{
    m_GunMaps = Vector_Create(sizeof(LARA_SKIN_GUN_MAP));

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

cleanup:
    JSON_ValueFree(doc);
}

void Lara_Skin_Shutdown(void)
{
    if (m_GunMaps != nullptr) {
        Vector_Free(m_GunMaps);
        m_GunMaps = nullptr;
    }
}

int32_t Lara_Skin_GetOutfitCount(void)
{
    return NUM_LARA_SKINS;
}

bool Lara_Skin_IsOutfitAvailable(const LARA_SKIN_TYPE skin_type)
{
    return m_Outfits[skin_type].is_defined;
}

const LARA_SKIN_OUTFIT *Lara_Skin_GetOutfit(const LARA_SKIN_TYPE skin_type)
{
    return &m_Outfits[skin_type];
}

int32_t Lara_Skin_GetExtraMeshOffset(const LARA_SKIN_EXTRA_MESH mesh)
{
    ASSERT(mesh >= 0 && mesh < NUM_EXTRA_MESHES);
    return m_ExtraMeshOffsets[mesh];
}
