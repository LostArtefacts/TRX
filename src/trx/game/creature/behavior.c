#include <trx/game/creature.h>
#include <trx/game/objects/vars.h>

static bool m_AlliesHostile = false;

void Creature_Reset(void)
{
    Creature_SetAlliesHostile(false);
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
    return Object_IsType(item->object_id, g_EnemyObjects)
        || (Creature_AreAlliesHostile() && Creature_IsAlly(item));
}

bool Creature_IsAlly(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_AllyObjects);
}

bool Creature_IsAllyTargetingEnemy(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_AllyTargetingEnemies);
}
