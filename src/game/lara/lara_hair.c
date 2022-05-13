#include "game/lara/lara_hair.h"

#include "config.h"
#include "game/draw.h"
#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/matrix.h"
#include "util.h"

#include <stdint.h>

#define HAIR_SEGMENTS 6
#define HAIR_OFFSET_X (0) // left-right
#define HAIR_OFFSET_Y (20) // up-down
#define HAIR_OFFSET_Z (-45) // front-back

static bool m_FirstHair = false;
static PHD_3DPOS m_Hair[HAIR_SEGMENTS + 1] = { 0 };
static PHD_VECTOR m_HVel[HAIR_SEGMENTS + 1] = { 0 };

void Lara_Hair_Initialise(void)
{
    m_FirstHair = true;

    int32_t *bone = &g_AnimBones[g_Objects[O_HAIR].bone_index];

    m_Hair[0].y_rot = 0;
    m_Hair[0].x_rot = -PHD_90;

    for (int i = 1; i < HAIR_SEGMENTS + 1; i++, bone += 4) {
        m_Hair[i].x = *(bone + 1);
        m_Hair[i].y = *(bone + 2);
        m_Hair[i].z = *(bone + 3);
        m_Hair[i].x_rot = -PHD_90;
        m_Hair[i].y_rot = m_Hair[i].z_rot = 0;
        m_HVel[i].x = 0;
        m_HVel[i].y = 0;
        m_HVel[i].z = 0;
    }
}

void Lara_Hair_Control(bool in_cutscene)
{
    if (!g_Config.enable_braid || !g_Objects[O_HAIR].loaded) {
        return;
    }

    OBJECT_INFO *object;
    int32_t *bone, distance;
    int16_t *frame, *objptr, room_number;
    PHD_VECTOR pos;
    FLOOR_INFO *floor;
    int32_t i, water_level, height, size;
    SPHERE sphere[5];
    int32_t j, x, y, z;

    object = &g_Objects[O_LARA];

    if (g_Lara.hit_direction >= 0) {
        int16_t spaz;

        switch (g_Lara.hit_direction) {
        case DIR_NORTH:
            spaz = LA_SPAZ_FORWARD;
            break;

        case DIR_SOUTH:
            spaz = LA_SPAZ_BACK;
            break;

        case DIR_EAST:
            spaz = LA_SPAZ_RIGHT;
            break;

        default:
            spaz = LA_SPAZ_LEFT;
            break;
        }

        frame = g_Anims[spaz].frame_ptr;
        size = g_Anims[spaz].interpolation >> 8;

        frame += (int)(g_Lara.hit_frame * size);
    } else
        frame = GetBestFrame(g_LaraItem);

    Matrix_PushUnit();
    g_MatrixPtr->_03 = g_LaraItem->pos.x << W2V_SHIFT;
    g_MatrixPtr->_13 = g_LaraItem->pos.y << W2V_SHIFT;
    g_MatrixPtr->_23 = g_LaraItem->pos.z << W2V_SHIFT;
    Matrix_RotYXZ(
        g_LaraItem->pos.y_rot, g_LaraItem->pos.x_rot, g_LaraItem->pos.z_rot);

    bone = &g_AnimBones[object->bone_index];

    Matrix_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);
    int32_t *packed_rotation = (int32_t *)(frame + FRAME_ROT);
    Matrix_RotYXZpack(packed_rotation[LM_HIPS]);

    // hips
    Matrix_Push();
    objptr = g_Lara.mesh_ptrs[LM_HIPS];
    Matrix_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
    sphere[0].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    sphere[0].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    sphere[0].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    sphere[0].r = (int32_t) * (objptr + 3);
    Matrix_Pop();

    // torso
    Matrix_TranslateRel(*(bone + 1 + 24), *(bone + 2 + 24), *(bone + 3 + 24));
    Matrix_RotYXZpack(packed_rotation[LM_TORSO]);
    Matrix_RotYXZ(g_Lara.torso_y_rot, g_Lara.torso_x_rot, g_Lara.torso_z_rot);
    Matrix_Push();
    objptr =
        g_Meshes[g_Objects[O_LARA].mesh_index + LM_TORSO]; // ignore shotgun
    Matrix_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
    sphere[1].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    sphere[1].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    sphere[1].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    sphere[1].r = (int32_t) * (objptr + 3);
    Matrix_Pop();

    // right arm
    Matrix_Push();
    Matrix_TranslateRel(*(bone + 1 + 28), *(bone + 2 + 28), *(bone + 3 + 28));
    Matrix_RotYXZpack(packed_rotation[LM_UARM_R]);
    objptr = g_Lara.mesh_ptrs[LM_UARM_R];
    Matrix_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
    sphere[3].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    sphere[3].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    sphere[3].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    sphere[3].r = (int32_t) * (objptr + 3) * 3 / 2;
    Matrix_Pop();

    // left arm
    Matrix_Push();
    Matrix_TranslateRel(*(bone + 1 + 40), *(bone + 2 + 40), *(bone + 3 + 40));
    Matrix_RotYXZpack(packed_rotation[LM_UARM_L]);
    objptr = g_Lara.mesh_ptrs[LM_UARM_L];
    Matrix_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
    sphere[4].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    sphere[4].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    sphere[4].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    sphere[4].r = (int32_t) * (objptr + 3) * 3 / 2;
    Matrix_Pop();

    // head
    Matrix_TranslateRel(*(bone + 1 + 52), *(bone + 2 + 52), *(bone + 3 + 52));
    Matrix_RotYXZpack(packed_rotation[LM_HEAD]);
    Matrix_RotYXZ(g_Lara.head_y_rot, g_Lara.head_x_rot, g_Lara.head_z_rot);
    Matrix_Push();
    objptr = g_Lara.mesh_ptrs[LM_HEAD];
    Matrix_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
    sphere[2].x = g_MatrixPtr->_03 >> W2V_SHIFT;
    sphere[2].y = g_MatrixPtr->_13 >> W2V_SHIFT;
    sphere[2].z = g_MatrixPtr->_23 >> W2V_SHIFT;
    sphere[2].r = (int32_t) * (objptr + 3);
    Matrix_Pop();

    Matrix_TranslateRel(HAIR_OFFSET_X, HAIR_OFFSET_Y, HAIR_OFFSET_Z);

    pos.x = g_MatrixPtr->_03 >> W2V_SHIFT;
    pos.y = g_MatrixPtr->_13 >> W2V_SHIFT;
    pos.z = g_MatrixPtr->_23 >> W2V_SHIFT;
    Matrix_Pop();

    bone = &g_AnimBones[g_Objects[O_HAIR].bone_index];

    if (m_FirstHair) {
        m_FirstHair = false;

        m_Hair[0].x = pos.x;
        m_Hair[0].y = pos.y;
        m_Hair[0].z = pos.z;

        for (i = 0; i < HAIR_SEGMENTS; i++, bone += 4) {
            Matrix_PushUnit();
            g_MatrixPtr->_03 = m_Hair[i].x << W2V_SHIFT;
            g_MatrixPtr->_13 = m_Hair[i].y << W2V_SHIFT;
            g_MatrixPtr->_23 = m_Hair[i].z << W2V_SHIFT;
            Matrix_RotYXZ(m_Hair[i].y_rot, m_Hair[i].x_rot, 0);
            Matrix_TranslateRel(*(bone + 1), *(bone + 2), *(bone + 3));

            m_Hair[i + 1].x = g_MatrixPtr->_03 >> W2V_SHIFT;
            m_Hair[i + 1].y = g_MatrixPtr->_13 >> W2V_SHIFT;
            m_Hair[i + 1].z = g_MatrixPtr->_23 >> W2V_SHIFT;

            Matrix_Pop();
        }
    } else {
        m_Hair[0].x = pos.x;
        m_Hair[0].y = pos.y;
        m_Hair[0].z = pos.z;

        room_number = g_LaraItem->room_number;

        if (in_cutscene)
            water_level = NO_HEIGHT;
        else {
            x = g_LaraItem->pos.x
                + (frame[FRAME_BOUND_MIN_X] + frame[FRAME_BOUND_MAX_X]) / 2;
            y = g_LaraItem->pos.y
                + (frame[FRAME_BOUND_MIN_Y] + frame[FRAME_BOUND_MAX_Y]) / 2;
            z = g_LaraItem->pos.z
                + (frame[FRAME_BOUND_MIN_Z] + frame[FRAME_BOUND_MAX_Z]) / 2;
            water_level = Room_GetWaterHeight(x, y, z, room_number);
        }

        for (i = 1; i < HAIR_SEGMENTS + 1; i++, bone += 4) {
            m_HVel[0].x = m_Hair[i].x;
            m_HVel[0].y = m_Hair[i].y;
            m_HVel[0].z = m_Hair[i].z;

            if (!in_cutscene) {
                floor = Room_GetFloor(
                    m_Hair[i].x, m_Hair[i].y, m_Hair[i].z, &room_number);
                height = Room_GetHeight(
                    floor, m_Hair[i].x, m_Hair[i].y, m_Hair[i].z);
            } else
                height = 32767;

            m_Hair[i].x += m_HVel[i].x * 3 / 4;
            m_Hair[i].y += m_HVel[i].y * 3 / 4;
            m_Hair[i].z += m_HVel[i].z * 3 / 4;

            switch (g_Lara.water_status) {
            case LWS_ABOVE_WATER:
                m_Hair[i].y += 10;
                if (water_level != NO_HEIGHT && m_Hair[i].y > water_level)
                    m_Hair[i].y = water_level;
                else if (m_Hair[i].y > height) {
                    m_Hair[i].x = m_HVel[0].x;
                    m_Hair[i].z = m_HVel[0].z;
                }
                break;

            case LWS_UNDERWATER:
            case LWS_SURFACE:
                if (m_Hair[i].y < water_level) {
                    m_Hair[i].y = water_level;
                } else if (m_Hair[i].y > height) {
                    m_Hair[i].y = height;
                }
                break;
            }

            for (j = 0; j < 5; j++) {
                x = m_Hair[i].x - sphere[j].x;
                y = m_Hair[i].y - sphere[j].y;
                z = m_Hair[i].z - sphere[j].z;

                distance = x * x + y * y + z * z;

                if (distance < SQUARE(sphere[j].r)) {
                    distance = Math_Sqrt(distance);

                    if (distance == 0)
                        distance = 1;

                    m_Hair[i].x = sphere[j].x + x * sphere[j].r / distance;
                    m_Hair[i].y = sphere[j].y + y * sphere[j].r / distance;
                    m_Hair[i].z = sphere[j].z + z * sphere[j].r / distance;
                }
            }

            distance = Math_Sqrt(
                SQUARE(m_Hair[i].z - m_Hair[i - 1].z)
                + SQUARE(m_Hair[i].x - m_Hair[i - 1].x));
            m_Hair[i - 1].y_rot = Math_Atan(
                m_Hair[i].z - m_Hair[i - 1].z, m_Hair[i].x - m_Hair[i - 1].x);
            m_Hair[i - 1].x_rot =
                -Math_Atan(distance, m_Hair[i].y - m_Hair[i - 1].y);

            Matrix_PushUnit();
            g_MatrixPtr->_03 = m_Hair[i - 1].x << W2V_SHIFT;
            g_MatrixPtr->_13 = m_Hair[i - 1].y << W2V_SHIFT;
            g_MatrixPtr->_23 = m_Hair[i - 1].z << W2V_SHIFT;
            Matrix_RotYXZ(m_Hair[i - 1].y_rot, m_Hair[i - 1].x_rot, 0);

            if (i == HAIR_SEGMENTS) {
                Matrix_TranslateRel(*(bone - 3), *(bone - 2), *(bone - 1));
            } else {
                Matrix_TranslateRel(*(bone + 1), *(bone + 2), *(bone + 3));
            }

            m_Hair[i].x = g_MatrixPtr->_03 >> W2V_SHIFT;
            m_Hair[i].y = g_MatrixPtr->_13 >> W2V_SHIFT;
            m_Hair[i].z = g_MatrixPtr->_23 >> W2V_SHIFT;

            m_HVel[i].x = m_Hair[i].x - m_HVel[0].x;
            m_HVel[i].y = m_Hair[i].y - m_HVel[0].y;
            m_HVel[i].z = m_Hair[i].z - m_HVel[0].z;

            Matrix_Pop();
        }
    }
}

void Lara_Hair_Draw(void)
{
    if (!g_Config.enable_braid || !g_Objects[O_HAIR].loaded) {
        return;
    }

    OBJECT_INFO *object = &g_Objects[O_HAIR];
    int16_t **mesh = &g_Meshes[object->mesh_index];

    for (int i = 0; i < HAIR_SEGMENTS; i++) {
        Matrix_Push();

        Matrix_TranslateAbs(m_Hair[i].x, m_Hair[i].y, m_Hair[i].z);
        Matrix_RotY(m_Hair[i].y_rot);
        Matrix_RotX(m_Hair[i].x_rot);
        Output_DrawPolygons(*mesh++, 1);

        Matrix_Pop();
    }
}
