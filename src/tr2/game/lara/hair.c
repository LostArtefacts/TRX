#include "game/lara/hair.h"

#include "game/items.h"
#include "game/lara/misc.h"
#include "game/output.h"
#include "game/random.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/game/lara/common.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

#define HAIR_SEGMENTS 6

typedef struct {
    XYZ_32 pos;
    XYZ_16 rot;
} HAIR_SEGMENT;

static bool m_IsFirstHair;
static SPHERE m_HairSpheres[HAIR_SEGMENTS - 1];
static XYZ_32 m_HairVelocity[HAIR_SEGMENTS + 1];
static HAIR_SEGMENT m_HairSegments[HAIR_SEGMENTS + 1];
static int32_t m_HairWind;

static void M_CalculateSpheres(const ANIM_FRAME *frame);
static void M_CalculateSpheres_I(
    const ANIM_FRAME *frame_1, const ANIM_FRAME *frame_2, int32_t frac,
    int32_t rate);

static void M_CalculateSpheres(const ANIM_FRAME *const frame)
{
    const XYZ_16 *mesh_rots = frame->mesh_rots;
    Matrix_TranslateRel16(frame->offset);
    Matrix_Rot16(mesh_rots[LM_HIPS]);

    Matrix_Push();
    const OBJECT_MESH *mesh = g_Lara.mesh_ptrs[LM_HIPS];
    Matrix_TranslateRel16(mesh->center);
    m_HairSpheres[0].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[0].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[0].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[0].r = mesh->radius;
    Matrix_Pop();

    const ANIM_BONE *bone = Object_GetBone(Object_Get(O_LARA), 0);
    Matrix_TranslateRel32(bone[LM_TORSO - 1].pos);
    if (Lara_IsM16Active()) {
        mesh_rots =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
    }

    Matrix_Rot16(mesh_rots[LM_TORSO]);
    Matrix_Rot16(g_Lara.torso_rot);
    Matrix_Push();
    mesh = g_Lara.mesh_ptrs[LM_TORSO];
    Matrix_TranslateRel16(mesh->center);
    m_HairSpheres[1].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[1].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[1].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[1].r = mesh->radius;
    Matrix_Pop();

    Matrix_Push();
    Matrix_TranslateRel32(bone[LM_UARM_R - 1].pos);
    Matrix_Rot16(mesh_rots[LM_UARM_R]);

    mesh = g_Lara.mesh_ptrs[LM_UARM_R];
    Matrix_TranslateRel16(mesh->center);
    m_HairSpheres[3].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[3].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[3].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[3].r = mesh->radius * 3 / 2;
    Matrix_Pop();

    Matrix_Push();
    Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
    Matrix_Rot16(mesh_rots[LM_UARM_L]);
    mesh = g_Lara.mesh_ptrs[LM_UARM_L];
    Matrix_TranslateRel16(mesh->center);
    m_HairSpheres[4].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[4].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[4].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[4].r = mesh->radius * 3 / 2;
    Matrix_Pop();

    Matrix_TranslateRel32(bone[LM_HEAD - 1].pos);
    Matrix_Rot16(mesh_rots[LM_HEAD]);
    Matrix_Rot16(g_Lara.head_rot);

    Matrix_Push();
    mesh = g_Lara.mesh_ptrs[LM_HEAD];
    Matrix_TranslateRel16(mesh->center);
    m_HairSpheres[2].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[2].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[2].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[2].r = mesh->radius;
    Matrix_Pop();

    Matrix_TranslateRel(0, -23, -55);
}

static void M_CalculateSpheres_I(
    const ANIM_FRAME *const frame_1, const ANIM_FRAME *const frame_2,
    const int32_t frac, const int32_t rate)
{
    const XYZ_16 *mesh_rots_1 = frame_1->mesh_rots;
    const XYZ_16 *mesh_rots_2 = frame_2->mesh_rots;
    Matrix_InitInterpolate(frac, rate);
    Matrix_TranslateRel16_ID(frame_1->offset, frame_2->offset);
    Matrix_Rot16_ID(mesh_rots_1[LM_HIPS], mesh_rots_2[LM_HIPS]);

    Matrix_Push_I();
    const OBJECT_MESH *mesh = g_Lara.mesh_ptrs[LM_HIPS];
    Matrix_TranslateRel16_I(mesh->center);
    Matrix_Interpolate();
    m_HairSpheres[0].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[0].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[0].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[0].r = mesh->radius;
    Matrix_Pop_I();

    const ANIM_BONE *bone = Object_GetBone(Object_Get(O_LARA), 0);
    Matrix_TranslateRel32_I(bone[LM_TORSO - 1].pos);
    if (Lara_IsM16Active()) {
        mesh_rots_1 =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        mesh_rots_2 = mesh_rots_1;
    }

    Matrix_Rot16_ID(mesh_rots_1[LM_TORSO], mesh_rots_2[LM_TORSO]);
    Matrix_Rot16_I(g_Lara.torso_rot);

    Matrix_Push_I();
    mesh = g_Lara.mesh_ptrs[LM_TORSO];
    Matrix_TranslateRel16_I(mesh->center);
    Matrix_Interpolate();
    m_HairSpheres[1].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[1].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[1].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[1].r = mesh->radius;
    Matrix_Pop_I();

    Matrix_Push_I();
    Matrix_TranslateRel32_I(bone[LM_UARM_R - 1].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_UARM_R], mesh_rots_2[LM_UARM_R]);

    mesh = g_Lara.mesh_ptrs[LM_UARM_R];
    Matrix_TranslateRel16_I(mesh->center);
    Matrix_Interpolate();
    m_HairSpheres[3].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[3].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[3].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[3].r = mesh->radius * 3 / 2;
    Matrix_Pop_I();

    Matrix_Push_I();
    Matrix_TranslateRel32_I(bone[LM_UARM_L - 1].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_UARM_L], mesh_rots_2[LM_UARM_L]);

    mesh = g_Lara.mesh_ptrs[LM_UARM_L];
    Matrix_TranslateRel16_I(mesh->center);
    Matrix_Interpolate();
    m_HairSpheres[4].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[4].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[4].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[4].r = mesh->radius * 3 / 2;
    Matrix_Pop_I();

    Matrix_TranslateRel32_I(bone[LM_HEAD - 1].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_HEAD], mesh_rots_2[LM_HEAD]);
    Matrix_Rot16_I(g_Lara.head_rot);

    Matrix_Push_I();
    mesh = g_Lara.mesh_ptrs[LM_HEAD];
    Matrix_TranslateRel16_I(mesh->center);
    Matrix_Interpolate();
    m_HairSpheres[2].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    m_HairSpheres[2].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    m_HairSpheres[2].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    m_HairSpheres[2].r = mesh->radius;
    Matrix_Pop_I();

    Matrix_TranslateRel_I(0, -23, -55);
    Matrix_Interpolate();
}

void Lara_Hair_Initialise(void)
{
    const OBJECT *const obj = Object_Get(O_LARA_HAIR);
    m_IsFirstHair = true;
    m_HairSegments[0].rot.x = -DEG_90;
    m_HairSegments[0].rot.y = 0;
    for (int32_t i = 0; i < HAIR_SEGMENTS; i++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, i);
        m_HairSegments[i + 1].pos = bone->pos;
        m_HairSegments[i + 1].rot.x = -DEG_90;
        m_HairSegments[i + 1].rot.y = 0;
        m_HairSegments[i + 1].rot.z = 0;
        m_HairVelocity[i].x = 0;
        m_HairVelocity[i].y = 0;
        m_HairVelocity[i].z = 0;
    }
}

void Lara_Hair_Control(const bool in_cutscene)
{
    const ANIM_FRAME *frame_1;
    const ANIM_FRAME *frame_2;
    int32_t frac;
    int32_t rate;
    const ANIM_FRAME *const hit_frame = Lara_GetHitFrame(g_LaraItem);
    if (hit_frame != nullptr) {
        frame_1 = hit_frame;
        frac = 0;
    } else {
        ANIM_FRAME *frmptr[2];
        frac = Item_GetFrames(g_LaraItem, frmptr, &rate);
        frame_1 = frmptr[0];
        frame_2 = frmptr[1];
    }

    Matrix_PushUnit();
    g_MatrixPtr->_03 = g_LaraItem->pos.x << W2V_SHIFT;
    g_MatrixPtr->_13 = g_LaraItem->pos.y << W2V_SHIFT;
    g_MatrixPtr->_23 = g_LaraItem->pos.z << W2V_SHIFT;
    Matrix_Rot16(g_LaraItem->rot);

    if (frac == 0) {
        M_CalculateSpheres(frame_1);
    } else {
        M_CalculateSpheres_I(frame_1, frame_2, frac, rate);
    }

    const XYZ_32 pos = {
        .x = g_MatrixPtr->_03 >> W2V_SHIFT,
        .y = g_MatrixPtr->_13 >> W2V_SHIFT,
        .z = g_MatrixPtr->_23 >> W2V_SHIFT,
    };
    Matrix_Pop();

    const OBJECT *const obj = Object_Get(O_LARA_HAIR);

    HAIR_SEGMENT *const fs = &m_HairSegments[0];
    fs->pos.x = pos.x;
    fs->pos.y = pos.y;
    fs->pos.z = pos.z;

    if (m_IsFirstHair) {
        m_IsFirstHair = false;
        for (int32_t i = 1; i <= HAIR_SEGMENTS; i++) {
            const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
            const HAIR_SEGMENT *const ps = &m_HairSegments[i - 1];
            HAIR_SEGMENT *const s = &m_HairSegments[i];

            Matrix_PushUnit();
            g_MatrixPtr->_03 = ps->pos.x << W2V_SHIFT;
            g_MatrixPtr->_13 = ps->pos.y << W2V_SHIFT;
            g_MatrixPtr->_23 = ps->pos.z << W2V_SHIFT;
            Matrix_RotY(ps->rot.y);
            Matrix_RotX(ps->rot.x);
            Matrix_TranslateRel32(bone->pos);

            s->pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
            s->pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
            s->pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;

            Matrix_Pop();
        }
        m_HairWind = 0;
        return;
    }

    int16_t room_num = g_LaraItem->room_num;
    int32_t water_height;
    if (in_cutscene) {
        water_height = NO_HEIGHT;
    } else {
        water_height = Room_GetWaterHeight(
            g_LaraItem->pos.x
                + (frame_1->bounds.min.x + frame_1->bounds.max.x) / 2,
            g_LaraItem->pos.y
                + (frame_1->bounds.max.y + frame_1->bounds.min.y) / 2,
            g_LaraItem->pos.z
                + (frame_1->bounds.max.z + frame_1->bounds.min.z) / 2,
            room_num);
    }

    const SECTOR *const sector =
        Room_GetSector(fs->pos.x, fs->pos.y, fs->pos.z, &room_num);
    int32_t height = Room_GetHeight(sector, fs->pos.x, fs->pos.y, fs->pos.z);
    if (height < fs->pos.y) {
        height = g_LaraItem->floor;
    }

    if (Room_Get(room_num)->flags & RF_NOT_INSIDE) {
        const int32_t random = Random_GetDraw() & 7;
        if (random != 0) {
            m_HairWind += random - 4;
            if (m_HairWind < 0) {
                m_HairWind = 0;
            } else if (m_HairWind >= 8) {
                m_HairWind--;
            }
        }
    } else {
        m_HairWind = 0;
    }

    for (int32_t i = 1; i <= HAIR_SEGMENTS; i++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
        HAIR_SEGMENT *const ps = &m_HairSegments[i - 1];
        HAIR_SEGMENT *const s = &m_HairSegments[i];

        m_HairVelocity[0] = s->pos;

        s->pos.x += m_HairVelocity[i].x * 3 / 4;
        s->pos.y += m_HairVelocity[i].y * 3 / 4;
        s->pos.z += m_HairVelocity[i].z * 3 / 4;

        switch (g_Lara.water_status) {
        case LWS_ABOVE_WATER:
            s->pos.y += 10;
            if (water_height != NO_HEIGHT && s->pos.y > water_height) {
                s->pos.y = water_height;
            } else if (s->pos.y > height) {
                s->pos.y = height;
            } else {
                s->pos.z += m_HairWind;
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

        for (int32_t j = 0; j < 5; j++) {
            const SPHERE *const sphere = &m_HairSpheres[j];
            const int32_t dx = s->pos.x - sphere->x;
            const int32_t dy = s->pos.y - sphere->y;
            const int32_t dz = s->pos.z - sphere->z;
            int32_t dist = SQUARE(dz) + SQUARE(dy) + SQUARE(dx);
            if (dist < SQUARE(sphere->r)) {
                dist = Math_Sqrt(dist);
                CLAMPL(dist, 1);
                s->pos.x = sphere->x + sphere->r * dx / dist;
                s->pos.y = sphere->y + sphere->r * dy / dist;
                s->pos.z = sphere->z + sphere->r * dz / dist;
            }
        }

        const int32_t dx = s->pos.x - ps->pos.x;
        const int32_t dz = s->pos.z - ps->pos.z;
        const int32_t distance = Math_Sqrt(SQUARE(dx) + SQUARE(dz));
        ps->rot.y = Math_Atan(dz, dx);
        ps->rot.x = -Math_Atan(distance, s->pos.y - ps->pos.y);

        Matrix_PushUnit();
        Matrix_TranslateSet(ps->pos.x, ps->pos.y, ps->pos.z);
        Matrix_RotY(ps->rot.y);
        Matrix_RotX(ps->rot.x);

        if (i == HAIR_SEGMENTS) {
            const ANIM_BONE *const last_bone = bone - 1;
            Matrix_TranslateRel32(last_bone->pos);
        } else {
            Matrix_TranslateRel32(bone->pos);
        }

        s->pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        s->pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        s->pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;

        m_HairVelocity[i].x = s->pos.x - m_HairVelocity[0].x;
        m_HairVelocity[i].y = s->pos.y - m_HairVelocity[0].y;
        m_HairVelocity[i].z = s->pos.z - m_HairVelocity[0].z;

        Matrix_Pop();
    }
}

void Lara_Hair_Draw(void)
{
    const OBJECT *const obj = Object_Get(O_LARA_HAIR);
    for (int32_t i = 0; i < HAIR_SEGMENTS; i++) {
        const HAIR_SEGMENT *const s = &m_HairSegments[i];
        Matrix_Push();
        Matrix_TranslateAbs32(s->pos);
        Matrix_RotY(s->rot.y);
        Matrix_RotX(s->rot.x);
        Object_DrawMesh(obj->mesh_idx + i, 1, false);
        Matrix_Pop();
    }
}
