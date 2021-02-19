#include "3dsystem/3d_gen.h"
#include "3dsystem/3d_insert.h"
#include "game/const.h"
#include "game/data.h"
#include "game/draw.h"
#include "specific/game.h"
#include "specific/output.h"
#include "util.h"

void __cdecl PrintRooms(int16_t room_number)
{
    ROOM_INFO* r = &RoomInfo[room_number];
    if (r->flags & UNDERWATER) {
        S_SetupBelowWater(CameraUnderwater);
    } else {
        S_SetupAboveWater(CameraUnderwater);
    }

    r->bound_active = 0;

    phd_PushMatrix();
    phd_TranslateAbs(r->x, r->y, r->z);

    PhdLeft = r->bound_left;
    PhdRight = r->bound_right;
    PhdTop = r->bound_top;
    PhdBottom = r->bound_bottom;

    S_InsertRoom(r->data);

    for (int i = r->item_number; i != NO_ITEM; i = Items[i].next_item) {
        ITEM_INFO* item = &Items[i];
        if (item->status != INVISIBLE) {
            Objects[item->object_number].draw_routine(item);
        }
    }

    for (int i = 0; i < r->num_meshes; i++) {
        MESH_INFO* mesh = &r->mesh[i];
        if (StaticObjects[mesh->static_number].flags & 2) {
            phd_PushMatrix();
            phd_TranslateAbs(mesh->x, mesh->y, mesh->z);
            phd_RotY(mesh->y_rot);
            int clip =
                S_GetObjectBounds(&StaticObjects[mesh->static_number].x_minp);
            if (clip) {
                S_CalculateStaticLight(mesh->shade);
                phd_PutPolygons(
                    Meshes[StaticObjects[mesh->static_number].mesh_number],
                    clip);
            }
            phd_PopMatrix();
        }
    }

    for (int i = r->fx_number; i != NO_ITEM; i = Effects[i].next_fx) {
        DrawEffect(i);
    }

    phd_PopMatrix();

    r->bound_left = PhdWinMaxX;
    r->bound_bottom = 0;
    r->bound_right = 0;
    r->bound_top = PhdWinMaxY;
}

void __cdecl DrawLara(ITEM_INFO* item)
{
    OBJECT_INFO* object;
    int16_t* frame;
    int16_t* frmptr[2];
    PHD_MATRIX saved_matrix;

    int top = PhdTop;
    int left = PhdLeft;
    int bottom = PhdBottom;
    int right = PhdRight;
    PhdBottom = PhdWinMaxY;
    PhdTop = 0;
    PhdLeft = 0;
    PhdRight = PhdWinMaxX;

    if (Lara.hit_direction < 0) {
        int32_t rate;
        int32_t frac = GetFrames(item, frmptr, &rate);
        if (frac) {
            DrawLaraInt(item, frmptr[0], frmptr[1], frac, rate);
            PhdLeft = left;
            PhdRight = right;
            PhdTop = top;
            PhdBottom = bottom;
            return;
        }
    }

    object = &Objects[item->object_number];
    if (Lara.hit_direction >= 0) {
        switch (Lara.hit_direction) {
        case DIR_NORTH:
            frame = Anims[AA_SPAZ_FORWARD].frame_ptr;
            break;
        case DIR_EAST:
            frame = Anims[AA_SPAZ_RIGHT].frame_ptr;
            break;
        case DIR_SOUTH:
            frame = Anims[AA_SPAZ_BACK].frame_ptr;
            break;
        case DIR_WEST:
            frame = Anims[AA_SPAZ_LEFT].frame_ptr;
            break;
        }

        frame += Lara.hit_frames * (object->nmeshes * 2 + FRAME_ROT);
    } else {
        frame = frmptr[0];
    }

    S_PrintShadow(object->shadow_size, frame, item);
    phd_PushMatrix();
    phd_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int32_t clip = S_GetObjectBounds(frame);
    if (!clip) {
        phd_PopMatrix();
        return;
    }

    phd_PushMatrix();

    CalculateObjectLighting(item, frame);

    int32_t* bone = &AnimBones[object->bone_index];
    int32_t* packed_rotation = (int32_t*)(frame + FRAME_ROT);

    phd_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);
    phd_RotYXZpack(packed_rotation[LM_HIPS]);
    phd_PutPolygons(Lara.mesh_ptrs[LM_HIPS], clip);

    phd_PushMatrix();

    phd_TranslateRel(bone[1], bone[2], bone[3]);
    phd_RotYXZpack(packed_rotation[LM_THIGH_L]);
    phd_PutPolygons(Lara.mesh_ptrs[LM_THIGH_L], clip);

    phd_TranslateRel(bone[5], bone[6], bone[7]);
    phd_RotYXZpack(packed_rotation[LM_CALF_L]);
    phd_PutPolygons(Lara.mesh_ptrs[LM_CALF_L], clip);

    phd_TranslateRel(bone[9], bone[10], bone[11]);
    phd_RotYXZpack(packed_rotation[LM_FOOT_L]);
    phd_PutPolygons(Lara.mesh_ptrs[LM_FOOT_L], clip);

    phd_PopMatrix();

    phd_PushMatrix();

    phd_TranslateRel(bone[13], bone[14], bone[15]);
    phd_RotYXZpack(packed_rotation[LM_THIGH_R]);
    phd_PutPolygons(Lara.mesh_ptrs[LM_THIGH_R], clip);

    phd_TranslateRel(bone[17], bone[18], bone[19]);
    phd_RotYXZpack(packed_rotation[LM_CALF_R]);
    phd_PutPolygons(Lara.mesh_ptrs[LM_CALF_R], clip);

    phd_TranslateRel(bone[21], bone[22], bone[23]);
    phd_RotYXZpack(packed_rotation[LM_FOOT_R]);
    phd_PutPolygons(Lara.mesh_ptrs[LM_FOOT_R], clip);

    phd_PopMatrix();

    phd_TranslateRel(bone[25], bone[26], bone[27]);
    phd_RotYXZpack(packed_rotation[LM_TORSO]);
    phd_RotYXZ(Lara.torso_y_rot, Lara.torso_x_rot, Lara.torso_z_rot);
    phd_PutPolygons(Lara.mesh_ptrs[LM_TORSO], clip);

    phd_PushMatrix();

    phd_TranslateRel(bone[53], bone[54], bone[55]);
    phd_RotYXZpack(packed_rotation[LM_HEAD]);
    phd_RotYXZ(Lara.head_y_rot, Lara.head_x_rot, Lara.head_z_rot);
    phd_PutPolygons(Lara.mesh_ptrs[LM_HEAD], clip);

    phd_PopMatrix();

    int fire_arms = 0;
    if (Lara.gun_status == LGS_READY || Lara.gun_status == LGS_DRAW
        || Lara.gun_status == LGS_UNDRAW) {
        fire_arms = Lara.gun_type;
    }

    switch (fire_arms) {
    case LGT_UNARMED:
        phd_PushMatrix();

        phd_TranslateRel(bone[29], bone[30], bone[31]);
        phd_RotYXZpack(packed_rotation[LM_UARM_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel(bone[33], bone[34], bone[35]);
        phd_RotYXZpack(packed_rotation[LM_LARM_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel(bone[37], bone[38], bone[39]);
        phd_RotYXZpack(packed_rotation[LM_HAND_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_HAND_R], clip);

        phd_PopMatrix();

        phd_PushMatrix();

        phd_TranslateRel(bone[41], bone[42], bone[43]);
        phd_RotYXZpack(packed_rotation[LM_UARM_L]);
        phd_PutPolygons(Lara.mesh_ptrs[11], clip);

        phd_TranslateRel(bone[45], bone[46], bone[47]);
        phd_RotYXZpack(packed_rotation[LM_LARM_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel(bone[49], bone[50], bone[51]);
        phd_RotYXZpack(packed_rotation[LM_HAND_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_HAND_L], clip);

        phd_PopMatrix();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        phd_PushMatrix();

        phd_TranslateRel(bone[29], bone[30], bone[31]);

        PhdMatrixPtr->_00 = PhdMatrixPtr[-2]._00;
        PhdMatrixPtr->_01 = PhdMatrixPtr[-2]._01;
        PhdMatrixPtr->_02 = PhdMatrixPtr[-2]._02;
        PhdMatrixPtr->_10 = PhdMatrixPtr[-2]._10;
        PhdMatrixPtr->_11 = PhdMatrixPtr[-2]._11;
        PhdMatrixPtr->_12 = PhdMatrixPtr[-2]._12;
        PhdMatrixPtr->_20 = PhdMatrixPtr[-2]._20;
        PhdMatrixPtr->_21 = PhdMatrixPtr[-2]._21;
        PhdMatrixPtr->_22 = PhdMatrixPtr[-2]._22;

        packed_rotation =
            (int32_t*)(Lara.right_arm.frame_base + Lara.right_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_RotYXZ(
            Lara.right_arm.y_rot, Lara.right_arm.x_rot, Lara.right_arm.z_rot);
        phd_RotYXZpack(packed_rotation[LM_UARM_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel(bone[33], bone[34], bone[35]);
        phd_RotYXZpack(packed_rotation[LM_LARM_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel(bone[37], bone[38], bone[39]);
        phd_RotYXZpack(packed_rotation[LM_HAND_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_HAND_R], clip);

        if (Lara.right_arm.flash_gun) {
            saved_matrix._00 = PhdMatrixPtr->_00;
            saved_matrix._01 = PhdMatrixPtr->_01;
            saved_matrix._02 = PhdMatrixPtr->_02;
            saved_matrix._03 = PhdMatrixPtr->_03;
            saved_matrix._10 = PhdMatrixPtr->_10;
            saved_matrix._11 = PhdMatrixPtr->_11;
            saved_matrix._12 = PhdMatrixPtr->_12;
            saved_matrix._13 = PhdMatrixPtr->_13;
            saved_matrix._20 = PhdMatrixPtr->_20;
            saved_matrix._21 = PhdMatrixPtr->_21;
            saved_matrix._22 = PhdMatrixPtr->_22;
            saved_matrix._23 = PhdMatrixPtr->_23;
        }

        phd_PopMatrix();

        phd_PushMatrix();

        phd_TranslateRel(bone[41], bone[42], bone[43]);

        PhdMatrixPtr->_00 = PhdMatrixPtr[-2]._00;
        PhdMatrixPtr->_01 = PhdMatrixPtr[-2]._01;
        PhdMatrixPtr->_02 = PhdMatrixPtr[-2]._02;
        PhdMatrixPtr->_10 = PhdMatrixPtr[-2]._10;
        PhdMatrixPtr->_11 = PhdMatrixPtr[-2]._11;
        PhdMatrixPtr->_12 = PhdMatrixPtr[-2]._12;
        PhdMatrixPtr->_20 = PhdMatrixPtr[-2]._20;
        PhdMatrixPtr->_21 = PhdMatrixPtr[-2]._21;
        PhdMatrixPtr->_22 = PhdMatrixPtr[-2]._22;

        packed_rotation =
            (int32_t*)(Lara.left_arm.frame_base + Lara.left_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_RotYXZ(
            Lara.left_arm.y_rot, Lara.left_arm.x_rot, Lara.left_arm.z_rot);
        phd_RotYXZpack(packed_rotation[LM_UARM_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_UARM_L], clip);

        phd_TranslateRel(bone[45], bone[46], bone[47]);
        phd_RotYXZpack(packed_rotation[LM_LARM_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel(bone[49], bone[50], bone[51]);
        phd_RotYXZpack(packed_rotation[LM_HAND_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_HAND_L], clip);

        if (Lara.left_arm.flash_gun) {
            DrawGunFlash(fire_arms, clip);
        }
        if (Lara.right_arm.flash_gun) {
            PhdMatrixPtr->_00 = saved_matrix._00;
            PhdMatrixPtr->_01 = saved_matrix._01;
            PhdMatrixPtr->_02 = saved_matrix._02;
            PhdMatrixPtr->_03 = saved_matrix._03;
            PhdMatrixPtr->_10 = saved_matrix._10;
            PhdMatrixPtr->_11 = saved_matrix._11;
            PhdMatrixPtr->_12 = saved_matrix._12;
            PhdMatrixPtr->_13 = saved_matrix._13;
            PhdMatrixPtr->_20 = saved_matrix._20;
            PhdMatrixPtr->_21 = saved_matrix._21;
            PhdMatrixPtr->_22 = saved_matrix._22;
            PhdMatrixPtr->_23 = saved_matrix._23;
            DrawGunFlash(fire_arms, clip);
        }

        phd_PopMatrix();
        break;

    case LGT_SHOTGUN:
        phd_PushMatrix();

        packed_rotation =
            (int32_t*)(Lara.right_arm.frame_base + Lara.right_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_TranslateRel(bone[29], bone[30], bone[31]);
        phd_RotYXZpack(packed_rotation[LM_UARM_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel(bone[33], bone[34], bone[35]);
        phd_RotYXZpack(packed_rotation[LM_LARM_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel(bone[37], bone[38], bone[39]);
        phd_RotYXZpack(packed_rotation[LM_HAND_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_HAND_R], clip);

        phd_PopMatrix();

        phd_PushMatrix();

        packed_rotation =
            (int32_t*)(Lara.left_arm.frame_base + Lara.left_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_TranslateRel(bone[41], bone[42], bone[43]);
        phd_RotYXZpack(packed_rotation[LM_UARM_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_UARM_L], clip);

        phd_TranslateRel(bone[45], bone[46], bone[47]);
        phd_RotYXZpack(packed_rotation[LM_LARM_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel(bone[49], bone[50], bone[51]);
        phd_RotYXZpack(packed_rotation[LM_HAND_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_HAND_L], clip);

        phd_PopMatrix();
        break;
    }

    phd_PopMatrix();
    phd_PopMatrix();
    PhdLeft = left;
    PhdRight = right;
    PhdTop = top;
    PhdBottom = bottom;
}

void __cdecl DrawGunFlash(int32_t weapon_type, int32_t clip)
{
    int light;
    int g_len;

    switch (weapon_type) {
    case LGT_MAGNUMS:
        light = 16 * 256;
        g_len = 155;
        break;

    case LGT_UZIS:
        light = 10 * 256;
        g_len = 180;
        break;

    default:
        light = 20 * 256;
        g_len = 155;
        break;
    }

    phd_TranslateRel(0, g_len, 55);
    phd_RotYXZ(0, -90 * ONE_DEGREE, (PHD_ANGLE)(GetRandomDraw() * 2));
    S_CalculateStaticLight(light);
    phd_PutPolygons(Meshes[Objects[O_GUN_FLASH].mesh_index], clip);
}

void Tomb1MInjectGameDraw()
{
    INJECT(0x004171E0, PrintRooms);
    INJECT(0x00417AA0, DrawLara);
}
