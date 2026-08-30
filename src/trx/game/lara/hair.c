#include <trx/game/lara/hair.h>

#include <trx/config.h>
#include <trx/core/math/geom.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/game.h>
#include <trx/game/lara.h>
#include <trx/game/lara/mesh.h>
#include <trx/game/lara/pose.h>
#include <trx/game/lara/skin/joints.h>
#include <trx/game/lara/skin/seam.h>
#include <trx/game/matrix.h>
#include <trx/game/objects/common.h>
#include <trx/game/output/common.h>
#include <trx/game/output/state.h>
#include <trx/game/output/water.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks.h>

#include <math.h>

#define M_MAX_BRAIDS 2
#define M_HAIR_SEGMENTS 6
#define M_HAIR_SPHERES 5
#define M_BONE_IDX(segment)                                                    \
    (segment == M_HAIR_SEGMENTS ? (segment - 2) : (segment - 1))

// A ring of vertices two braid meshes share, found once at outfit apply and
// pinned by the weld each frame. Empty when the meshes share no ring, which
// leaves that seam drawn exactly as authored.
typedef struct {
    int32_t count;
    SEAM_VERTEX_PAIR pairs[SEAM_MAX_VERTEX_PAIRS];
} M_SEGMENT_SEAM;

static bool m_IsFirstHair[M_MAX_BRAIDS];
static SPHERE m_HairSpheres[M_HAIR_SPHERES];
static XYZ_32 m_HairVelocity[M_MAX_BRAIDS][M_HAIR_SEGMENTS + 1];
static HAIR_SEGMENT m_HairSegments[M_MAX_BRAIDS][M_HAIR_SEGMENTS + 1];

// The braid weld state, kept between outfit applies.
//
// The braid meshes alternate: the odd ones are anchors, drawn rigid on their
// hair node, and the even ones are bridges whose rings are both taken from
// the neighbouring anchors - the split the OG skinning tables hardcode. A
// bridge renders no vertex of its own, so the yaw its node picks up from the
// hair physics (noise while a link hangs vertically) cancels out instead of
// twisting the surface.
static struct {
    bool enabled;
    // Per bridge, the ring it shares with the anchor above it (toward the
    // head) and below it (toward the tip); only even entries are used.
    M_SEGMENT_SEAM upper[M_HAIR_SEGMENTS];
    M_SEGMENT_SEAM lower[M_HAIR_SEGMENTS];
    // The ring bridge 0 pins onto the head mesh, per pigtail; the mapping is
    // authored per outfit (see LARA_SKIN_BRAID_HEAD_SEAM).
    M_SEGMENT_SEAM head[M_MAX_BRAIDS];
    // A single reused buffer for the welded vertices of the segment currently
    // being staged; sized to the largest segment mesh.
    struct {
        XYZ_F *pos;
        XYZ_F *normal;
        int32_t capacity;
    } scratch;
    // The pairing rotation each bridge's lower ring last settled on, per
    // pigtail; the hysteresis in M_PickShift needs it kept between frames.
    int32_t shifts[M_MAX_BRAIDS][M_HAIR_SEGMENTS];
    // The segment meshes deformed while welding was last active, so a later
    // outfit that does not weld them can be handed back authored geometry.
    int32_t welded_indices[M_MAX_BRAIDS * M_HAIR_SEGMENTS];
    int32_t welded_count;
} m_Joints = {};

static void M_CalculateSpheres(
    const ANIM_FRAME *const frame, const XYZ_32 offset_pos)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    const LARA_POSE *const pose = Lara_Pose_Get();
    const XYZ_16 *mesh_rots = pose != nullptr ? pose->rots : frame->mesh_rots;

    Matrix_TranslateRel16(pose != nullptr ? pose->offset : frame->offset);
    Matrix_Rot16(mesh_rots[LM_HIPS]);

    Matrix_Push();
    const OBJECT_MESH *mesh = Lara_Mesh_Get(LM_HIPS);
    Matrix_TranslateRel16(mesh->center);
    m_HairSpheres[0].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[0].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[0].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[0].r = mesh->radius;
    Matrix_Pop();

    const ANIM_BONE *bone = Lara_Skin_GetBoneBase();
    Matrix_TranslateRel32(bone[LM_TORSO - 1].pos);
    if (Lara_IsMachineGunActive() && pose == nullptr) {
        mesh_rots =
            lara->right_arm.frame_base[lara->right_arm.frame_num].mesh_rots;
    }

    Matrix_Rot16(mesh_rots[LM_TORSO]);
    Matrix_Rot16(lara->interp.result.torso_rot);
    Matrix_Push();
    mesh = Lara_Mesh_Get(LM_TORSO);
    Matrix_TranslateRel16(mesh->center);
    m_HairSpheres[1].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[1].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[1].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[1].r = mesh->radius;
    Matrix_Pop();

    Matrix_Push();
    Matrix_TranslateRel32(bone[LM_UARM_R - 1].pos);
    Matrix_Rot16(mesh_rots[LM_UARM_R]);

    mesh = Lara_Mesh_Get(LM_UARM_R);
    Matrix_TranslateRel16(mesh->center);
    m_HairSpheres[3].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[3].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[3].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[3].r = mesh->radius * 3 / 2;
    Matrix_Pop();

    Matrix_Push();
    Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
    Matrix_Rot16(mesh_rots[LM_UARM_L]);
    mesh = Lara_Mesh_Get(LM_UARM_L);
    Matrix_TranslateRel16(mesh->center);
    m_HairSpheres[4].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[4].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[4].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[4].r = mesh->radius * 3 / 2;
    Matrix_Pop();

    Matrix_TranslateRel32(bone[LM_HEAD - 1].pos);
    Matrix_Rot16(mesh_rots[LM_HEAD]);
    Matrix_Rot16(lara->interp.result.head_rot);

    Matrix_Push();
    mesh = Lara_Mesh_Get(LM_HEAD);
    Matrix_TranslateRel16(mesh->center);
    m_HairSpheres[2].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[2].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[2].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[2].r = mesh->radius;
    Matrix_Pop();

    Matrix_TranslateRel32(offset_pos);
}

static void M_CalculateSpheres_I(
    const ANIM_FRAME *const frame_1, const ANIM_FRAME *const frame_2,
    const int32_t frac, const int32_t rate, const XYZ_32 offset_pos)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    const XYZ_16 *mesh_rots_1 = frame_1->mesh_rots;
    const XYZ_16 *mesh_rots_2 = frame_2->mesh_rots;
    Matrix_InitInterpolate(frac, rate);
    Matrix_TranslateRel16_ID(frame_1->offset, frame_2->offset);
    Matrix_Rot16_ID(mesh_rots_1[LM_HIPS], mesh_rots_2[LM_HIPS]);

    Matrix_Push_I();
    const OBJECT_MESH *mesh = Lara_Mesh_Get(LM_HIPS);
    Matrix_TranslateRel16_I(mesh->center);
    Matrix_Interpolate();
    m_HairSpheres[0].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[0].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[0].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[0].r = mesh->radius;
    Matrix_Pop_I();

    const ANIM_BONE *bone = Lara_Skin_GetBoneBase();
    Matrix_TranslateRel32_I(bone[LM_TORSO - 1].pos);
    if (Lara_IsMachineGunActive()) {
        mesh_rots_1 =
            lara->right_arm.frame_base[lara->right_arm.frame_num].mesh_rots;
        mesh_rots_2 = mesh_rots_1;
    }

    Matrix_Rot16_ID(mesh_rots_1[LM_TORSO], mesh_rots_2[LM_TORSO]);
    Matrix_Rot16_I(lara->interp.result.torso_rot);

    Matrix_Push_I();
    mesh = Lara_Mesh_Get(LM_TORSO);
    Matrix_TranslateRel16_I(mesh->center);
    Matrix_Interpolate();
    m_HairSpheres[1].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[1].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[1].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[1].r = mesh->radius;
    Matrix_Pop_I();

    Matrix_Push_I();
    Matrix_TranslateRel32_I(bone[LM_UARM_R - 1].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_UARM_R], mesh_rots_2[LM_UARM_R]);

    mesh = Lara_Mesh_Get(LM_UARM_R);
    Matrix_TranslateRel16_I(mesh->center);
    Matrix_Interpolate();
    m_HairSpheres[3].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[3].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[3].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[3].r = mesh->radius * 3 / 2;
    Matrix_Pop_I();

    Matrix_Push_I();
    Matrix_TranslateRel32_I(bone[LM_UARM_L - 1].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_UARM_L], mesh_rots_2[LM_UARM_L]);

    mesh = Lara_Mesh_Get(LM_UARM_L);
    Matrix_TranslateRel16_I(mesh->center);
    Matrix_Interpolate();
    m_HairSpheres[4].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[4].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[4].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[4].r = mesh->radius * 3 / 2;
    Matrix_Pop_I();

    Matrix_TranslateRel32_I(bone[LM_HEAD - 1].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_HEAD], mesh_rots_2[LM_HEAD]);
    Matrix_Rot16_I(lara->interp.result.head_rot);

    Matrix_Push_I();
    mesh = Lara_Mesh_Get(LM_HEAD);
    Matrix_TranslateRel16_I(mesh->center);
    Matrix_Interpolate();
    m_HairSpheres[2].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[2].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[2].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[2].r = mesh->radius;
    Matrix_Pop_I();

    Matrix_TranslateRel32_I(offset_pos);
    Matrix_Interpolate();
}

static void M_ReduceTorsoSphere(void)
{
    m_HairSpheres[1].r -= (m_HairSpheres[1].r >> 2) + (m_HairSpheres[1].r >> 3);
    m_HairSpheres[1].pos.x =
        (m_HairSpheres[1].pos.x + m_HairSpheres[2].pos.x) >> 1;
    m_HairSpheres[1].pos.y =
        (m_HairSpheres[1].pos.y + m_HairSpheres[2].pos.y) >> 1;
    m_HairSpheres[1].pos.z =
        (m_HairSpheres[1].pos.z + m_HairSpheres[2].pos.z) >> 1;
}

static void M_Control(
    const bool in_cutscene, const int32_t braid_idx, const XYZ_32 offset_pos,
    const bool is_twin_setup)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();

    const ANIM_FRAME *frame_1;
    const ANIM_FRAME *frame_2;
    int32_t frac;
    int32_t rate;
    const ANIM_FRAME *const hit_frame = Lara_GetHitFrame(lara_item);
    if (!in_cutscene && hit_frame != nullptr) {
        frame_1 = hit_frame;
        frac = 0;
    } else {
        ANIM_FRAME *frmptr[2];
        frac = Item_GetFrames(lara_item, frmptr, &rate);
        frame_1 = frmptr[0];
        frame_2 = frmptr[1];
    }

    // A cutscene poses her without an animation behind it, and the frames go
    // with the pose when the scene ends. There is nothing to hang the braid
    // off until she is animating again.
    if (frame_1 == nullptr) {
        return;
    }

    Matrix_PushUnit();
    Matrix_TranslateSet32(lara_item->pos);
    Matrix_Rot16(lara_item->rot);

    if (frac == 0 || Lara_Pose_Get() != nullptr) {
        M_CalculateSpheres(frame_1, offset_pos);
    } else {
        M_CalculateSpheres_I(frame_1, frame_2, frac, rate, offset_pos);
    }

    if (is_twin_setup) {
        M_ReduceTorsoSphere();
    }

    const XYZ_32 pos = {
        .x = g_MatrixPtr->_03 >> W2V_SHIFT,
        .y = g_MatrixPtr->_13 >> W2V_SHIFT,
        .z = g_MatrixPtr->_23 >> W2V_SHIFT,
    };
    Matrix_Pop();

    const ANIM_BONE *const bones = Lara_Skin_GetBraidBoneBase(braid_idx);

    HAIR_SEGMENT *const fs = &m_HairSegments[braid_idx][0];
    fs->pos = pos;

    if (m_IsFirstHair[braid_idx]) {
        m_IsFirstHair[braid_idx] = false;
        for (int32_t i = 1; i <= M_HAIR_SEGMENTS; i++) {
            const ANIM_BONE *const bone = &bones[M_BONE_IDX(i)];
            const HAIR_SEGMENT *const ps = &m_HairSegments[braid_idx][i - 1];
            HAIR_SEGMENT *const s = &m_HairSegments[braid_idx][i];

            Matrix_PushUnit();
            Matrix_TranslateSet32(ps->pos);
            Matrix_RotY(ps->rot.y);
            Matrix_RotX(ps->rot.x);
            Matrix_TranslateRel32(bone->pos);

            s->pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
            s->pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
            s->pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;

            Matrix_Pop();
        }
        return;
    }

    int16_t room_num = lara_item->room_num;
    int32_t water_height;
    if (in_cutscene) {
        water_height = NO_HEIGHT;
    } else {
        water_height = Room_GetWaterHeight(
            (XYZ_32) {
                lara_item->pos.x
                    + (frame_1->bounds.min.x + frame_1->bounds.max.x) / 2,
                lara_item->pos.y
                    + (frame_1->bounds.max.y + frame_1->bounds.min.y) / 2,
                lara_item->pos.z
                    + (frame_1->bounds.max.z + frame_1->bounds.min.z) / 2,
            },
            room_num);
    }

    const SECTOR *const sector = Room_GetSector(fs->pos, &room_num);
    int32_t height = Room_GetHeight(sector, fs->pos);
    if (height < fs->pos.y) {
        height = lara_item->floor;
    }

    const XZ_32 smoke_wind = Sparks_GetSmokeWind();
    const int32_t hair_wind_z = Sparks_GetHairWindZ();

    XYZ_32 *const velocity = m_HairVelocity[braid_idx];
    for (int32_t i = 1; i <= M_HAIR_SEGMENTS; i++) {
        HAIR_SEGMENT *const ps = &m_HairSegments[braid_idx][i - 1];
        HAIR_SEGMENT *const s = &m_HairSegments[braid_idx][i];

        velocity[0] = s->pos;

        s->pos.x += velocity[i].x * 3 / 4;
        s->pos.y += velocity[i].y * 3 / 4;
        s->pos.z += velocity[i].z * 3 / 4;

        if (g_Config.visuals.breeze_mode == BREEZE_MODE_TR3) {
            if (lara_info->water_status == LWS_ABOVE_WATER
                && Room_Get(room_num)->flags.wind) {
                s->pos.x += smoke_wind.x;
                s->pos.z += smoke_wind.z;
            }

            if (water_height == NO_HEIGHT || s->pos.y < water_height) {
                s->pos.y += 10;
                if (water_height != NO_HEIGHT && s->pos.y > water_height) {
                    s->pos.y = water_height;
                }
            }

            if (s->pos.y > height) {
                s->pos.x = velocity[0].x;
                if (s->pos.y - height <= STEP_L) {
                    s->pos.y = height;
                }
                s->pos.z = velocity[0].z;
            }
        } else {
            LARA_WATER_STATE water_status = lara_info->water_status;
            if (water_status == LWS_WADE
                && (water_height == NO_HEIGHT || s->pos.y <= water_height)) {
                water_status = LWS_ABOVE_WATER;
            }
            switch (water_status) {
            case LWS_ABOVE_WATER:
                s->pos.y += 10;
                if (water_height != NO_HEIGHT && s->pos.y > water_height) {
                    s->pos.y = water_height;
                } else if (s->pos.y > height) {
                    s->pos.y = height;
                } else {
                    s->pos.z += hair_wind_z;
                }
                break;

            case LWS_UNDERWATER:
            case LWS_SURFACE:
            case LWS_WADE:
                CLAMP(s->pos.y, water_height, height);
                break;

            default:
                break;
            }
        }

        for (int32_t j = 0; j < M_HAIR_SPHERES; j++) {
            const SPHERE *const sphere = &m_HairSpheres[j];
            const int32_t dx = s->pos.x - sphere->pos.x;
            const int32_t dy = s->pos.y - sphere->pos.y;
            const int32_t dz = s->pos.z - sphere->pos.z;
            int32_t dist = SQUARE(dz) + SQUARE(dy) + SQUARE(dx);
            if (dist < SQUARE(sphere->r)) {
                dist = Math_Sqrt(dist);
                CLAMPL(dist, 1);
                s->pos.x = sphere->pos.x + sphere->r * dx / dist;
                s->pos.y = sphere->pos.y + sphere->r * dy / dist;
                s->pos.z = sphere->pos.z + sphere->r * dz / dist;
            }
        }

        const int32_t dx = s->pos.x - ps->pos.x;
        const int32_t dz = s->pos.z - ps->pos.z;
        const int32_t distance = Math_Sqrt(SQUARE(dx) + SQUARE(dz));
        ps->rot.y = Math_Atan(dz, dx);
        ps->rot.x = -Math_Atan(distance, s->pos.y - ps->pos.y);

        Matrix_PushUnit();
        Matrix_TranslateSet32(ps->pos);
        Matrix_RotY(ps->rot.y);
        Matrix_RotX(ps->rot.x);

        const ANIM_BONE *const bone = &bones[M_BONE_IDX(i)];
        Matrix_TranslateRel32(bone->pos);

        s->pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        s->pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        s->pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;

        velocity[i].x = s->pos.x - velocity[0].x;
        velocity[i].y = s->pos.y - velocity[0].y;
        velocity[i].z = s->pos.z - velocity[0].z;

        Matrix_Pop();
    }
}

static void M_EnsureScratch(const int32_t vertex_count)
{
    if (vertex_count <= m_Joints.scratch.capacity) {
        return;
    }
    m_Joints.scratch.pos =
        Memory_Realloc(m_Joints.scratch.pos, sizeof(XYZ_F) * vertex_count);
    m_Joints.scratch.normal =
        Memory_Realloc(m_Joints.scratch.normal, sizeof(XYZ_F) * vertex_count);
    m_Joints.scratch.capacity = vertex_count;
}

// Copies a mesh's authored vertices and normals into the scratch buffers, the
// starting point both the runtime weld and the restore path build on.
static void M_SeedFromMesh(const OBJECT_MESH *const mesh)
{
    const bool has_normals = mesh->num_lights > 0;
    for (int32_t i = 0; i < mesh->num_vertices; i++) {
        m_Joints.scratch.pos[i] = (XYZ_F) {
            mesh->vertices[i].x,
            mesh->vertices[i].y,
            mesh->vertices[i].z,
        };
        m_Joints.scratch.normal[i] = has_normals && i < mesh->num_lights
            ? (XYZ_F) { mesh->lighting.normals[i].x,
                        mesh->lighting.normals[i].y,
                        mesh->lighting.normals[i].z }
            : (XYZ_F) { 0.0f, 0.0f, 0.0f };
    }
}

static void M_RestoreMesh(const int32_t mesh_idx)
{
    const OBJECT_MESH *const mesh = Object_GetMesh(mesh_idx);
    M_EnsureScratch(mesh->num_vertices);
    M_SeedFromMesh(mesh);
    Output_DispatchObjectMeshGeometry(
        mesh_idx, m_Joints.scratch.pos,
        mesh->num_lights > 0 ? m_Joints.scratch.normal : nullptr, nullptr);
}

static void M_ResetJoints(void)
{
    const int32_t mesh_count = Object_GetMeshCount();
    for (int32_t i = 0; i < m_Joints.welded_count; i++) {
        // A level reload rebuilds the mesh table, so indices recorded under the
        // previous level can point past it - notably the clone slots, whose
        // buffers are gone. Nothing to restore there.
        if (m_Joints.welded_indices[i] < mesh_count) {
            M_RestoreMesh(m_Joints.welded_indices[i]);
        }
    }
    m_Joints.welded_count = 0;
    m_Joints.enabled = false;
    for (int32_t j = 0; j < M_HAIR_SEGMENTS; j++) {
        m_Joints.upper[j].count = 0;
        m_Joints.lower[j].count = 0;
    }
    for (int32_t i = 0; i < M_MAX_BRAIDS; i++) {
        m_Joints.head[i].count = 0;
        for (int32_t j = 0; j < M_HAIR_SEGMENTS; j++) {
            m_Joints.shifts[i][j] = 0;
        }
    }
}

static void M_Shutdown(void)
{
    Memory_FreePointer(&m_Joints.scratch.pos);
    Memory_FreePointer(&m_Joints.scratch.normal);
    m_Joints.scratch.capacity = 0;
}

// Orders a seam's pairs to walk the ring's perimeter, by angle around the
// ring's centroid. The braid rings lie in their mesh's xy plane. The weld
// relies on this order to rotate the pairing without breaking the ring.
static void M_SortSeamRing(
    M_SEGMENT_SEAM *const seam, const OBJECT_MESH *const mesh)
{
    const int32_t n = seam->count;
    float cx = 0.0f;
    float cy = 0.0f;
    for (int32_t k = 0; k < n; k++) {
        cx += mesh->vertices[seam->pairs[k].vertex_a].x / (float)n;
        cy += mesh->vertices[seam->pairs[k].vertex_a].y / (float)n;
    }
    float angles[SEAM_MAX_VERTEX_PAIRS];
    for (int32_t k = 0; k < n; k++) {
        const XYZ_16 v = mesh->vertices[seam->pairs[k].vertex_a];
        angles[k] = atan2f(v.y - cy, v.x - cx);
    }
    for (int32_t a = 0; a < n - 1; a++) {
        for (int32_t b = a + 1; b < n; b++) {
            if (angles[b] < angles[a]) {
                const float t = angles[a];
                angles[a] = angles[b];
                angles[b] = t;
                const SEAM_VERTEX_PAIR p = seam->pairs[a];
                seam->pairs[a] = seam->pairs[b];
                seam->pairs[b] = p;
            }
        }
    }
}

// Where each vertex of a neighbour's ring sits in the frame of the bridge
// mesh currently staged (g_WMatrixPtr).
static void M_SeamTargets(
    const M_SEGMENT_SEAM *const seam, const OBJECT_MESH *const source,
    const MATRIX *const source_matrix, XYZ_F *const targets)
{
    for (int32_t k = 0; k < seam->count; k++) {
        targets[k] = Lara_Seam_TransformPos(
            source_matrix, g_WMatrixPtr,
            XYZ_32_From16(source->vertices[seam->pairs[k].vertex_b]));
    }
}

// The roll, in the ring's plane, that carries the bridge's authored ring onto
// the delivered targets - how far the source frame has rotated against the
// bridge's own.
static float M_RingRoll(
    const M_SEGMENT_SEAM *const seam, const OBJECT_MESH *const mesh,
    const XYZ_F *const targets)
{
    const int32_t n = seam->count;
    float acx = 0.0f;
    float acy = 0.0f;
    float tcx = 0.0f;
    float tcy = 0.0f;
    for (int32_t k = 0; k < n; k++) {
        const XYZ_16 v = mesh->vertices[seam->pairs[k].vertex_a];
        acx += v.x / (float)n;
        acy += v.y / (float)n;
        tcx += targets[k].x / (float)n;
        tcy += targets[k].y / (float)n;
    }
    float dot = 0.0f;
    float cross = 0.0f;
    for (int32_t k = 0; k < n; k++) {
        const XYZ_16 v = mesh->vertices[seam->pairs[k].vertex_a];
        const float ax = v.x - acx;
        const float ay = v.y - acy;
        const float tx = targets[k].x - tcx;
        const float ty = targets[k].y - tcy;
        dot += ax * tx + ay * ty;
        cross += ax * ty - ay * tx;
    }
    return atan2f(cross, dot);
}

// Chooses the rotation of the lower pairing that lines up with the two anchors'
// relative roll. The comparison is made in the ring's plane with the upper
// ring's roll factored out, so the bridge's own frame drops out of the choice;
// and the previous rotation is kept unless the winner is better by a margin -
// without it the idle sway flicks the ring back and forth across the switching
// point. Only rotations are tried, never reflections, so the ring's winding
// holds.
static int32_t M_PickShift(
    const M_SEGMENT_SEAM *const seam, const OBJECT_MESH *const mesh,
    const XYZ_F *const targets, const float upper_roll,
    int32_t *const shift_state)
{
    const int32_t n = seam->count;
    float acx = 0.0f;
    float acy = 0.0f;
    float tcx = 0.0f;
    float tcy = 0.0f;
    for (int32_t k = 0; k < n; k++) {
        const XYZ_16 v = mesh->vertices[seam->pairs[k].vertex_a];
        acx += v.x / (float)n;
        acy += v.y / (float)n;
        tcx += targets[k].x / (float)n;
        tcy += targets[k].y / (float)n;
    }

    const float rc = cosf(upper_roll);
    const float rs = sinf(upper_roll);
    float ax[SEAM_MAX_VERTEX_PAIRS];
    float ay[SEAM_MAX_VERTEX_PAIRS];
    float tx[SEAM_MAX_VERTEX_PAIRS];
    float ty[SEAM_MAX_VERTEX_PAIRS];
    float spread = 0.0f;
    for (int32_t k = 0; k < n; k++) {
        const XYZ_16 v = mesh->vertices[seam->pairs[k].vertex_a];
        const float rx = v.x - acx;
        const float ry = v.y - acy;
        ax[k] = rx * rc - ry * rs;
        ay[k] = rx * rs + ry * rc;
        tx[k] = targets[k].x - tcx;
        ty[k] = targets[k].y - tcy;
        spread += rx * rx + ry * ry;
    }

    float costs[SEAM_MAX_VERTEX_PAIRS];
    for (int32_t shift = 0; shift < n; shift++) {
        float cost = 0.0f;
        for (int32_t k = 0; k < n; k++) {
            const int32_t m = (k + shift) % n;
            const float dx = tx[k] - ax[m];
            const float dy = ty[k] - ay[m];
            cost += dx * dx + dy * dy;
        }
        costs[shift] = cost;
    }

    int32_t best_shift = 0;
    for (int32_t shift = 1; shift < n; shift++) {
        if (costs[shift] < costs[best_shift]) {
            best_shift = shift;
        }
    }
    int32_t shift = *shift_state >= 0 && *shift_state < n ? *shift_state : 0;
    if (costs[best_shift] + spread < costs[shift]) {
        shift = best_shift;
    }
    *shift_state = shift;
    return shift;
}

// Moves a bridge ring onto the source ring's delivered positions, normals
// included so lighting stays continuous across the seam.
static void M_ApplySeam(
    const OBJECT_MESH *const mesh, const OBJECT_MESH *const source,
    const MATRIX *const source_matrix, const M_SEGMENT_SEAM *const seam,
    const int32_t shift, const XYZ_F *const targets)
{
    const bool has_normals = mesh->num_lights > 0;
    const bool source_has_normals = source->num_lights > 0;
    const int32_t n = seam->count;
    for (int32_t k = 0; k < n; k++) {
        const int32_t va = seam->pairs[(k + shift) % n].vertex_a;
        const int32_t vb = seam->pairs[k].vertex_b;
        m_Joints.scratch.pos[va] = targets[k];
        if (has_normals && source_has_normals && vb < source->num_lights) {
            m_Joints.scratch.normal[va] = Lara_Seam_TransformNormal(
                source_matrix, g_WMatrixPtr, source->lighting.normals[vb]);
        }
    }
}

// Rebuilds a bridge's rings from the anchors either side of it: the upper
// ring keeps its authored pairing - the head mesh's for bridge 0 - and the
// lower ring rotates its pairing to fit, absorbing however much the two
// anchors have rolled against each other.
static void M_WeldBridge(
    const int32_t braid_idx, const int32_t seg_base, const int32_t j,
    const MATRIX *const anchor_matrices)
{
    const int32_t mesh_idx = seg_base + j;
    const OBJECT_MESH *const mesh = Object_GetMesh(mesh_idx);
    M_EnsureScratch(mesh->num_vertices);
    M_SeedFromMesh(mesh);

    const M_SEGMENT_SEAM *upper;
    const OBJECT_MESH *upper_mesh;
    const MATRIX *upper_matrix;
    if (j == 0) {
        const LARA_INFO *const lara = Lara_GetLaraInfo();
        upper = &m_Joints.head[braid_idx];
        upper_mesh = Lara_Mesh_Get(LM_HEAD);
        upper_matrix = Lara_Joints_GetMeshMatrix(LM_HEAD);
        if (upper_matrix == nullptr) {
            upper_matrix = &lara->mesh_pos_matrices[LM_HEAD];
        }
    } else {
        upper = &m_Joints.upper[j];
        upper_mesh = Object_GetMesh(seg_base + j - 1);
        upper_matrix = &anchor_matrices[j - 1];
    }

    XYZ_F targets[SEAM_MAX_VERTEX_PAIRS];
    float upper_roll = 0.0f;
    if (upper->count > 0) {
        M_SeamTargets(upper, upper_mesh, upper_matrix, targets);
        upper_roll = M_RingRoll(upper, mesh, targets);
        M_ApplySeam(mesh, upper_mesh, upper_matrix, upper, 0, targets);
    }

    const M_SEGMENT_SEAM *const lower = &m_Joints.lower[j];
    if (j + 1 < M_HAIR_SEGMENTS && lower->count > 0) {
        const OBJECT_MESH *const lower_mesh = Object_GetMesh(seg_base + j + 1);
        const MATRIX *const lower_matrix = &anchor_matrices[j + 1];
        M_SeamTargets(lower, lower_mesh, lower_matrix, targets);
        const int32_t shift = M_PickShift(
            lower, mesh, targets, upper_roll, &m_Joints.shifts[braid_idx][j]);
        M_ApplySeam(mesh, lower_mesh, lower_matrix, lower, shift, targets);
    }

    Output_DispatchObjectMeshGeometry(
        mesh_idx, m_Joints.scratch.pos,
        mesh->num_lights > 0 ? m_Joints.scratch.normal : nullptr, nullptr);
}

// A render roll for every segment. The physics yaws a node from atan2 of its
// horizontal drift, which is noise while the link hangs vertically; instead
// of following it, each segment rolls to re-aim the previous segment's
// sideways axis, starting from the head's, so roll flows down the chain. The
// physics itself is left as the OG has it.
static void M_CalculateRenderRolls(
    const int32_t braid_idx, int16_t *const rolls)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const int16_t head_yaw = lara_item->rot.y + lara->interp.result.torso_rot.y
        + lara->interp.result.head_rot.y;

    Matrix_PushUnit();
    Matrix_RotY(head_yaw);
    double rx = g_MatrixPtr->_00;
    double ry = g_MatrixPtr->_10;
    double rz = g_MatrixPtr->_20;
    Matrix_Pop();

    for (int32_t j = 0; j < M_HAIR_SEGMENTS; j++) {
        const HAIR_SEGMENT *const s = &m_HairSegments[braid_idx][j];
        Matrix_PushUnit();
        Matrix_RotY(s->interp.result.rot.y);
        Matrix_RotX(s->interp.result.rot.x);
        const double ax = g_MatrixPtr->_00;
        const double ay = g_MatrixPtr->_10;
        const double az = g_MatrixPtr->_20;
        const double bx = g_MatrixPtr->_01;
        const double by = g_MatrixPtr->_11;
        const double bz = g_MatrixPtr->_21;
        Matrix_Pop();

        const double roll =
            atan2(rx * bx + ry * by + rz * bz, rx * ax + ry * ay + rz * az);
        rolls[j] = (int16_t)(int32_t)(roll * (DEG_180 / M_PI));
        const double rc = cos(roll);
        const double rs = sin(roll);
        rx = ax * rc + bx * rs;
        ry = ay * rc + by * rs;
        rz = az * rc + bz * rs;
    }
}

void Lara_Hair_Initialise(void)
{
    for (int32_t i = 0; i < M_MAX_BRAIDS; i++) {
        const ANIM_BONE *const bones = Lara_Skin_GetBraidBoneBase(i);
        if (bones == nullptr) {
            continue;
        }

        m_IsFirstHair[i] = true;
        m_HairSegments[i][0].rot.x = -DEG_90;
        m_HairSegments[i][0].rot.y = 0;

        for (int32_t j = 1; j <= M_HAIR_SEGMENTS; j++) {
            const ANIM_BONE *const bone = &bones[M_BONE_IDX(j)];
            m_HairSegments[i][j].pos = bone->pos;
            m_HairSegments[i][j].rot.x = -DEG_90;
            m_HairSegments[i][j].rot.y = 0;
            m_HairSegments[i][j].rot.z = 0;
            m_HairVelocity[i][j - 1] = (XYZ_32) {};
        }
    }
}

void Lara_Hair_Control(const bool in_cutscene)
{
    if (!Lara_Hair_IsActive()) {
        return;
    }

    const LARA_SKIN_BRAID *const braid = Lara_Skin_GetBraid();
    for (int32_t i = 0; i < braid->count; i++) {
        M_Control(in_cutscene, i, braid->setup[i].position, braid->count > 1);
    }
}

void Lara_Hair_InitJoints(const LARA_SKIN_OUTFIT *const outfit)
{
    M_ResetJoints();

    // Braid welding rides on the body-joint feature; outfits that do not opt
    // into joints (every TR1-TR3 outfit) keep their braid untouched.
    if (outfit->joints_obj_id == NO_OBJECT || !Lara_Skin_IsBraidSupported()) {
        return;
    }

    const int32_t braid_base = Lara_Skin_GetBraidMeshIdx(0);
    const ANIM_BONE *const bones = Lara_Skin_GetBraidBoneBase(0);
    if (braid_base < 0 || bones == nullptr) {
        return;
    }

    // At rest every segment shares one orientation, so a shared ring reduces
    // to a translation by the bone between the two meshes. Only the lower
    // rings are sorted: the weld rotates their pairing, which needs the pairs
    // to walk the ring's perimeter.
    const XYZ_32 zero = {};
    int32_t total_pairs = 0;
    int32_t max_vertices = 0;
    for (int32_t j = 0; j < M_HAIR_SEGMENTS; j += 2) {
        const OBJECT_MESH *const mesh = Object_GetMesh(braid_base + j);
        max_vertices = MAX(max_vertices, mesh->num_vertices);
        if (j > 0) {
            const OBJECT_MESH *const prev = Object_GetMesh(braid_base + j - 1);
            const XYZ_32 offset = bones[M_BONE_IDX(j)].pos;
            Lara_Seam_FindSharedVertices(
                m_Joints.upper[j].pairs, &m_Joints.upper[j].count,
                SEAM_MAX_VERTEX_PAIRS, mesh, prev, &offset, &zero);
            total_pairs += m_Joints.upper[j].count;
        }
        if (j + 1 < M_HAIR_SEGMENTS) {
            const OBJECT_MESH *const next = Object_GetMesh(braid_base + j + 1);
            const XYZ_32 offset = bones[M_BONE_IDX(j + 1)].pos;
            Lara_Seam_FindSharedVertices(
                m_Joints.lower[j].pairs, &m_Joints.lower[j].count,
                SEAM_MAX_VERTEX_PAIRS, mesh, next, &zero, &offset);
            M_SortSeamRing(&m_Joints.lower[j], mesh);
            total_pairs += m_Joints.lower[j].count;
        }
    }

    if (total_pairs == 0) {
        LOG_INFO("braid segments share no ring; welding disabled");
        return;
    }

    M_EnsureScratch(max_vertices);
    m_Joints.enabled = true;

    // A second pigtail shares the first's meshes, so give it its own copies to
    // deform; otherwise the two would fight over one buffer and only the last
    // drawn would close. The refresh builds the copies' render buffers.
    const LARA_SKIN_BRAID *const braid = Lara_Skin_GetBraid();

    // Copy the authored head-attach ring for each pigtail, dropping any pair
    // that names a vertex past either mesh, so the weld does not have to
    // bounds-check every frame.
    const OBJECT_MESH *const head_mesh = Lara_Mesh_Get(LM_HEAD);
    const OBJECT_MESH *const seg0_mesh = Object_GetMesh(braid_base);
    for (int32_t i = 0; i < braid->count; i++) {
        const LARA_SKIN_BRAID_HEAD_SEAM *const src = &braid->setup[i].head_seam;
        m_Joints.head[i].count = 0;
        for (int32_t k = 0; k < src->count; k++) {
            if (src->pairs[k].vertex_a >= seg0_mesh->num_vertices
                || src->pairs[k].vertex_b >= head_mesh->num_vertices) {
                continue;
            }
            m_Joints.head[i].pairs[m_Joints.head[i].count++] = src->pairs[k];
        }
    }

    // Record which meshes the draw will deform - the bridges - so a later
    // outfit can restore them. Restoring a mesh that was never deformed is a
    // no-op.
    for (int32_t i = 0; i < braid->count; i++) {
        const int32_t seg_base = Lara_Skin_GetBraidMeshIdx(i);
        for (int32_t j = 0; j < M_HAIR_SEGMENTS; j += 2) {
            m_Joints.welded_indices[m_Joints.welded_count++] = seg_base + j;
        }
    }
}

void Lara_Hair_Draw(void)
{
    if (!Lara_Hair_IsActive()) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    const LARA_SKIN_BRAID *const braid = Lara_Skin_GetBraid();

    for (int32_t i = 0; i < braid->count; i++) {
        // Until the first control pass has chained the segments off the head,
        // they still sit at their bone offsets near the world origin - drawn,
        // the welded ring would stretch from there to the head.
        if (m_IsFirstHair[i]) {
            continue;
        }
        const int32_t seg_base = Lara_Skin_GetBraidMeshIdx(i);

        int16_t rolls[M_HAIR_SEGMENTS];
        M_CalculateRenderRolls(i, rolls);

        // Anchors first, so the bridges between them can pin to their staged
        // frames - the same two passes the OG DrawHair makes.
        MATRIX anchor_matrices[M_HAIR_SEGMENTS] = {};
        for (int32_t j = 1; j < M_HAIR_SEGMENTS; j += 2) {
            const HAIR_SEGMENT *const s = &m_HairSegments[i][j];
            Matrix_Push();
            Matrix_TranslateAbs32(s->interp.result.pos);
            Matrix_RotY(s->interp.result.rot.y);
            Matrix_RotX(s->interp.result.rot.x);
            Matrix_RotZ(rolls[j]);
            anchor_matrices[j] = *g_WMatrixPtr;

            Output_Water_PushLaraMesh(
                LM_HEAD,
                (GAME_VECTOR) { .pos = s->interp.result.pos,
                                .room_num = lara_item->room_num },
                Object_GetMesh(seg_base + j)->radius);
            Object_DrawMesh(seg_base + j, CLIP_FULLY_VISIBLE, false);
            Output_Water_PopLaraMesh();
            Matrix_Pop();
        }

        for (int32_t j = 0; j < M_HAIR_SEGMENTS; j += 2) {
            const HAIR_SEGMENT *const s = &m_HairSegments[i][j];
            Matrix_Push();
            Matrix_TranslateAbs32(s->interp.result.pos);
            Matrix_RotY(s->interp.result.rot.y);
            Matrix_RotX(s->interp.result.rot.x);
            Matrix_RotZ(rolls[j]);

            if (m_Joints.enabled) {
                M_WeldBridge(i, seg_base, j, anchor_matrices);
            }

            Output_Water_PushLaraMesh(
                LM_HEAD,
                (GAME_VECTOR) { .pos = s->interp.result.pos,
                                .room_num = lara_item->room_num },
                Object_GetMesh(seg_base + j)->radius);
            Object_DrawMesh(seg_base + j, CLIP_FULLY_VISIBLE, false);
            Output_Water_PopLaraMesh();
            Matrix_Pop();
        }
    }
}

bool Lara_Hair_IsActive(void)
{
    return Object_Get(O_LARA)->loaded && Lara_Skin_IsBraidSupported();
}

int32_t Lara_Hair_GetBraidCount(void)
{
    return M_MAX_BRAIDS;
}

int32_t Lara_Hair_GetSegmentCount(void)
{
    return M_HAIR_SEGMENTS;
}

HAIR_SEGMENT *Lara_Hair_GetSegment(
    const int32_t braid_idx, const int32_t segment_idx)
{
    ASSERT(braid_idx >= 0 && braid_idx < M_MAX_BRAIDS);
    ASSERT(segment_idx >= 0 && segment_idx < M_HAIR_SEGMENTS);
    return &m_HairSegments[braid_idx][segment_idx];
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
