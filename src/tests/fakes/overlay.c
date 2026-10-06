// The overlay flags a script reads: whether something asks for Lara's health
// bar, and whether the cinematic bars cover the screen. Record pickups shown
// in the corner as show_pickup calls.

#include <fakes/overlay.h>

#include <harness/fake_calls.h>

#include <trx/game/inventory_ring/control.h>
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

// Record interface requests without performing them.
void Overlay_SetBottomText(const OVERLAY_TEXT text)
{
    const char *const caption =
        text.kind == OVERLAY_TEXT_LITERAL ? text.literal : "";
    FAKE_RECORD("set_caption", FV_STR(caption));
}

void InvRing_ShowItemQuantity(const char *const fmt, const int32_t qty)
{
    FAKE_RECORD("set_caption_count", FV(qty));
}

void InvRing_ClearItemQuantity(void)
{
    FAKE_RECORD("clear_caption_count");
}

void Overlay_ShowArrow(const OVERLAY_ARROW arrow, const bool show)
{
    FAKE_RECORD("show_arrow", FV((int32_t)arrow), FV(show));
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

void Overlay_AddDisplayPickup(const OBJECT_ID obj_id)
{
    FAKE_RECORD("show_pickup", FV(obj_id));
}

FAKE_ON_RESET(M_Reset)
