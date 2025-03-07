#include "game/lara/hair.h"

#include "game/items.h"
#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/lara/common.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

#include <stdint.h>

#define HAIR_SEGMENTS 6
#define HAIR_OFFSET_X (0) // left-right
#define HAIR_OFFSET_Y (20) // up-down
#define HAIR_OFFSET_Z (-45) // front-back

static bool m_FirstHair = false;
static GAME_OBJECT_ID m_LaraType = O_LARA;
static HAIR_SEGMENT m_Hair[HAIR_SEGMENTS + 1] = {};
static XYZ_32 m_HVel[HAIR_SEGMENTS + 1] = {};

static int16_t M_GetRoom(int32_t x, int32_t y, int32_t z);

static int16_t M_GetRoom(int32_t x, int32_t y, int32_t z)
{
    int16_t room_num = Room_GetIndexFromPos(x, y, z);
    if (room_num != NO_ROOM) {
        return room_num;
    }
    return g_LaraItem->room_num;
}

int32_t Lara_Hair_GetSegmentCount(void)
{
    return HAIR_SEGMENTS;
}

HAIR_SEGMENT *Lara_Hair_GetSegment(int32_t n)
{
    return &m_Hair[n];
}

bool Lara_Hair_IsActive(void)
{
    return g_Config.visuals.enable_braid && Object_Get(O_HAIR)->loaded
        && Object_Get(m_LaraType)->loaded;
}

void Lara_Hair_Initialise(void)
{
    m_FirstHair = true;
    Lara_Hair_SetLaraType(O_LARA);

    const OBJECT *const obj = Object_Get(O_HAIR);

    m_Hair[0].rot.y = 0;
    m_Hair[0].rot.x = -DEG_90;

    for (int32_t i = 1; i < HAIR_SEGMENTS + 1; i++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
        m_Hair[i].pos = bone->pos;
        m_Hair[i].rot.x = -DEG_90;
        m_Hair[i].rot.y = 0;
        m_Hair[i].rot.z = 0;
        m_HVel[i].x = 0;
        m_HVel[i].y = 0;
        m_HVel[i].z = 0;
    }
}

void Lara_Hair_SetLaraType(GAME_OBJECT_ID lara_type)
{
    m_LaraType = lara_type;
}

void Lara_Hair_Control(void)
{
    if (!Lara_Hair_IsActive()) {
        return;
    }

    int32_t distance;
    const OBJECT_MESH *mesh;
    int16_t room_num;
    ANIM_FRAME *frmptr[2];
    XYZ_32 pos;
    const SECTOR *sector;
    int32_t i;
    int32_t water_level;
    int32_t height;
    int32_t frac;
    int32_t rate;
    SPHERE sphere[5];
    int32_t j;
    int32_t x;
    int32_t y;
    int32_t z;

    const bool in_cutscene = m_LaraType != O_LARA;
    const ANIM_FRAME *const hit_frame = Lara_GetHitFrame(g_LaraItem);
    const ANIM_FRAME *frame;
    if (!in_cutscene && hit_frame != nullptr) {
        frame = hit_frame;
        frac = 0;
    } else {
        frame = Item_GetBestFrame(g_LaraItem);
        frac = Item_GetFrames(g_LaraItem, frmptr, &rate);
    }

    Matrix_PushUnit();
    Matrix_TranslateSet(
        g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z);
    Matrix_Rot16(g_LaraItem->rot);

    const OBJECT *const obj = Object_Get(m_LaraType);
    const ANIM_BONE *bone = Object_GetBone(obj, 0);
    if (frac) {
        const XYZ_16 *const mesh_rots_1 = frmptr[0]->mesh_rots;
        const XYZ_16 *const mesh_rots_2 = frmptr[1]->mesh_rots;

        Matrix_InitInterpolate(frac, rate);
        Matrix_TranslateRel16_ID(frmptr[0]->offset, frmptr[1]->offset);
        Matrix_Rot16_ID(mesh_rots_1[LM_HIPS], mesh_rots_2[LM_HIPS]);

        // hips
        Matrix_Push_I();
        mesh = Object_GetMesh(obj->mesh_idx + LM_HIPS);
        Matrix_TranslateRel16_I(mesh->center);
        Matrix_Interpolate();
        sphere[0].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[0].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[0].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[0].r = mesh->radius;
        Matrix_Pop_I();

        // torso
        Matrix_TranslateRel32_I(bone[LM_TORSO - 1].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_TORSO], mesh_rots_2[LM_TORSO]);
        Matrix_Rot16_I(g_Lara.interp.result.torso_rot);
        Matrix_Push_I();
        mesh = Object_GetMesh(obj->mesh_idx + LM_TORSO);
        Matrix_TranslateRel16_I(mesh->center);
        Matrix_Interpolate();
        sphere[1].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[1].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[1].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[1].r = mesh->radius;
        Matrix_Pop_I();

        // right arm
        Matrix_Push_I();
        Matrix_TranslateRel32_I(bone[LM_UARM_R - 1].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_UARM_R], mesh_rots_2[LM_UARM_R]);
        mesh = Object_GetMesh(obj->mesh_idx + LM_UARM_R);
        Matrix_TranslateRel16_I(mesh->center);
        Matrix_Interpolate();
        sphere[3].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[3].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[3].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[3].r = mesh->radius * 3 / 2;
        Matrix_Pop_I();

        // left arm
        Matrix_Push_I();
        Matrix_TranslateRel32_I(bone[LM_UARM_L - 1].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_UARM_L], mesh_rots_2[LM_UARM_L]);
        mesh = Object_GetMesh(obj->mesh_idx + LM_UARM_L);
        Matrix_TranslateRel16_I(mesh->center);
        Matrix_Interpolate();
        sphere[4].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[4].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[4].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[4].r = mesh->radius * 3 / 2;
        Matrix_Pop_I();

        // head
        Matrix_TranslateRel32_I(bone[LM_HEAD - 1].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_HEAD], mesh_rots_2[LM_HEAD]);
        Matrix_Rot16_I(g_Lara.interp.result.head_rot);
        Matrix_Push_I();
        mesh = Object_GetMesh(obj->mesh_idx + LM_HEAD);
        Matrix_TranslateRel16_I(mesh->center);
        Matrix_Interpolate();
        sphere[2].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[2].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[2].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[2].r = mesh->radius;
        Matrix_Pop_I();

        Matrix_TranslateRel_I(HAIR_OFFSET_X, HAIR_OFFSET_Y, HAIR_OFFSET_Z);
        Matrix_Interpolate();

    } else {
        const XYZ_16 *const mesh_rots = frame->mesh_rots;
        Matrix_TranslateRel16(frame->offset);
        Matrix_Rot16(mesh_rots[LM_HIPS]);

        // hips
        Matrix_Push();
        mesh = Object_GetMesh(obj->mesh_idx + LM_HIPS);
        Matrix_TranslateRel16(mesh->center);
        sphere[0].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[0].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[0].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[0].r = mesh->radius;
        Matrix_Pop();

        // torso
        Matrix_TranslateRel32(bone[LM_TORSO - 1].pos);
        Matrix_Rot16(mesh_rots[LM_TORSO]);
        Matrix_Rot16(g_Lara.interp.result.torso_rot);
        Matrix_Push();
        mesh = Object_GetMesh(obj->mesh_idx + LM_TORSO);
        Matrix_TranslateRel16(mesh->center);
        sphere[1].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[1].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[1].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[1].r = mesh->radius;
        Matrix_Pop();

        // right arm
        Matrix_Push();
        Matrix_TranslateRel32(bone[LM_UARM_R - 1].pos);
        Matrix_Rot16(mesh_rots[LM_UARM_R]);
        mesh = Object_GetMesh(obj->mesh_idx + LM_UARM_R);
        Matrix_TranslateRel16(mesh->center);
        sphere[3].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[3].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[3].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[3].r = mesh->radius * 3 / 2;
        Matrix_Pop();

        // left arm
        Matrix_Push();
        Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
        Matrix_Rot16(mesh_rots[LM_UARM_L]);
        mesh = Object_GetMesh(obj->mesh_idx + LM_UARM_L);
        Matrix_TranslateRel16(mesh->center);
        sphere[4].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[4].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[4].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[4].r = mesh->radius * 3 / 2;
        Matrix_Pop();

        // head
        Matrix_TranslateRel32(bone[LM_HEAD - 1].pos);
        Matrix_Rot16(mesh_rots[LM_HEAD]);
        Matrix_Rot16(g_Lara.interp.result.head_rot);
        Matrix_Push();
        mesh = Object_GetMesh(obj->mesh_idx + LM_HEAD);
        Matrix_TranslateRel16(mesh->center);
        sphere[2].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[2].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[2].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[2].r = mesh->radius;
        Matrix_Pop();

        Matrix_TranslateRel(HAIR_OFFSET_X, HAIR_OFFSET_Y, HAIR_OFFSET_Z);
    }

    pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    Matrix_Pop();

    const OBJECT *const hair_obj = Object_Get(O_HAIR);

    m_Hair[0].pos = pos;

    if (m_FirstHair) {
        m_FirstHair = false;

        for (i = 0; i < HAIR_SEGMENTS; i++) {
            const ANIM_BONE *const hair_bone = Object_GetBone(hair_obj, i);
            Matrix_PushUnit();
            Matrix_TranslateSet(
                m_Hair[i].pos.x, m_Hair[i].pos.y, m_Hair[i].pos.z);
            Matrix_RotY(m_Hair[i].rot.y);
            Matrix_RotX(m_Hair[i].rot.x);
            Matrix_TranslateRel32(hair_bone->pos);

            m_Hair[i + 1].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
            m_Hair[i + 1].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
            m_Hair[i + 1].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;

            Matrix_Pop();
        }
    } else {
        if (in_cutscene) {
            room_num = M_GetRoom(pos.x, pos.y, pos.z);
            water_level = NO_HEIGHT;
        } else {
            room_num = g_LaraItem->room_num;
            x = g_LaraItem->pos.x
                + (frame->bounds.min.x + frame->bounds.max.x) / 2;
            y = g_LaraItem->pos.y
                + (frame->bounds.min.y + frame->bounds.max.y) / 2;
            z = g_LaraItem->pos.z
                + (frame->bounds.min.z + frame->bounds.max.z) / 2;
            water_level = Room_GetWaterHeight(x, y, z, room_num);
        }

        for (i = 1; i < HAIR_SEGMENTS + 1; i++) {
            const ANIM_BONE *const hair_bone = Object_GetBone(hair_obj, i - 1);
            m_HVel[0] = m_Hair[i].pos;

            sector = Room_GetSector(
                m_Hair[i].pos.x, m_Hair[i].pos.y, m_Hair[i].pos.z, &room_num);
            height = Room_GetHeight(
                sector, m_Hair[i].pos.x, m_Hair[i].pos.y, m_Hair[i].pos.z);

            m_Hair[i].pos.x += m_HVel[i].x * 3 / 4;
            m_Hair[i].pos.y += m_HVel[i].y * 3 / 4;
            m_Hair[i].pos.z += m_HVel[i].z * 3 / 4;

            switch (g_Lara.water_status) {
            case LWS_ABOVE_WATER:
            case LWS_WADE:
                m_Hair[i].pos.y += 10;
                if (water_level != NO_HEIGHT && m_Hair[i].pos.y > water_level)
                    m_Hair[i].pos.y = water_level;
                else if (m_Hair[i].pos.y > height) {
                    m_Hair[i].pos.x = m_HVel[0].x;
                    m_Hair[i].pos.z = m_HVel[0].z;
                    if (m_Hair[i].pos.y - height <= STEP_L) {
                        m_Hair[i].pos.y = height;
                    }
                }
                break;

            case LWS_UNDERWATER:
            case LWS_SURFACE:
            case LWS_CHEAT:
                if (m_Hair[i].pos.y < water_level) {
                    m_Hair[i].pos.y = water_level;
                } else if (m_Hair[i].pos.y > height) {
                    m_Hair[i].pos.y = height;
                }
                break;
            }

            for (j = 0; j < 5; j++) {
                x = m_Hair[i].pos.x - sphere[j].pos.x;
                y = m_Hair[i].pos.y - sphere[j].pos.y;
                z = m_Hair[i].pos.z - sphere[j].pos.z;

                distance = x * x + y * y + z * z;

                if (distance < SQUARE(sphere[j].r)) {
                    distance = Math_Sqrt(distance);

                    if (distance == 0)
                        distance = 1;

                    m_Hair[i].pos.x =
                        sphere[j].pos.x + x * sphere[j].r / distance;
                    m_Hair[i].pos.y =
                        sphere[j].pos.y + y * sphere[j].r / distance;
                    m_Hair[i].pos.z =
                        sphere[j].pos.z + z * sphere[j].r / distance;
                }
            }

            distance = Math_Sqrt(
                SQUARE(m_Hair[i].pos.z - m_Hair[i - 1].pos.z)
                + SQUARE(m_Hair[i].pos.x - m_Hair[i - 1].pos.x));
            m_Hair[i - 1].rot.y = Math_Atan(
                m_Hair[i].pos.z - m_Hair[i - 1].pos.z,
                m_Hair[i].pos.x - m_Hair[i - 1].pos.x);
            m_Hair[i - 1].rot.x =
                -Math_Atan(distance, m_Hair[i].pos.y - m_Hair[i - 1].pos.y);

            Matrix_PushUnit();
            Matrix_TranslateSet(
                m_Hair[i - 1].pos.x, m_Hair[i - 1].pos.y, m_Hair[i - 1].pos.z);
            Matrix_RotY(m_Hair[i - 1].rot.y);
            Matrix_RotX(m_Hair[i - 1].rot.x);

            if (i == HAIR_SEGMENTS) {
                const ANIM_BONE *const last_bone = hair_bone - 1;
                Matrix_TranslateRel32(last_bone->pos);
            } else {
                Matrix_TranslateRel32(hair_bone->pos);
            }

            m_Hair[i].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
            m_Hair[i].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
            m_Hair[i].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;

            m_HVel[i].x = m_Hair[i].pos.x - m_HVel[0].x;
            m_HVel[i].y = m_Hair[i].pos.y - m_HVel[0].y;
            m_HVel[i].z = m_Hair[i].pos.z - m_HVel[0].z;

            Matrix_Pop();
        }
    }
}

void Lara_Hair_Draw(void)
{
    if (!Lara_Hair_IsActive()) {
        return;
    }

    const OBJECT *const obj = Object_Get(O_HAIR);
    int16_t mesh_idx = obj->mesh_idx;
    if ((g_Lara.mesh_effects & (1 << LM_HEAD))
        && obj->mesh_count >= HAIR_SEGMENTS * 2) {
        mesh_idx += HAIR_SEGMENTS;
    }

    for (int i = 0; i < HAIR_SEGMENTS; i++) {
        Matrix_Push();

        Matrix_TranslateAbs32(m_Hair[i].interp.result.pos);
        Matrix_RotY(m_Hair[i].interp.result.rot.y);
        Matrix_RotX(m_Hair[i].interp.result.rot.x);
        Object_DrawMesh(mesh_idx + i, 1, false);

        Matrix_Pop();
    }
}
