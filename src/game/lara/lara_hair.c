#include "game/lara/lara_hair.h"

#include "config.h"
#include "game/items.h"
#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/matrix.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>

#define HAIR_SEGMENTS 6
#define HAIR_OFFSET_X (0) // left-right
#define HAIR_OFFSET_Y (20) // up-down
#define HAIR_OFFSET_Z (-45) // front-back

static bool m_FirstHair = false;
static GAME_OBJECT_ID m_LaraType = O_LARA;
static struct {
    VECTOR_3D pos;
    VECTOR_3D rot;
} m_Hair[HAIR_SEGMENTS + 1] = { 0 };
static VECTOR_3D m_HVel[HAIR_SEGMENTS + 1] = { 0 };

static int16_t Lara_Hair_GetRoom(int32_t x, int32_t y, int32_t z);

void Lara_Hair_Initialise(void)
{
    m_FirstHair = true;
    Lara_Hair_SetLaraType(O_LARA);

    int32_t *bone = &g_AnimBones[g_Objects[O_HAIR].bone_index];

    m_Hair[0].rot.y = 0;
    m_Hair[0].rot.x = -PHD_90;

    for (int i = 1; i < HAIR_SEGMENTS + 1; i++, bone += 4) {
        m_Hair[i].pos.x = *(bone + 1);
        m_Hair[i].pos.y = *(bone + 2);
        m_Hair[i].pos.z = *(bone + 3);
        m_Hair[i].rot.x = -PHD_90;
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
    if (!g_Config.enable_braid || !g_Objects[O_HAIR].loaded
        || !g_Objects[m_LaraType].loaded) {
        return;
    }

    bool in_cutscene;
    OBJECT_INFO *object;
    int32_t *bone, distance;
    int16_t *frame, *objptr, room_number;
    int16_t *frm_ptr[2];
    int16_t **mesh_base;
    VECTOR_3D pos;
    FLOOR_INFO *floor;
    int32_t i, water_level, height, size, frac, rate;
    SPHERE sphere[5];
    int32_t j, x, y, z;

    in_cutscene = m_LaraType != O_LARA;
    object = &g_Objects[m_LaraType];
    mesh_base = &g_Meshes[object->mesh_index];

    if (!in_cutscene && g_Lara.hit_direction >= 0) {
        int16_t spaz = object->anim_index;

        switch (g_Lara.hit_direction) {
        case DIR_NORTH:
            spaz += LA_SPAZ_FORWARD;
            break;

        case DIR_SOUTH:
            spaz += LA_SPAZ_BACK;
            break;

        case DIR_EAST:
            spaz += LA_SPAZ_RIGHT;
            break;

        default:
            spaz += LA_SPAZ_LEFT;
            break;
        }

        frame = g_Anims[spaz].frame_ptr;
        size = g_Anims[spaz].interpolation >> 8;

        frame += (int)(g_Lara.hit_frame * size);
        frac = 0;
    } else {
        frame = Item_GetBestFrame(g_LaraItem);
        frac = Item_GetFrames(g_LaraItem, frm_ptr, &rate);
    }

    Matrix_PushUnit();
    Matrix_TranslateSet(
        g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z);
    Matrix_RotYXZ(g_LaraItem->rot.y, g_LaraItem->rot.x, g_LaraItem->rot.z);

    bone = &g_AnimBones[object->bone_index];
    if (frac) {
        Matrix_InitInterpolate(frac, rate);
        int32_t *packed_rotation1 = (int32_t *)(frm_ptr[0] + FRAME_ROT);
        int32_t *packed_rotation2 = (int32_t *)(frm_ptr[1] + FRAME_ROT);
        Matrix_TranslateRel_ID(
            frm_ptr[0][FRAME_POS_X], frm_ptr[0][FRAME_POS_Y],
            frm_ptr[0][FRAME_POS_Z], frm_ptr[1][FRAME_POS_X],
            frm_ptr[1][FRAME_POS_Y], frm_ptr[1][FRAME_POS_Z]);
        Matrix_RotYXZpack_I(
            packed_rotation1[LM_HIPS], packed_rotation2[LM_HIPS]);

        // hips
        Matrix_Push_I();
        objptr = mesh_base[LM_HIPS];
        Matrix_TranslateRel_I(*objptr, *(objptr + 1), *(objptr + 2));
        Matrix_Interpolate();
        sphere[0].x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[0].y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[0].z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[0].r = (int32_t) * (objptr + 3);
        Matrix_Pop_I();

        // torso
        Matrix_TranslateRel_I(
            *(bone + 1 + 24), *(bone + 2 + 24), *(bone + 3 + 24));
        Matrix_RotYXZpack_I(
            packed_rotation1[LM_TORSO], packed_rotation2[LM_TORSO]);
        Matrix_RotYXZ_I(
            g_Lara.torso_y_rot, g_Lara.torso_x_rot, g_Lara.torso_z_rot);
        Matrix_Push_I();
        objptr = g_Meshes[object->mesh_index + LM_TORSO]; // ignore shotgun
        Matrix_TranslateRel_I(*objptr, *(objptr + 1), *(objptr + 2));
        Matrix_Interpolate();
        sphere[1].x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[1].y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[1].z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[1].r = (int32_t) * (objptr + 3);
        Matrix_Pop_I();

        // right arm
        Matrix_Push_I();
        Matrix_TranslateRel_I(
            *(bone + 1 + 28), *(bone + 2 + 28), *(bone + 3 + 28));
        Matrix_RotYXZpack_I(
            packed_rotation1[LM_UARM_R], packed_rotation2[LM_UARM_R]);
        objptr = mesh_base[LM_UARM_R];
        Matrix_TranslateRel_I(*objptr, *(objptr + 1), *(objptr + 2));
        Matrix_Interpolate();
        sphere[3].x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[3].y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[3].z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[3].r = (int32_t) * (objptr + 3) * 3 / 2;
        Matrix_Pop_I();

        // left arm
        Matrix_Push_I();
        Matrix_TranslateRel_I(
            *(bone + 1 + 40), *(bone + 2 + 40), *(bone + 3 + 40));
        Matrix_RotYXZpack_I(
            packed_rotation1[LM_UARM_L], packed_rotation2[LM_UARM_L]);
        objptr = mesh_base[LM_UARM_L];
        Matrix_TranslateRel_I(*objptr, *(objptr + 1), *(objptr + 2));
        Matrix_Interpolate();
        sphere[4].x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[4].y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[4].z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[4].r = (int32_t) * (objptr + 3) * 3 / 2;
        Matrix_Pop_I();

        // head
        Matrix_TranslateRel_I(
            *(bone + 1 + 52), *(bone + 2 + 52), *(bone + 3 + 52));
        Matrix_RotYXZpack_I(
            packed_rotation1[LM_HEAD], packed_rotation2[LM_HEAD]);
        Matrix_RotYXZ_I(
            g_Lara.head_y_rot, g_Lara.head_x_rot, g_Lara.head_z_rot);
        Matrix_Push_I();
        objptr = mesh_base[LM_HEAD];
        Matrix_TranslateRel_I(*objptr, *(objptr + 1), *(objptr + 2));
        Matrix_Interpolate();
        sphere[2].x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[2].y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[2].z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[2].r = (int32_t) * (objptr + 3);
        Matrix_Pop_I();

        Matrix_TranslateRel_I(HAIR_OFFSET_X, HAIR_OFFSET_Y, HAIR_OFFSET_Z);
        Matrix_Interpolate();

    } else {
        Matrix_TranslateRel(
            frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);
        int32_t *packed_rotation = (int32_t *)(frame + FRAME_ROT);
        Matrix_RotYXZpack(packed_rotation[LM_HIPS]);

        // hips
        Matrix_Push();
        objptr = mesh_base[LM_HIPS];
        Matrix_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
        sphere[0].x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[0].y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[0].z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[0].r = (int32_t) * (objptr + 3);
        Matrix_Pop();

        // torso
        Matrix_TranslateRel(
            *(bone + 1 + 24), *(bone + 2 + 24), *(bone + 3 + 24));
        Matrix_RotYXZpack(packed_rotation[LM_TORSO]);
        Matrix_RotYXZ(
            g_Lara.torso_y_rot, g_Lara.torso_x_rot, g_Lara.torso_z_rot);
        Matrix_Push();
        objptr = g_Meshes[object->mesh_index + LM_TORSO]; // ignore shotgun
        Matrix_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
        sphere[1].x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[1].y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[1].z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[1].r = (int32_t) * (objptr + 3);
        Matrix_Pop();

        // right arm
        Matrix_Push();
        Matrix_TranslateRel(
            *(bone + 1 + 28), *(bone + 2 + 28), *(bone + 3 + 28));
        Matrix_RotYXZpack(packed_rotation[LM_UARM_R]);
        objptr = mesh_base[LM_UARM_R];
        Matrix_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
        sphere[3].x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[3].y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[3].z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[3].r = (int32_t) * (objptr + 3) * 3 / 2;
        Matrix_Pop();

        // left arm
        Matrix_Push();
        Matrix_TranslateRel(
            *(bone + 1 + 40), *(bone + 2 + 40), *(bone + 3 + 40));
        Matrix_RotYXZpack(packed_rotation[LM_UARM_L]);
        objptr = mesh_base[LM_UARM_L];
        Matrix_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
        sphere[4].x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[4].y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[4].z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[4].r = (int32_t) * (objptr + 3) * 3 / 2;
        Matrix_Pop();

        // head
        Matrix_TranslateRel(
            *(bone + 1 + 52), *(bone + 2 + 52), *(bone + 3 + 52));
        Matrix_RotYXZpack(packed_rotation[LM_HEAD]);
        Matrix_RotYXZ(g_Lara.head_y_rot, g_Lara.head_x_rot, g_Lara.head_z_rot);
        Matrix_Push();
        objptr = mesh_base[LM_HEAD];
        Matrix_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
        sphere[2].x = g_MatrixPtr->_03 >> W2V_SHIFT;
        sphere[2].y = g_MatrixPtr->_13 >> W2V_SHIFT;
        sphere[2].z = g_MatrixPtr->_23 >> W2V_SHIFT;
        sphere[2].r = (int32_t) * (objptr + 3);
        Matrix_Pop();

        Matrix_TranslateRel(HAIR_OFFSET_X, HAIR_OFFSET_Y, HAIR_OFFSET_Z);
    }

    pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    Matrix_Pop();

    bone = &g_AnimBones[g_Objects[O_HAIR].bone_index];

    m_Hair[0].pos = pos;

    if (m_FirstHair) {
        m_FirstHair = false;

        for (i = 0; i < HAIR_SEGMENTS; i++, bone += 4) {
            Matrix_PushUnit();
            Matrix_TranslateSet(
                m_Hair[i].pos.x, m_Hair[i].pos.y, m_Hair[i].pos.z);
            Matrix_RotYXZ(m_Hair[i].rot.y, m_Hair[i].rot.x, 0);
            Matrix_TranslateRel(*(bone + 1), *(bone + 2), *(bone + 3));

            m_Hair[i + 1].pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
            m_Hair[i + 1].pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
            m_Hair[i + 1].pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;

            Matrix_Pop();
        }
    } else {
        if (in_cutscene) {
            room_number = Lara_Hair_GetRoom(pos.x, pos.y, pos.z);
            water_level = NO_HEIGHT;
        } else {
            room_number = g_LaraItem->room_number;
            x = g_LaraItem->pos.x
                + (frame[FRAME_BOUND_MIN_X] + frame[FRAME_BOUND_MAX_X]) / 2;
            y = g_LaraItem->pos.y
                + (frame[FRAME_BOUND_MIN_Y] + frame[FRAME_BOUND_MAX_Y]) / 2;
            z = g_LaraItem->pos.z
                + (frame[FRAME_BOUND_MIN_Z] + frame[FRAME_BOUND_MAX_Z]) / 2;
            water_level = Room_GetWaterHeight(x, y, z, room_number);
        }

        for (i = 1; i < HAIR_SEGMENTS + 1; i++, bone += 4) {
            m_HVel[0] = m_Hair[i].pos;

            floor = Room_GetFloor(
                m_Hair[i].pos.x, m_Hair[i].pos.y, m_Hair[i].pos.z,
                &room_number);
            height = Room_GetHeight(
                floor, m_Hair[i].pos.x, m_Hair[i].pos.y, m_Hair[i].pos.z);

            m_Hair[i].pos.x += m_HVel[i].x * 3 / 4;
            m_Hair[i].pos.y += m_HVel[i].y * 3 / 4;
            m_Hair[i].pos.z += m_HVel[i].z * 3 / 4;

            switch (g_Lara.water_status) {
            case LWS_ABOVE_WATER:
                m_Hair[i].pos.y += 10;
                if (water_level != NO_HEIGHT && m_Hair[i].pos.y > water_level)
                    m_Hair[i].pos.y = water_level;
                else if (m_Hair[i].pos.y > height) {
                    m_Hair[i].pos.x = m_HVel[0].x;
                    if (m_Hair[i].pos.y - height <= STEP_L) {
                        m_Hair[i].pos.y = height;
                    }
                    m_Hair[i].pos.z = m_HVel[0].z;
                }
                break;

            case LWS_UNDERWATER:
            case LWS_SURFACE:
                if (m_Hair[i].pos.y < water_level) {
                    m_Hair[i].pos.y = water_level;
                } else if (m_Hair[i].pos.y > height) {
                    m_Hair[i].pos.y = height;
                }
                break;
            }

            for (j = 0; j < 5; j++) {
                x = m_Hair[i].pos.x - sphere[j].x;
                y = m_Hair[i].pos.y - sphere[j].y;
                z = m_Hair[i].pos.z - sphere[j].z;

                distance = x * x + y * y + z * z;

                if (distance < SQUARE(sphere[j].r)) {
                    distance = Math_Sqrt(distance);

                    if (distance == 0)
                        distance = 1;

                    m_Hair[i].pos.x = sphere[j].x + x * sphere[j].r / distance;
                    m_Hair[i].pos.y = sphere[j].y + y * sphere[j].r / distance;
                    m_Hair[i].pos.z = sphere[j].z + z * sphere[j].r / distance;
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
            Matrix_RotYXZ(m_Hair[i - 1].rot.y, m_Hair[i - 1].rot.x, 0);

            if (i == HAIR_SEGMENTS) {
                Matrix_TranslateRel(*(bone - 3), *(bone - 2), *(bone - 1));
            } else {
                Matrix_TranslateRel(*(bone + 1), *(bone + 2), *(bone + 3));
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
    if (!g_Config.enable_braid || !g_Objects[O_HAIR].loaded
        || !g_Objects[m_LaraType].loaded) {
        return;
    }

    OBJECT_INFO *object = &g_Objects[O_HAIR];
    int16_t mesh_index = object->mesh_index;
    if ((g_Lara.mesh_effects & (1 << LM_HEAD))
        && object->nmeshes >= HAIR_SEGMENTS * 2) {
        mesh_index += HAIR_SEGMENTS;
    }
    int16_t **mesh = &g_Meshes[mesh_index];

    for (int i = 0; i < HAIR_SEGMENTS; i++) {
        Matrix_Push();

        Matrix_TranslateAbs(m_Hair[i].pos.x, m_Hair[i].pos.y, m_Hair[i].pos.z);
        Matrix_RotY(m_Hair[i].rot.y);
        Matrix_RotX(m_Hair[i].rot.x);
        Output_DrawPolygons(*mesh++, 1);

        Matrix_Pop();
    }
}

static int16_t Lara_Hair_GetRoom(int32_t x, int32_t y, int32_t z)
{
    int16_t room_num = Room_GetIndexFromPos(x, y, z);
    if (room_num != NO_ROOM) {
        return room_num;
    }
    return g_LaraItem->room_number;
}
