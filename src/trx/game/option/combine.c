#include <trx/game/option/combine.h>

#include <trx/core/utils.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/control.h>
#include <trx/game/inventory_ring/draw.h>
#include <trx/game/inventory_ring/priv.h>
#include <trx/game/inventory_ring/vars.h>
#include <trx/game/objects/names.h>
#include <trx/game/output.h>
#include <trx/game/sound.h>
#include <trx/game/ui.h>
#include <trx/game/ui/elements/frame.h>
#include <trx/game/ui/elements/modal.h>

// How far the objects on offer stand from the middle of the screen, and how
// far back the camera stands from them. The camera stands further back than
// the ring behind this one, which is what draws the objects smaller.
#define M_RING_RADIUS 400
#define M_CAMERA_DISTANCE 1100
#define M_BACKDROP_OPACITY 0.75f

// The room the frame keeps for the objects it holds.
#define M_FRAME_WIDTH 260.0f
#define M_FRAME_HEIGHT 150.0f
#define M_FRAME_PAD 10.0f

typedef struct {
    bool is_ready;
    OBJECT_ID choice;
    INV_RING ring;
    // Copies rather than the ring items themselves, which the ring behind
    // this one goes on turning every frame.
    INVENTORY_ITEM items[INV_RING_MAX_ITEMS];
    INVENTORY_ITEM *list[INV_RING_MAX_ITEMS];
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_DrawHint(void *const user_data)
{
    UI_BeginStack(UI_STACK_HORIZONTAL);
    UI_LabelFmt(
        "\\{input menu_confirm} %s", GS("general/actions/combine_item"));
    UI_Spacer(60.0f, 0.0f);
    UI_LabelFmt("\\{input menu_back} %s", GS("general/actions/cancel"));
    UI_EndStack();
}

static void M_Snapshot(M_PRIV *const p)
{
    INV_RING *const ring = &p->ring;
    ring->prev_radius = ring->radius;
    ring->prev_camera_y = ring->camera.pos.y;
    ring->prev_camera_pitch = ring->camera_pitch;
    ring->prev_ring_rot_y = ring->ring_pos.rot.y;
    for (int32_t i = 0; i < ring->number_of_objects; i++) {
        INVENTORY_ITEM *const inv_item = ring->list[i];
        inv_item->prev_x_rot_pt = inv_item->x_rot_pt;
        inv_item->prev_x_rot = inv_item->x_rot;
        inv_item->prev_y_rot = inv_item->y_rot;
        inv_item->prev_y_trans = inv_item->y_trans;
        inv_item->prev_z_trans = inv_item->z_trans;
        inv_item->prev_manual_rot = inv_item->manual_rot;
    }
}

// Puts the ring straight into the state it settles in, so that the objects on
// offer stand still the moment the backdrop appears.
static void M_Settle(M_PRIV *const p)
{
    INV_RING *const ring = &p->ring;
    // Selected rather than open, which is what holds the object it rests on
    // still instead of turning it.
    ring->status = RNG_SELECTED;
    ring->status_target = RNG_SELECTED;
    ring->status_frames = 0;
    ring->motion = (INV_RING_MOTION) {};
    ring->radius = M_RING_RADIUS;
    ring->camera_distance = M_CAMERA_DISTANCE;
    ring->camera.pos.y = INV_RING_CAMERA_HEIGHT;
    ring->ring_pos.rot.y = -DEG_90 - ring->current_object * ring->angle_adder;
    M_Snapshot(p);
}

static void M_Init(M_PRIV *const p, const OBJECT_ID object_id)
{
    OBJECT_ID partners[INV_RING_MAX_ITEMS];
    const int32_t count =
        Inv_GetCombinePartners(object_id, partners, INV_RING_MAX_ITEMS);
    int16_t item_count = 0;
    for (int32_t i = 0; i < count; i++) {
        const INVENTORY_ITEM *const inv_item =
            InvRing_GetByObjectID(partners[i]);
        if (inv_item != nullptr) {
            p->items[item_count] = *inv_item;
            p->list[item_count] = &p->items[item_count];
            item_count++;
        }
    }
    if (item_count == 0) {
        return;
    }

    const INV_RING_VISIBLE visible = {
        .items = p->list,
        .count = item_count,
    };
    p->ring.mode = g_InvRing_Mode;
    InvRing_InitRing(&p->ring, RT_MAIN, &visible, 0);
    M_Settle(p);
    p->is_ready = true;
    p->choice = NO_OBJECT;
    Sound_Effect(SFX_MENU_SPININ, nullptr, SPM_ALWAYS);
}

static void M_Leave(M_PRIV *const p)
{
    g_InputDB.menu_back = true;
    g_InputDB.menu_confirm = false;
    InvRing_ClearButtonHint();
    p->is_ready = false;
}

void Option_Combine_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }

    if (!p->is_ready) {
        M_Init(p, inv_item->object_id);
        if (!p->is_ready) {
            M_Leave(p);
            return;
        }
    }

    INV_RING *const ring = &p->ring;
    M_Snapshot(p);
    InvRing_CalcAdders(ring, INV_RING_ROTATE_DURATION);
    InvRing_SetButtonHintDrawer(M_DrawHint, nullptr);
    // The ring behind this one names what it rests on; this one names its own.
    InvRing_RemoveItemTexts();

    if (!ring->rotating) {
        if (g_Input.menu_right && ring->number_of_objects > 1) {
            InvRing_RotateLeft(ring);
            Sound_Effect(SFX_MENU_ROTATE, nullptr, SPM_ALWAYS);
        } else if (g_Input.menu_left && ring->number_of_objects > 1) {
            InvRing_RotateRight(ring);
            Sound_Effect(SFX_MENU_ROTATE, nullptr, SPM_ALWAYS);
        } else if (g_InputDB.menu_confirm) {
            p->choice = ring->list[ring->current_object]->object_id;
            M_Leave(p);
            return;
        } else if (g_InputDB.menu_back) {
            Sound_Effect(SFX_MENU_SPINOUT, nullptr, SPM_ALWAYS);
            M_Leave(p);
            return;
        }
    }

    for (int32_t frame = 0; frame < INV_RING_FRAMES; frame++) {
        for (int32_t i = 0; i < ring->number_of_objects; i++) {
            InvRing_UpdateInventoryItem(ring, ring->list[i]);
        }
        InvRing_DoMotions(ring);
    }
}

void Option_Combine_Draw(void)
{
    M_PRIV *const p = &m_Priv;
    if (!p->is_ready) {
        return;
    }
    Output_Overlay_DrawBlackRectangle(M_BACKDROP_OPACITY, false);
    Output_Flush();
    InvRing_DrawItems(&p->ring);

    UI_BeginModal(0.5f, 0.5f);
    UI_BeginAnchor(0.5f, 0.5f);
    UI_BeginFrame(UI_FRAME_OUTLINE_ONLY);
    UI_BeginPad(M_FRAME_PAD, M_FRAME_PAD);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_CENTER },
        .spacing = { .v = M_FRAME_PAD },
    });
    UI_Spacer(M_FRAME_WIDTH, M_FRAME_HEIGHT);
    UI_Label(Object_GetName(p->ring.list[p->ring.current_object]->object_id));
    UI_EndStack();
    UI_EndPad();
    UI_EndFrame();
    UI_EndAnchor();
    UI_EndModal();
}

void Option_Combine_Close(void)
{
    M_PRIV *const p = &m_Priv;
    p->is_ready = false;
    InvRing_ClearButtonHint();
}

OBJECT_ID Option_Combine_TakeChoice(void)
{
    M_PRIV *const p = &m_Priv;
    const OBJECT_ID choice = p->choice;
    p->choice = NO_OBJECT;
    return choice;
}
