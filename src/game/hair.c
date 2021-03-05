#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/const.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/hair.h"
#include "game/types.h"
#include "game/vars.h"
#include "config.h"
#include "util.h"

#define HAIR_SEGMENTS 6
#define HAIR_OFFSET_X (0) // left-right
#define HAIR_OFFSET_Y (20) // up-down
#define HAIR_OFFSET_Z (-45) // front-back

static int FirstHair;
static PHD_3DPOS Hair[HAIR_SEGMENTS + 1];
static PHD_VECTOR HVel[HAIR_SEGMENTS + 1];

void InitialiseHair()
{
    FirstHair = 1;

    int32_t* bone = &AnimBones[Objects[O_HAIR].bone_index];

    Hair[0].y_rot = 0;
    Hair[0].x_rot = -PHD_90;

    for (int i = 1; i < HAIR_SEGMENTS + 1; i++, bone += 4) {
        Hair[i].x = *(bone + 1);
        Hair[i].y = *(bone + 2);
        Hair[i].z = *(bone + 3);
        Hair[i].x_rot = -PHD_90;
        Hair[i].y_rot = Hair[i].z_rot = 0;
        HVel[i].x = 0;
        HVel[i].y = 0;
        HVel[i].z = 0;
    }
}

void HairControl(int in_cutscene)
{
    if (!T1MConfig.enable_braid || !Objects[O_HAIR].loaded) {
        return;
    }

    OBJECT_INFO* object;
    int32_t *bone, distance;
    int16_t *frame, *objptr, room_number;
    PHD_VECTOR pos;
    FLOOR_INFO* floor;
    int32_t i, water_level, height, size;
    SPHERE sphere[5];
    int32_t j, x, y, z;

    object = &Objects[O_LARA];

    if (Lara.hit_direction >= 0) {
        int16_t spaz;

        switch (Lara.hit_direction) {
        case DIR_NORTH:
            spaz = AA_SPAZ_FORWARD;
            break;

        case DIR_SOUTH:
            spaz = AA_SPAZ_BACK;
            break;

        case DIR_EAST:
            spaz = AA_SPAZ_RIGHT;
            break;

        default:
            spaz = AA_SPAZ_LEFT;
            break;
        }

        frame = Anims[spaz].frame_ptr;
        size = Anims[spaz].interpolation >> 8;

        frame += (int)(Lara.hit_frame * size);
    } else
        frame = GetBestFrame(LaraItem);

    phd_PushUnitMatrix();
    PhdMatrixPtr->_03 = LaraItem->pos.x << W2V_SHIFT;
    PhdMatrixPtr->_13 = LaraItem->pos.y << W2V_SHIFT;
    PhdMatrixPtr->_23 = LaraItem->pos.z << W2V_SHIFT;
    phd_RotYXZ(LaraItem->pos.y_rot, LaraItem->pos.x_rot, LaraItem->pos.z_rot);

    bone = &AnimBones[object->bone_index];

    phd_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);
    int32_t* packed_rotation = (int32_t*)(frame + FRAME_ROT);
    phd_RotYXZpack(packed_rotation[LM_HIPS]);

    // hips
    phd_PushMatrix();
    objptr = Lara.mesh_ptrs[LM_HIPS];
    phd_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
    sphere[0].x = PhdMatrixPtr->_03 >> W2V_SHIFT;
    sphere[0].y = PhdMatrixPtr->_13 >> W2V_SHIFT;
    sphere[0].z = PhdMatrixPtr->_23 >> W2V_SHIFT;
    sphere[0].r = (int32_t) * (objptr + 3);
    phd_PopMatrix();

    // torso
    phd_TranslateRel(*(bone + 1 + 24), *(bone + 2 + 24), *(bone + 3 + 24));
    phd_RotYXZpack(packed_rotation[LM_TORSO]);
    phd_RotYXZ(Lara.torso_y_rot, Lara.torso_x_rot, Lara.torso_z_rot);
    phd_PushMatrix();
    objptr = Lara.mesh_ptrs[LM_TORSO];
    phd_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
    sphere[1].x = PhdMatrixPtr->_03 >> W2V_SHIFT;
    sphere[1].y = PhdMatrixPtr->_13 >> W2V_SHIFT;
    sphere[1].z = PhdMatrixPtr->_23 >> W2V_SHIFT;
    sphere[1].r = (int32_t) * (objptr + 3);
    phd_PopMatrix();

    // right arm
    phd_PushMatrix();
    phd_TranslateRel(*(bone + 1 + 28), *(bone + 2 + 28), *(bone + 3 + 28));
    phd_RotYXZpack(packed_rotation[LM_UARM_R]);
    objptr = Lara.mesh_ptrs[LM_UARM_R];
    phd_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
    sphere[3].x = PhdMatrixPtr->_03 >> W2V_SHIFT;
    sphere[3].y = PhdMatrixPtr->_13 >> W2V_SHIFT;
    sphere[3].z = PhdMatrixPtr->_23 >> W2V_SHIFT;
    sphere[3].r = (int32_t) * (objptr + 3) * 3 / 2;
    phd_PopMatrix();

    // left arm
    phd_PushMatrix();
    phd_TranslateRel(*(bone + 1 + 40), *(bone + 2 + 40), *(bone + 3 + 40));
    phd_RotYXZpack(packed_rotation[LM_UARM_L]);
    objptr = Lara.mesh_ptrs[LM_UARM_L];
    phd_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
    sphere[4].x = PhdMatrixPtr->_03 >> W2V_SHIFT;
    sphere[4].y = PhdMatrixPtr->_13 >> W2V_SHIFT;
    sphere[4].z = PhdMatrixPtr->_23 >> W2V_SHIFT;
    sphere[4].r = (int32_t) * (objptr + 3) * 3 / 2;
    phd_PopMatrix();

    // head
    phd_TranslateRel(*(bone + 1 + 52), *(bone + 2 + 52), *(bone + 3 + 52));
    phd_RotYXZpack(packed_rotation[LM_HEAD]);
    phd_RotYXZ(Lara.head_y_rot, Lara.head_x_rot, Lara.head_z_rot);
    phd_PushMatrix();
    objptr = Lara.mesh_ptrs[LM_HEAD];
    phd_TranslateRel(*objptr, *(objptr + 1), *(objptr + 2));
    sphere[2].x = PhdMatrixPtr->_03 >> W2V_SHIFT;
    sphere[2].y = PhdMatrixPtr->_13 >> W2V_SHIFT;
    sphere[2].z = PhdMatrixPtr->_23 >> W2V_SHIFT;
    sphere[2].r = (int32_t) * (objptr + 3);
    phd_PopMatrix();

    phd_TranslateRel(HAIR_OFFSET_X, HAIR_OFFSET_Y, HAIR_OFFSET_Z);

    pos.x = PhdMatrixPtr->_03 >> W2V_SHIFT;
    pos.y = PhdMatrixPtr->_13 >> W2V_SHIFT;
    pos.z = PhdMatrixPtr->_23 >> W2V_SHIFT;
    phd_PopMatrix();

    bone = &AnimBones[Objects[O_HAIR].bone_index];

    if (FirstHair) {
        FirstHair = 0;

        Hair[0].x = pos.x;
        Hair[0].y = pos.y;
        Hair[0].z = pos.z;

        for (i = 0; i < HAIR_SEGMENTS; i++, bone += 4) {
            phd_PushUnitMatrix();
            PhdMatrixPtr->_03 = Hair[i].x << W2V_SHIFT;
            PhdMatrixPtr->_13 = Hair[i].y << W2V_SHIFT;
            PhdMatrixPtr->_23 = Hair[i].z << W2V_SHIFT;
            phd_RotYXZ(Hair[i].y_rot, Hair[i].x_rot, 0);
            phd_TranslateRel(*(bone + 1), *(bone + 2), *(bone + 3));

            Hair[i + 1].x = PhdMatrixPtr->_03 >> W2V_SHIFT;
            Hair[i + 1].y = PhdMatrixPtr->_13 >> W2V_SHIFT;
            Hair[i + 1].z = PhdMatrixPtr->_23 >> W2V_SHIFT;

            phd_PopMatrix();
        }
    } else {
        Hair[0].x = pos.x;
        Hair[0].y = pos.y;
        Hair[0].z = pos.z;

        room_number = LaraItem->room_number;

        if (in_cutscene)
            water_level = NO_HEIGHT;
        else {
            x = LaraItem->pos.x
                + (frame[FRAME_BOUND_MIN_X] + frame[FRAME_BOUND_MAX_X]) / 2;
            y = LaraItem->pos.y
                + (frame[FRAME_BOUND_MIN_Y] + frame[FRAME_BOUND_MAX_Y]) / 2;
            z = LaraItem->pos.z
                + (frame[FRAME_BOUND_MIN_Z] + frame[FRAME_BOUND_MAX_Z]) / 2;
            water_level = GetWaterHeight(x, y, z, room_number);
        }

        for (i = 1; i < HAIR_SEGMENTS + 1; i++, bone += 4) {
            HVel[0].x = Hair[i].x;
            HVel[0].y = Hair[i].y;
            HVel[0].z = Hair[i].z;

            if (!in_cutscene) {
                floor = GetFloor(Hair[i].x, Hair[i].y, Hair[i].z, &room_number);
                height = GetHeight(floor, Hair[i].x, Hair[i].y, Hair[i].z);
            } else
                height = 32767;

            Hair[i].x += HVel[i].x * 3 / 4;
            Hair[i].y += HVel[i].y * 3 / 4;
            Hair[i].z += HVel[i].z * 3 / 4;

            switch (Lara.water_status) {
            case LWS_ABOVEWATER:
                Hair[i].y += 10;
                if (water_level != NO_HEIGHT && Hair[i].y > water_level)
                    Hair[i].y = water_level;
                else if (Hair[i].y > height) {
                    Hair[i].x = HVel[0].x;
                    Hair[i].z = HVel[0].z;
                }
                break;

            case LWS_UNDERWATER:
            case LWS_SURFACE:
                if (Hair[i].y < water_level) {
                    Hair[i].y = water_level;
                } else if (Hair[i].y > height) {
                    Hair[i].y = height;
                }
                break;
            }

            for (j = 0; j < 5; j++) {
                x = Hair[i].x - sphere[j].x;
                y = Hair[i].y - sphere[j].y;
                z = Hair[i].z - sphere[j].z;

                distance = x * x + y * y + z * z;

                if (distance < SQUARE(sphere[j].r)) {
                    distance = phd_sqrt(distance);

                    if (distance == 0)
                        distance = 1;

                    Hair[i].x = sphere[j].x + x * sphere[j].r / distance;
                    Hair[i].y = sphere[j].y + y * sphere[j].r / distance;
                    Hair[i].z = sphere[j].z + z * sphere[j].r / distance;
                }
            }

            distance = phd_sqrt(
                SQUARE(Hair[i].z - Hair[i - 1].z)
                + SQUARE(Hair[i].x - Hair[i - 1].x));
            Hair[i - 1].y_rot =
                phd_atan(Hair[i].z - Hair[i - 1].z, Hair[i].x - Hair[i - 1].x);
            Hair[i - 1].x_rot = -phd_atan(distance, Hair[i].y - Hair[i - 1].y);

            phd_PushUnitMatrix();
            PhdMatrixPtr->_03 = Hair[i - 1].x << W2V_SHIFT;
            PhdMatrixPtr->_13 = Hair[i - 1].y << W2V_SHIFT;
            PhdMatrixPtr->_23 = Hair[i - 1].z << W2V_SHIFT;
            phd_RotYXZ(Hair[i - 1].y_rot, Hair[i - 1].x_rot, 0);

            if (i == HAIR_SEGMENTS) {
                phd_TranslateRel(*(bone - 3), *(bone - 2), *(bone - 1));
            } else {
                phd_TranslateRel(*(bone + 1), *(bone + 2), *(bone + 3));
            }

            Hair[i].x = PhdMatrixPtr->_03 >> W2V_SHIFT;
            Hair[i].y = PhdMatrixPtr->_13 >> W2V_SHIFT;
            Hair[i].z = PhdMatrixPtr->_23 >> W2V_SHIFT;

            HVel[i].x = Hair[i].x - HVel[0].x;
            HVel[i].y = Hair[i].y - HVel[0].y;
            HVel[i].z = Hair[i].z - HVel[0].z;

            phd_PopMatrix();
        }
    }
}

void DrawHair()
{
    if (!T1MConfig.enable_braid || !Objects[O_HAIR].loaded) {
        return;
    }

    OBJECT_INFO* object = &Objects[O_HAIR];
    int16_t** mesh = &Meshes[object->mesh_index];

    for (int i = 0; i < HAIR_SEGMENTS; i++) {
        phd_PushMatrix();

        phd_TranslateAbs(Hair[i].x, Hair[i].y, Hair[i].z);
        phd_RotY(Hair[i].y_rot);
        phd_RotX(Hair[i].x_rot);
        phd_PutPolygons(*mesh++, 1);

        phd_PopMatrix();
    }
}
