#include <trx/game/creature.h>
#include <trx/game/objects/vars.h>
#include <trx/vector.h>

static bool m_AlliesHostile = false;
static VECTOR *m_AllyObjects = nullptr;
static VECTOR *m_AllyTargetingObjects = nullptr;

__attribute__((constructor)) static void M_Init(void)
{
    m_AllyObjects = Vector_Create(sizeof(OBJECT_ID));
    m_AllyTargetingObjects = Vector_Create(sizeof(OBJECT_ID));
}

__attribute__((destructor)) static void M_Shutdown(void)
{
#define L_DELETE_VECTOR(vec)                                                   \
    if (vec != nullptr) {                                                      \
        Vector_Free(vec);                                                      \
        vec = nullptr;                                                         \
    }

    L_DELETE_VECTOR(m_AllyObjects);
    L_DELETE_VECTOR(m_AllyTargetingObjects);

#undef L_DELETE_VECTOR
}

void Creature_Reset(void)
{
    Creature_SetAlliesHostile(false);
    Vector_Clear(m_AllyObjects);
    Vector_Clear(m_AllyTargetingObjects);
}

bool Creature_AreAlliesHostile(void)
{
    return m_AlliesHostile;
}

void Creature_SetAlliesHostile(bool enable)
{
    m_AlliesHostile = enable;
}

bool Creature_IsHostile(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_CreatureObjects)
        && (!Creature_IsAlly(item) || Creature_AreAlliesHostile());
}

bool Creature_IsAlly(const ITEM *const item)
{
    return Vector_Contains(m_AllyObjects, (void *)&item->object_id);
}

bool Creature_IsAllyTargetingEnemy(const ITEM *const item)
{
    return Vector_Contains(m_AllyTargetingObjects, (void *)&item->object_id);
}

void Creature_AddAlly(const OBJECT_ID obj_id)
{
    Vector_Add(m_AllyObjects, (void *)&obj_id);
}

void Creature_AddAllyTargetingEnemy(const OBJECT_ID obj_id)
{
    Vector_Add(m_AllyTargetingObjects, (void *)&obj_id);
}
