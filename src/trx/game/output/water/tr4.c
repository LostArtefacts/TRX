#include <trx/config.h>
#include <trx/core/colors.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output/state.h>
#include <trx/game/output/water/priv.h>
#include <trx/game/rooms.h>

// Stores each Lara mesh's room and the ambient values of the two rooms she
// spans, matching the original once-per-frame ambient selection.
static bool m_IsMeshSubmerged[LM_NUMBER_OF] = {};
static RGB_888 m_MeshAmbient[LM_NUMBER_OF] = {};
static RGB_888 m_Ambient[2] = {};
static bool m_IsInWater = false;
static int32_t m_Surface = NO_HEIGHT;

// Maps a joint index to its mesh in the original order.
static const LARA_MESH m_JointMesh[LM_NUMBER_OF] = {
    LM_HIPS,   LM_THIGH_L, LM_CALF_L, LM_FOOT_L, LM_THIGH_R,
    LM_CALF_R, LM_FOOT_R,  LM_TORSO,  LM_HEAD,   LM_UARM_R,
    LM_LARM_R, LM_HAND_R,  LM_UARM_L, LM_LARM_L, LM_HAND_L,
};

static void M_ObserveLaraFrame(void)
{
    const ITEM *const item = Lara_GetItem();
    bool has_dry = false;
    bool has_wet = false;
    m_Surface = NO_HEIGHT;

    // Reads Lara's water state from her position so a single submerged limb
    // does not put her in water.
    int16_t item_room_num = item->room_num;
    Room_GetSector(item->pos, &item_room_num);
    m_IsInWater = Room_Get(item_room_num)->flags.underwater;

    for (int32_t joint = LM_NUMBER_OF - 1; joint >= 0; joint--) {
        const LARA_MESH mesh = m_JointMesh[joint];
        XYZ_32 pos = {};
        if (!Lara_GetMeshPos(mesh, &pos)) {
            Lara_GetJointAbsPosition(&pos, mesh);
        }
        // Applies the original lift before reading the room.
        if (mesh == LM_TORSO) {
            pos.y -= 120;
        } else if (mesh == LM_HEAD) {
            pos.y -= 60;
        }

        int16_t room_num = item->room_num;
        Room_GetSector(pos, &room_num);
        const ROOM *const room = Room_Get(room_num);

        if (room->flags.underwater) {
            const int32_t surface = Room_GetWaterHeight(pos, room_num);
            if (surface != NO_HEIGHT) {
                m_Surface = surface;
            }
            if (!has_wet) {
                m_Ambient[1] = room->ambient_rgb;
                has_wet = true;
            }
        } else if (!has_dry) {
            m_Ambient[0] = room->ambient_rgb;
            has_dry = true;
        }
    }

    // Falls back to the room Lara stands in, because a frame whose joints
    // all read as dry would otherwise leave her in water with no surface to
    // cut her at and no water room to take the color from.
    if (m_IsInWater && !has_wet) {
        m_Ambient[1] = Room_Get(item_room_num)->ambient_rgb;
        has_wet = true;
    }
    if (m_IsInWater && m_Surface == NO_HEIGHT) {
        m_Surface = Room_GetWaterHeight(item->pos, item_room_num);
    }

    if (!has_dry) {
        m_Ambient[0] = m_Ambient[1];
    }
    if (!has_wet) {
        m_Ambient[1] = m_Ambient[0];
    }
}

// Reads a mesh's room and its ambient light from the position it is drawn at,
// so that both describe the frame on screen rather than the frame before it.
static void M_ObserveLaraMesh(const LARA_MESH mesh, GAME_VECTOR pos)
{
    if (mesh == WATER_LARA_MESH_OTHER) {
        return;
    }
    // Applies the original lift before reading the room.
    if (mesh == LM_TORSO) {
        pos.pos.y -= 120;
    } else if (mesh == LM_HEAD) {
        pos.pos.y -= 60;
    }
    Room_GetSector(pos.pos, &pos.room_num);
    const ROOM *const room = Room_Get(pos.room_num);
    m_IsMeshSubmerged[mesh] = room->flags.underwater;
    m_MeshAmbient[mesh] = room->ambient_rgb;
}

static WATER_EFFECTS M_GetEffects(
    const bool is_below_water, const bool is_camera_underwater)
{
    return (WATER_EFFECTS) {
        // Uses water-room lighting for geometry in a water room and reserves
        // the water colour for underwater views.
        .shade = !is_below_water && is_camera_underwater,
        .room_shade = is_camera_underwater,
        .wibble = is_camera_underwater,
    };
}

static bool M_IsObjectWibbleEnabled(void)
{
    return true;
}

static RGB_F M_GetSubmergedAmbientDelta(void)
{
    return (RGB_F) {
        .r = (m_Ambient[1].r - m_Ambient[0].r) / 255.0f,
        .g = (m_Ambient[1].g - m_Ambient[0].g) / 255.0f,
        .b = (m_Ambient[1].b - m_Ambient[0].b) / 255.0f,
    };
}

static WATER_LARA_MESH M_DecideLaraMesh(
    const LARA_MESH mesh, const GAME_VECTOR pos, const int32_t radius)
{
    if (!m_IsInWater
        || g_Config.visuals.water_tint_mode != WATER_TINT_MODE_RESPONSIVE) {
        return (WATER_LARA_MESH) { .tint = Output_GetTint() };
    }
    // Lights the mesh with the dry room and shifts the part below the water
    // surface towards the water room.
    return (WATER_LARA_MESH) {
        .tint = Output_GetTint(),
        .has_surface = m_Surface != NO_HEIGHT,
        .surface = m_Surface,
        .has_submerged_ambient = true,
        .submerged_ambient_delta = M_GetSubmergedAmbientDelta(),
    };
}

static const RGB_888 *M_GetMeshAmbient(const LARA_MESH mesh)
{
    if (mesh == WATER_LARA_MESH_OTHER) {
        return nullptr;
    }
    return &m_MeshAmbient[mesh];
}

// Controls Lara's water-room lighting contribution when water colour is
// represented through lighting.
static const RGB_888 *M_GetLaraAmbient(
    const LARA_MESH parent, const LARA_MESH child)
{
    if (!m_IsInWater) {
        return nullptr;
    }

    switch (g_Config.visuals.water_tint_mode) {
    case WATER_TINT_MODE_WHOLE:
        // Keeps the single ambient light the original gives Lara, which comes
        // from the room her torso sits in.
        return nullptr;

    case WATER_TINT_MODE_PER_MESH:
        break;

    case WATER_TINT_MODE_RESPONSIVE:
        // Lights the whole mesh with the dry room, so that the shift below
        // the water surface lands on the water room.
        return &m_Ambient[0];

    case WATER_TINT_MODE_NUMBER_OF:
        ASSERT_FAIL();
        return nullptr;
    }

    const RGB_888 *const to = M_GetMeshAmbient(child);
    return to != nullptr ? to : M_GetMeshAmbient(parent);
}

// Interpolates lighting across a joint spanning two rooms so its vertices
// carry the ambient light from each end.
static bool M_GetLaraAmbientSpan(
    const LARA_MESH parent, const LARA_MESH child, RGB_888 *const out_from)
{
    if (!m_IsInWater || parent == child
        || g_Config.visuals.water_tint_mode != WATER_TINT_MODE_PER_MESH) {
        return false;
    }
    const RGB_888 *const from = M_GetMeshAmbient(parent);
    const RGB_888 *const to = M_GetMeshAmbient(child);
    if (from == nullptr || to == nullptr
        || (from->r == to->r && from->g == to->g && from->b == to->b)) {
        return false;
    }
    *out_from = *from;
    return true;
}

const WATER_MODEL g_WaterModelTR4 = {
    .get_effects = M_GetEffects,
    .is_object_wibble_enabled = M_IsObjectWibbleEnabled,
    .observe_lara_frame = M_ObserveLaraFrame,
    .observe_lara_mesh = M_ObserveLaraMesh,
    .decide_lara_mesh = M_DecideLaraMesh,
    .get_lara_ambient = M_GetLaraAmbient,
    .get_lara_ambient_span = M_GetLaraAmbientSpan,
};
