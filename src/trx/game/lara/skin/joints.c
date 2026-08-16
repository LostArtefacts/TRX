#include <trx/game/lara/skin/joints.h>

#include <trx/core/math/geom.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/debug.h>
#include <trx/game/lara.h>
#include <trx/game/lara/skin/seam.h>
#include <trx/game/matrix.h>
#include <trx/game/output.h>

#define M_STACK_SIZE LM_NUMBER_OF

typedef struct {
    int16_t parent_mesh;
    int16_t child_mesh;
    int32_t joint_mesh_idx;
    struct {
        int32_t count;
        SEAM_VERTEX_PAIR pairs[SEAM_MAX_VERTEX_PAIRS];
    } parent, child;
    // Per-frame vertex positions handed to the renderer. Floats, because a
    // ring vertex pinned across bone frames rarely lands on the int16 grid,
    // and the leftover quantization reads as a visible crack up close.
    int32_t vertex_count;
    XYZ_F *positions;
    // Per-frame vertex normals, welded the same way as the positions: a ring
    // vertex takes the normal of the limb vertex it sits on so object lighting
    // shades the seam continuously instead of stepping across it.
    XYZ_F *normals;
} M_JOINT;

typedef struct {
    XYZ_32 body_pos[LM_NUMBER_OF];
    XYZ_32 joint_pos[LM_NUMBER_OF];
} M_BIND_POSE;

typedef struct {
    bool is_enabled;
    M_JOINT joints[LM_NUMBER_OF];
    // The matrices the body meshes were staged with this frame. The pose
    // cache in LARA_INFO holds pre-interpolation matrices, which diverge from
    // the rendered pose whenever a frame is blended, so the seams are pinned
    // against these instead.
    MATRIX mesh_matrices[LM_NUMBER_OF];
} M_STATE;

static M_STATE m_State = {};

static void M_CalculateBindPose(
    const OBJECT *const obj, XYZ_32 positions[LM_NUMBER_OF])
{
    XYZ_32 stack[M_STACK_SIZE] = {};
    int32_t stack_top = 0;

    XYZ_32 pos = {};
    positions[0] = pos;

    for (int32_t mesh = 1; mesh < obj->mesh_count; mesh++) {
        const ANIM_BONE *bone = Object_GetBone(obj, mesh - 1);

        if (bone->matrix_pop) {
            ASSERT(stack_top > 0);
            pos = stack[--stack_top];
        }

        if (bone->matrix_push) {
            ASSERT(stack_top < M_STACK_SIZE);
            stack[stack_top++] = pos;
        }

        pos.x += bone->pos.x;
        pos.y += bone->pos.y;
        pos.z += bone->pos.z;

        positions[mesh] = pos;
    }
}

static void M_CalculateJointInfo(
    const OBJECT *const mesh_obj, const OBJECT *const joint_obj,
    const M_BIND_POSE *const bind_pose)
{
    int16_t stack[M_STACK_SIZE];
    int32_t stack_top = 0;
    int16_t parent = 0;

    m_State.joints[0].joint_mesh_idx = -1;

    for (int32_t mesh = 1; mesh < mesh_obj->mesh_count; mesh++) {
        const ANIM_BONE *bone = Object_GetBone(mesh_obj, mesh - 1);

        if (bone->matrix_pop) {
            ASSERT(stack_top > 0);
            parent = stack[--stack_top];
        }

        if (bone->matrix_push) {
            ASSERT(stack_top < M_STACK_SIZE);
            stack[stack_top++] = parent;
        }

        M_JOINT *const joint = &m_State.joints[mesh];
        joint->parent_mesh = parent;
        joint->child_mesh = mesh;
        joint->joint_mesh_idx = joint_obj->mesh_idx + mesh;

        const OBJECT_MESH *parent_mesh =
            Object_GetMesh(mesh_obj->mesh_idx + parent);
        const OBJECT_MESH *child_mesh =
            Object_GetMesh(mesh_obj->mesh_idx + mesh);
        const OBJECT_MESH *joint_mesh = Object_GetMesh(joint->joint_mesh_idx);

        Lara_Seam_FindSharedVertices(
            joint->parent.pairs, &joint->parent.count, SEAM_MAX_VERTEX_PAIRS,
            parent_mesh, joint_mesh, &bind_pose->body_pos[parent],
            &bind_pose->joint_pos[mesh]);
        Lara_Seam_FindSharedVertices(
            joint->child.pairs, &joint->child.count, SEAM_MAX_VERTEX_PAIRS,
            child_mesh, joint_mesh, &bind_pose->body_pos[mesh],
            &bind_pose->joint_pos[mesh]);

        joint->vertex_count = joint_mesh->num_vertices;
        joint->positions = Memory_Alloc(sizeof(XYZ_F) * joint->vertex_count);
        joint->normals = Memory_Alloc(sizeof(XYZ_F) * joint->vertex_count);

        parent = mesh;
    }
}

static void M_Reset(void)
{
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        Memory_FreePointer(&m_State.joints[i].positions);
        Memory_FreePointer(&m_State.joints[i].normals);
    }
    m_State = (M_STATE) {};
}

static void M_Shutdown(void)
{
    M_Reset();
}

// Welds the joint sleeve to the two limbs it bridges. The joint is drawn with
// the child's matrix, so the child-side ring is snapped straight onto the
// child mesh's own vertices; the parent-side ring lives in a different frame
// and is transformed across. Both rings track the limbs exactly as the joint
// bends, closing the gap the static sleeve would otherwise leave.
static void M_PinSeam(
    const OBJECT_MESH *const mesh, const M_JOINT *const joint,
    const MATRIX *const parent, const MATRIX *const child)
{
    const bool joint_has_normals = mesh->num_lights > 0;
    for (int32_t i = 0; i < joint->vertex_count; i++) {
        joint->positions[i] = (XYZ_F) {
            mesh->vertices[i].x,
            mesh->vertices[i].y,
            mesh->vertices[i].z,
        };
        joint->normals[i] = joint_has_normals && i < mesh->num_lights
            ? (XYZ_F) { mesh->lighting.normals[i].x,
                        mesh->lighting.normals[i].y,
                        mesh->lighting.normals[i].z }
            : (XYZ_F) { 0.0f, 0.0f, 0.0f };
    }

    const OBJECT_MESH *const child_mesh = Lara_Mesh_Get(joint->child_mesh);
    const bool child_has_normals = child_mesh->num_lights > 0;
    for (int32_t i = 0; i < joint->child.count; i++) {
        const SEAM_VERTEX_PAIR *const pair = &joint->child.pairs[i];
        if (pair->vertex_a >= child_mesh->num_vertices) {
            continue;
        }
        joint->positions[pair->vertex_b] = (XYZ_F) {
            child_mesh->vertices[pair->vertex_a].x,
            child_mesh->vertices[pair->vertex_a].y,
            child_mesh->vertices[pair->vertex_a].z,
        };
        // The joint is drawn in the child's frame, so its child-side normal is
        // copied straight across.
        if (child_has_normals && pair->vertex_a < child_mesh->num_lights) {
            const XYZ_16 n = child_mesh->lighting.normals[pair->vertex_a];
            joint->normals[pair->vertex_b] = (XYZ_F) { n.x, n.y, n.z };
        }
    }

    const OBJECT_MESH *const parent_mesh = Lara_Mesh_Get(joint->parent_mesh);
    const bool parent_has_normals = parent_mesh->num_lights > 0;
    for (int32_t i = 0; i < joint->parent.count; i++) {
        const SEAM_VERTEX_PAIR *const pair = &joint->parent.pairs[i];
        if (pair->vertex_a >= parent_mesh->num_vertices) {
            continue;
        }
        joint->positions[pair->vertex_b] = Lara_Seam_TransformPos(
            parent, child,
            XYZ_32_From16(parent_mesh->vertices[pair->vertex_a]));
        if (parent_has_normals && pair->vertex_a < parent_mesh->num_lights) {
            joint->normals[pair->vertex_b] = Lara_Seam_TransformNormal(
                parent, child, parent_mesh->lighting.normals[pair->vertex_a]);
        }
    }
}

void Lara_Joints_Initialise(const LARA_SKIN_OUTFIT *const outfit)
{
    M_Reset();
    if (outfit->joints_obj_id == NO_OBJECT) {
        return;
    }

    const OBJECT *const mesh_obj = Object_Get(outfit->mesh_obj_id);
    if (!mesh_obj->loaded) {
        return;
    }

    const OBJECT *const joints_obj = Object_Get(outfit->joints_obj_id);
    if (!joints_obj->loaded) {
        return;
    }

    M_BIND_POSE bind_pose = {};
    M_CalculateBindPose(mesh_obj, bind_pose.body_pos);
    M_CalculateBindPose(joints_obj, bind_pose.joint_pos);
    M_CalculateJointInfo(mesh_obj, joints_obj, &bind_pose);

    m_State.is_enabled = true;
}

void Lara_Joints_SwapSingle(
    const LARA_MESH mesh_idx, const LARA_SKIN_OUTFIT *const outfit)
{
    if (!m_State.is_enabled) {
        return;
    }

    if (outfit->joints_obj_id == NO_OBJECT) {
        return;
    }

    if (mesh_idx == LM_FIRST) {
        return;
    }

    const OBJECT *const joints_obj = Object_Get(outfit->joints_obj_id);
    if (!joints_obj->loaded) {
        return;
    }

    M_JOINT *const joint = &m_State.joints[mesh_idx];
    joint->joint_mesh_idx = joints_obj->mesh_idx + mesh_idx;
}

const MATRIX *Lara_Joints_GetMeshMatrix(const LARA_MESH mesh_idx)
{
    if (!m_State.is_enabled) {
        return nullptr;
    }
    return &m_State.mesh_matrices[mesh_idx];
}

void Lara_Joints_StashMatrix(const LARA_MESH mesh_idx, const bool interpolated)
{
    if (!m_State.is_enabled) {
        return;
    }

    // Reproduces the matrix Output_DrawObjectMesh_I stages the mesh with.
    if (interpolated) {
        Matrix_Push();
        Matrix_Interpolate();
        m_State.mesh_matrices[mesh_idx] = *g_WMatrixPtr;
        Matrix_Pop();
    } else {
        m_State.mesh_matrices[mesh_idx] = *g_WMatrixPtr;
    }
}

void Lara_Joints_Draw(
    const LARA_MESH mesh_idx, const CLIP clip, const bool interpolated)
{
    if (!m_State.is_enabled) {
        return;
    }

    const M_JOINT *const joint = &m_State.joints[mesh_idx];
    if (joint->joint_mesh_idx < 0) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (!Item_IsMeshVisible(lara_item, joint->parent_mesh)
        || !Item_IsMeshVisible(lara_item, joint->child_mesh)) {
        return;
    }

    const OBJECT_MESH *const mesh = Object_GetMesh(joint->joint_mesh_idx);
    const MATRIX *const parent = &m_State.mesh_matrices[joint->parent_mesh];
    const MATRIX *const child = &m_State.mesh_matrices[joint->child_mesh];

    M_PinSeam(mesh, joint, parent, child);
    const XYZ_F *const normals =
        mesh->num_lights > 0 ? joint->normals : nullptr;
    Output_DispatchObjectMeshGeometry(
        joint->joint_mesh_idx, joint->positions, normals);

    if (interpolated) {
        Output_DrawObjectMesh_I(mesh, clip);
    } else {
        Output_DrawObjectMesh(mesh, clip);
    }
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
