#include <trx/game/lara/skin/joints.h>

#include <trx/debug.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>

#define M_STACK_SIZE LM_NUMBER_OF
#define M_MAX_VERTEX_PAIRS 32

typedef struct {
    uint16_t body_vertex;
    uint16_t joint_vertex;
} M_VERTEX_PAIR;

typedef struct {
    int16_t parent_mesh;
    int16_t child_mesh;
    int32_t joint_mesh_idx;
    struct {
        int32_t count;
        M_VERTEX_PAIR pairs[M_MAX_VERTEX_PAIRS];
    } parent, child;
} M_JOINT;

typedef struct {
    XYZ_32 body_pos[LM_NUMBER_OF];
    XYZ_32 joint_pos[LM_NUMBER_OF];
} M_BIND_POSE;

typedef struct {
    bool is_enabled;
    M_JOINT joints[LM_NUMBER_OF];
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

static bool M_VertexMatches(const XYZ_32 a, const XYZ_32 b)
{
    return ABS(a.x - b.x) <= 1 && ABS(a.y - b.y) <= 1 && ABS(a.z - b.z) <= 1;
}

static void M_FindSharedVertices(
    M_VERTEX_PAIR *const pairs, int32_t *const pair_count,
    const OBJECT_MESH *const body_mesh, const OBJECT_MESH *const joint_mesh,
    const XYZ_32 *const body_pos, const XYZ_32 *const joint_pos)
{
    *pair_count = 0;
    for (int32_t body = 0; body < body_mesh->num_vertices; body++) {
        const XYZ_32 body_world = {
            body_mesh->vertices[body].x + body_pos->x,
            body_mesh->vertices[body].y + body_pos->y,
            body_mesh->vertices[body].z + body_pos->z,
        };
        for (int32_t joint = 0; joint < joint_mesh->num_vertices; joint++) {
            const XYZ_32 joint_world = {
                joint_mesh->vertices[joint].x + joint_pos->x,
                joint_mesh->vertices[joint].y + joint_pos->y,
                joint_mesh->vertices[joint].z + joint_pos->z,
            };
            if (!M_VertexMatches(body_world, joint_world)) {
                continue;
            }

            ASSERT(*pair_count < M_MAX_VERTEX_PAIRS);
            pairs[*pair_count].body_vertex = body;
            pairs[*pair_count].joint_vertex = joint;
            (*pair_count)++;
        }
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

        M_FindSharedVertices(
            joint->parent.pairs, &joint->parent.count, parent_mesh, joint_mesh,
            &bind_pose->body_pos[parent], &bind_pose->joint_pos[mesh]);
        M_FindSharedVertices(
            joint->child.pairs, &joint->child.count, child_mesh, joint_mesh,
            &bind_pose->body_pos[mesh], &bind_pose->joint_pos[mesh]);

        parent = mesh;
    }
}

void Lara_Joints_Initialise(const LARA_SKIN_OUTFIT *const outfit)
{
    m_State.is_enabled = false;
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

    OBJECT_MESH *const mesh = Object_GetMesh(joint->joint_mesh_idx);
    
    // TODO: ?

    if (interpolated) {
        Output_DrawObjectMesh_I(mesh, clip);
    } else {
        Output_DrawObjectMesh(mesh, clip);
    }
}
