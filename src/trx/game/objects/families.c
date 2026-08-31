#include <trx/game/objects/families.h>

#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/objects/vars.h>

#include <string.h>

// Store each object's families in a per-object list, using null for none;
// family lists are short and remain session-local because they share the mod
// lifetime.
static VECTOR **m_Members = nullptr;
static int32_t m_MemberCount = 0;

// Define each family's shipped membership until family membership is
// supplied as data.
static const struct {
    OBJECT_FAMILY family;
    const OBJECT_ID *objects;
} m_Seed[] = {
    // clang-format off
    { OBJ_FAMILY_ANIM,               g_AnimObjects },
    { OBJ_FAMILY_BOSS,               g_BossObjects },
    { OBJ_FAMILY_CREATURE,           g_CreatureObjects },
    { OBJ_FAMILY_DOOR,               g_DoorObjects },
    { OBJ_FAMILY_ELEVATED_PICKUP,    g_ElevatedPickupObjects },
    { OBJ_FAMILY_GAME_SPRITE,        g_GameSpriteObjects },
    { OBJ_FAMILY_GENERIC_INV_OPTION, g_GenericInvOptions },
    { OBJ_FAMILY_GUN,                g_GunObjects },
    { OBJ_FAMILY_AMMO,               g_GunAmmoObjects },
    { OBJ_FAMILY_HEAVY_MISSILE,      g_HeavyMissileObjects },
    { OBJ_FAMILY_HEAVY_SHATTERABLE,  g_HeavyShatterableObjects },
    { OBJ_FAMILY_INVENTORY,          g_InvObjects },
    { OBJ_FAMILY_LOYAL,              g_LoyalObjects },
    { OBJ_FAMILY_PUSHABLE,           g_MovableBlockObjects },
    { OBJ_FAMILY_NO_HIT_REACTION,    g_NoHitReactionObjects },
    { OBJ_FAMILY_NULL,               g_NullObjects },
    { OBJ_FAMILY_PICKUP,             g_PickupObjects },
    { OBJ_FAMILY_PROJECTILE,         g_ProjectileObjects },
    { OBJ_FAMILY_QUEST,              g_QuestObjects },
    { OBJ_FAMILY_RECEPTACLE,         g_ReceptacleObjects },
    { OBJ_FAMILY_SECRET,             g_SecretObjects },
    { OBJ_FAMILY_SHATTERABLE,        g_ShatterableObjects },
    { OBJ_FAMILY_SHOAL,              g_ShoalObjects },
    { OBJ_FAMILY_SMASHABLE,          g_SmashableObjects },
    { OBJ_FAMILY_SWITCH,             g_SwitchObjects },
    { OBJ_FAMILY_TRAPDOOR,           g_TrapdoorObjects },
    { OBJ_FAMILY_WATER,              g_WaterObjects },
    { OBJ_FAMILY_WATER_SPRITE,       g_WaterSpriteObjects },
    // clang-format on
};

// Allocate entries through the specified object position so a newly minted
// object can record its families.
static void M_EnsureRoom(const OBJECT_ID object_id)
{
    if (object_id < m_MemberCount) {
        return;
    }
    const int32_t count = object_id + 1;
    m_Members = Memory_Realloc(m_Members, sizeof(VECTOR *) * count);
    memset(
        &m_Members[m_MemberCount], 0,
        sizeof(VECTOR *) * (count - m_MemberCount));
    m_MemberCount = count;
}

static void M_Init(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(m_Seed); i++) {
        for (int32_t j = 0; m_Seed[i].objects[j] != NO_OBJECT; j++) {
            ObjectFamily_Add(m_Seed[i].objects[j], m_Seed[i].family);
        }
    }
}

static void M_Shutdown(void)
{
    for (int32_t i = 0; i < m_MemberCount; i++) {
        if (m_Members[i] != nullptr) {
            Vector_Free(m_Members[i]);
        }
    }
    Memory_FreePointer(&m_Members);
    m_MemberCount = 0;
}

bool ObjectFamily_Has(const OBJECT_ID object_id, const OBJECT_FAMILY family)
{
    ASSERT(Catalog_IsValidID(CATALOG_FAMILIES, family));
    if (object_id < 0 || object_id >= m_MemberCount
        || m_Members[object_id] == nullptr) {
        return false;
    }
    return Vector_Contains(m_Members[object_id], &family);
}

void ObjectFamily_Add(const OBJECT_ID object_id, const OBJECT_FAMILY family)
{
    ASSERT(Catalog_IsValidID(CATALOG_FAMILIES, family));
    ASSERT(Catalog_IsValidID(CATALOG_OBJECTS, object_id));
    if (ObjectFamily_Has(object_id, family)) {
        return;
    }
    M_EnsureRoom(object_id);
    if (m_Members[object_id] == nullptr) {
        m_Members[object_id] = Vector_Create(sizeof(OBJECT_FAMILY));
    }
    Vector_Add(m_Members[object_id], &family);
}

void ObjectFamily_Remove(const OBJECT_ID object_id, const OBJECT_FAMILY family)
{
    ASSERT(Catalog_IsValidID(CATALOG_FAMILIES, family));
    if (object_id >= 0 && object_id < m_MemberCount
        && m_Members[object_id] != nullptr) {
        Vector_Remove(m_Members[object_id], &family);
    }
}

REGISTER_BASE_SUBSYSTEM(.init = M_Init, .shutdown = M_Shutdown)
