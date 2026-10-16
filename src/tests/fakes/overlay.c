// The overlay flags a script reads: whether something asks for Lara's health
// bar, and how deep the cinematic bars are. A script asking for bars sets
// where they are going; the test says where they are. Record pickups shown in
// the corner as show_pickup calls. The caption and the arrows a script sets
// read back as the overlay holds them.

#include <fakes/overlay.h>

#include <harness/fake_calls.h>

#include <trx/game/inventory_ring/control.h>
#include <trx/game/output/overlay.h>
#include <trx/game/overlay.h>

static bool m_ForcedHealthBar;
static float m_Letterbox;
static float m_LetterboxTarget;
static OVERLAY_TEXT m_Caption;
static bool m_Arrows[OVERLAY_ARROW_NUMBER_OF];

static void M_Reset(void)
{
    m_ForcedHealthBar = false;
    m_Letterbox = 0.0f;
    m_LetterboxTarget = 0.0f;
    m_Caption = (OVERLAY_TEXT) {};
    for (int32_t i = 0; i < OVERLAY_ARROW_NUMBER_OF; i++) {
        m_Arrows[i] = false;
    }
}

void FakeOverlay_ForceHealthBar(const bool show)
{
    m_ForcedHealthBar = show;
}

// Record interface requests without performing them.
void Overlay_SetBottomText(const OVERLAY_TEXT text)
{
    const char *const caption =
        text.kind == OVERLAY_TEXT_LITERAL ? text.literal : "";
    FAKE_RECORD("set_caption", FV_STR(caption));
    m_Caption = text;
}

const char *Overlay_GetTopText(void)
{
    return nullptr;
}

const char *Overlay_GetBottomText(void)
{
    return m_Caption.kind == OVERLAY_TEXT_LITERAL ? m_Caption.literal : nullptr;
}

bool Overlay_IsBottomTextFlashing(void)
{
    return false;
}

bool Overlay_IsArrowShown(const OVERLAY_ARROW arrow)
{
    return m_Arrows[arrow];
}

bool Overlay_IsVersionShown(void)
{
    return false;
}

void InvRing_ShowItemQuantity(const char *const fmt, const int32_t qty)
{
    FAKE_RECORD("set_caption_count", FV(qty));
}

void InvRing_ClearItemQuantity(void)
{
    FAKE_RECORD("clear_caption_count");
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

void Overlay_AddDisplayPickup(const OBJECT_ID obj_id)
{
    FAKE_RECORD("show_pickup", FV(obj_id));
}

FAKE_ON_RESET(M_Reset)
