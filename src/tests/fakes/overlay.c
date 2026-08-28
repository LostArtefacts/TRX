// The overlay flags a script reads: whether something asks for Lara's health
// bar, and whether the cinematic bars have the screen.

#include <fakes/overlay.h>

#include <harness/fake_calls.h>

#include <trx/game/output/overlay.h>
#include <trx/game/overlay.h>

static bool m_ForcedHealthBar;
static bool m_Letterbox;

static void M_Reset(void)
{
    m_ForcedHealthBar = false;
    m_Letterbox = false;
}

void FakeOverlay_ForceHealthBar(const bool show)
{
    m_ForcedHealthBar = show;
}

bool Overlay_IsHealthBarForced(void)
{
    return m_ForcedHealthBar;
}

void FakeOverlay_SetLetterbox(const bool shown)
{
    m_Letterbox = shown;
}

bool Output_Overlay_HasLetterbox(void)
{
    return m_Letterbox;
}

FAKE_ON_RESET(M_Reset)
