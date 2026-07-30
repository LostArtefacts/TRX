// The ally system as a single flag and two counters. The surface only ever sets
// the flag and names an object; what the engine then does with the pathing is
// not what these assertions are about.

#include <fakes/creatures.h>

#include <trx/game/objects/ids.h>

FAKE_CREATURE_CALLS g_FakeCreatureCalls;
static bool m_AlliesHostile;

bool Creature_AreAlliesHostile(void)
{
    return m_AlliesHostile;
}

void Creature_SetAlliesHostile(const bool enable)
{
    m_AlliesHostile = enable;
}

void Creature_AddAlly(const OBJECT_ID obj_id)
{
    g_FakeCreatureCalls.add_ally++;
    g_FakeCreatureCalls.last_ally_object_id = obj_id;
}

void Creature_AddAllyTargetingEnemy(const OBJECT_ID obj_id)
{
    g_FakeCreatureCalls.add_ally_target++;
    g_FakeCreatureCalls.last_ally_target_object_id = obj_id;
}

void FakeCreatures_Reset(void)
{
    m_AlliesHostile = false;
    g_FakeCreatureCalls = (FAKE_CREATURE_CALLS) {};
}
