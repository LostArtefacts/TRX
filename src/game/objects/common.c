#include "game/objects/common.h"

#include "config.h"
#include "game/collide.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/output.h"
#include "game/room.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/matrix.h"
#include "util.h"

const GAME_OBJECT_ID g_EnemyObjects[] = {
    O_WOLF,    O_BEAR,     O_BAT,      O_CROCODILE, O_ALLIGATOR, O_LION,
    O_LIONESS, O_PUMA,     O_APE,      O_RAT,       O_VOLE,      O_TREX,
    O_RAPTOR,  O_WARRIOR1, O_WARRIOR2, O_WARRIOR3,  O_CENTAUR,   O_MUMMY,
    O_LARSON,  O_PIERRE,   O_SKATEKID, O_COWBOY,    O_BALDY,     O_NATLA,
    O_TORSO,   NO_OBJECT,
};

const GAME_OBJECT_ID g_PlaceholderObjects[] = {
    O_STATUE,
    O_PODS,
    O_BIG_POD,
    NO_OBJECT,
};

const GAME_OBJECT_ID g_PickupObjects[] = {
    O_GUN_ITEM,     O_SHOTGUN_ITEM,  O_MAGNUM_ITEM,   O_UZI_ITEM,
    O_SG_AMMO_ITEM, O_MAG_AMMO_ITEM, O_UZI_AMMO_ITEM, O_MEDI_ITEM,
    O_BIGMEDI_ITEM, O_PUZZLE_ITEM1,  O_PUZZLE_ITEM2,  O_PUZZLE_ITEM3,
    O_PUZZLE_ITEM4, O_KEY_ITEM1,     O_KEY_ITEM2,     O_KEY_ITEM3,
    O_KEY_ITEM4,    O_PICKUP_ITEM1,  O_PICKUP_ITEM2,  O_LEADBAR_ITEM,
    O_SCION_ITEM2,  NO_OBJECT,
};

const GAME_OBJECT_ID g_GunObjects[] = {
    O_SHOTGUN_ITEM,
    O_MAGNUM_ITEM,
    O_UZI_ITEM,
    NO_OBJECT,
};

const GAME_OBJECT_PAIR g_GunAmmoObjectMap[] = {
    { O_SHOTGUN_ITEM, O_SG_AMMO_ITEM },
    { O_MAGNUM_ITEM, O_MAG_AMMO_ITEM },
    { O_UZI_ITEM, O_UZI_AMMO_ITEM },
    { NO_OBJECT, NO_OBJECT },
};

bool Object_IsObjectType(
    GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr)
{
    for (int i = 0; test_arr[i] != NO_OBJECT; i++) {
        if (test_arr[i] == object_id) {
            return true;
        }
    }
    return false;
}

void Object_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, 0, 1);
    }
}

void Object_CollisionTrap(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_ACTIVE) {
        if (Lara_TestBoundsCollide(item, coll->radius)) {
            Collide_TestCollision(item, lara_item);
        }
    } else if (item->status != IS_INVISIBLE) {
        Object_Collision(item_num, lara_item, coll);
    }
}

void Object_DrawDummyItem(ITEM_INFO *item)
{
}

void Object_DrawSpriteItem(ITEM_INFO *item)
{
    Output_DrawSprite(
        item->pos.x, item->pos.y, item->pos.z,
        g_Objects[item->object_number].mesh_index - item->frame_number,
        item->shade);
}

void Object_DrawPickupItem(ITEM_INFO *item)
{
    if (!g_Config.enable_3d_pickups) {
        Object_DrawSpriteItem(item);
        return;
    }

    // Convert item to menu display item.
    int16_t item_num_option = Inv_GetItemOption(item->object_number);
    // Save the frame number.
    int16_t old_frame_number = item->frame_number;
    // Modify item to be the anim for inv item and animation 0.
    Item_SwitchToObjAnim(item, 0, 0, item_num_option);

    OBJECT_INFO *object = &g_Objects[item_num_option];

    const FRAME_INFO *frame = g_Anims[item->anim_number].frame_ptr;

    // Restore the old frame number in case we need to get the sprite again.
    item->frame_number = old_frame_number;

    // Fall back to normal sprite rendering if not found.
    if (object->nmeshes < 0) {
        Object_DrawSpriteItem(item);
        return;
    }

    // Good news is there is a mesh, we just need to work out where to put it

    // First - Is there floor under the item?
    // This is mostly true, but for example the 4 items in the Obelisk of
    // Khamoon the 4 items are sitting on top of a static mesh which is not
    // floor.
    FLOOR_INFO *floor = Room_GetFloor(
        item->pos.x, item->pos.y, item->pos.z, &item->room_number);
    int16_t floor_height =
        Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

    // Assume this is our offset.
    int16_t offset = floor_height;
    // Is the floor "just below" the item?
    int16_t floor_mapped_delta = ABS(floor_height - item->pos.y);
    if (floor_mapped_delta > WALL_L / 4 || floor_mapped_delta == 0) {
        // No, now we need to move it a bit.
        // First get the sprite that was to be used,

        int16_t spr_num =
            g_Objects[item->object_number].mesh_index - item->frame_number;
        PHD_SPRITE *sprite = &g_PhdSpriteInfo[spr_num];

        // and get the animation bounding box, which is not the mesh one.
        int16_t min_y = frame->bounds.min.y;
        int16_t max_y = frame->bounds.max.y;
        int16_t anim_y = frame->offset.y;

        // Different objects need different heuristics.
        switch (item_num_option) {
        case O_GUN_OPTION:
        case O_SHOTGUN_OPTION:
        case O_MAGNUM_OPTION:
        case O_UZI_OPTION:
        case O_MAG_AMMO_OPTION:
        case O_UZI_AMMO_OPTION:
        case O_EXPLOSIVE_OPTION:
        case O_LEADBAR_OPTION:
        case O_PICKUP_OPTION1:
        case O_PICKUP_OPTION2:
        case O_SCION_OPTION:
            // Ignore the sprite and just position based upon the anim.
            offset = item->pos.y + (min_y - anim_y) / 2;
            break;
        case O_MEDI_OPTION:
        case O_BIGMEDI_OPTION:
        case O_SG_AMMO_OPTION:
        case O_PUZZLE_OPTION1:
        case O_PUZZLE_OPTION2:
        case O_PUZZLE_OPTION3:
        case O_PUZZLE_OPTION4:
        case O_KEY_OPTION1:
        case O_KEY_OPTION2:
        case O_KEY_OPTION3:
        case O_KEY_OPTION4: {
            // Take the difference from the bottom of the sprite and the bottom
            // of the animation and divide it by 8.
            // 8 was chosen because in testing it positioned objects correctly.
            // Specifically the 4 items in the Obelisk of Khamoon and keys.
            // Some objects have a centred mesh and some have one that is from
            // the bottom, for the centred ones; move up from the
            // bottom is necessary.
            int centred = ABS(min_y + max_y) < 8;
            if (floor_mapped_delta) {
                offset = item->pos.y - ABS(min_y - sprite->y1) / 8;
            } else if (centred) {
                offset = item->pos.y + min_y;
            }
            break;
        }
        }
    }

    Matrix_Push();
    Matrix_TranslateAbs(
        item->interp.result.pos.x, offset, item->interp.result.pos.z);
    Matrix_RotYXZ(
        item->interp.result.rot.y, item->interp.result.rot.x,
        item->interp.result.rot.z);

    Output_CalculateLight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);

    frame = object->frame_base;
    int32_t clip = Output_GetObjectBounds(&frame->bounds);
    if (clip) {
        // From this point on the function is a slightly customised version
        // of the code in DrawAnimatingItem starting with the line that
        // matches the following line.
        int32_t bit = 1;
        int16_t **meshpp = &g_Meshes[object->mesh_index];
        int32_t *bone = &g_AnimBones[object->bone_index];

        Matrix_TranslateRel(frame->offset.x, frame->offset.y, frame->offset.z);

        int32_t *packed_rotation = frame->mesh_rots;
        Matrix_RotYXZpack(*packed_rotation++);

        if (item->mesh_bits & bit) {
            Output_DrawPolygons(*meshpp++, clip);
        }

        for (int i = 1; i < object->nmeshes; i++) {
            int32_t bone_extra_flags = *bone;
            if (bone_extra_flags & BEB_POP) {
                Matrix_Pop();
            }

            if (bone_extra_flags & BEB_PUSH) {
                Matrix_Push();
            }

            Matrix_TranslateRel(bone[1], bone[2], bone[3]);
            Matrix_RotYXZpack(*packed_rotation++);

            // Extra rotation is ignored in this case as it's not needed.

            bit <<= 1;
            if (item->mesh_bits & bit) {
                Output_DrawPolygons(*meshpp, clip);
            }

            bone += 4;
            meshpp++;
        }
    }

    Matrix_Pop();
}

void Object_DrawAnimatingItem(ITEM_INFO *item)
{
    static int16_t null_rotation[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    FRAME_INFO *frmptr[2];
    int32_t rate;
    int32_t frac = Item_GetFrames(item, frmptr, &rate);
    OBJECT_INFO *object = &g_Objects[item->object_number];

    if (object->shadow_size) {
        Output_DrawShadow(object->shadow_size, &frmptr[0]->bounds, item);
    }

    Matrix_Push();
    Matrix_TranslateAbs(
        item->interp.result.pos.x, item->interp.result.pos.y,
        item->interp.result.pos.z);
    Matrix_RotYXZ(
        item->interp.result.rot.y, item->interp.result.rot.x,
        item->interp.result.rot.z);

    int32_t clip = Output_GetObjectBounds(&frmptr[0]->bounds);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Output_CalculateObjectLighting(item, &frmptr[0]->bounds);
    int16_t *extra_rotation = item->data ? item->data : &null_rotation;

    int32_t bit = 1;
    int16_t **meshpp = &g_Meshes[object->mesh_index];
    int32_t *bone = &g_AnimBones[object->bone_index];

    if (!frac) {
        Matrix_TranslateRel(
            frmptr[0]->offset.x, frmptr[0]->offset.y, frmptr[0]->offset.z);

        int32_t *packed_rotation = frmptr[0]->mesh_rots;
        Matrix_RotYXZpack(*packed_rotation++);

        if (item->mesh_bits & bit) {
            Output_DrawPolygons(*meshpp++, clip);
        }

        for (int i = 1; i < object->nmeshes; i++) {
            int32_t bone_extra_flags = *bone;
            if (bone_extra_flags & BEB_POP) {
                Matrix_Pop();
            }

            if (bone_extra_flags & BEB_PUSH) {
                Matrix_Push();
            }

            Matrix_TranslateRel(bone[1], bone[2], bone[3]);
            Matrix_RotYXZpack(*packed_rotation++);

            if (bone_extra_flags & BEB_ROT_Y) {
                Matrix_RotY(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_X) {
                Matrix_RotX(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_Z) {
                Matrix_RotZ(*extra_rotation++);
            }

            bit <<= 1;
            if (item->mesh_bits & bit) {
                Output_DrawPolygons(*meshpp, clip);
            }

            bone += 4;
            meshpp++;
        }
    } else {
        Matrix_InitInterpolate(frac, rate);
        Matrix_TranslateRel_ID(
            frmptr[0]->offset.x, frmptr[0]->offset.y, frmptr[0]->offset.z,
            frmptr[1]->offset.x, frmptr[1]->offset.y, frmptr[1]->offset.z);
        int32_t *packed_rotation1 = frmptr[0]->mesh_rots;
        int32_t *packed_rotation2 = frmptr[1]->mesh_rots;
        Matrix_RotYXZpack_I(*packed_rotation1++, *packed_rotation2++);

        if (item->mesh_bits & bit) {
            Output_DrawPolygons_I(*meshpp++, clip);
        }

        for (int i = 1; i < object->nmeshes; i++) {
            int32_t bone_extra_flags = *bone;
            if (bone_extra_flags & BEB_POP) {
                Matrix_Pop_I();
            }

            if (bone_extra_flags & BEB_PUSH) {
                Matrix_Push_I();
            }

            Matrix_TranslateRel_I(bone[1], bone[2], bone[3]);
            Matrix_RotYXZpack_I(*packed_rotation1++, *packed_rotation2++);

            if (bone_extra_flags & BEB_ROT_Y) {
                Matrix_RotY_I(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_X) {
                Matrix_RotX_I(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_Z) {
                Matrix_RotZ_I(*extra_rotation++);
            }

            bit <<= 1;
            if (item->mesh_bits & bit) {
                Output_DrawPolygons_I(*meshpp, clip);
            }

            bone += 4;
            meshpp++;
        }
    }

    Matrix_Pop();
}

void Object_DrawUnclippedItem(ITEM_INFO *item)
{
    int32_t left = g_PhdLeft;
    int32_t top = g_PhdTop;
    int32_t right = g_PhdRight;
    int32_t bottom = g_PhdBottom;

    g_PhdLeft = Viewport_GetMinX();
    g_PhdTop = Viewport_GetMinY();
    g_PhdRight = Viewport_GetMaxX();
    g_PhdBottom = Viewport_GetMaxY();

    Object_DrawAnimatingItem(item);

    g_PhdLeft = left;
    g_PhdTop = top;
    g_PhdRight = right;
    g_PhdBottom = bottom;
}
