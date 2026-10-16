// The overlay flags a script reads: whether something asks for Lara's health
// bar, and how deep the cinematic bars are. A script asking for bars sets
// where they are going; the test says where they are. The arrows a script
// sets read back as the overlay holds them.

#include <fakes/overlay.h>

#include <harness/fake_calls.h>

#include <trx/game/output/overlay.h>
#include <trx/game/overlay.h>

static bool m_ForcedHealthBar;
static float m_Letterbox;
static float m_LetterboxTarget;
static bool m_Arrows[OVERLAY_ARROW_NUMBER_OF];

static void M_Reset(void)
{
    m_ForcedHealthBar = false;
    m_Letterbox = 0.0f;
    m_LetterboxTarget = 0.0f;
    for (int32_t i = 0; i < OVERLAY_ARROW_NUMBER_OF; i++) {
        m_Arrows[i] = false;
    }
}

void FakeOverlay_ForceHealthBar(const bool show)
{
    m_ForcedHealthBar = show;
}

const char *Overlay_GetTopText(void)
{
    return nullptr;
}

bool Overlay_IsArrowShown(const OVERLAY_ARROW arrow)
{
    return m_Arrows[arrow];
}

bool Overlay_IsVersionShown(void)
{
    return false;
}

void Overlay_ShowArrow(const OVERLAY_ARROW which, const bool show)
{
    const int32_t arrow = which;
    FAKE_RECORD("show_arrow", FV(arrow), FV(show));
    m_Arrows[which] = show;
}

bool Overlay_IsHealthBarForced(void)
{
    return m_ForcedHealthBar;
}

void FakeOverlay_SetLetterbox(const float depth)
{
    m_Letterbox = depth;
}

void Output_Overlay_SlideLetterbox(const float ratio)
{
    m_LetterboxTarget = ratio;
}

float Output_Overlay_GetLetterbox(void)
{
    return m_Letterbox;
}

float Output_Overlay_GetLetterboxTarget(void)
{
    return m_LetterboxTarget;
}

bool Output_Overlay_HasLetterbox(void)
{
    return m_Letterbox > 0.0f;
}

FAKE_ON_RESET(M_Reset)
