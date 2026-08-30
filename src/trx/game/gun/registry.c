#include <trx/game/gun/registry.h>

#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/catalog/table.h>

// Record declarations in file link order. Constructors may run before built-in
// catalog identities exist, so processing happens later.
static VECTOR *m_Declared = nullptr;

CATALOG_TABLE_DEFINE(m_Weapons, CATALOG_WEAPONS, WEAPON_INFO);

// How many identities have their empty weapon, and how many weapons are
// declared.
static CATALOG_ID m_SeededCount = 0;
static int32_t m_DeclaredCount = 0;

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
    if (m_Declared != nullptr) {
        Vector_Free(m_Declared);
        m_Declared = nullptr;
    }
}

void Gun_Registry_Register(const WEAPON_INFO *const info)
{
    ASSERT(info != nullptr);
    ASSERT(info->gun_type > LGT_UNARMED);

    if (m_Declared == nullptr) {
        m_Declared = Vector_Create(sizeof(const WEAPON_INFO *));
    }
    Vector_Add(m_Declared, &info);
}

void Gun_Registry_Seed(void)
{
    m_SeededCount = 0;
    m_DeclaredCount = 0;
    M_SeedNew();

    for (CATALOG_ID i = 0; i < Catalog_GetBuiltInCount(CATALOG_WEAPONS); i++) {
        IGNORE(Catalog_BindSlot(CATALOG_WEAPONS, i, i));
    }

    for (int32_t i = 0; m_Declared != nullptr && i < m_Declared->count; i++) {
        const WEAPON_INFO *const info =
            *(const WEAPON_INFO **)Vector_Get(m_Declared, i);
        ASSERT(Gun_Registry_IsValidType(info->gun_type));
        WEAPON_INFO *const weapon =
            CatalogTable_Get(&m_Weapons, info->gun_type);
        ASSERT(!weapon->is_declared);
        *weapon = *info;
        weapon->is_declared = true;
        m_DeclaredCount++;
    }
}

int32_t Gun_Registry_GetCount(void)
{
    return m_DeclaredCount;
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

bool Gun_Registry_IsValidType(const LARA_GUN_TYPE gun_type)
{
    return Catalog_IsValidID(CATALOG_WEAPONS, gun_type);
}
