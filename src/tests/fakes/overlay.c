// How deep the cinematic bars are. A script asking for bars sets where they
// are going; the test says where they are.

#include <fakes/overlay.h>

#include <harness/fake_calls.h>

#include <trx/game/output/overlay.h>

static float m_Letterbox;
static float m_LetterboxTarget;

static void M_Reset(void)
{
    m_Letterbox = 0.0f;
    m_LetterboxTarget = 0.0f;
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
