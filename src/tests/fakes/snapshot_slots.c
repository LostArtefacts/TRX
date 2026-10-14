// Hands out the overlay's snapshot slots without capturing anything, so fade
// layers that hold a snapshot or a backdrop can be made and closed without a
// renderer.

#include <trx/game/output/overlay.h>

static bool m_InUse[OUTPUT_OVERLAY_SNAPSHOT_SLOTS] = {};

int32_t Output_Overlay_CaptureSnapshotSlot(void)
{
    for (int32_t i = 0; i < OUTPUT_OVERLAY_SNAPSHOT_SLOTS; i++) {
        if (!m_InUse[i]) {
            m_InUse[i] = true;
            return i;
        }
    }
    return -1;
}

void Output_Overlay_CaptureGameSnapshot(void)
{
}

void Output_Overlay_FreeSnapshotSlot(const int32_t slot)
{
    if (slot >= 0 && slot < OUTPUT_OVERLAY_SNAPSHOT_SLOTS) {
        m_InUse[slot] = false;
    }
}
