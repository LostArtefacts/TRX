#include <trx/game/ui/mesh_slots.h>

#include <trx/game/interpolation.h>

static UI_MESH_SLOT m_Slots[UI_MESH_SLOT_MAX];
static uint32_t m_SlotGens[UI_MESH_SLOT_MAX];
static HANDLE_REGISTRY m_SlotHandles = {
    .gens = m_SlotGens,
    .capacity = UI_MESH_SLOT_MAX,
};

static float M_Lerp(const float from, const float to, const double rate)
{
    return from + (float)((to - from) * rate);
}

// Turns the short way around, so a model crossing the wrap does not spin back
// through every angle between.
static int32_t M_LerpAngle(
    const int32_t from, const int32_t to, const double rate)
{
    int32_t delta = (to - from) & 0xFFFF;
    if (delta > 0x8000) {
        delta -= 0x10000;
    }
    return from + (int32_t)(delta * rate);
}

TRX_HANDLE UI_MeshSlot_Acquire(void)
{
    for (int32_t i = 0; i < UI_MESH_SLOT_MAX; i++) {
        if (!m_Slots[i].in_use) {
            m_Slots[i] = (UI_MESH_SLOT) { .in_use = true };
            Handle_RegistryBump(&m_SlotHandles, i);
            return Handle_RegistryMint(&m_SlotHandles, i);
        }
    }
    return (TRX_HANDLE) { .id = -1 };
}

void UI_MeshSlot_Release(const TRX_HANDLE handle)
{
    if (UI_MeshSlot_Resolve(handle) == nullptr) {
        return;
    }
    m_Slots[handle.id] = (UI_MESH_SLOT) {};
    Handle_RegistryBump(&m_SlotHandles, handle.id);
}

UI_MESH_SLOT *UI_MeshSlot_Resolve(const TRX_HANDLE handle)
{
    if (!Handle_RegistryIsLive(&m_SlotHandles, handle)
        || !m_Slots[handle.id].in_use) {
        return nullptr;
    }
    return &m_Slots[handle.id];
}

void UI_MeshSlot_Move(
    const TRX_HANDLE handle, const OBJECT_ID object_id,
    const uint32_t mesh_mask, const UI_MESH_POSE pose)
{
    UI_MESH_SLOT *const slot = UI_MeshSlot_Resolve(handle);
    if (slot == nullptr) {
        return;
    }
    // A model that was hidden, or that names a different object than it did,
    // starts where it is put rather than travelling there from wherever the
    // slot was last used.
    slot->has_prev = slot->visible && slot->object_id == object_id;
    slot->prev = slot->cur;
    slot->cur = pose;
    slot->object_id = object_id;
    slot->mesh_mask = mesh_mask;
    slot->visible = true;
}

void UI_MeshSlot_Hide(const TRX_HANDLE handle)
{
    UI_MESH_SLOT *const slot = UI_MeshSlot_Resolve(handle);
    if (slot != nullptr) {
        slot->visible = false;
        slot->has_prev = false;
    }
}

bool UI_MeshSlots_AnyShown(void)
{
    for (int32_t i = 0; i < UI_MESH_SLOT_MAX; i++) {
        if (m_Slots[i].in_use && m_Slots[i].visible) {
            return true;
        }
    }
    return false;
}

int32_t UI_MeshSlots_Collect(UI_MESH_DRAW *const out, const int32_t max)
{
    const double rate = Interpolation_GetRate();
    int32_t count = 0;
    for (int32_t i = 0; i < UI_MESH_SLOT_MAX && count < max; i++) {
        const UI_MESH_SLOT *const slot = &m_Slots[i];
        if (!slot->in_use || !slot->visible) {
            continue;
        }
        const UI_MESH_POSE from = slot->has_prev ? slot->prev : slot->cur;
        out[count++] = (UI_MESH_DRAW) {
            .object_id = slot->object_id,
            .mesh_mask = slot->mesh_mask,
            .pose = {
                .x = M_Lerp(from.x, slot->cur.x, rate),
                .y = M_Lerp(from.y, slot->cur.y, rate),
                .w = M_Lerp(from.w, slot->cur.w, rate),
                .h = M_Lerp(from.h, slot->cur.h, rate),
                .rot = {
                    .x = (int16_t)M_LerpAngle(from.rot.x, slot->cur.rot.x, rate),
                    .y = (int16_t)M_LerpAngle(from.rot.y, slot->cur.rot.y, rate),
                    .z = (int16_t)M_LerpAngle(from.rot.z, slot->cur.rot.z, rate),
                },
            },
        };
    }
    return count;
}

void UI_MeshSlots_Reset(void)
{
    Handle_RegistryBumpAll(&m_SlotHandles);
    for (int32_t i = 0; i < UI_MESH_SLOT_MAX; i++) {
        m_Slots[i] = (UI_MESH_SLOT) {};
    }
}
