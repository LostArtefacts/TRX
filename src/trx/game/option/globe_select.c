#include <trx/game/option/globe_select.h>

#include <trx/debug.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_string.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring.h>
#include <trx/game/overlay.h>
#include <trx/game/savegame.h>
#include <trx/utils.h>

typedef struct {
    XYZ_16 rot;
    int32_t start_level_ordinal;
    int32_t completion_level_ordinal;
    uint32_t prereq_mask;
    const char *area_name;
    uint8_t mesh_idx;
} M_GLOBE_ENTRY;

static const M_GLOBE_ENTRY m_GlobeEntries[] = {
    {
        .rot = { -1536, -7936, 1536 },
        .start_level_ordinal = 1,
        .completion_level_ordinal = 4,
        .prereq_mask = 0u,
        .area_name = "India",
        .mesh_idx = 2,
    },
    {
        .rot = { 1024, -512, -256 },
        .start_level_ordinal = 5,
        .completion_level_ordinal = 8,
        .prereq_mask = 1u << 0,
        .area_name = "South Pacific Islands",
        .mesh_idx = 5,
    },
    {
        .rot = { 2560, 21248, -4096 },
        .start_level_ordinal = 13,
        .completion_level_ordinal = 15,
        .prereq_mask = 1u << 0,
        .area_name = "Nevada Desert",
        .mesh_idx = 4,
    },
    {
        .rot = { -3328, 29440, 1024 },
        .start_level_ordinal = -1,
        .completion_level_ordinal = -1,
        .prereq_mask = 0u,
        .area_name = nullptr, // Unused Peru
        .mesh_idx = 3,
    },
    {
        .rot = { 3072, -20992, 6400 },
        .start_level_ordinal = 9,
        .completion_level_ordinal = 12,
        .prereq_mask = 1u << 0,
        .area_name = "London",
        .mesh_idx = 1,
    },
    {
        .rot = { -5120, -15360, -18688 },
        .start_level_ordinal = 16,
        .completion_level_ordinal = 19,
        .prereq_mask = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 4),
        .area_name = "Antarctica",
        .mesh_idx = 6,
    },
};

#define M_GLOBE_ENTRY_COUNT ARRAY_SIZE(m_GlobeEntries)

static bool M_IsLevelCompleted(const int32_t level_ordinal)
{
    if (level_ordinal < 0) {
        return false;
    }
    const GF_LEVEL *const level =
        GF_GetLevelByOrdinalNumber(GFLT_MAIN, level_ordinal);
    if (level == nullptr) {
        return false;
    }
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    return resume != nullptr && resume->level_completed;
}

static int32_t M_GetNextSelectableIndex(
    const INV_RING *const ring, const int32_t direction)
{
    if (direction == 0) {
        return ring->globe_select.selection;
    }

    for (size_t step = 0; step < M_GLOBE_ENTRY_COUNT; step++) {
        int32_t idx = ring->globe_select.selection + direction * (step + 1);
        while (idx < 0) {
            idx += M_GLOBE_ENTRY_COUNT;
        }
        idx %= M_GLOBE_ENTRY_COUNT;
        if (ring->globe_select.selectable[idx]) {
            return idx;
        }
    }
    return ring->globe_select.selection;
}

static bool M_UpdateRotAxis(int16_t *const cur, const int16_t target)
{
    int16_t ang = target - *cur;
    if (ang >= 128 || ang <= -128) {
        *cur += ang >> 3;
        return false;
    }
    *cur = target;
    return true;
}

static bool M_IsAligned(INV_RING *const ring)
{
    const M_GLOBE_ENTRY *const entry =
        &m_GlobeEntries[ring->globe_select.selection];
    int32_t axes = 0;
    axes += M_UpdateRotAxis(&ring->globe_select.rot.x, entry->rot.x) ? 1 : 0;
    axes += M_UpdateRotAxis(&ring->globe_select.rot.y, entry->rot.y) ? 1 : 0;
    axes += M_UpdateRotAxis(&ring->globe_select.rot.z, entry->rot.z) ? 1 : 0;
    return axes == 3;
}

int32_t Option_GlobeSelect_AreaFromMeshIdx(const int32_t mesh_idx)
{
    for (size_t i = 0; i < M_GLOBE_ENTRY_COUNT; i++) {
        if (m_GlobeEntries[i].mesh_idx == mesh_idx) {
            return i;
        }
    }
    return -1;
}

void Option_GlobeSelect_UpdateSelectable(INV_RING *const ring)
{
    ring->globe_select.selection = -1;
    ring->globe_select.rot.x = 0;
    ring->globe_select.rot.y = 0;
    ring->globe_select.rot.z = 0;
    ring->globe_select.meshes_drawn = 0x0FFFu;
    ring->globe_select.confirmed = false;
    for (int32_t i = 0; i < 6; i++) {
        ring->globe_select.selectable[i] = false;
        ring->globe_select.start_level_num[i] = -1;
    }

    uint32_t completed_mask = 0u;
    for (size_t i = 0; i < M_GLOBE_ENTRY_COUNT; i++) {
        if (M_IsLevelCompleted(m_GlobeEntries[i].completion_level_ordinal)) {
            completed_mask |= 1u << i;
        }
    }

    int32_t selectable_count = 0;
    for (size_t i = 0; i < M_GLOBE_ENTRY_COUNT; i++) {
        ring->globe_select.selectable[i] = false;
        ring->globe_select.start_level_num[i] = -1;

        const M_GLOBE_ENTRY *const entry = &m_GlobeEntries[i];
        if (entry->area_name == nullptr) {
            continue;
        }

        const GF_LEVEL *const start_level =
            GF_GetLevelByOrdinalNumber(GFLT_MAIN, entry->start_level_ordinal);
        ring->globe_select.start_level_num[i] = start_level->num;

        if ((completed_mask & (1u << i)) != 0u) {
            continue;
        }
        if ((completed_mask & entry->prereq_mask) != entry->prereq_mask) {
            continue;
        }

        if (start_level == nullptr) {
            continue;
        }

        ring->globe_select.selectable[i] = true;
        selectable_count++;
    }

    ring->globe_select.meshes_drawn = 0x0FFFu;
    for (size_t i = 0; i < M_GLOBE_ENTRY_COUNT; i++) {
        if (ring->globe_select.start_level_num[i] < 0) {
            ring->globe_select.meshes_drawn &=
                ~(1 << m_GlobeEntries[i].mesh_idx);
        }
    }

    Overlay_ShowArrow(UI_OVERLAY_ARROW_BCL, selectable_count > 1);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BCR, selectable_count > 1);

    if (ring->globe_select.selection < 0
        || ring->globe_select.selection >= (int32_t)M_GLOBE_ENTRY_COUNT
        || !ring->globe_select.selectable[ring->globe_select.selection]) {
        ring->globe_select.selection = -1;
        for (size_t i = 0; i < M_GLOBE_ENTRY_COUNT; i++) {
            if (ring->globe_select.selectable[i]) {
                ring->globe_select.selection = i;
                break;
            }
        }
    }
}

void Option_GlobeSelect_Control(
    INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    INV_RING *const ring = InvRing_GetActiveRing();
    if (ring == nullptr || ring->mode != INV_GLOBE_SELECT_MODE) {
        return;
    }

    Overlay_SetTopText((OVERLAY_TEXT) {
        .kind = UI_OVERLAY_TEXT_GS_KEY,
        .fmt_gs_key = GS_ID(INVENTORY_RING_HEADING_FMT),
        .literal = GS_ID(INVENTORY_RING_HEADING_ADVENTURE),
    });

    if (ring->globe_select.selection < 0) {
        Overlay_SetBottomText((OVERLAY_TEXT) { 0 });
        ring->globe_select.confirmed = false;
        return;
    }

    const bool aligned = M_IsAligned(ring);

    if (aligned && !is_busy) {
        if (g_Input.menu_left) {
            ring->globe_select.selection = M_GetNextSelectableIndex(ring, -1);
        } else if (g_Input.menu_right) {
            ring->globe_select.selection = M_GetNextSelectableIndex(ring, 1);
        }
    }

    const M_GLOBE_ENTRY *const entry =
        &m_GlobeEntries[ring->globe_select.selection];
    if (entry->area_name != nullptr) {
        Overlay_SetBottomText((OVERLAY_TEXT) {
            .kind = UI_OVERLAY_TEXT_LITERAL,
            .fmt_gs_key = GS_ID(INVENTORY_RING_OBJECT_NAME_FMT),
            .literal = entry->area_name,
        });
    } else {
        Overlay_SetBottomText((OVERLAY_TEXT) { 0 });
    }

    if (g_InputDB.menu_confirm && !is_busy) {
        if (!aligned) {
            g_InputDB.menu_confirm = false;
            return;
        }
        ring->globe_select.confirmed = true;
    }
}

void Option_GlobeSelect_Draw(INVENTORY_ITEM *const inv_item)
{
}

void Option_GlobeSelect_Close(void)
{
}

void Option_GlobeSelect_Shutdown(void)
{
}
