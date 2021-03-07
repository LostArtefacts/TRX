#include "3dsystem/3d_gen.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/sphere.h"
#include "game/vars.h"
#include "util.h"

int32_t TestCollision(ITEM_INFO* item, ITEM_INFO* lara_item)
{
    SPHERE slist_baddie[34];
    SPHERE slist_lara[34];

    uint32_t flags = 0;
    int32_t num1 = GetSpheres(item, slist_baddie, 1);
    int32_t num2 = GetSpheres(lara_item, slist_lara, 1);

    for (int i = 0; i < num1; i++) {
        SPHERE* ptr1 = &slist_baddie[i];
        if (ptr1->r <= 0) {
            continue;
        }
        for (int j = 0; j < num2; j++) {
            SPHERE* ptr2 = &slist_lara[j];
            if (ptr2->r <= 0) {
                continue;
            }
            int32_t x = ptr2->x - ptr1->x;
            int32_t y = ptr2->y - ptr1->y;
            int32_t z = ptr2->z - ptr1->z;
            int32_t r = ptr2->r + ptr1->r;
            int32_t d = SQUARE(x) + SQUARE(y) + SQUARE(z);
            int32_t r2 = SQUARE(r);
            if (d < r2) {
                flags |= 1 << i;
                break;
            }
        }
    }

    item->touch_bits = flags;
    return flags;
}

int32_t GetSpheres(ITEM_INFO* item, SPHERE* ptr, int32_t world_space)
{
    static int16_t null_rotation[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    if (!item) {
        return 0;
    }

    int32_t x;
    int32_t y;
    int32_t z;
    if (world_space) {
        x = item->pos.x;
        y = item->pos.y;
        z = item->pos.z;
        phd_PushUnitMatrix();
        PhdMatrixPtr->_03 = 0;
        PhdMatrixPtr->_13 = 0;
        PhdMatrixPtr->_23 = 0;
    } else {
        x = 0;
        y = 0;
        z = 0;
        phd_PushMatrix();
        phd_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    }

    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int16_t* frame = GetBestFrame(item);
    phd_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);

    int32_t* packed_rotation = (int32_t*)(frame + FRAME_ROT);
    phd_RotYXZpack(*packed_rotation++);

    OBJECT_INFO* object = &Objects[item->object_number];
    int16_t** meshpp = &Meshes[object->mesh_index];
    int32_t* bone = &AnimBones[object->bone_index];

    int16_t* objptr = *meshpp++;
    phd_PushMatrix();
    phd_TranslateRel(objptr[0], objptr[1], objptr[2]);
    ptr->x = x + (PhdMatrixPtr->_03 >> W2V_SHIFT);
    ptr->y = y + (PhdMatrixPtr->_13 >> W2V_SHIFT);
    ptr->z = z + (PhdMatrixPtr->_23 >> W2V_SHIFT);
    ptr->r = objptr[3];
    ptr++;
    phd_PopMatrix();

    int16_t* extra_rotation = item->data ? item->data : &null_rotation;
    for (int i = 1; i < object->nmeshes; i++) {
        int32_t bone_extra_flags = bone[0];
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

        objptr = *meshpp++;
        phd_PushMatrix();
        phd_TranslateRel(objptr[0], objptr[1], objptr[2]);
        ptr->x = x + (PhdMatrixPtr->_03 >> W2V_SHIFT);
        ptr->y = y + (PhdMatrixPtr->_13 >> W2V_SHIFT);
        ptr->z = z + (PhdMatrixPtr->_23 >> W2V_SHIFT);
        ptr->r = objptr[3];
        phd_PopMatrix();

        ptr++;
        bone += 4;
    }

    phd_PopMatrix();
    return object->nmeshes;
}

void GetJointAbsPosition(ITEM_INFO* item, PHD_VECTOR* vec, int32_t joint)
{
    OBJECT_INFO* object = &Objects[item->object_number];

    phd_PushUnitMatrix();
    PhdMatrixPtr->_03 = 0;
    PhdMatrixPtr->_13 = 0;
    PhdMatrixPtr->_23 = 0;

    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int16_t* frame = GetBestFrame(item);
    phd_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);

    int32_t* packed_rotation = (int32_t*)(frame + FRAME_ROT);
    phd_RotYXZpack(*packed_rotation++);

    int32_t* bone = &AnimBones[object->bone_index];

    int16_t* extra_rotation = (int16_t*)item->data;
    for (int i = 0; i < joint; i++) {
        int32_t bone_extra_flags = bone[0];
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

        bone += 4;
    }

    phd_TranslateRel(vec->x, vec->y, vec->z);
    vec->x = (PhdMatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
    vec->y = (PhdMatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
    vec->z = (PhdMatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
    phd_PopMatrix();
}

void BaddieBiteEffect(ITEM_INFO* item, BITE_INFO* bite)
{
    PHD_VECTOR pos;
    pos.x = bite->x;
    pos.y = bite->y;
    pos.z = bite->z;
    GetJointAbsPosition(item, &pos, bite->mesh_num);
    DoBloodSplat(
        pos.x, pos.y, pos.z, item->speed, item->pos.y_rot, item->room_number);
}

void T1MInjectGameSphere()
{
    INJECT(0x00439130, TestCollision);
    INJECT(0x00439260, GetSpheres);
    INJECT(0x00439550, GetJointAbsPosition);
    INJECT(0x004396F0, BaddieBiteEffect);
}
