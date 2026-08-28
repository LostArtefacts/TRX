#include <trx/debug.h>
#include <trx/game/output/state.h>
#include <trx/game/output/water.h>
#include <trx/game/output/water/priv.h>
#include <trx/version.h>

typedef struct {
    LARA_MESH parent;
    LARA_MESH child;
} M_LARA_MESH;

static M_LARA_MESH m_LaraMeshStack[8] = {};
static int32_t m_LaraMeshDepth = 0;
static WATER_EFFECTS m_Effects = {};
static bool m_IsSubmerged = false;

static const WATER_MODEL *M_GetModel(void)
{
    return g_TRVersion >= 4 ? &g_WaterModelTR4 : &g_WaterModelTR123;
}

static M_LARA_MESH M_GetCurrentLaraMesh(void)
{
    if (m_LaraMeshDepth == 0) {
        return (M_LARA_MESH) {
            .parent = WATER_LARA_MESH_OTHER,
            .child = WATER_LARA_MESH_OTHER,
        };
    }
    return m_LaraMeshStack[m_LaraMeshDepth - 1];
}

static void M_Setup(const bool is_below_water, const bool is_camera_underwater)
{
    m_IsSubmerged = is_below_water;
    m_Effects = M_GetModel()->get_effects(is_below_water, is_camera_underwater);
}

static void M_PushLara(
    const LARA_MESH parent, const LARA_MESH child, const GAME_VECTOR pos,
    const int32_t radius)
{
    const WATER_LARA_MESH decision =
        M_GetModel()->decide_lara_mesh(child, pos, radius);
    ASSERT(m_LaraMeshDepth < (int32_t)ARRAY_SIZE(m_LaraMeshStack));
    m_LaraMeshStack[m_LaraMeshDepth++] = (M_LARA_MESH) {
        .parent = parent,
        .child = child,
    };
    Output_PushWaterLine((OUTPUT_WATER_LINE) {
        .is_enabled = decision.has_surface,
        .world_y = (float)decision.surface,
        .has_submerged_ambient = decision.has_submerged_ambient,
        .submerged_ambient_delta = decision.submerged_ambient_delta,
    });
    Output_PushTintOverride(decision.tint);
}

void Output_Water_SetupAboveWater(const bool is_camera_underwater)
{
    M_Setup(false, is_camera_underwater);
}

void Output_Water_SetupBelowWater(const bool is_camera_underwater)
{
    M_Setup(true, is_camera_underwater);
}

bool Output_Water_IsShadeEnabled(void)
{
    return m_Effects.shade;
}

bool Output_Water_IsRoomShadeEnabled(void)
{
    return m_Effects.room_shade;
}

bool Output_Water_IsWibbleEnabled(void)
{
    return m_Effects.wibble;
}

bool Output_Water_IsObjectWibbleEnabled(void)
{
    return M_GetModel()->is_object_wibble_enabled() && m_Effects.wibble;
}

bool Output_Water_IsSubmerged(void)
{
    return m_IsSubmerged;
}

void Output_Water_ObserveLaraFrame(void)
{
    const WATER_MODEL *const model = M_GetModel();
    if (model->observe_lara_frame != nullptr) {
        model->observe_lara_frame();
    }
}

void Output_Water_ObserveLaraMesh(const LARA_MESH mesh, const GAME_VECTOR pos)
{
    const WATER_MODEL *const model = M_GetModel();
    if (model->observe_lara_mesh != nullptr) {
        model->observe_lara_mesh(mesh, pos);
    }
}

void Output_Water_PushLaraMesh(
    const LARA_MESH mesh, const GAME_VECTOR pos, const int32_t radius)
{
    M_PushLara(mesh, mesh, pos, radius);
}

void Output_Water_PushLaraJoint(
    const LARA_MESH parent, const LARA_MESH child, const GAME_VECTOR pos,
    const int32_t radius)
{
    M_PushLara(parent, child, pos, radius);
}

void Output_Water_PopLaraMesh(void)
{
    ASSERT(m_LaraMeshDepth > 0);
    m_LaraMeshDepth--;
    Output_PopTintOverride();
    Output_PopWaterLine();
}

const RGB_888 *Output_Water_GetLaraMeshAmbient(void)
{
    const WATER_MODEL *const model = M_GetModel();
    if (m_LaraMeshDepth == 0 || model->get_lara_ambient == nullptr) {
        return nullptr;
    }
    const M_LARA_MESH current = M_GetCurrentLaraMesh();
    return model->get_lara_ambient(current.parent, current.child);
}

bool Output_Water_GetLaraAmbientSpan(RGB_888 *const out_from)
{
    const WATER_MODEL *const model = M_GetModel();
    if (m_LaraMeshDepth == 0 || model->get_lara_ambient_span == nullptr) {
        return false;
    }
    const M_LARA_MESH current = M_GetCurrentLaraMesh();
    return model->get_lara_ambient_span(
        current.parent, current.child, out_from);
}
