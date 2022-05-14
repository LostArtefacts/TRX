#include "game/draw.h"

#include "config.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/objects.h"
#include "game/output.h"
#include "game/room.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/matrix.h"
#include "util.h"

#include <stdint.h>

void DrawDummyItem(ITEM_INFO *item)
{
}

void DrawPickupItem(ITEM_INFO *item)
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
    item->anim_number = g_Objects[item_num_option].anim_index;
    item->frame_number = g_Anims[item->anim_number].frame_base;

    OBJECT_INFO *object = &g_Objects[item_num_option];

    int16_t *frmptr[2];
    int32_t rate;
    int32_t frac = Item_GetFrames(item, frmptr, &rate);

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
        int16_t min_y = frmptr[0][FRAME_BOUND_MIN_Y];
        int16_t max_y = frmptr[0][FRAME_BOUND_MAX_Y];
        int16_t anim_y = frmptr[0][FRAME_POS_Y];

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
    Matrix_TranslateAbs(item->pos.x, offset, item->pos.z);

    Output_CalculateLight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);

    int16_t *frame = &object->frame_base[object->nmeshes * 2 + 10];
    int32_t clip = Output_GetObjectBounds(frame);
    if (clip) {
        // From this point on the function is a slightly customised version
        // of the code in DrawAnimatingItem starting with the line that
        // matches the following line.
        int32_t bit = 1;
        int16_t **meshpp = &g_Meshes[object->mesh_index];
        int32_t *bone = &g_AnimBones[object->bone_index];

        if (!frac) {
            Matrix_TranslateRel(
                frmptr[0][FRAME_POS_X], frmptr[0][FRAME_POS_Y],
                frmptr[0][FRAME_POS_Z]);

            int32_t *packed_rotation = (int32_t *)(frmptr[0] + FRAME_ROT);
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
        } else {
            // This should never happen but is here "just in case".
            Matrix_InitInterpolate(frac, rate);
            Matrix_TranslateRel_ID(
                frmptr[0][FRAME_POS_X], frmptr[0][FRAME_POS_Y],
                frmptr[0][FRAME_POS_Z], frmptr[1][FRAME_POS_X],
                frmptr[1][FRAME_POS_Y], frmptr[1][FRAME_POS_Z]);
            int32_t *packed_rotation1 = (int32_t *)(frmptr[0] + FRAME_ROT);
            int32_t *packed_rotation2 = (int32_t *)(frmptr[1] + FRAME_ROT);
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

                // Extra rotation is ignored in this case as it's not needed.

                bit <<= 1;
                if (item->mesh_bits & bit) {
                    Output_DrawPolygons_I(*meshpp, clip);
                }

                bone += 4;
                meshpp++;
            }
        }
    }

    Matrix_Pop();
}

void DrawAnimatingItem(ITEM_INFO *item)
{
    static int16_t null_rotation[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    int16_t *frmptr[2];
    int32_t rate;
    int32_t frac = Item_GetFrames(item, frmptr, &rate);
    OBJECT_INFO *object = &g_Objects[item->object_number];

    if (object->shadow_size) {
        Output_DrawShadow(object->shadow_size, frmptr[0], item);
    }

    Matrix_Push();
    Matrix_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    Matrix_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
    int32_t clip = Output_GetObjectBounds(frmptr[0]);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Output_CalculateObjectLighting(item, frmptr[0]);
    int16_t *extra_rotation = item->data ? item->data : &null_rotation;

    int32_t bit = 1;
    int16_t **meshpp = &g_Meshes[object->mesh_index];
    int32_t *bone = &g_AnimBones[object->bone_index];

    if (!frac) {
        Matrix_TranslateRel(
            frmptr[0][FRAME_POS_X], frmptr[0][FRAME_POS_Y],
            frmptr[0][FRAME_POS_Z]);

        int32_t *packed_rotation = (int32_t *)(frmptr[0] + FRAME_ROT);
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
            frmptr[0][FRAME_POS_X], frmptr[0][FRAME_POS_Y],
            frmptr[0][FRAME_POS_Z], frmptr[1][FRAME_POS_X],
            frmptr[1][FRAME_POS_Y], frmptr[1][FRAME_POS_Z]);
        int32_t *packed_rotation1 = (int32_t *)(frmptr[0] + FRAME_ROT);
        int32_t *packed_rotation2 = (int32_t *)(frmptr[1] + FRAME_ROT);
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

void DrawUnclippedItem(ITEM_INFO *item)
{
    int32_t left = g_PhdLeft;
    int32_t top = g_PhdTop;
    int32_t right = g_PhdRight;
    int32_t bottom = g_PhdBottom;

    g_PhdLeft = Viewport_GetMinX();
    g_PhdTop = Viewport_GetMinY();
    g_PhdRight = Viewport_GetMaxX();
    g_PhdBottom = Viewport_GetMaxY();

    DrawAnimatingItem(item);

    g_PhdLeft = left;
    g_PhdTop = top;
    g_PhdRight = right;
    g_PhdBottom = bottom;
}
