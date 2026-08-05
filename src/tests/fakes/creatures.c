// The ally system as a single flag and two counters. The surface only ever sets
// the flag and names an object; what the engine then does with the pathing is
// not what these assertions are about.

#include <fakes/creatures.h>

#include <harness/fake_calls.h>

#include <trx/game/objects/ids.h>

static bool m_AlliesHostile;

static void M_Reset(void)
{
    m_AlliesHostile = false;
}

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
    FAKE_RECORD("add_ally", FV(obj_id));
}

void Creature_AddAllyTargetingEnemy(const OBJECT_ID obj_id)
{
    FAKE_RECORD("add_ally_target", FV(obj_id));
}

FAKE_ON_RESET(M_Reset)
