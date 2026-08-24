// Sprite shadows start as a rotated rectangle on the X/Z plane. The rectangle
// is clipped into facets, and each facet is placed on the floor plane below it.
//
// Use room mesh first because it is the visible floor. Each room has a lazy
// per-sector index that maps sector cells to nearby mesh faces. Mesh faces are
// kept only when they match floor data closely enough; this rejects scenery
// drawn as floor, such as foliage or textured portals.
//
// Sector floor data is the fallback. It fills parts of the shadow not covered
// by accepted mesh facets, including ground hidden below scenery and walkable
// item floors.
//
// The staged shadow starts on the facet under the item, or on the facet nearest
// the item's floor if none contains the item. It then spreads across joined
// facets while the floor stays level, slopes gently, or falls away. A high
// upward step blocks the spread. Overlapping reached facets are drawn from the
// highest visible surface down.

#include <trx/game/output/sources/shadows.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/game/anims/walk.h>
#include <trx/game/cutseq/playback.h>
#include <trx/game/game_buf.h>
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

#include <float.h>
#include <math.h>
#include <string.h>

#define M_SHADOW_FLOOR_GAP 4
#define M_SHADOW_MAX_SECTOR_SPAN 3
#define M_SHADOW_MAX_POLY_POINTS 8
#define M_SHADOW_MAX_ROOMS 8
#define M_SHADOW_SOLID_TOLERANCE 32
#define M_SHADOW_MIN_FACET_AREA 1.0f
#define M_SHADOW_MAX_FACETS 32
#define M_SHADOW_JOIN_REACH 2.0f

// Maximum upward step between joined facets that still counts as continuous
// ground.
#define M_SHADOW_STEP_TOLERANCE 16.0f

// Clipping can leave shared vertices a fraction apart. Round them back to the
// same world unit before the facet is staged.
#define M_SHADOW_ROUND(F) ((int32_t)((F) < 0.0f ? (F) - 0.5f : (F) + 0.5f))

// Keep floor-height samples away from edges, where they can read the adjacent
// floor instead.
#define M_SHADOW_FACET_INSET 0.15f

typedef struct {
    float x;
    float z;
} M_POINT;

typedef struct {
    M_POINT points[M_SHADOW_MAX_POLY_POINTS];
    int32_t count;
} M_POLY;

typedef struct {
    M_POLY poly;
    float plane[3];
    M_POINT center;
    float height;
    int16_t room_num;
    bool staged;
} M_FACET;

typedef struct {
    XYZ_32 anchor_pos;
    int32_t anchor_floor;
    int16_t yaw;
    int32_t half_x;
    int32_t half_z;
    int16_t room_num;
    float u_min;
    float u_span;
    float v_min;
    float v_span;
    float w;
    OUTPUT_TEXTURE_SIZE atlas_size;
    RGBA_8888 color;
    int32_t reach;
} M_SHADOW_CTX;

typedef struct {
    const FACE *faces;
    int32_t face_count;
    int32_t size_x;
    int32_t size_z;
    int32_t *cell_start;
    int32_t *entries;
    int32_t *face_stamp;
} M_ROOM_INDEX;

typedef struct {
    MESH_BATCHER *batcher;
    OUTPUT_MESH *mesh_low;
    OUTPUT_MESH *mesh_high;
    M_ROOM_INDEX *room_index;
    TRX_HANDLE room_epoch;
    int32_t query_stamp;
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
    // Disable depth writes so the transparent shadow does not occlude later
    // geometry or need a second sorted pass.
    MeshBuilder_AddFan(builder, SCENE_PASS_TRANSPARENT, false, false);
    return MeshBuilder_Seal(builder);
}

static bool M_GetLaraBounds(const ITEM *const item, BOUNDS_16 *const out)
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

// Computes the shared anchor used by both shadow styles. Lara uses the floor
// under that anchor because animation can move her away from the item origin.
static void M_GetPlacement(
    const ITEM *const item, const BOUNDS_16 *const bounds,
    XYZ_32 *const anchor_pos, int32_t *const floor)
{
    const XYZ_32 offset = {
        .x = (bounds->min.x + bounds->max.x) / 2,
        .z = (bounds->min.z + bounds->max.z) / 2,
    };
    *anchor_pos = XYZ_32_OffsetLocalYaw(
        item->interp.result.pos, offset, item->interp.result.rot.y);
    *floor = item->interp.result.floor;
    if (item != Lara_GetItem()) {
        return;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(*anchor_pos, &room_num);
    const int32_t height = Room_GetHeight(sector, *anchor_pos);
    if (height != NO_HEIGHT) {
        *floor = height;
    }
}

static void M_ClipPoly(
    M_POLY *const poly, const float a, const float b, const float c)
{
    M_POLY out = {};
    for (int32_t i = 0; i < poly->count; i++) {
        const M_POINT cur = poly->points[i];
        const M_POINT next = poly->points[(i + 1) % poly->count];
        const float cur_dist = (a * cur.x) + (b * cur.z) - c;
        const float next_dist = (a * next.x) + (b * next.z) - c;
        if (cur_dist <= 0.0f && out.count < M_SHADOW_MAX_POLY_POINTS) {
            out.points[out.count++] = cur;
        }
        if ((cur_dist > 0.0f) != (next_dist > 0.0f)
            && out.count < M_SHADOW_MAX_POLY_POINTS) {
            const float t = cur_dist / (cur_dist - next_dist);
            out.points[out.count++] = (M_POINT) {
                .x = cur.x + (next.x - cur.x) * t,
                .z = cur.z + (next.z - cur.z) * t,
            };
        }
    }
    *poly = out;
}

static M_POINT M_GetPolyCenter(const M_POLY *const poly)
{
    M_POINT center = {};
    for (int32_t i = 0; i < poly->count; i++) {
        center.x += poly->points[i].x;
        center.z += poly->points[i].z;
    }
    center.x /= (float)poly->count;
    center.z /= (float)poly->count;
    return center;
}

static void M_ClipToSplitHalf(
    M_POLY *const poly, const SPLIT_TYPE split_type, const int32_t sector_x,
    const int32_t sector_z, const bool far_half)
{
    const float sign = far_half ? -1.0f : 1.0f;
    const bool is_nwse = split_type == SPLIT_NWSE_SOLID
        || split_type == SPLIT_NWSE_PORTAL_SW
        || split_type == SPLIT_NWSE_PORTAL_NE;
    if (is_nwse) {
        M_ClipPoly(
            poly, sign, sign, sign * (float)(sector_x + sector_z + WALL_L));
    } else {
        M_ClipPoly(poly, sign, -sign, sign * (float)(sector_x - sector_z));
    }
}

static bool M_GetSectorPlane(
    const M_POLY *const poly, const SECTOR *const sector,
    const M_SHADOW_CTX *const ctx, float plane[3])
{
    const M_POINT center = M_GetPolyCenter(poly);
    float sample_x[3];
    float sample_z[3];
    float sample_y[3];
    for (int32_t i = 0; i < 3; i++) {
        const M_POINT point = poly->points[i * poly->count / 3];
        // Sample inside the polygon, not on its boundary. Sector splits and
        // room edges can make an edge point read the adjacent floor.
        sample_x[i] = point.x + (center.x - point.x) * M_SHADOW_FACET_INSET;
        sample_z[i] = point.z + (center.z - point.z) * M_SHADOW_FACET_INSET;
        const XYZ_32 test_pos = {
            .x = (int32_t)sample_x[i],
            .y = ctx->anchor_floor,
            .z = (int32_t)sample_z[i],
        };
        const int32_t height = Room_GetHeight(sector, test_pos);
        if (height == NO_HEIGHT
            || ABS(height - ctx->anchor_floor) > ctx->reach) {
            return false;
        }
        sample_y[i] = (float)height;
    }

    const float dx1 = sample_x[1] - sample_x[0];
    const float dz1 = sample_z[1] - sample_z[0];
    const float dx2 = sample_x[2] - sample_x[0];
    const float dz2 = sample_z[2] - sample_z[0];
    const float det = (dx1 * dz2) - (dx2 * dz1);
    if (ABS(det) < 1.0f) {
        plane[0] = 0.0f;
        plane[1] = 0.0f;
        plane[2] = sample_y[0];
        return true;
    }

    const float dy1 = sample_y[1] - sample_y[0];
    const float dy2 = sample_y[2] - sample_y[0];
    plane[0] = ((dy1 * dz2) - (dy2 * dz1)) / det;
    plane[1] = ((dx1 * dy2) - (dx2 * dy1)) / det;
    plane[2] =
        sample_y[0] - (plane[0] * sample_x[0]) - (plane[1] * sample_z[0]);
    return true;
}

static bool M_GetTrianglePlane(const XYZ_32 tri[3], float plane[3])
{
    const float ax = (float)(tri[1].x - tri[0].x);
    const float ay = (float)(tri[1].y - tri[0].y);
    const float az = (float)(tri[1].z - tri[0].z);
    const float bx = (float)(tri[2].x - tri[0].x);
    const float by = (float)(tri[2].y - tri[0].y);
    const float bz = (float)(tri[2].z - tri[0].z);
    const float nx = (ay * bz) - (az * by);
    const float ny = (az * bx) - (ax * bz);
    const float nz = (ax * by) - (ay * bx);
    const float len = sqrtf((nx * nx) + (ny * ny) + (nz * nz));
    if (len < 1.0f || ABS(ny) < len * 0.05f) {
        return false;
    }

    plane[0] = -nx / ny;
    plane[1] = -nz / ny;
    plane[2] = (float)tri[0].y - (plane[0] * (float)tri[0].x)
        - (plane[1] * (float)tri[0].z);
    return true;
}

static void M_ClipToTriangle(M_POLY *const poly, const XYZ_32 tri[3])
{
    const float area =
        (float)(tri[1].x - tri[0].x) * (float)(tri[2].z - tri[0].z)
        - (float)(tri[2].x - tri[0].x) * (float)(tri[1].z - tri[0].z);
    const float sign = area < 0.0f ? -1.0f : 1.0f;
    for (int32_t i = 0; i < 3; i++) {
        const XYZ_32 from = tri[i];
        const XYZ_32 to = tri[(i + 1) % 3];
        const float a = sign * (float)(to.z - from.z);
        const float b = -sign * (float)(to.x - from.x);
        M_ClipPoly(poly, a, b, (a * (float)from.x) + (b * (float)from.z));
    }
}

static float M_GetPolyArea(const M_POLY *const poly)
{
    float area = 0.0f;
    for (int32_t i = 0; i < poly->count; i++) {
        const M_POINT cur = poly->points[i];
        const M_POINT next = poly->points[(i + 1) % poly->count];
        area += (cur.x * next.z) - (next.x * cur.z);
    }
    return ABS(area) / 2.0f;
}

static void M_StageFacet(
    const M_SHADOW_CTX *const ctx, const M_POLY *const poly,
    const float plane[3])
{
    const OUTPUT_TEXTURE_SIZE atlas_size[3] = {
        ctx->atlas_size,
        ctx->atlas_size,
        ctx->atlas_size,
    };
    const RGBA_8888 color[3] = { ctx->color, ctx->color, ctx->color };

    XYZ_32 facet_pos[M_SHADOW_MAX_POLY_POINTS];
    OUTPUT_UVW facet_uvw[M_SHADOW_MAX_POLY_POINTS];
    for (int32_t i = 0; i < poly->count; i++) {
        const M_POINT point = poly->points[i];
        const float height =
            (plane[0] * point.x) + (plane[1] * point.z) + plane[2];
        facet_pos[i] = (XYZ_32) {
            .x = M_SHADOW_ROUND(point.x),
            .y = M_SHADOW_ROUND(height) - M_SHADOW_FLOOR_GAP,
            .z = M_SHADOW_ROUND(point.z),
        };

        const XYZ_32 local = XYZ_32_UnrotateYaw(
            (XYZ_32) {
                .x = facet_pos[i].x - ctx->anchor_pos.x,
                .z = facet_pos[i].z - ctx->anchor_pos.z,
            },
            ctx->yaw);
        float u_frac =
            (float)(local.x + ctx->half_x) / (float)(ctx->half_x * 2);
        float v_frac =
            (float)(ctx->half_z - local.z) / (float)(ctx->half_z * 2);
        CLAMP(u_frac, 0.0f, 1.0f);
        CLAMP(v_frac, 0.0f, 1.0f);
        facet_uvw[i] = (OUTPUT_UVW) {
            .u = ctx->u_min + (ctx->u_span * u_frac),
            .v = ctx->v_min + (ctx->v_span * v_frac),
            .w = ctx->w,
        };
    }

    for (int32_t i = 1; i < poly->count - 1; i++) {
        const XYZ_32 tri_pos[3] = {
            facet_pos[0],
            facet_pos[i],
            facet_pos[i + 1],
        };
        const OUTPUT_UVW tri_uvw[3] = {
            facet_uvw[0],
            facet_uvw[i],
            facet_uvw[i + 1],
        };
        OutputSource_PolyFX_StageTriExtUV(
            tri_pos, tri_uvw, atlas_size, nullptr, color,
            VERT_NO_LIGHTING | VERT_NO_WIBBLE, DRAW_BLEND_SUB);
    }
}

static void M_GetFaceCorners(
    const ROOM *const room, const FACE *const face, XYZ_32 corners[4])
{
    for (int32_t i = 0; i < face->vertex_count; i++) {
        const XYZ_16 pos = room->mesh.vertices[face->vertices[i]].pos;
        corners[i] = (XYZ_32) {
            .x = room->pos.x + pos.x,
            .y = room->pos.y + pos.y,
            .z = room->pos.z + pos.z,
        };
    }
}

static bool M_GetFaceCells(
    const ROOM *const room, const FACE *const face, int32_t cells[4])
{
    XYZ_32 corners[4];
    M_GetFaceCorners(room, face, corners);

    int32_t min_x = corners[0].x;
    int32_t max_x = corners[0].x;
    int32_t min_z = corners[0].z;
    int32_t max_z = corners[0].z;
    for (int32_t i = 1; i < face->vertex_count; i++) {
        min_x = MIN(min_x, corners[i].x);
        max_x = MAX(max_x, corners[i].x);
        min_z = MIN(min_z, corners[i].z);
        max_z = MAX(max_z, corners[i].z);
    }

    cells[0] = (min_x - room->pos.x) >> WALL_SHIFT;
    cells[1] = (max_x - room->pos.x) >> WALL_SHIFT;
    cells[2] = (min_z - room->pos.z) >> WALL_SHIFT;
    cells[3] = (max_z - room->pos.z) >> WALL_SHIFT;
    if (cells[1] < 0 || cells[0] > room->size.x - 1 || cells[3] < 0
        || cells[2] > room->size.z - 1) {
        return false;
    }
    CLAMP(cells[0], 0, room->size.x - 1);
    CLAMP(cells[1], 0, room->size.x - 1);
    CLAMP(cells[2], 0, room->size.z - 1);
    CLAMP(cells[3], 0, room->size.z - 1);
    return true;
}

static void M_BuildRoomIndex(M_ROOM_INDEX *const index, const ROOM *const room)
{
    const int32_t cell_count = room->size.x * room->size.z;
    index->faces = room->mesh.all_faces.data;
    index->face_count = room->mesh.all_faces.count;
    index->size_x = room->size.x;
    index->size_z = room->size.z;
    index->cell_start =
        GameBuf_Alloc(sizeof(int32_t) * (cell_count + 1), GBUF_ROOM_FACE_INDEX);
    index->face_stamp = GameBuf_Alloc(
        sizeof(int32_t) * MAX(index->face_count, 1), GBUF_ROOM_FACE_INDEX);
    memset(index->cell_start, 0, sizeof(int32_t) * (cell_count + 1));
    memset(index->face_stamp, 0, sizeof(int32_t) * MAX(index->face_count, 1));

    for (int32_t f = 0; f < index->face_count; f++) {
        int32_t cells[4];
        if (!M_GetFaceCells(room, &index->faces[f], cells)) {
            continue;
        }
        for (int32_t cz = cells[2]; cz <= cells[3]; cz++) {
            for (int32_t cx = cells[0]; cx <= cells[1]; cx++) {
                index->cell_start[(cz * index->size_x) + cx + 1]++;
            }
        }
    }
    for (int32_t i = 1; i <= cell_count; i++) {
        index->cell_start[i] += index->cell_start[i - 1];
    }

    const int32_t entry_count = index->cell_start[cell_count];
    index->entries = GameBuf_Alloc(
        sizeof(int32_t) * MAX(entry_count, 1), GBUF_ROOM_FACE_INDEX);
    int32_t *cursor = Memory_Alloc(sizeof(int32_t) * cell_count);
    memcpy(cursor, index->cell_start, sizeof(int32_t) * cell_count);
    for (int32_t f = 0; f < index->face_count; f++) {
        int32_t cells[4];
        if (!M_GetFaceCells(room, &index->faces[f], cells)) {
            continue;
        }
        for (int32_t cz = cells[2]; cz <= cells[3]; cz++) {
            for (int32_t cx = cells[0]; cx <= cells[1]; cx++) {
                index->entries[cursor[(cz * index->size_x) + cx]++] = f;
            }
        }
    }
    Memory_FreePointer(&cursor);
}

static M_ROOM_INDEX *M_GetRoomIndex(const int16_t room_num)
{
    M_PRIV *const p = &m_Priv;
    if (Room_FromHandle(p->room_epoch) == nullptr) {
        p->room_index = GameBuf_Alloc(
            sizeof(M_ROOM_INDEX) * Room_GetCount(), GBUF_ROOM_FACE_INDEX);
        memset(p->room_index, 0, sizeof(M_ROOM_INDEX) * Room_GetCount());
        p->room_epoch = Room_GetHandle(0);
    }

    M_ROOM_INDEX *const index = &p->room_index[room_num];
    if (index->cell_start == nullptr) {
        M_BuildRoomIndex(index, Room_Get(room_num));
    }
    return index;
}

static bool M_IsPointInPoly(
    const M_POLY *const poly, const float x, const float z)
{
    float area = 0.0f;
    for (int32_t i = 0; i < poly->count; i++) {
        const M_POINT cur = poly->points[i];
        const M_POINT next = poly->points[(i + 1) % poly->count];
        area += (cur.x * next.z) - (next.x * cur.z);
    }
    const float sign = area < 0.0f ? -1.0f : 1.0f;
    for (int32_t i = 0; i < poly->count; i++) {
        const M_POINT cur = poly->points[i];
        const M_POINT next = poly->points[(i + 1) % poly->count];
        const float side =
            ((next.x - cur.x) * (z - cur.z)) - ((next.z - cur.z) * (x - cur.x));
        if (side * sign < 0.0f) {
            return false;
        }
    }
    return true;
}

static float M_GetPolyHeight(const float plane[3], const M_POINT point)
{
    return (plane[0] * point.x) + (plane[1] * point.z) + plane[2];
}

static float M_GetDistanceToPoly(const M_POLY *const poly, const M_POINT point)
{
    float best = FLT_MAX;
    for (int32_t i = 0; i < poly->count; i++) {
        const M_POINT from = poly->points[i];
        const M_POINT to = poly->points[(i + 1) % poly->count];
        const float edge_x = to.x - from.x;
        const float edge_z = to.z - from.z;
        const float len2 = (edge_x * edge_x) + (edge_z * edge_z);
        float along = 0.0f;
        if (len2 > 0.0f) {
            along =
                (((point.x - from.x) * edge_x) + ((point.z - from.z) * edge_z))
                / len2;
            CLAMP(along, 0.0f, 1.0f);
        }
        const float dx = point.x - (from.x + (edge_x * along));
        const float dz = point.z - (from.z + (edge_z * along));
        best = MIN(best, (dx * dx) + (dz * dz));
    }
    return sqrtf(best);
}

// Clipping can put a corner from one facet anywhere along another facet's edge.
static int32_t M_GetFacetJoin(
    const M_POLY *const a, const M_POLY *const b, M_POINT at[2])
{
    int32_t count = 0;
    for (int32_t i = 0; i < a->count && count < 2; i++) {
        if (M_GetDistanceToPoly(b, a->points[i]) <= M_SHADOW_JOIN_REACH) {
            at[count++] = a->points[i];
        }
    }
    for (int32_t i = 0; i < b->count && count < 2; i++) {
        if (M_GetDistanceToPoly(a, b->points[i]) <= M_SHADOW_JOIN_REACH) {
            at[count++] = b->points[i];
        }
    }
    return count;
}

// Downward joins and gentle upward slopes continue the floor. Upward steps do
// not.
static bool M_DoesFloorRunOn(
    const M_SHADOW_CTX *const ctx, const M_FACET *const from,
    const M_FACET *const to)
{
    M_POINT at[2];
    const int32_t count = M_GetFacetJoin(&from->poly, &to->poly, at);
    if (count == 0) {
        return false;
    }

    for (int32_t i = 0; i < count; i++) {
        const float height = M_GetPolyHeight(to->plane, at[i]);
        if (height
            < M_GetPolyHeight(from->plane, at[i]) - M_SHADOW_STEP_TOLERANCE) {
            return false;
        }
    }
    return true;
}

static const SECTOR *M_GetSectorAt(
    const int16_t room_num, const int32_t x, const int32_t z)
{
    const ROOM *room = Room_Get(room_num);
    const SECTOR *sector = Room_GetWorldSector(room, x, z);
    while (sector->portal_room.wall != NO_ROOM) {
        room = Room_Get(sector->portal_room.wall);
        sector = Room_GetWorldSector(room, x, z);
    }
    return sector;
}

static void M_AddRoom(
    int16_t rooms[M_SHADOW_MAX_ROOMS], int32_t *const count,
    const int16_t room_num)
{
    for (int32_t i = 0; i < *count; i++) {
        if (rooms[i] == room_num) {
            return;
        }
    }
    if (*count < M_SHADOW_MAX_ROOMS) {
        rooms[(*count)++] = room_num;
    }
}

static int32_t M_CollectMeshFacets(
    const M_SHADOW_CTX *const ctx, const M_POLY *const shadow_poly,
    const float min_x, const float max_x, const float min_z, const float max_z,
    M_FACET *const facets)
{
    int16_t rooms[M_SHADOW_MAX_ROOMS] = { ctx->room_num };
    int32_t room_count = 1;
    const PORTALS *const portals = Room_Get(ctx->room_num)->portals;
    for (int32_t i = 0; portals != nullptr && i < portals->count; i++) {
        const PORTAL *const portal = &portals->portal[i];
        if (portal->normal.y != 0) {
            continue;
        }
        const ROOM *const room = Room_Get(portal->room_num);
        const float room_min_x = (float)room->pos.x;
        const float room_min_z = (float)room->pos.z;
        const float room_max_x = room_min_x + (float)(room->size.x * WALL_L);
        const float room_max_z = room_min_z + (float)(room->size.z * WALL_L);
        if (room_max_x < min_x || room_min_x > max_x || room_max_z < min_z
            || room_min_z > max_z) {
            continue;
        }
        M_AddRoom(rooms, &room_count, portal->room_num);
    }

    const float corner_x[4] = { min_x, max_x, max_x, min_x };
    const float corner_z[4] = { min_z, min_z, max_z, max_z };
    for (int32_t i = 0; i < 4; i++) {
        int16_t room_num = ctx->room_num;
        Room_GetSector(
            (XYZ_32) {
                .x = (int32_t)corner_x[i],
                .y = ctx->anchor_floor,
                .z = (int32_t)corner_z[i],
            },
            &room_num);
        M_AddRoom(rooms, &room_count, room_num);
    }

    M_PRIV *const p = &m_Priv;
    int32_t staged = 0;
    int32_t facet_count = 0;
    for (int32_t r = 0; r < room_count; r++) {
        const ROOM *const room = Room_Get(rooms[r]);
        const M_ROOM_INDEX *const index = M_GetRoomIndex(rooms[r]);
        int32_t first_cell_x = ((int32_t)min_x - room->pos.x) >> WALL_SHIFT;
        int32_t last_cell_x = ((int32_t)max_x - room->pos.x) >> WALL_SHIFT;
        int32_t first_cell_z = ((int32_t)min_z - room->pos.z) >> WALL_SHIFT;
        int32_t last_cell_z = ((int32_t)max_z - room->pos.z) >> WALL_SHIFT;
        if (last_cell_x < 0 || first_cell_x > index->size_x - 1
            || last_cell_z < 0 || first_cell_z > index->size_z - 1) {
            continue;
        }
        CLAMP(first_cell_x, 0, index->size_x - 1);
        CLAMP(last_cell_x, 0, index->size_x - 1);
        CLAMP(first_cell_z, 0, index->size_z - 1);
        CLAMP(last_cell_z, 0, index->size_z - 1);

        const int32_t stamp = ++p->query_stamp;
        for (int32_t cz = first_cell_z; cz <= last_cell_z; cz++) {
            for (int32_t cx = first_cell_x; cx <= last_cell_x; cx++) {
                const int32_t cell = (cz * index->size_x) + cx;
                for (int32_t e = index->cell_start[cell];
                     e < index->cell_start[cell + 1]; e++) {
                    const int32_t f = index->entries[e];
                    if (index->face_stamp[f] == stamp) {
                        continue;
                    }
                    index->face_stamp[f] = stamp;

                    const FACE *const face = &index->faces[f];
                    XYZ_32 corners[4];
                    M_GetFaceCorners(room, face, corners);

                    int32_t face_min_x = corners[0].x;
                    int32_t face_max_x = corners[0].x;
                    int32_t face_min_z = corners[0].z;
                    int32_t face_max_z = corners[0].z;
                    for (int32_t i = 1; i < face->vertex_count; i++) {
                        face_min_x = MIN(face_min_x, corners[i].x);
                        face_max_x = MAX(face_max_x, corners[i].x);
                        face_min_z = MIN(face_min_z, corners[i].z);
                        face_max_z = MAX(face_max_z, corners[i].z);
                    }
                    if ((float)face_max_x < min_x || (float)face_min_x > max_x
                        || (float)face_max_z < min_z
                        || (float)face_min_z > max_z) {
                        continue;
                    }

                    const int32_t tri_count = face->vertex_count == 4 ? 2 : 1;
                    for (int32_t t = 0; t < tri_count; t++) {
                        const XYZ_32 tri[3] = {
                            corners[0],
                            corners[t + 1],
                            corners[t + 2],
                        };
                        float plane[3];
                        if (!M_GetTrianglePlane(tri, plane)) {
                            continue;
                        }

                        M_POLY facet = *shadow_poly;
                        M_ClipToTriangle(&facet, tri);
                        if (facet.count < 3
                            || M_GetPolyArea(&facet)
                                < M_SHADOW_MIN_FACET_AREA) {
                            continue;
                        }

                        const M_POINT center = M_GetPolyCenter(&facet);
                        const int32_t probe_x = M_SHADOW_ROUND(center.x);
                        const int32_t probe_z = M_SHADOW_ROUND(center.z);

                        // Some meshes look like floor but are only scenery,
                        // such as foliage or textured portals. Keep only
                        // faces that match the floor data closely enough.
                        const XYZ_32 probe_pos = { probe_x, ctx->anchor_floor,
                                                   probe_z };
                        int16_t ground_room = ctx->room_num;
                        const SECTOR *const ground_sector =
                            Room_GetSector(probe_pos, &ground_room);
                        const int32_t solid =
                            Room_GetHeight(ground_sector, probe_pos);
                        const SECTOR *const own_sector =
                            M_GetSectorAt(rooms[r], probe_x, probe_z);
                        if (Room_GetPitSector(own_sector, probe_x, probe_z)
                                != own_sector
                            || (solid != NO_HEIGHT
                                && ABS(M_GetPolyHeight(plane, center)
                                       - (float)solid)
                                    > (float)M_SHADOW_SOLID_TOLERANCE)) {
                            continue;
                        }

                        // Ignore walkways above the item and pit floors below
                        // its reachable floor range.
                        const float height = M_GetPolyHeight(plane, center);
                        if (ABS(height - (float)ctx->anchor_floor)
                            > (float)ctx->reach) {
                            continue;
                        }

                        if (facet_count >= M_SHADOW_MAX_FACETS) {
                            continue;
                        }
                        facets[facet_count++] = (M_FACET) {
                            .poly = facet,
                            .plane = { plane[0], plane[1], plane[2] },
                            .center = center,
                            .height = height,
                            .room_num = rooms[r],
                        };
                    }
                }
            }
        }
    }

    int32_t order[M_SHADOW_MAX_FACETS];
    for (int32_t i = 0; i < facet_count; i++) {
        order[i] = i;
    }
    for (int32_t i = 1; i < facet_count; i++) {
        const int32_t cur = order[i];
        int32_t j = i - 1;
        while (j >= 0 && facets[order[j]].height > facets[cur].height) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = cur;
    }

    return facet_count;
}

static void M_CollectSectorFacets(
    const M_SHADOW_CTX *const ctx, const M_POLY *const shadow_poly,
    const float min_x, const float max_x, const float min_z, const float max_z,
    M_FACET *const facets, int32_t *const facet_count)
{
    const int32_t mesh_count = *facet_count;
    const int32_t first_x = (int32_t)min_x >> WALL_SHIFT;
    const int32_t first_z = (int32_t)min_z >> WALL_SHIFT;
    const int32_t last_x = MIN(
        (int32_t)max_x >> WALL_SHIFT, first_x + M_SHADOW_MAX_SECTOR_SPAN - 1);
    const int32_t last_z = MIN(
        (int32_t)max_z >> WALL_SHIFT, first_z + M_SHADOW_MAX_SECTOR_SPAN - 1);

    for (int32_t sx = first_x; sx <= last_x; sx++) {
        for (int32_t sz = first_z; sz <= last_z; sz++) {
            const int32_t sector_x = sx << WALL_SHIFT;
            const int32_t sector_z = sz << WALL_SHIFT;
            M_POLY cell = *shadow_poly;
            M_ClipPoly(&cell, -1.0f, 0.0f, (float)-sector_x);
            M_ClipPoly(&cell, 1.0f, 0.0f, (float)(sector_x + WALL_L));
            M_ClipPoly(&cell, 0.0f, -1.0f, (float)-sector_z);
            M_ClipPoly(&cell, 0.0f, 1.0f, (float)(sector_z + WALL_L));
            if (cell.count < 3) {
                continue;
            }

            const M_POINT cell_center = M_GetPolyCenter(&cell);
            int16_t room_num = ctx->room_num;
            const SECTOR *const sector = Room_GetSector(
                (XYZ_32) {
                    .x = (int32_t)cell_center.x,
                    .y = ctx->anchor_floor,
                    .z = (int32_t)cell_center.z,
                },
                &room_num);
            const bool is_split = sector->floor.is_split;

            for (int32_t half = 0; half < (is_split ? 2 : 1); half++) {
                M_POLY facet = cell;
                if (is_split) {
                    M_ClipToSplitHalf(
                        &facet, sector->floor.split.type, sector_x, sector_z,
                        half == 1);
                }
                if (facet.count < 3
                    || M_GetPolyArea(&facet) < M_SHADOW_MIN_FACET_AREA) {
                    continue;
                }

                // Sector facets only fill areas not already covered by mesh.
                const M_POINT center = M_GetPolyCenter(&facet);
                bool taken = false;
                for (int32_t i = 0; i < mesh_count; i++) {
                    taken |=
                        M_IsPointInPoly(&facets[i].poly, center.x, center.z);
                }
                if (taken || *facet_count >= M_SHADOW_MAX_FACETS) {
                    continue;
                }

                float plane[3];
                if (!M_GetSectorPlane(&facet, sector, ctx, plane)) {
                    continue;
                }
                facets[(*facet_count)++] = (M_FACET) {
                    .poly = facet,
                    .plane = { plane[0], plane[1], plane[2] },
                    .center = center,
                    .height = M_GetPolyHeight(plane, center),
                    .room_num = ctx->room_num,
                };
            }
        }
    }
}

static void M_StageFacets(
    const M_SHADOW_CTX *const ctx, M_FACET *const facets,
    const int32_t facet_count)
{
    int32_t order[M_SHADOW_MAX_FACETS];
    for (int32_t i = 0; i < facet_count; i++) {
        order[i] = i;
    }
    for (int32_t i = 1; i < facet_count; i++) {
        const int32_t cur = order[i];
        int32_t j = i - 1;
        while (j >= 0 && facets[order[j]].height > facets[cur].height) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = cur;
    }

    int32_t seed = -1;
    for (int32_t i = 0; i < facet_count; i++) {
        if (M_IsPointInPoly(
                &facets[i].poly, (float)ctx->anchor_pos.x,
                (float)ctx->anchor_pos.z)) {
            seed = i;
            break;
        }
        if (seed == -1
            || ABS(facets[i].height - (float)ctx->anchor_floor)
                < ABS(facets[seed].height - (float)ctx->anchor_floor)) {
            seed = i;
        }
    }

    bool reached[M_SHADOW_MAX_FACETS] = {};
    if (seed != -1) {
        reached[seed] = true;
        for (bool grown = true; grown;) {
            grown = false;
            for (int32_t i = 0; i < facet_count; i++) {
                if (reached[i]) {
                    continue;
                }
                for (int32_t j = 0; j < facet_count; j++) {
                    if (reached[j]
                        && M_DoesFloorRunOn(ctx, &facets[j], &facets[i])) {
                        reached[i] = true;
                        grown = true;
                        break;
                    }
                }
            }
        }
    }

    for (int32_t i = 0; i < facet_count; i++) {
        M_FACET *const facet = &facets[order[i]];
        if (!reached[order[i]]) {
            continue;
        }
        bool covered = false;
        for (int32_t j = 0; j < i; j++) {
            covered |=
                facets[order[j]].staged
                && M_IsPointInPoly(
                    &facets[order[j]].poly, facet->center.x, facet->center.z);
        }
        if (covered) {
            continue;
        }

        facet->staged = true;
        M_StageFacet(ctx, &facet->poly, facet->plane);
    }
}

static bool M_DrawSprite(
    const int32_t size, const BOUNDS_16 *const place_bounds,
    const BOUNDS_16 *const size_bounds, const ITEM *const item)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return false;
    }

    const OBJECT *const shadow_obj = Object_Get(O_SHADOW);
    if (!shadow_obj->loaded) {
        return false;
    }

    const int32_t half_x =
        size * (size_bounds->max.x - size_bounds->min.x) * 3 / 1024;
    const int32_t half_z =
        size * (size_bounds->max.z - size_bounds->min.z) * 3 / 1024;
    if (half_x <= 0 || half_z <= 0) {
        return false;
    }

    // OG: shadow intensity is based on Lara's height above the floor, even for
    // non-Lara items.
    int32_t c = ((4096 - ABS(item->floor - item->pos.y)) >> 4) - 1;
    if (g_Config.rendering.lighting_curve == LIGHTING_CURVE_SATURATE) {
        c >>= 1;
    }
    CLAMP(c, 32, 255);

    XYZ_32 anchor_pos;
    int32_t anchor_floor;
    M_GetPlacement(item, place_bounds, &anchor_pos, &anchor_floor);

    const int32_t sprite_idx = shadow_obj->mesh_idx;
    const int32_t uvw_idx = Output_Textures_GetSpriteUVWIndex(sprite_idx, 0);
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

    const M_SHADOW_CTX ctx = {
        .anchor_pos = anchor_pos,
        .anchor_floor = anchor_floor,
        .yaw = item->interp.result.rot.y,
        .half_x = half_x,
        .half_z = half_z,
        .room_num = item->room_num,
        .u_min = u_min,
        .u_span = u_max - u_min,
        .v_min = v_min,
        .v_span = v_max - v_min,
        .w = sprite_uvw[0].w,
        .atlas_size = Output_Textures_GetAtlasSize(uvw_idx / 4),
        .color = { c, c, c, 255 },
        .reach = MAX(-place_bounds->min.y, STEP_L),
    };

    const XYZ_32 local_corners[4] = {
        { .x = -half_x, .z = half_z },
        { .x = half_x, .z = half_z },
        { .x = half_x, .z = -half_z },
        { .x = -half_x, .z = -half_z },
    };
    M_POLY shadow_poly = { .count = 4 };
    float min_x = 0.0f;
    float max_x = 0.0f;
    float min_z = 0.0f;
    float max_z = 0.0f;
    for (int32_t i = 0; i < 4; i++) {
        const XYZ_32 corner =
            XYZ_32_OffsetLocalYaw(anchor_pos, local_corners[i], ctx.yaw);
        shadow_poly.points[i] = (M_POINT) {
            .x = (float)corner.x,
            .z = (float)corner.z,
        };
        min_x = i == 0 ? shadow_poly.points[i].x : MIN(min_x, corner.x);
        max_x = i == 0 ? shadow_poly.points[i].x : MAX(max_x, corner.x);
        min_z = i == 0 ? shadow_poly.points[i].z : MIN(min_z, corner.z);
        max_z = i == 0 ? shadow_poly.points[i].z : MAX(max_z, corner.z);
    }

    // Keep subtractive shadow color neutral. Only alpha follows item tint.
    RGBA_F tint = Output_GetTint();
    tint.r = 1.0f;
    tint.g = 1.0f;
    tint.b = 1.0f;
    Output_PushTintOverride(tint);

    M_FACET facets[M_SHADOW_MAX_FACETS];
    int32_t facet_count = M_CollectMeshFacets(
        &ctx, &shadow_poly, min_x, max_x, min_z, max_z, facets);
    M_CollectSectorFacets(
        &ctx, &shadow_poly, min_x, max_x, min_z, max_z, facets, &facet_count);
    M_StageFacets(&ctx, facets, facet_count);

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
        // Keep the black shadow synced with item alpha.
        .tint = Output_GetTint(),
        .sort_layer = -1,
        .room = Output_GetCurrentRoom(),
    };
    // XXX: Mesh batcher currently collects transparent faces during the opaque
    // pass, so the transparent shadow must be staged there.
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_OPAQUE);
}

void OutputSource_Shadows_Init(MESH_BATCHER *const batcher)
{
    M_PRIV *const p = &m_Priv;
    p->batcher = batcher;

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
    p->room_index = nullptr;
    p->room_epoch = (TRX_HANDLE) {};
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

    BOUNDS_16 lara_bounds;
    if (M_GetLaraBounds(item, &lara_bounds)) {
        bounds = &lara_bounds;
    }

    const BOUNDS_16 *size_bounds = bounds;
    if (item == Lara_GetItem()) {
        const BOUNDS_16 *const cutscene_bounds = CutSeq_GetLaraShadowBounds();
        if (cutscene_bounds != nullptr) {
            size_bounds = cutscene_bounds;
        }
    }

    if (g_Config.visuals.shadow_type == SHADOW_TYPE_SPRITE) {
        if (M_DrawSprite(size, bounds, size_bounds, item)) {
            return;
        }
    }

    const int32_t x_size =
        (size_bounds->max.x - size_bounds->min.x) * size / 1024;
    const int32_t z_size =
        (size_bounds->max.z - size_bounds->min.z) * size / 1024;

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
