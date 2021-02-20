#include "3dsystem/3d_gen.h"
#include "3dsystem/3d_insert.h"
#include "3dsystem/scalespr.h"
#include "game/const.h"
#include "game/data.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/health.h"
#include "specific/output.h"
#include "mod.h"
#include "util.h"

int32_t __cdecl DrawPhaseCinematic()
{
    S_InitialisePolyList();
    S_ClearScreen();
    CameraUnderwater = 0;
    for (int i = 0; i < RoomsToDrawNum; i++) {
        int32_t room_num = RoomsToDraw[i];
        ROOM_INFO* r = &RoomInfo[room_num];
        r->top = 0;
        r->left = 0;
        r->right = PhdWinMaxX;
        r->bottom = PhdWinMaxY;
        PrintRooms(room_num);
    }
    S_OutputPolyList();
    Camera.number_frames = S_DumpScreen();
    S_AniamteTextures(Camera.number_frames);
    return Camera.number_frames;
}

int32_t __cdecl DrawPhaseGame()
{
    S_InitialisePolyList();
    DrawRooms(Camera.pos.room_number);
    DrawGameInfo();
    S_OutputPolyList();
    Camera.number_frames = S_DumpScreen();
    S_AniamteTextures(Camera.number_frames);
    return Camera.number_frames;
}

void __cdecl DrawRooms(int16_t current_room)
{
    PhdLeft = 0;
    PhdTop = 0;
    PhdRight = PhdWinMaxX;
    PhdBottom = PhdWinMaxY;

    ROOM_INFO* r = &RoomInfo[current_room];
    r->left = PhdLeft;
    r->top = PhdTop;
    r->right = PhdRight;
    r->bottom = PhdBottom;
    r->bound_active = 1;

    RoomsToDrawNum = 0;
    RoomsToDraw[RoomsToDrawNum++] = current_room;

    CameraUnderwater = r->flags & RF_UNDERWATER;

    phd_PushMatrix();
    phd_TranslateAbs(r->x, r->y, r->z);
    if (r->doors) {
        for (int i = 0; i < r->doors->count; i++) {
            DOOR_INFO* door = &r->doors->door[i];
            if (SetRoomBounds(&door->x, door->room_num, r)) {
                GetRoomBounds(door->room_num);
            }
        }
    }
    phd_PopMatrix();
    S_ClearScreen();

    for (int i = 0; i < RoomsToDrawNum; i++) {
        PrintRooms(RoomsToDraw[i]);
    }

    if (Objects[O_LARA].loaded) {
        if (RoomInfo[LaraItem->room_number].flags & RF_UNDERWATER) {
            S_SetupBelowWater(CameraUnderwater);
        } else {
            S_SetupAboveWater(CameraUnderwater);
        }
        DrawLara(LaraItem);
    }
}

void __cdecl GetRoomBounds(int16_t room_num)
{
    ROOM_INFO* r = &RoomInfo[room_num];
    phd_PushMatrix();
    phd_TranslateAbs(r->x, r->y, r->z);
    if (r->doors) {
        for (int i = 0; i < r->doors->count; i++) {
            DOOR_INFO* door = &r->doors->door[i];
            if (SetRoomBounds(&door->x, door->room_num, r)) {
                GetRoomBounds(door->room_num);
            }
        }
    }
    phd_PopMatrix();
}

int32_t __cdecl SetRoomBounds(
    int16_t* objptr, int16_t room_num, ROOM_INFO* parent)
{
    // TODO: the way the game passes the objptr is dangerous and relies on
    // layout of DOOR_INFO

    if ((objptr[0] * (parent->x + objptr[3] - W2VMatrix._03))
            + (objptr[1] * (parent->y + objptr[4] - W2VMatrix._13))
            + (objptr[2] * (parent->z + objptr[5] - W2VMatrix._23))
        >= 0) {
        return 0;
    }

    int32_t left = parent->right;
    int32_t right = parent->left;
    int32_t top = parent->bottom;
    int32_t bottom = parent->top;

    objptr += 3;
    int32_t z_toofar = 0;
    int32_t z_behind = 0;

    const PHD_MATRIX* mptr = PhdMatrixPtr;
    for (int i = 0; i < 4; i++) {
        int32_t xv = mptr->_00 * objptr[0] + mptr->_01 * objptr[1]
            + mptr->_02 * objptr[2] + mptr->_03;
        int32_t yv = mptr->_10 * objptr[0] + mptr->_11 * objptr[1]
            + mptr->_12 * objptr[2] + mptr->_13;
        int32_t zv = mptr->_20 * objptr[0] + mptr->_21 * objptr[1]
            + mptr->_22 * objptr[2] + mptr->_23;
        DoorVBuf[i].xv = xv;
        DoorVBuf[i].yv = yv;
        DoorVBuf[i].zv = zv;
        objptr += 3;

        if (zv > 0) {
            if (zv > PhdFarZ) {
                z_toofar++;
            }

            zv /= PhdPersp;
            int32_t xs, ys;
            if (zv) {
                xs = PhdWinCenterX + xv / zv;
                ys = PhdWinCenterY + yv / zv;
            } else {
                xs = xv >= 0 ? PhdRight : PhdLeft;
                ys = yv >= 0 ? PhdBottom : PhdTop;
            }

            if (xs < left) {
                left = xs;
            }
            if (xs > right) {
                right = xs;
            }
            if (ys < top) {
                top = ys;
            }
            if (ys > bottom) {
                bottom = ys;
            }
        } else {
            z_behind++;
        }
    }

    if (z_behind == 4 || z_toofar == 4) {
        return 0;
    }

    if (z_behind > 0) {
        DOOR_VBUF* dest = &DoorVBuf[0];
        DOOR_VBUF* last = &DoorVBuf[3];
        for (int i = 0; i < 4; i++) {
            if ((dest->zv < 0) ^ (last->zv < 0)) {
                if (dest->xv < 0 && last->xv < 0) {
                    left = 0;
                } else if (dest->xv > 0 && last->xv > 0) {
                    right = PhdWinMaxX;
                } else {
                    left = 0;
                    right = PhdWinMaxX;
                }

                if (dest->yv < 0 && last->yv < 0) {
                    top = 0;
                } else if (dest->yv > 0 && last->yv > 0) {
                    bottom = PhdWinMaxY;
                } else {
                    top = 0;
                    bottom = PhdWinMaxY;
                }
            }

            last = dest;
            dest++;
        }
    }

    if (left < parent->left) {
        left = parent->left;
    }
    if (right > parent->right) {
        right = parent->right;
    }
    if (top < parent->top) {
        top = parent->top;
    }
    if (bottom > parent->bottom) {
        bottom = parent->bottom;
    }

    if (left >= right || top >= bottom) {
        return 0;
    }

    ROOM_INFO* r = &RoomInfo[room_num];
    if (left < r->left) {
        r->left = left;
    }
    if (top < r->top) {
        r->top = top;
    }
    if (right > r->right) {
        r->right = right;
    }
    if (bottom > r->bottom) {
        r->bottom = bottom;
    }

    if (!r->bound_active) {
        RoomsToDraw[RoomsToDrawNum++] = room_num;
        r->bound_active = 1;
    }
    return 1;
}

void __cdecl PrintRooms(int16_t room_number)
{
    ROOM_INFO* r = &RoomInfo[room_number];
    if (r->flags & RF_UNDERWATER) {
        S_SetupBelowWater(CameraUnderwater);
    } else {
        S_SetupAboveWater(CameraUnderwater);
    }

    r->bound_active = 0;

    phd_PushMatrix();
    phd_TranslateAbs(r->x, r->y, r->z);

    PhdLeft = r->left;
    PhdRight = r->right;
    PhdTop = r->top;
    PhdBottom = r->bottom;

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

    r->left = PhdWinMaxX;
    r->bottom = 0;
    r->right = 0;
    r->top = PhdWinMaxY;
}

void __cdecl DrawEffect(int16_t fxnum)
{
    FX_INFO* fx = &Effects[fxnum];
    OBJECT_INFO* object = &Objects[fx->object_number];
    if (!object->loaded) {
        return;
    }

    if (object->nmeshes < 0) {
        S_DrawSprite(
            fx->pos.x, fx->pos.y, fx->pos.z,
            object->mesh_index - fx->frame_number, 4096);
    } else {
        phd_PushMatrix();
        phd_TranslateAbs(fx->pos.x, fx->pos.y, fx->pos.z);
        if (PhdMatrixPtr->_23 > PhdNearZ && PhdMatrixPtr->_23 < PhdFarZ) {
            phd_RotYXZ(fx->pos.y_rot, fx->pos.x_rot, fx->pos.z_rot);
            if (object->nmeshes) {
                S_CalculateStaticLight(fx->shade);
                phd_PutPolygons(Meshes[object->mesh_index], -1);
            } else {
                S_CalculateLight(
                    fx->pos.x, fx->pos.y, fx->pos.z, fx->room_number);
                phd_PutPolygons(Meshes[fx->frame_number], -1);
            }
        }
        phd_PopMatrix();
    }
}

void __cdecl DrawSpriteItem(ITEM_INFO* item)
{
    S_DrawSprite(
        item->pos.x, item->pos.y, item->pos.z,
        Objects[item->object_number].mesh_index - item->frame_number,
        item->shade);
}

void __cdecl DrawDummyItem(ITEM_INFO* item)
{
}

void __cdecl DrawAnimatingItem(ITEM_INFO* item)
{
    static int16_t null_rotation[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    int16_t* frmptr[2];
    int32_t rate;
    int32_t frac = GetFrames(item, frmptr, &rate);
    OBJECT_INFO* object = &Objects[item->object_number];

    if (object->shadow_size) {
        S_PrintShadow(object->shadow_size, frmptr[0], item);
    }

    phd_PushMatrix();
    phd_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
    int32_t clip = S_GetObjectBounds(frmptr[0]);
    if (!clip) {
        phd_PopMatrix();
        return;
    }

    CalculateObjectLighting(item, frmptr[0]);
    int16_t* extra_rotation = item->data ? item->data : &null_rotation;

    int32_t bit = 1;
    int16_t** meshpp = &Meshes[object->mesh_index];
    int32_t* bone = &AnimBones[object->bone_index];

    if (!frac) {
        phd_TranslateRel(
            frmptr[0][FRAME_POS_X], frmptr[0][FRAME_POS_Y],
            frmptr[0][FRAME_POS_Z]);

        int32_t* packed_rotation = (int32_t*)(frmptr[0] + FRAME_ROT);
        phd_RotYXZpack(*packed_rotation++);

        if (item->mesh_bits & bit) {
            phd_PutPolygons(*meshpp++, clip);
        }

        for (int i = 1; i < object->nmeshes; i++) {
            int32_t bone_extra_flags = *bone;
            if (bone_extra_flags & BEB_POP) {
                phd_PopMatrix();
            }

            if (bone_extra_flags & BEB_PUSH) {
                phd_PushMatrix();
            }

            phd_TranslateRel(bone[1], bone[2], bone[3]);
            phd_RotYXZpack(*packed_rotation++);

            if (bone_extra_flags & BEB_ROT_Y) {
                phd_RotY(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_X) {
                phd_RotX(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_Z) {
                phd_RotZ(*extra_rotation++);
            }

            bit <<= 1;
            if (item->mesh_bits & bit) {
                phd_PutPolygons(*meshpp, clip);
            }

            bone += 4;
            meshpp++;
        }
    } else {
        InitInterpolate(frac, rate);
        phd_TranslateRel_ID(
            frmptr[0][FRAME_POS_X], frmptr[0][FRAME_POS_Y],
            frmptr[0][FRAME_POS_Z], frmptr[1][FRAME_POS_X],
            frmptr[1][FRAME_POS_Y], frmptr[1][FRAME_POS_Z]);
        int32_t* packed_rotation1 = (int32_t*)(frmptr[0] + FRAME_ROT);
        int32_t* packed_rotation2 = (int32_t*)(frmptr[1] + FRAME_ROT);
        phd_RotYXZpack_I(*packed_rotation1++, *packed_rotation2++);

        if (item->mesh_bits & bit) {
            phd_PutPolygons_I(*meshpp++, clip);
        }

        for (int i = 1; i < object->nmeshes; i++) {
            int32_t bone_extra_flags = *bone;
            if (bone_extra_flags & BEB_POP) {
                phd_PopMatrix_I();
            }

            if (bone_extra_flags & BEB_PUSH) {
                phd_PushMatrix_I();
            }

            phd_TranslateRel_I(bone[1], bone[2], bone[3]);
            phd_RotYXZpack_I(*packed_rotation1++, *packed_rotation2++);

            if (bone_extra_flags & BEB_ROT_Y) {
                phd_RotY_I(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_X) {
                phd_RotX_I(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_Z) {
                phd_RotZ_I(*extra_rotation++);
            }

            bit <<= 1;
            if (item->mesh_bits & bit) {
                phd_PutPolygons_I(*meshpp, clip);
            }

            bone += 4;
            meshpp++;
        }
    }

    phd_PopMatrix();
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
            saved_matrix = *PhdMatrixPtr;
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
            *PhdMatrixPtr = saved_matrix;
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

#ifdef TOMB1M_FEAT_UI
        if (Lara.right_arm.flash_gun) {
            saved_matrix = *PhdMatrixPtr;
        }
#endif

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

#ifdef TOMB1M_FEAT_UI
        if (Lara.right_arm.flash_gun) {
            *PhdMatrixPtr = saved_matrix;
            DrawGunFlash(fire_arms, clip);
        }
#endif

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
    int32_t light;
    int32_t len;
    int32_t off;

    switch (weapon_type) {
    case LGT_MAGNUMS:
        light = 16 * 256;
        len = 155;
        off = 55;
        break;

    case LGT_UZIS:
        light = 10 * 256;
        len = 180;
        off = 55;
        break;

#ifdef TOMB1M_FEAT_UI
    case LGT_SHOTGUN:
        light = 10 * 256;
        len = 285;
        off = 0;
        break;
#endif

    default:
        light = 20 * 256;
        len = 155;
        off = 55;
        break;
    }

    phd_TranslateRel(0, len, off);
    phd_RotYXZ(0, -90 * ONE_DEGREE, (PHD_ANGLE)(GetRandomDraw() * 2));
    S_CalculateStaticLight(light);
    phd_PutPolygons(Meshes[Objects[O_GUN_FLASH].mesh_index], clip);
}

void __cdecl CalculateObjectLighting(ITEM_INFO* item, int16_t* frame)
{
    if (item->shade >= 0) {
        S_CalculateStaticLight(item->shade);
        return;
    }

    phd_PushUnitMatrix();
    PhdMatrixPtr->_23 = 0;
    PhdMatrixPtr->_13 = 0;
    PhdMatrixPtr->_03 = 0;

    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
    phd_TranslateRel(
        (frame[FRAME_BOUND_MIN_X] + frame[FRAME_BOUND_MAX_X]) / 2,
        (frame[FRAME_BOUND_MIN_Y] + frame[FRAME_BOUND_MAX_Y]) / 2,
        (frame[FRAME_BOUND_MIN_Z] + frame[FRAME_BOUND_MAX_Z]) / 2);

    int32_t x = (PhdMatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
    int32_t y = (PhdMatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
    int32_t z = (PhdMatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;

    phd_PopMatrix();

    S_CalculateLight(x, y, z, item->room_number);
}

void __cdecl DrawLaraInt(
    ITEM_INFO* item, int16_t* frame1, int16_t* frame2, int frac, int rate)
{
    PHD_MATRIX saved_matrix;

    OBJECT_INFO* object = &Objects[item->object_number];
    int16_t* bounds = GetBoundsAccurate(item);

    S_PrintShadow(object->shadow_size, bounds, item);
    phd_PushMatrix();
    phd_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int32_t clip = S_GetObjectBounds(frame1);
    if (!clip) {
        phd_PopMatrix();
        return;
    }

    phd_PushMatrix();

    CalculateObjectLighting(item, frame1);

    int32_t* bone = &AnimBones[object->bone_index];
    int32_t* packed_rotation1 = (int32_t*)(frame1 + FRAME_ROT);
    int32_t* packed_rotation2 = (int32_t*)(frame2 + FRAME_ROT);

    InitInterpolate(frac, rate);

    phd_TranslateRel_ID(
        frame1[FRAME_POS_X], frame1[FRAME_POS_Y], frame1[FRAME_POS_Z],
        frame2[FRAME_POS_X], frame2[FRAME_POS_Y], frame2[FRAME_POS_Z]);

    phd_RotYXZpack_I(packed_rotation1[LM_HIPS], packed_rotation2[LM_HIPS]);
    phd_PutPolygons_I(Lara.mesh_ptrs[LM_HIPS], clip);

    phd_PushMatrix_I();

    phd_TranslateRel_I(bone[1], bone[2], bone[3]);
    phd_RotYXZpack_I(
        packed_rotation1[LM_THIGH_L], packed_rotation2[LM_THIGH_L]);
    phd_PutPolygons_I(Lara.mesh_ptrs[LM_THIGH_L], clip);

    phd_TranslateRel_I(bone[5], bone[6], bone[7]);
    phd_RotYXZpack_I(packed_rotation1[LM_CALF_L], packed_rotation2[LM_CALF_L]);
    phd_PutPolygons_I(Lara.mesh_ptrs[LM_CALF_L], clip);

    phd_TranslateRel_I(bone[9], bone[10], bone[11]);
    phd_RotYXZpack_I(packed_rotation1[LM_FOOT_L], packed_rotation2[LM_FOOT_L]);
    phd_PutPolygons_I(Lara.mesh_ptrs[LM_FOOT_L], clip);

    phd_PopMatrix_I();

    phd_PushMatrix_I();

    phd_TranslateRel_I(bone[13], bone[14], bone[15]);
    phd_RotYXZpack_I(
        packed_rotation1[LM_THIGH_R], packed_rotation2[LM_THIGH_R]);
    phd_PutPolygons_I(Lara.mesh_ptrs[LM_THIGH_R], clip);

    phd_TranslateRel_I(bone[17], bone[18], bone[19]);
    phd_RotYXZpack_I(packed_rotation1[LM_CALF_R], packed_rotation2[LM_CALF_R]);
    phd_PutPolygons_I(Lara.mesh_ptrs[LM_CALF_R], clip);

    phd_TranslateRel_I(bone[21], bone[22], bone[23]);
    phd_RotYXZpack_I(packed_rotation1[LM_FOOT_R], packed_rotation2[LM_FOOT_R]);
    phd_PutPolygons_I(Lara.mesh_ptrs[LM_FOOT_R], clip);

    phd_PopMatrix_I();

    phd_TranslateRel_I(bone[25], bone[26], bone[27]);
    phd_RotYXZpack_I(packed_rotation1[LM_TORSO], packed_rotation2[LM_TORSO]);
    phd_RotYXZ_I(Lara.torso_y_rot, Lara.torso_x_rot, Lara.torso_z_rot);
    phd_PutPolygons_I(Lara.mesh_ptrs[LM_TORSO], clip);

    phd_PushMatrix_I();

    phd_TranslateRel_I(bone[53], bone[54], bone[55]);
    phd_RotYXZpack_I(packed_rotation1[LM_HEAD], packed_rotation2[LM_HEAD]);
    phd_RotYXZ_I(Lara.head_y_rot, Lara.head_x_rot, Lara.head_z_rot);
    phd_PutPolygons_I(Lara.mesh_ptrs[LM_HEAD], clip);

    phd_PopMatrix_I();

    int fire_arms = 0;
    if (Lara.gun_status == LGS_READY || Lara.gun_status == LGS_DRAW
        || Lara.gun_status == LGS_UNDRAW) {
        fire_arms = Lara.gun_type;
    }

    switch (fire_arms) {
    case LGT_UNARMED:
        phd_PushMatrix_I();

        phd_TranslateRel_I(bone[29], bone[30], bone[31]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_UARM_R], packed_rotation2[LM_UARM_R]);
        phd_PutPolygons_I(Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel_I(bone[33], bone[34], bone[35]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_LARM_R], packed_rotation2[LM_LARM_R]);
        phd_PutPolygons_I(Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel_I(bone[37], bone[38], bone[39]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_HAND_R], packed_rotation2[LM_HAND_R]);
        phd_PutPolygons_I(Lara.mesh_ptrs[LM_HAND_R], clip);

        phd_PopMatrix_I();

        phd_PushMatrix_I();

        phd_TranslateRel_I(bone[41], bone[42], bone[43]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_UARM_L], packed_rotation2[LM_UARM_L]);
        phd_PutPolygons_I(Lara.mesh_ptrs[11], clip);

        phd_TranslateRel_I(bone[45], bone[46], bone[47]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_LARM_L], packed_rotation2[LM_LARM_L]);
        phd_PutPolygons_I(Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel_I(bone[49], bone[50], bone[51]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_HAND_L], packed_rotation2[LM_HAND_L]);
        phd_PutPolygons_I(Lara.mesh_ptrs[LM_HAND_L], clip);

        phd_PopMatrix_I();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        phd_PushMatrix_I();

        phd_TranslateRel_I(bone[29], bone[30], bone[31]);
        InterpolateArmMatrix();

        packed_rotation1 =
            (int32_t*)(Lara.right_arm.frame_base + Lara.right_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_RotYXZ(
            Lara.right_arm.y_rot, Lara.right_arm.x_rot, Lara.right_arm.z_rot);
        phd_RotYXZpack(packed_rotation1[LM_UARM_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel(bone[33], bone[34], bone[35]);
        phd_RotYXZpack(packed_rotation1[LM_LARM_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel(bone[37], bone[38], bone[39]);
        phd_RotYXZpack(packed_rotation1[LM_HAND_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_HAND_R], clip);

        if (Lara.right_arm.flash_gun) {
            saved_matrix = *PhdMatrixPtr;
        }

        phd_PopMatrix_I();

        phd_PushMatrix_I();

        phd_TranslateRel_I(bone[41], bone[42], bone[43]);
        InterpolateArmMatrix();

        packed_rotation1 =
            (int32_t*)(Lara.left_arm.frame_base + Lara.left_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_RotYXZ(
            Lara.left_arm.y_rot, Lara.left_arm.x_rot, Lara.left_arm.z_rot);
        phd_RotYXZpack(packed_rotation1[LM_UARM_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_UARM_L], clip);

        phd_TranslateRel(bone[45], bone[46], bone[47]);
        phd_RotYXZpack(packed_rotation1[LM_LARM_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel(bone[49], bone[50], bone[51]);
        phd_RotYXZpack(packed_rotation1[LM_HAND_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_HAND_L], clip);

        if (Lara.left_arm.flash_gun) {
            DrawGunFlash(fire_arms, clip);
        }

        if (Lara.right_arm.flash_gun) {
            *PhdMatrixPtr = saved_matrix;
            DrawGunFlash(fire_arms, clip);
        }

        phd_PopMatrix_I();
        break;

    case LGT_SHOTGUN:
        phd_PushMatrix_I();
        InterpolateMatrix();

        packed_rotation1 =
            (int32_t*)(Lara.right_arm.frame_base + Lara.right_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_TranslateRel(bone[29], bone[30], bone[31]);
        phd_RotYXZpack(packed_rotation1[LM_UARM_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel(bone[33], bone[34], bone[35]);
        phd_RotYXZpack(packed_rotation1[LM_LARM_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel(bone[37], bone[38], bone[39]);
        phd_RotYXZpack(packed_rotation1[LM_HAND_R]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_HAND_R], clip);

#ifdef TOMB1M_FEAT_UI
        if (Lara.right_arm.flash_gun) {
            saved_matrix = *PhdMatrixPtr;
        }
#endif

        phd_PopMatrix();

        phd_PushMatrix();

        packed_rotation1 =
            (int32_t*)(Lara.left_arm.frame_base + Lara.left_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_TranslateRel(bone[41], bone[42], bone[43]);
        phd_RotYXZpack(packed_rotation1[LM_UARM_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_UARM_L], clip);

        phd_TranslateRel(bone[45], bone[46], bone[47]);
        phd_RotYXZpack(packed_rotation1[LM_LARM_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel(bone[49], bone[50], bone[51]);
        phd_RotYXZpack(packed_rotation1[LM_HAND_L]);
        phd_PutPolygons(Lara.mesh_ptrs[LM_HAND_L], clip);

#ifdef TOMB1M_FEAT_UI
        if (Lara.right_arm.flash_gun) {
            *PhdMatrixPtr = saved_matrix;
            DrawGunFlash(fire_arms, clip);
        }
#endif

        phd_PopMatrix_I();
        break;
    }

    phd_PopMatrix();
    phd_PopMatrix();
}

void __cdecl InitInterpolate(int32_t frac, int32_t rate)
{
    IMFrac = frac;
    IMRate = rate;
    IMMatrixPtr = &IMMatrixStack[0];
    IMMatrixPtr->_00 = PhdMatrixPtr->_00;
    IMMatrixPtr->_01 = PhdMatrixPtr->_01;
    IMMatrixPtr->_02 = PhdMatrixPtr->_02;
    IMMatrixPtr->_03 = PhdMatrixPtr->_03;
    IMMatrixPtr->_10 = PhdMatrixPtr->_10;
    IMMatrixPtr->_11 = PhdMatrixPtr->_11;
    IMMatrixPtr->_12 = PhdMatrixPtr->_12;
    IMMatrixPtr->_13 = PhdMatrixPtr->_13;
    IMMatrixPtr->_20 = PhdMatrixPtr->_20;
    IMMatrixPtr->_21 = PhdMatrixPtr->_21;
    IMMatrixPtr->_22 = PhdMatrixPtr->_22;
    IMMatrixPtr->_23 = PhdMatrixPtr->_23;
}

void __cdecl phd_PushMatrix_I()
{
    phd_PushMatrix();
    IMMatrixPtr[1]._00 = IMMatrixPtr[0]._00;
    IMMatrixPtr[1]._01 = IMMatrixPtr[0]._01;
    IMMatrixPtr[1]._02 = IMMatrixPtr[0]._02;
    IMMatrixPtr[1]._03 = IMMatrixPtr[0]._03;
    IMMatrixPtr[1]._10 = IMMatrixPtr[0]._10;
    IMMatrixPtr[1]._11 = IMMatrixPtr[0]._11;
    IMMatrixPtr[1]._12 = IMMatrixPtr[0]._12;
    IMMatrixPtr[1]._13 = IMMatrixPtr[0]._13;
    IMMatrixPtr[1]._20 = IMMatrixPtr[0]._20;
    IMMatrixPtr[1]._21 = IMMatrixPtr[0]._21;
    IMMatrixPtr[1]._22 = IMMatrixPtr[0]._22;
    IMMatrixPtr[1]._23 = IMMatrixPtr[0]._23;
    IMMatrixPtr++;
}

void __cdecl phd_PopMatrix_I()
{
    phd_PopMatrix();
    IMMatrixPtr--;
}

void __cdecl phd_TranslateRel_I(int32_t x, int32_t y, int32_t z)
{
    phd_TranslateRel(x, y, z);
    PHD_MATRIX* old_matrix = PhdMatrixPtr;
    PhdMatrixPtr = IMMatrixPtr;
    phd_TranslateRel(x, y, z);
    PhdMatrixPtr = old_matrix;
}

void __cdecl phd_TranslateRel_ID(
    int32_t x, int32_t y, int32_t z, int32_t x2, int32_t y2, int32_t z2)
{
    phd_TranslateRel(x, y, z);
    PHD_MATRIX* old_matrix = PhdMatrixPtr;
    PhdMatrixPtr = IMMatrixPtr;
    phd_TranslateRel(x2, y2, z2);
    PhdMatrixPtr = old_matrix;
}

void __cdecl phd_RotY_I(PHD_ANGLE ang)
{
    phd_RotY(ang);
    PHD_MATRIX* old_matrix = PhdMatrixPtr;
    PhdMatrixPtr = IMMatrixPtr;
    phd_RotY(ang);
    PhdMatrixPtr = old_matrix;
}

void __cdecl phd_RotX_I(PHD_ANGLE ang)
{
    phd_RotX(ang);
    PHD_MATRIX* old_matrix = PhdMatrixPtr;
    PhdMatrixPtr = IMMatrixPtr;
    phd_RotX(ang);
    PhdMatrixPtr = old_matrix;
}

void __cdecl phd_RotZ_I(PHD_ANGLE ang)
{
    phd_RotZ(ang);
    PHD_MATRIX* old_matrix = PhdMatrixPtr;
    PhdMatrixPtr = IMMatrixPtr;
    phd_RotZ(ang);
    PhdMatrixPtr = old_matrix;
}

void __cdecl phd_RotYXZ_I(PHD_ANGLE y, PHD_ANGLE x, PHD_ANGLE z)
{
    phd_RotYXZ(y, x, z);
    PHD_MATRIX* old_matrix = PhdMatrixPtr;
    PhdMatrixPtr = IMMatrixPtr;
    phd_RotYXZ(y, x, z);
    PhdMatrixPtr = old_matrix;
}

void __cdecl phd_RotYXZpack_I(int32_t r1, int32_t r2)
{
    phd_RotYXZpack(r1);
    PHD_MATRIX* old_matrix = PhdMatrixPtr;
    PhdMatrixPtr = IMMatrixPtr;
    phd_RotYXZpack(r2);
    PhdMatrixPtr = old_matrix;
}

void __cdecl phd_PutPolygons_I(int16_t* ptr, int clip)
{
    phd_PushMatrix();
    InterpolateMatrix();
    phd_PutPolygons(ptr, clip);
    phd_PopMatrix();
}

void __cdecl InterpolateMatrix()
{
    PHD_MATRIX* mptr = PhdMatrixPtr;
    PHD_MATRIX* iptr = IMMatrixPtr;

    if (IMRate == 2) {
        mptr->_00 = (mptr->_00 + iptr->_00) / 2;
        mptr->_01 = (mptr->_01 + iptr->_01) / 2;
        mptr->_02 = (mptr->_02 + iptr->_02) / 2;
        mptr->_03 = (mptr->_03 + iptr->_03) / 2;
        mptr->_10 = (mptr->_10 + iptr->_10) / 2;
        mptr->_11 = (mptr->_11 + iptr->_11) / 2;
        mptr->_12 = (mptr->_12 + iptr->_12) / 2;
        mptr->_13 = (mptr->_13 + iptr->_13) / 2;
        mptr->_20 = (mptr->_20 + iptr->_20) / 2;
        mptr->_21 = (mptr->_21 + iptr->_21) / 2;
        mptr->_22 = (mptr->_22 + iptr->_22) / 2;
        mptr->_23 = (mptr->_23 + iptr->_23) / 2;
    } else {
        mptr->_00 += ((iptr->_00 - mptr->_00) * IMFrac) / IMRate;
        mptr->_01 += ((iptr->_01 - mptr->_01) * IMFrac) / IMRate;
        mptr->_02 += ((iptr->_02 - mptr->_02) * IMFrac) / IMRate;
        mptr->_03 += ((iptr->_03 - mptr->_03) * IMFrac) / IMRate;
        mptr->_10 += ((iptr->_10 - mptr->_10) * IMFrac) / IMRate;
        mptr->_11 += ((iptr->_11 - mptr->_11) * IMFrac) / IMRate;
        mptr->_12 += ((iptr->_12 - mptr->_12) * IMFrac) / IMRate;
        mptr->_13 += ((iptr->_13 - mptr->_13) * IMFrac) / IMRate;
        mptr->_20 += ((iptr->_20 - mptr->_20) * IMFrac) / IMRate;
        mptr->_21 += ((iptr->_21 - mptr->_21) * IMFrac) / IMRate;
        mptr->_22 += ((iptr->_22 - mptr->_22) * IMFrac) / IMRate;
        mptr->_23 += ((iptr->_23 - mptr->_23) * IMFrac) / IMRate;
    }
}

void __cdecl InterpolateArmMatrix()
{
    PHD_MATRIX* mptr = PhdMatrixPtr;
    PHD_MATRIX* iptr = IMMatrixPtr;

    if (IMRate == 2) {
        mptr->_00 = mptr[-2]._00;
        mptr->_01 = mptr[-2]._01;
        mptr->_02 = mptr[-2]._02;
        mptr->_03 = (mptr->_03 + iptr->_03) / 2;
        mptr->_10 = mptr[-2]._10;
        mptr->_11 = mptr[-2]._11;
        mptr->_12 = mptr[-2]._12;
        mptr->_13 = (mptr->_13 + iptr->_13) / 2;
        mptr->_20 = mptr[-2]._20;
        mptr->_21 = mptr[-2]._21;
        mptr->_22 = mptr[-2]._22;
        mptr->_23 = (mptr->_23 + iptr->_23) / 2;
    } else {
        mptr->_00 = mptr[-2]._00;
        mptr->_01 = mptr[-2]._01;
        mptr->_02 = mptr[-2]._02;
        mptr->_03 += ((iptr->_03 - mptr->_03) * IMFrac) / IMRate;
        mptr->_10 = mptr[-2]._10;
        mptr->_11 = mptr[-2]._11;
        mptr->_12 = mptr[-2]._12;
        mptr->_13 += ((iptr->_13 - mptr->_13) * IMFrac) / IMRate;
        mptr->_20 = mptr[-2]._20;
        mptr->_21 = mptr[-2]._21;
        mptr->_22 = mptr[-2]._22;
        mptr->_23 += ((iptr->_23 - mptr->_23) * IMFrac) / IMRate;
    }
}

int32_t __cdecl GetFrames(ITEM_INFO* item, int16_t* frmptr[], int32_t* rate)
{
    ANIM_STRUCT* anim = &Anims[item->anim_number];
    frmptr[0] = anim->frame_ptr;
    frmptr[1] = anim->frame_ptr;

    *rate = anim->interpolation;

    int32_t frm = item->frame_number - anim->frame_base;
    int32_t first = frm / anim->interpolation;
    int32_t frame_size = Objects[item->object_number].nmeshes * 2 + 10;

    frmptr[0] += first * frame_size;
    frmptr[1] = frmptr[0] + frame_size;

    int32_t interp = frm % anim->interpolation;
    if (!interp) {
        return 0;
    }

    int32_t second = anim->interpolation * (first + 1);
    if (second > anim->frame_end) {
        *rate = anim->frame_end + anim->interpolation - second;
    }

    return interp;
}

int16_t* __cdecl GetBoundsAccurate(ITEM_INFO* item)
{
    int32_t rate;
    int16_t* frmptr[2];

    int32_t frac = GetFrames(item, frmptr, &rate);
    if (!frac) {
        return frmptr[0];
    }

    for (int i = 0; i < 6; i++) {
        int16_t a = frmptr[0][i];
        int16_t b = frmptr[1][i];
        InterpolatedBounds[i] = a + (((b - a) * frac) / rate);
    }
    return InterpolatedBounds;
}

int16_t* __cdecl GetBestFrame(ITEM_INFO* item)
{
    int16_t* frmptr[2];
    int32_t rate;
    int32_t frac = GetFrames(item, frmptr, &rate);
    if (frac <= rate / 2) {
        return frmptr[0];
    } else {
        return frmptr[1];
    }
}

void Tomb1MInjectGameDraw()
{
    INJECT(0x00416BE0, DrawPhaseCinematic);
    INJECT(0x00416C70, DrawPhaseGame);
    INJECT(0x00416CB0, DrawRooms);
    INJECT(0x00416E30, GetRoomBounds);
    INJECT(0x00416EB0, SetRoomBounds);
    INJECT(0x004171E0, PrintRooms);
    INJECT(0x00417400, DrawEffect);
    INJECT(0x00417510, DrawSpriteItem);
    INJECT(0x00417550, DrawAnimatingItem);
    INJECT(0x00417AA0, DrawLara);
    INJECT(0x004185B0, CalculateObjectLighting);
    INJECT(0x00418680, DrawLaraInt);
    INJECT(0x00419A60, InterpolateMatrix);
    INJECT(0x00419C30, InterpolateArmMatrix);
    INJECT(0x00419D30, GetFrames);
    INJECT(0x00419DD0, GetBoundsAccurate);
    INJECT(0x00419E50, GetBestFrame);
}
