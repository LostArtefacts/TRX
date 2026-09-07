#include <trx/game/gun/registry.h>

#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/catalog/table.h>
#include <trx/game/gun/routines.h>

CATALOG_TABLE_DEFINE(m_Weapons, CATALOG_WEAPONS, WEAPON_INFO);

static VECTOR *m_Strings = nullptr;

// How many identities have their empty weapon.
static CATALOG_ID m_SeededCount = 0;

// Give every identity minted since the last call the weapon that stands for
// none. A catalog table record starts as zero, and zero is a real object and
// a real key.
static void M_SeedNew(void)
{
    const CATALOG_ID count = Catalog_GetCount(CATALOG_WEAPONS);
    for (CATALOG_ID i = m_SeededCount; i < count; i++) {
        WEAPON_INFO *const weapon = CatalogTable_Get(&m_Weapons, i);
        *weapon = (WEAPON_INFO) {
            .gun_type = (LARA_GUN_TYPE)i,
            .is_available = true,
            .equip_input_role = (INPUT_ROLE)-1,
            .glow.scale = 1.0f,
            .gun_object_id = NO_OBJECT,
            .ammo_object_id = NO_OBJECT,
            .anim_object_id = NO_OBJECT,
            .shell_object_id = NO_OBJECT,
            .projectile_object_id = NO_OBJECT,
        };
    }
    m_SeededCount = count;
}

__attribute__((destructor)) static void M_Shutdown(void)
{
    for (int32_t i = 0; m_Strings != nullptr && i < m_Strings->count; i++) {
        Memory_Free(*(char **)Vector_Get(m_Strings, i));
    }
    if (m_Strings != nullptr) {
        Vector_Free(m_Strings);
        m_Strings = nullptr;
    }
}

static const char *M_KeepString(const char *const str)
{
    if (str == nullptr) {
        return nullptr;
    }
    char *const made = Memory_DupStr(str);
    if (m_Strings == nullptr) {
        m_Strings = Vector_Create(sizeof(char *));
    }
    Vector_Add(m_Strings, &made);
    return made;
}

static const char *M_MakeSaveKey(const char *const fmt, const char *const key)
{
    char *const made = String_Format(fmt, key);
    const char *const kept = M_KeepString(made);
    Memory_Free(made);
    return kept;
}

static void M_Init(void)
{
    Gun_Registry_Seed();
}

void Gun_Registry_Seed(void)
{
    m_SeededCount = 0;
    M_SeedNew();

    for (CATALOG_ID i = 0; i < Catalog_GetBuiltInCount(CATALOG_WEAPONS); i++) {
        IGNORE(Catalog_BindSlot(CATALOG_WEAPONS, i, i));
    }
}

RESULT Gun_Registry_SetKind(const WEAPON_TYPE type, WEAPON_INFO *const target)
{
    const GUN_KIND_ROUTINES *const routines = Gun_Routines_GetKind(type);
    FAIL_IF(
        routines == nullptr, "the engine holds no weapon of kind %d",
        (int32_t)type);
    target->type = type;
    target->draw_func = routines->draw_func;
    target->undraw_func = routines->undraw_func;
    target->draw_meshes_func = routines->draw_meshes_func;
    target->control_func = routines->control_func;
    return OK;
}

RESULT Gun_Registry_Declare(
    const LARA_GUN_TYPE gun_type, const WEAPON_TYPE type,
    WEAPON_INFO *const target)
{
    FAIL_IF(
        !Gun_Registry_IsValidType(gun_type), "there is no weapon %d",
        (int32_t)gun_type);
    FAIL_IF(
        target->is_declared, "%s is already declared",
        Catalog_IDToKey(CATALOG_WEAPONS, gun_type));

    MUST(Gun_Registry_SetKind(type, target));
    const char *const key = Catalog_IDToKey(CATALOG_WEAPONS, gun_type);
    target->save_ammo_key = M_MakeSaveKey("%s", key);
    target->save_resume_ammo_key = M_MakeSaveKey("%s_ammo", key);
    target->save_resume_has_key = M_MakeSaveKey("has_%s", key);
    target->is_declared = true;
    return OK;
}

int32_t Gun_Registry_GetCount(void)
{
    M_SeedNew();
    int32_t count = 0;
    CATALOG_FOR_EACH(CATALOG_WEAPONS, i)
    {
        const WEAPON_INFO *const weapon = CatalogTable_TryGet(&m_Weapons, i);
        if (weapon != nullptr && weapon->is_declared) {
            count++;
        }
    }
    return count;
}

const WEAPON_INFO *Gun_Registry_GetByIndex(const int32_t idx)
{
    M_SeedNew();
    int32_t count = 0;
    CATALOG_FOR_EACH(CATALOG_WEAPONS, i)
    {
        const WEAPON_INFO *const weapon = CatalogTable_TryGet(&m_Weapons, i);
        if (weapon == nullptr || !weapon->is_declared) {
            continue;
        }
        if (count == idx) {
            return weapon;
        }
        count++;
    }
    return nullptr;
}

void Gun_Registry_SetInputRole(
    const LARA_GUN_TYPE gun_type, const INPUT_ROLE role)
{
    WEAPON_INFO *const weapon = Gun_Registry_Get(gun_type);
    if ((int32_t)role >= 0) {
        CATALOG_FOR_EACH(CATALOG_WEAPONS, i)
        {
            WEAPON_INFO *const other = CatalogTable_TryGet(&m_Weapons, i);
            if (other == nullptr || other == weapon
                || other->equip_input_role != role) {
                continue;
            }
            LOG_INFO(
                "%s takes the %s key from %s",
                Catalog_IDToKey(CATALOG_WEAPONS, gun_type),
                ENUM_MAP_TO_STRING(INPUT_ROLE, role),
                Catalog_IDToKey(CATALOG_WEAPONS, other->gun_type));
            other->equip_input_role = (INPUT_ROLE)-1;
        }
    }
    weapon->equip_input_role = role;
}

WEAPON_INFO *Gun_Registry_Get(const LARA_GUN_TYPE gun_type)
{
    ASSERT(Gun_Registry_IsValidType(gun_type));
    M_SeedNew();
    return CatalogTable_Get(&m_Weapons, gun_type);
}

const char *Gun_Registry_KeepString(const char *const str)
{
    return M_KeepString(str);
}

bool Gun_Registry_IsValidType(const LARA_GUN_TYPE gun_type)
{
    return Catalog_IsValidID(CATALOG_WEAPONS, gun_type);
}

REGISTER_SUBSYSTEM(.init = M_Init)
