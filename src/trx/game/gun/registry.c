#include <trx/game/gun/registry.h>

#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/catalog/table.h>

// Record declarations in file link order. Constructors may run before built-in
// catalog identities exist, so processing happens later.
static VECTOR *m_Declared = nullptr;

CATALOG_TABLE_DEFINE(m_Weapons, CATALOG_WEAPONS, WEAPON_INFO);

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
    CATALOG_FOR_EACH(CATALOG_WEAPONS, i)
    {
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

    for (int32_t i = 0; m_Declared != nullptr && i < m_Declared->count; i++) {
        const WEAPON_INFO *const info =
            *(const WEAPON_INFO **)Vector_Get(m_Declared, i);
        ASSERT(Gun_Registry_IsValidType(info->gun_type));
        WEAPON_INFO *const weapon =
            CatalogTable_Get(&m_Weapons, info->gun_type);
        ASSERT(!weapon->is_declared);
        *weapon = *info;
        weapon->is_declared = true;
    }
}

int32_t Gun_Registry_GetCount(void)
{
    return m_Declared == nullptr ? 0 : m_Declared->count;
}

const WEAPON_INFO *Gun_Registry_GetByIndex(const int32_t idx)
{
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

WEAPON_INFO *Gun_Registry_Get(const LARA_GUN_TYPE gun_type)
{
    ASSERT(Gun_Registry_IsValidType(gun_type));
    return CatalogTable_Get(&m_Weapons, gun_type);
}

bool Gun_Registry_IsValidType(const LARA_GUN_TYPE gun_type)
{
    return Catalog_IsValidID(CATALOG_WEAPONS, gun_type);
}
