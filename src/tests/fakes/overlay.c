// The one overlay flag a script reads: whether something asks for Lara's
// health bar.

#include <fakes/overlay.h>

#include <harness/fake_calls.h>

#include <trx/game/overlay.h>

static bool m_ForcedHealthBar;

static void M_Reset(void)
{
    m_ForcedHealthBar = false;
}

void FakeOverlay_ForceHealthBar(const bool show)
{
    m_ForcedHealthBar = show;
}

bool Overlay_IsHealthBarForced(void)
{
    return m_ForcedHealthBar;
}

FAKE_ON_RESET(M_Reset)
