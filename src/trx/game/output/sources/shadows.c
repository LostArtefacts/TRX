#include <trx/game/output/sources/shadows.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/game/anims/walk.h>
#include <trx/game/collision/common.h>
#include <trx/game/lara/common.h>
#include <trx/game/lara/pose.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/output/bind.h>
#include <trx/game/output/mesh_batcher/mesh_builder.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/rooms.h>
#include <trx/version.h>

#define M_SHADOW_LINE_POINTS 4
#define M_SHADOW_GRID_POINTS (M_SHADOW_LINE_POINTS * M_SHADOW_LINE_POINTS)

typedef struct {
    MESH_BATCHER *batcher;
    OUTPUT_MESH *mesh_low;
    OUTPUT_MESH *mesh_high;
} M_PRIV;

static M_PRIV m_Priv;

static OUTPUT_MESH *M_GenerateShadow(
    MESH_BUILDER *const builder, const int32_t fidelity)
{
    const int32_t y = -5;
    const RGBA_8888 color = { 0, 0, 0, g_TRVersion == 4 ? 0x4F : 128 };
    const OUTPUT_MESH_VERTEX center = {
        .pos = { 0.0f, (float)y, 0.0f, 0.0f },
        .normal = { 0.0f, 0.0f, 0.0f },
        .flags = VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_WIBBLE,
        .uvw_idx = -1,
        .trapezoid_ratio = { 1.0f, 1.0f },
        .reflectivity = 1.0f,
        .shade = SHADE_NEUTRAL,
        .color = color,
    };

    MeshBuilder_AddVertex(builder, &center);
    for (int32_t i = 0; i <= fidelity; i++) {
        const int16_t angle = ((i * DEG_360) + DEG_180) / fidelity;
        const int32_t size = WALL_L / 2;
        const XYZ_32 point = XYZ_32_RotateYaw((XYZ_32) { .z = size }, angle);
        const int32_t x = point.x;
        const int32_t z = point.z;
        OUTPUT_MESH_VERTEX edge = center;
        edge.pos.x = x;
        edge.pos.z = z;
        MeshBuilder_AddVertex(builder, &edge);
    }
    // The shadow never writes depth: it lies on the floor and occludes
    // nothing, and its depth-writing copy would be a second sorted draw for an
    // instance drawn at partial coverage.
    MeshBuilder_AddFan(builder, SCENE_PASS_TRANSPARENT, false, false);
    return MeshBuilder_Seal(builder);
}

// Answers for Lara alone, whose shadow takes the floor under the place the
// frame or the pose moves her to rather than the floor her item stands on.
// Crawling puts the root away from her, and the item answers there instead.
// False for anything but Lara on her feet.
static bool M_GetLaraAnchor(
    const ITEM *const item, XYZ_32 *const anchor_pos, int32_t *const floor)
{
    const int16_t anim_state = item->current_anim_state;
    if (item != Lara_GetItem() || anim_state == LS(LS_CRAWL_IDLE)
        || anim_state == LS(LS_CRAWL_FORWARD) || anim_state == LS(LS_CRAWL_BACK)
        || anim_state == LS(LS_CRAWL_TURN_LEFT)
        || anim_state == LS(LS_CRAWL_TURN_RIGHT)) {
        return false;
    }

    XYZ_32 hips = {};
    Collide_GetJointAbsPosition(item, &hips, LM_HIPS);
    *anchor_pos = hips;

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(*anchor_pos, &room_num);
    const int32_t height = Room_GetHeight(sector, *anchor_pos);
    if (height != NO_HEIGHT) {
        *floor = height;
    }
    return true;
}

// Measures the box a pose puts around Lara, in the space her item stands in.
// A pose holds her through a scene while her item keeps the animation frame it
// stopped on, so the box that frame carries reports the wrong size. False for
// anything but Lara under a pose.
static bool M_GetPoseBounds(const ITEM *const item, BOUNDS_16 *const out)
{
    if (item != Lara_GetItem()) {
        return false;
    }
    const LARA_POSE *const pose = Lara_Pose_Get();
    if (pose == nullptr) {
        return false;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    XYZ_32 min = { INT32_MAX, INT32_MAX, INT32_MAX };
    XYZ_32 max = { INT32_MIN, INT32_MIN, INT32_MIN };

    Matrix_PushUnit();
    ANIM_WALK walk;
    Anim_Walk_Begin(
        &walk,
        &(ANIM_WALK_DESC) {
            .obj = obj,
            .pose = Anim_Pose_FromRots(pose->rots, pose->offset),
            .extra_rotations = item->extra_rotations,
        });
    while (Anim_Walk_Next(&walk)) {
        const OBJECT_MESH *const mesh =
            Object_GetMesh(obj->mesh_idx + walk.joint);
        for (int32_t i = 0; i < mesh->num_vertices; i++) {
            const XYZ_32 vertex =
                Anim_Walk_GetPos(&walk, XYZ_32_From16(mesh->vertices[i]));
            min.x = MIN(min.x, vertex.x);
            min.y = MIN(min.y, vertex.y);
            min.z = MIN(min.z, vertex.z);
            max.x = MAX(max.x, vertex.x);
            max.y = MAX(max.y, vertex.y);
            max.z = MAX(max.z, vertex.z);
        }
    }
    Anim_Walk_End(&walk);
    Matrix_Pop();

    if (min.x > max.x) {
        return false;
    }

    *out = (BOUNDS_16) {
        .min = { min.x, min.y, min.z },
        .max = { max.x, max.y, max.z },
    };
    return true;
}

// Where an item's shadow lies and the floor it lies on. Both shadow styles ask
// here, so that the two agree. An item stands at its own origin, which can sit
// away from the meshes it draws, so the middle of the box it carries is what
// the shadow follows. The Lara anchor already answers at her middle and takes
// no such offset.
static void M_GetPlacement(
    const ITEM *const item, const BOUNDS_16 *const bounds,
    XYZ_32 *const anchor_pos, int32_t *const floor)
{
    *anchor_pos = item->interp.result.pos;
    *floor = item->interp.result.floor;
    if (M_GetLaraAnchor(item, anchor_pos, floor)) {
        return;
    }

    const XYZ_32 offset = {
        .x = (bounds->min.x + bounds->max.x) / 2,
        .z = (bounds->min.z + bounds->max.z) / 2,
    };
    *anchor_pos =
        XYZ_32_OffsetLocalYaw(*anchor_pos, offset, item->interp.result.rot.y);
}

static bool M_DrawSprite(
    const int32_t size, const BOUNDS_16 *const bounds, const ITEM *const item)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return false;
    }

    const OBJECT *const shadow_obj = Object_Get(O_SHADOW);
    if (!shadow_obj->loaded) {
        return false;
    }

    // OG: shadow intensity is based on Lara's height above the floor, even for
    // non-Lara items.
    int32_t c = ((4096 - ABS(item->floor - item->pos.y)) >> 4) - 1;
    if (g_Config.rendering.lighting_curve == LIGHTING_CURVE_SATURATE) {
        c >>= 1;
    }
    CLAMP(c, 32, 255);

    const RGBA_8888 shadow_color = { c, c, c, 255 };
    const RGBA_8888 quad_color[4] = {
        shadow_color,
        shadow_color,
        shadow_color,
        shadow_color,
    };

    const int32_t x_size = size * (bounds->max.x - bounds->min.x) / 128;
    const int32_t z_size = size * (bounds->max.z - bounds->min.z) / 128;
    const int32_t x_dist = x_size / M_SHADOW_LINE_POINTS;
    const int32_t z_dist = z_size / M_SHADOW_LINE_POINTS;

    int32_t grid_local_x[M_SHADOW_GRID_POINTS];
    int32_t grid_local_z[M_SHADOW_GRID_POINTS];
    int32_t x = -x_dist - (x_dist >> 1);
    int32_t z = z_dist + (z_dist >> 1);
    int32_t grid_idx = 0;
    for (int32_t row = 0; row < M_SHADOW_LINE_POINTS; row++) {
        for (int32_t col = 0; col < M_SHADOW_LINE_POINTS; col++) {
            grid_local_x[grid_idx] = x;
            grid_local_z[grid_idx] = z;
            grid_idx++;
            x += x_dist;
        }
        x = -x_dist - (x_dist >> 1);
        z -= z_dist;
    }

    XYZ_32 anchor_pos;
    int32_t anchor_floor;
    M_GetPlacement(item, bounds, &anchor_pos, &anchor_floor);

    const int32_t base_y = anchor_floor - 16;

    // Compute the world-space grid points with floor-conforming Y offsets.
    XYZ_32 grid_world[M_SHADOW_GRID_POINTS];
    for (int32_t i = 0; i < M_SHADOW_GRID_POINTS; i++) {
        const XYZ_32 corner = XYZ_32_OffsetLocalYaw(
            anchor_pos, (XYZ_32) { .x = grid_local_x[i], .z = grid_local_z[i] },
            item->interp.result.rot.y);
        const int32_t wx = corner.x;
        const int32_t wz = corner.z;

        int16_t room_num = item->room_num;
        XYZ_32 test_pos = { wx, anchor_floor, wz };
        const SECTOR *const sector = Room_GetSector(test_pos, &room_num);
        int32_t height = Room_GetHeight(sector, test_pos);
        if (height == NO_HEIGHT) {
            height = anchor_floor;
        }
        if (ABS(height - anchor_floor) > 196) {
            height = anchor_floor;
        }

        grid_world[i] = (XYZ_32) {
            .x = wx,
            .y = base_y + (height - anchor_floor),
            .z = wz,
        };
    }

    const int32_t sprite_idx = shadow_obj->mesh_idx;
    const int32_t uvw_idx = Output_Textures_GetSpriteUVWIndex(sprite_idx, 0);
    const OUTPUT_TEXTURE_SIZE atlas_size =
        Output_Textures_GetAtlasSize(uvw_idx / 4);
    const OUTPUT_TEXTURE_SIZE quad_atlas_size[4] = {
        atlas_size,
        atlas_size,
        atlas_size,
        atlas_size,
    };

    OUTPUT_UVW sprite_uvw[4];
    for (int32_t i = 0; i < 4; i++) {
        const int32_t corner_uvw_idx =
            Output_Textures_GetSpriteUVWIndex(sprite_idx, i);
        sprite_uvw[i] = Output_Textures_GetUVW(corner_uvw_idx);
    }

    const float u_min =
        MIN(MIN(sprite_uvw[0].u, sprite_uvw[1].u),
            MIN(sprite_uvw[2].u, sprite_uvw[3].u));
    const float u_max =
        MAX(MAX(sprite_uvw[0].u, sprite_uvw[1].u),
            MAX(sprite_uvw[2].u, sprite_uvw[3].u));
    const float v_min =
        MIN(MIN(sprite_uvw[0].v, sprite_uvw[1].v),
            MIN(sprite_uvw[2].v, sprite_uvw[3].v));
    const float v_max =
        MAX(MAX(sprite_uvw[0].v, sprite_uvw[1].v),
            MAX(sprite_uvw[2].v, sprite_uvw[3].v));
    const float w = sprite_uvw[0].w;

    const float u_span = u_max - u_min;
    const float v_span = v_max - v_min;
    const float denom = (float)(M_SHADOW_LINE_POINTS - 1);

    // The shadow subtracts what it draws, so a colored tint would take an
    // uneven bite out of the floor and shift its hue rather than darken it.
    // Only the coverage carries over, which is what an item's fade rides on.
    RGBA_F tint = Output_GetTint();
    tint.r = 1.0f;
    tint.g = 1.0f;
    tint.b = 1.0f;
    Output_PushTintOverride(tint);

    for (int32_t row = 0; row < M_SHADOW_LINE_POINTS - 1; row++) {
        const float v0 = v_min + v_span * ((float)row / denom);
        const float v1 = v_min + v_span * ((float)(row + 1) / denom);
        for (int32_t col = 0; col < M_SHADOW_LINE_POINTS - 1; col++) {
            const float u0 = u_min + u_span * ((float)col / denom);
            const float u1 = u_min + u_span * ((float)(col + 1) / denom);

            const int32_t i0 = (row * M_SHADOW_LINE_POINTS) + col;
            const int32_t i1 = i0 + 1;
            const int32_t i2 = i0 + (M_SHADOW_LINE_POINTS + 1);
            const int32_t i3 = i0 + M_SHADOW_LINE_POINTS;

            const XYZ_32 quad_pos[4] = {
                grid_world[i0],
                grid_world[i1],
                grid_world[i2],
                grid_world[i3],
            };
            const OUTPUT_UVW quad_uvw[4] = {
                { u0, v0, w },
                { u1, v0, w },
                { u1, v1, w },
                { u0, v1, w },
            };

            OutputSource_PolyFX_StageQuadExtUV(
                quad_pos, quad_uvw, quad_atlas_size, nullptr, quad_color,
                VERT_NO_LIGHTING | VERT_NO_WIBBLE, DRAW_BLEND_SUB);
        }
    }

    Output_PopTintOverride();
    return true;
}

static void M_StageShadow(void)
{
    M_PRIV *const p = &m_Priv;
    OUTPUT_MESH *const mesh = g_Config.visuals.shadow_type == SHADOW_TYPE_CIRCLE
        ? p->mesh_high
        : p->mesh_low;
    const MESH_INSTANCE inst = {
        .mesh = mesh,
        .cwmatrix = *g_MatrixPtr,
        .wmatrix = *g_WMatrixPtr,
        // The shadow is black, so the tint only reaches it through its alpha:
        // an item drawn at partial coverage takes its shadow along with it.
        .tint = Output_GetTint(),
        .sort_layer = -1,
        .room = Output_GetCurrentRoom(),
    };
    // XXX: Mesh batcher currently collects the transparent faces for the
    // transparent pass in the opaque pass, so the shadow, even though
    // transparent, needs to be staged in the opaque pass to work.
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_OPAQUE);
}

void OutputSource_Shadows_Init(MESH_BATCHER *const batcher)
{
    M_PRIV *const p = &m_Priv;
    p->batcher = batcher;

    // Build low- and high-fidelity circular shadow meshes.
    MESH_BUILDER *const builder = MeshBuilder_Create();
    p->mesh_low = M_GenerateShadow(builder, 8);
    p->mesh_high = M_GenerateShadow(builder, 32);
    MeshBuilder_Destroy(builder);

    MeshBatcher_AddMesh(p->batcher, p->mesh_low);
    MeshBatcher_AddMesh(p->batcher, p->mesh_high);
}

void OutputSource_Shadows_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->mesh_low != nullptr) {
        if (p->batcher != nullptr) {
            MeshBatcher_RemoveMesh(p->batcher, p->mesh_low);
        }
        Output_Mesh_Destroy(p->mesh_low);
        p->mesh_low = nullptr;
    }
    if (p->mesh_high != nullptr) {
        if (p->batcher != nullptr) {
            MeshBatcher_RemoveMesh(p->batcher, p->mesh_high);
        }
        Output_Mesh_Destroy(p->mesh_high);
        p->mesh_high = nullptr;
    }
    p->batcher = nullptr;
}

void OutputSource_Shadows_Draw(
    const int16_t size, const BOUNDS_16 *bounds, const ITEM *const item)
{
    if (!item->enable_shadow) {
        return;
    }

    OUTPUT_ITEM_BIND *const bind = Output_Bind_GetItem(item);
    if (bind->shadow_drawn) {
        return;
    }
    bind->shadow_drawn = true;

    BOUNDS_16 pose_bounds;
    if (M_GetPoseBounds(item, &pose_bounds)) {
        bounds = &pose_bounds;
    }

    if (g_Config.visuals.shadow_type == SHADOW_TYPE_SPRITE) {
        if (M_DrawSprite(size, bounds, item)) {
            return;
        }
    }

    const int32_t x_size = (bounds->max.x - bounds->min.x) * size / 1024;
    const int32_t z_size = (bounds->max.z - bounds->min.z) * size / 1024;

    Matrix_Push();
    *g_MatrixPtr = g_ViewMatrix;
    *g_WMatrixPtr = g_IDMatrix;
    XYZ_32 anchor_pos;
    int32_t anchor_floor;
    M_GetPlacement(item, bounds, &anchor_pos, &anchor_floor);

    Matrix_TranslateAbs(anchor_pos.x, anchor_floor, anchor_pos.z);
    Matrix_RotY(item->interp.result.rot.y);
    Matrix_ScaleX((1 << W2V_SHIFT) * x_size / UNIT_SHADOW);
    Matrix_ScaleZ((1 << W2V_SHIFT) * z_size / UNIT_SHADOW);
    M_StageShadow();
    Matrix_Pop();
}
