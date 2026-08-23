#include <trx/game/gun/registry.h>

#include <trx/debug.h>
#include <trx/game/gun/const.h>

#include <string.h>

// The table is wide enough to hold a weapon from outside the enum, but a
// constructor can still access it without allocating.
static const WEAPON_INFO *m_Declared[MAX_WEAPONS] = {};
static WEAPON_INFO m_Weapons[MAX_WEAPONS] = {};
static int32_t m_DeclaredCount = 0;

void Gun_Registry_Register(const WEAPON_INFO *const info)
{
    ASSERT(info != nullptr);
    ASSERT(info->gun_type > LGT_UNARMED);
    ASSERT(info->gun_type < MAX_WEAPONS);
    ASSERT(m_Declared[info->gun_type] == nullptr);

    m_Declared[info->gun_type] = info;
    m_DeclaredCount++;
}

void Gun_Registry_Seed(void)
{
    memset(m_Weapons, 0, sizeof(m_Weapons));
    for (int32_t i = 0; i < MAX_WEAPONS; i++) {
        m_Weapons[i] = (WEAPON_INFO) {
            .gun_type = (LARA_GUN_TYPE)i,
            .equip_input_role = (INPUT_ROLE)-1,
            .glow.scale = 1.0f,
            .gun_object_id = NO_OBJECT,
            .ammo_object_id = NO_OBJECT,
            .anim_object_id = NO_OBJECT,
            .shell_object_id = NO_OBJECT,
            .projectile_object_id = NO_OBJECT,
        };
        if (m_Declared[i] != nullptr) {
            m_Weapons[i] = *m_Declared[i];
            m_Weapons[i].gun_type = (LARA_GUN_TYPE)i;
            m_Weapons[i].is_declared = true;
        }
    }
}

int32_t Gun_Registry_GetCount(void)
{
    return m_DeclaredCount;
}

const WEAPON_INFO *Gun_Registry_GetByIndex(const int32_t idx)
{
    int32_t count = 0;
    for (int32_t i = 0; i < MAX_WEAPONS; i++) {
        if (!m_Weapons[i].is_declared) {
            continue;
        }
        if (count == idx) {
            return &m_Weapons[i];
        }
        count++;
    }
    return nullptr;
}

WEAPON_INFO *Gun_Registry_Get(const LARA_GUN_TYPE gun_type)
{
    ASSERT(Gun_Registry_IsValidType(gun_type));
    return &m_Weapons[gun_type];
}

bool Gun_Registry_IsValidType(const LARA_GUN_TYPE gun_type)
{
    return gun_type >= LGT_UNARMED && gun_type < NUM_WEAPONS;
}
