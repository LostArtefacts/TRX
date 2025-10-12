#include "game/objects/common.h"

#include "game/inventory.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/output.h>

void Object_DrawSpriteItem(const ITEM *const item)
{
    const RGB_F tint = Output_GetTint();
    Output_DrawSprite(
        item->interp.result.pos.x, item->interp.result.pos.y,
        item->interp.result.pos.z,
        Object_Get(item->object_id)->mesh_idx - item->frame_num,
        item->shade.value_1 < 0 ? SHADE_NEUTRAL : item->shade.value_1, tint);
}

void Object_DrawPickupItem(const ITEM *const item)
{
    if (!g_Config.visuals.enable_3d_pickups) {
        Object_DrawSpriteItem(item);
        return;
    }

    // Convert item to menu display item.
    int16_t item_num_option = Inv_GetItemOption(item->object_id);

    // Save the frame number.
    int16_t old_frame_num = item->frame_num;
    // Modify item to be the anim for inv item and animation 0.
    Item_SwitchToObjAnim((ITEM *)item, 0, 0, item_num_option);
    const OBJECT *const obj = Object_Get(item_num_option);
    const ANIM_FRAME *frame = Item_GetAnim(item)->frame_ptr;
    // Restore the old frame number in case we need to get the sprite again.
    ((ITEM *)item)->frame_num = old_frame_num;

    // Fall back to normal sprite rendering if not found.
    if (obj->mesh_count < 0) {
        Object_DrawSpriteItem(item);
        return;
    }

    // Good news is there is a mesh, we just need to work out where to put it

    // First - Is there floor under the item?
    // This is mostly true, but for example the 4 items in the Obelisk of
    // Khamoon the 4 items are sitting on top of a static mesh which is not
    // floor.
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int16_t floor_height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    // Assume this is our offset.
    int16_t offset = floor_height;
    // Is the floor "just below" the item?
    int16_t floor_mapped_delta = ABS(floor_height - item->pos.y);
    if (floor_mapped_delta > WALL_L / 4 || floor_mapped_delta == 0) {
        // No, now we need to move it a bit.
        // First get the sprite that was to be used,

        const OBJECT *const spr_obj = Object_Get(item->object_id);
        const int16_t spr_num = spr_obj->mesh_idx - item->frame_num;
        const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(spr_num);

        // and get the animation bounding box, which is not the mesh one.
        int16_t min_y = frame->bounds.min.y;
        int16_t max_y = frame->bounds.max.y;
        int16_t anim_y = frame->offset.y;

        // Different objects need different heuristics.
        switch (item_num_option) {
        case O_PISTOL_OPTION:
        case O_SHOTGUN_OPTION:
        case O_MAGNUM_OPTION:
        case O_UZI_OPTION:
        case O_MAGNUM_AMMO_OPTION:
        case O_UZI_AMMO_OPTION:
        case O_EXPLOSIVE_OPTION:
        case O_LEADBAR_OPTION:
        case O_PICKUP_OPTION_1:
        case O_PICKUP_OPTION_2:
        case O_SCION_OPTION:
            // Ignore the sprite and just position based upon the anim.
            offset = item->pos.y + (min_y - anim_y) / 2;
            break;
        case O_SMALL_MEDIPACK_OPTION:
        case O_LARGE_MEDIPACK_OPTION:
        case O_SHOTGUN_AMMO_OPTION:
        case O_PUZZLE_OPTION_1:
        case O_PUZZLE_OPTION_2:
        case O_PUZZLE_OPTION_3:
        case O_PUZZLE_OPTION_4:
        case O_KEY_OPTION_1:
        case O_KEY_OPTION_2:
        case O_KEY_OPTION_3:
        case O_KEY_OPTION_4: {
            // Take the difference from the bottom of the sprite and the bottom
            // of the animation and divide it by 8.
            // 8 was chosen because in testing it positioned objects correctly.
            // Specifically the 4 items in the Obelisk of Khamoon and keys.
            // Some objects have a centred mesh and some have one that is from
            // the bottom, for the centred ones; move up from the
            // bottom is necessary.
            int centred = ABS(min_y + max_y) < 8;
            if (floor_mapped_delta) {
                offset = item->pos.y - ABS(min_y - sprite->y0) / 8;
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
    Matrix_Rot16(item->interp.result.rot);

    Output_CalculateLight(item->pos, room_num);

    frame = obj->frame_base;
    const CLIP clip = Output_CheckBoundsClip(&frame->bounds);
    if (clip != CLIP_NOT_VISIBLE) {
        // From this point on the function is a slightly customised version
        // of the code in DrawAnimatingItem starting with the line that
        // matches the following line.
        int32_t bit = 1;

        Matrix_TranslateRel16(frame->offset);
        Matrix_Rot16(frame->mesh_rots[0]);

        if (item->mesh_bits & bit) {
            Object_DrawMesh(obj->mesh_idx, clip, false);
        }

        for (int i = 1; i < obj->mesh_count; i++) {
            const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
            if (bone->matrix_pop) {
                Matrix_Pop();
            }

            if (bone->matrix_push) {
                Matrix_Push();
            }

            Matrix_TranslateRel32(bone->pos);
            Matrix_Rot16(frame->mesh_rots[i]);

            // Extra rotation is ignored in this case as it's not needed.

            bit <<= 1;
            if (item->mesh_bits & bit) {
                Object_DrawMesh(obj->mesh_idx + i, clip, false);
            }
        }
    }

    Matrix_Pop();
}

void Object_SetMeshReflective(
    const OBJECT_ID obj_id, const int32_t mesh_idx, const bool enabled)
{
    const OBJECT *const obj = Object_Get(obj_id);
    if (!obj->loaded) {
        return;
    }

    OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx + mesh_idx);
    mesh->enable_reflections = enabled;
    for (int32_t i = 0; i < mesh->num_tex_face4s; i++) {
        mesh->tex_face4s[i].enable_reflections = enabled;
    }
    for (int32_t i = 0; i < mesh->num_tex_face3s; i++) {
        mesh->tex_face3s[i].enable_reflections = enabled;
    }
    for (int32_t i = 0; i < mesh->num_flat_face4s; i++) {
        mesh->flat_face4s[i].enable_reflections = enabled;
    }
    for (int32_t i = 0; i < mesh->num_flat_face3s; i++) {
        mesh->flat_face3s[i].enable_reflections = enabled;
    }
    Output_DispatchObjectMeshUpdate(obj->mesh_idx + mesh_idx);
}

void Object_SetReflective(const OBJECT_ID obj_id, const bool enabled)
{
    const OBJECT *const obj = Object_Get(obj_id);
    for (int32_t i = 0; i < obj->mesh_count; i++) {
        Object_SetMeshReflective(obj_id, i, enabled);
    }
}
