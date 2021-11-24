#include "game/savegame.h"

#include "game/ai/pod.h"
#include "game/control.h"
#include "game/gamebuf.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/objects/pickup.h"
#include "game/objects/puzzle_hole.h"
#include "game/traps/movable_block.h"
#include "game/traps/rolling_block.h"
#include "global/const.h"
#include "global/vars.h"
#include "specific/s_shell.h"

#include <stddef.h>

#define SAVE_CREATURE (1 << 7)

static int SGCount = 0;
static char *SGPoint = NULL;

void InitialiseStartInfo()
{
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        ModifyStartInfo(i);
        g_SaveGame.start[i].available = 0;
    }
    g_SaveGame.start[g_GameFlow.gym_level_num].available = 1;
    g_SaveGame.start[g_GameFlow.first_level_num].available = 1;
}

void ModifyStartInfo(int32_t level_num)
{
    START_INFO *start = &g_SaveGame.start[level_num];

    start->got_pistols = 1;
    start->gun_type = LGT_PISTOLS;
    start->pistol_ammo = 1000;

    if (level_num == g_GameFlow.gym_level_num) {
        start->available = 1;
        start->costume = 1;
        start->num_medis = 0;
        start->num_big_medis = 0;
        start->num_scions = 0;
        start->pistol_ammo = 0;
        start->shotgun_ammo = 0;
        start->magnum_ammo = 0;
        start->uzi_ammo = 0;
        start->got_pistols = 0;
        start->got_shotgun = 0;
        start->got_magnums = 0;
        start->got_uzis = 0;
        start->gun_type = LGT_UNARMED;
        start->gun_status = LGS_ARMLESS;
    }

    if (level_num == g_GameFlow.first_level_num) {
        start->available = 1;
        start->costume = 0;
        start->num_medis = 0;
        start->num_big_medis = 0;
        start->num_scions = 0;
        start->shotgun_ammo = 0;
        start->magnum_ammo = 0;
        start->uzi_ammo = 0;
        start->got_shotgun = 0;
        start->got_magnums = 0;
        start->got_uzis = 0;
        start->gun_status = LGS_ARMLESS;
    }

    if ((g_SaveGame.bonus_flag & GBF_NGPLUS)
        && level_num != g_GameFlow.gym_level_num) {
        start->got_pistols = 1;
        start->got_shotgun = 1;
        start->got_magnums = 1;
        start->got_uzis = 1;
        start->shotgun_ammo = 1234;
        start->magnum_ammo = 1234;
        start->uzi_ammo = 1234;
        start->gun_type = LGT_UZIS;
    }
}

void CreateStartInfo(int level_num)
{
    START_INFO *start = &g_SaveGame.start[level_num];

    start->available = 1;
    start->costume = 0;

    start->pistol_ammo = 1000;
    if (Inv_RequestItem(O_GUN_ITEM)) {
        start->got_pistols = 1;
    } else {
        start->got_pistols = 0;
    }

    if (Inv_RequestItem(O_MAGNUM_ITEM)) {
        start->magnum_ammo = g_Lara.magnums.ammo;
        start->got_magnums = 1;
    } else {
        start->magnum_ammo = Inv_RequestItem(O_MAG_AMMO_ITEM) * MAGNUM_AMMO_QTY;
        start->got_magnums = 0;
    }

    if (Inv_RequestItem(O_UZI_ITEM)) {
        start->uzi_ammo = g_Lara.uzis.ammo;
        start->got_uzis = 1;
    } else {
        start->uzi_ammo = Inv_RequestItem(O_UZI_AMMO_ITEM) * UZI_AMMO_QTY;
        start->got_uzis = 0;
    }

    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        start->shotgun_ammo = g_Lara.shotgun.ammo;
        start->got_shotgun = 1;
    } else {
        start->shotgun_ammo =
            Inv_RequestItem(O_SG_AMMO_ITEM) * SHOTGUN_AMMO_QTY;
        start->got_shotgun = 0;
    }

    start->num_medis = Inv_RequestItem(O_MEDI_ITEM);
    start->num_big_medis = Inv_RequestItem(O_BIGMEDI_ITEM);
    start->num_scions = Inv_RequestItem(O_SCION_ITEM);

    start->gun_type = g_Lara.gun_type;
    if (g_Lara.gun_status == LGS_READY) {
        start->gun_status = LGS_READY;
    } else {
        start->gun_status = LGS_ARMLESS;
    }
}

void CreateSaveGameInfo()
{
    g_SaveGame.current_level = g_CurrentLevel;

    CreateStartInfo(g_CurrentLevel);

    g_SaveGame.num_pickup1 = Inv_RequestItem(O_PICKUP_ITEM1);
    g_SaveGame.num_pickup2 = Inv_RequestItem(O_PICKUP_ITEM2);
    g_SaveGame.num_puzzle1 = Inv_RequestItem(O_PUZZLE_ITEM1);
    g_SaveGame.num_puzzle2 = Inv_RequestItem(O_PUZZLE_ITEM2);
    g_SaveGame.num_puzzle3 = Inv_RequestItem(O_PUZZLE_ITEM3);
    g_SaveGame.num_puzzle4 = Inv_RequestItem(O_PUZZLE_ITEM4);
    g_SaveGame.num_key1 = Inv_RequestItem(O_KEY_ITEM1);
    g_SaveGame.num_key2 = Inv_RequestItem(O_KEY_ITEM2);
    g_SaveGame.num_key3 = Inv_RequestItem(O_KEY_ITEM3);
    g_SaveGame.num_key4 = Inv_RequestItem(O_KEY_ITEM4);
    g_SaveGame.num_leadbar = Inv_RequestItem(O_LEADBAR_ITEM);

    ResetSG();

    for (int i = 0; i < MAX_SAVEGAME_BUFFER; i++) {
        SGPoint[i] = 0;
    }

    WriteSG(&g_FlipStatus, sizeof(int32_t));
    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        int8_t flag = g_FlipMapTable[i] >> 8;
        WriteSG(&flag, sizeof(int8_t));
    }

    for (int i = 0; i < g_NumberCameras; i++) {
        WriteSG(&g_Camera.fixed[i].flags, sizeof(int16_t));
    }

    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        if (obj->save_position) {
            WriteSG(&item->pos, sizeof(PHD_3DPOS));
            WriteSG(&item->room_number, sizeof(int16_t));
            WriteSG(&item->speed, sizeof(int16_t));
            WriteSG(&item->fall_speed, sizeof(int16_t));
        }

        if (obj->save_anim) {
            WriteSG(&item->current_anim_state, sizeof(int16_t));
            WriteSG(&item->goal_anim_state, sizeof(int16_t));
            WriteSG(&item->required_anim_state, sizeof(int16_t));
            WriteSG(&item->anim_number, sizeof(int16_t));
            WriteSG(&item->frame_number, sizeof(int16_t));
        }

        if (obj->save_hitpoints) {
            WriteSG(&item->hit_points, sizeof(int16_t));
        }

        if (obj->save_flags) {
            uint16_t flags = item->flags + item->active + (item->status << 1)
                + (item->gravity_status << 3) + (item->collidable << 4);
            if (obj->intelligent && item->data) {
                flags |= SAVE_CREATURE;
            }
            WriteSG(&flags, sizeof(uint16_t));
            WriteSG(&item->timer, sizeof(int16_t));
            if (flags & SAVE_CREATURE) {
                CREATURE_INFO *creature = item->data;
                WriteSG(&creature->head_rotation, sizeof(int16_t));
                WriteSG(&creature->neck_rotation, sizeof(int16_t));
                WriteSG(&creature->maximum_turn, sizeof(int16_t));
                WriteSG(&creature->flags, sizeof(int16_t));
                WriteSG(&creature->mood, sizeof(int32_t));
            }
        }
    }

    WriteSGLara(&g_Lara);

    WriteSG(&g_FlipEffect, sizeof(int32_t));
    WriteSG(&g_FlipTimer, sizeof(int32_t));
}

void ExtractSaveGameInfo()
{
    int8_t tmp8;
    int16_t tmp16;
    int32_t tmp32;

    InitialiseLaraInventory(g_CurrentLevel);

    for (int i = 0; i < g_SaveGame.num_pickup1; i++) {
        Inv_AddItem(O_PICKUP_ITEM1);
    }

    for (int i = 0; i < g_SaveGame.num_pickup2; i++) {
        Inv_AddItem(O_PICKUP_ITEM2);
    }

    for (int i = 0; i < g_SaveGame.num_puzzle1; i++) {
        Inv_AddItem(O_PUZZLE_ITEM1);
    }

    for (int i = 0; i < g_SaveGame.num_puzzle2; i++) {
        Inv_AddItem(O_PUZZLE_ITEM2);
    }

    for (int i = 0; i < g_SaveGame.num_puzzle3; i++) {
        Inv_AddItem(O_PUZZLE_ITEM3);
    }

    for (int i = 0; i < g_SaveGame.num_puzzle4; i++) {
        Inv_AddItem(O_PUZZLE_ITEM4);
    }

    for (int i = 0; i < g_SaveGame.num_key1; i++) {
        Inv_AddItem(O_KEY_ITEM1);
    }

    for (int i = 0; i < g_SaveGame.num_key2; i++) {
        Inv_AddItem(O_KEY_ITEM2);
    }

    for (int i = 0; i < g_SaveGame.num_key3; i++) {
        Inv_AddItem(O_KEY_ITEM3);
    }

    for (int i = 0; i < g_SaveGame.num_key4; i++) {
        Inv_AddItem(O_KEY_ITEM4);
    }

    for (int i = 0; i < g_SaveGame.num_leadbar; i++) {
        Inv_AddItem(O_LEADBAR_ITEM);
    }

    ResetSG();

    ReadSG(&tmp32, sizeof(int32_t));
    if (tmp32) {
        FlipMap();
    }

    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        ReadSG(&tmp8, sizeof(int8_t));
        g_FlipMapTable[i] = tmp8 << 8;
    }

    for (int i = 0; i < g_NumberCameras; i++) {
        ReadSG(&g_Camera.fixed[i].flags, sizeof(int16_t));
    }

    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        if (obj->control == MovableBlockControl) {
            AlterFloorHeight(item, WALL_L);
        }
        if (obj->control == RollingBlockControl) {
            AlterFloorHeight(item, WALL_L * 2);
        }

        if (obj->save_position) {
            ReadSG(&item->pos, sizeof(PHD_3DPOS));
            ReadSG(&tmp16, sizeof(int16_t));
            ReadSG(&item->speed, sizeof(int16_t));
            ReadSG(&item->fall_speed, sizeof(int16_t));

            if (item->room_number != tmp16) {
                ItemNewRoom(i, tmp16);
            }

            if (obj->shadow_size) {
                FLOOR_INFO *floor =
                    GetFloor(item->pos.x, item->pos.y, item->pos.z, &tmp16);
                item->floor =
                    GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
            }
        }

        if (obj->save_anim) {
            ReadSG(&item->current_anim_state, sizeof(int16_t));
            ReadSG(&item->goal_anim_state, sizeof(int16_t));
            ReadSG(&item->required_anim_state, sizeof(int16_t));
            ReadSG(&item->anim_number, sizeof(int16_t));
            ReadSG(&item->frame_number, sizeof(int16_t));
        }

        if (obj->save_hitpoints) {
            ReadSG(&item->hit_points, sizeof(int16_t));
        }

        if (obj->save_flags) {
            ReadSG(&item->flags, sizeof(int16_t));
            ReadSG(&item->timer, sizeof(int16_t));

            if (item->flags & IF_KILLED_ITEM) {
                KillItem(i);
                item->status = IS_DEACTIVATED;
            } else {
                if ((item->flags & 1) && !item->active) {
                    AddActiveItem(i);
                }
                item->status = (item->flags & 6) >> 1;
                if (item->flags & 8) {
                    item->gravity_status = 1;
                }
                if (!(item->flags & 16)) {
                    item->collidable = 0;
                }
            }

            if (item->flags & SAVE_CREATURE) {
                EnableBaddieAI(i, 1);
                CREATURE_INFO *creature = item->data;
                if (creature) {
                    ReadSG(&creature->head_rotation, sizeof(int16_t));
                    ReadSG(&creature->neck_rotation, sizeof(int16_t));
                    ReadSG(&creature->maximum_turn, sizeof(int16_t));
                    ReadSG(&creature->flags, sizeof(int16_t));
                    ReadSG(&creature->mood, sizeof(int32_t));
                } else {
                    SkipSG(4 * 2 + 4);
                }
            } else if (obj->intelligent) {
                item->data = NULL;
            }

            item->flags &= 0xFF00;

            if (obj->collision == PuzzleHoleCollision
                && (item->status == IS_DEACTIVATED
                    || item->status == IS_ACTIVE)) {
                item->object_number += O_PUZZLE_DONE1 - O_PUZZLE_HOLE1;
            }

            if (obj->control == PodControl && item->status == IS_DEACTIVATED) {
                item->mesh_bits = 0x1FF;
                item->collidable = 0;
            }

            if (obj->collision == PickUpCollision
                && item->status == IS_DEACTIVATED) {
                RemoveDrawnItem(i);
            }
        }

        if (obj->control == MovableBlockControl
            && item->status == IS_NOT_ACTIVE) {
            AlterFloorHeight(item, -WALL_L);
        }

        if (obj->control == RollingBlockControl
            && item->current_anim_state != RBS_MOVING) {
            AlterFloorHeight(item, -WALL_L * 2);
        }

        if (item->object_number == O_PIERRE && item->hit_points <= 0
            && (item->flags & IF_ONESHOT)) {
            if (Inv_RequestItem(O_SCION_ITEM) == 1) {
                SpawnItem(item, O_MAGNUM_ITEM);
                SpawnItem(item, O_SCION_ITEM2);
                SpawnItem(item, O_KEY_ITEM1);
            }
            g_MusicTrackFlags[55] |= IF_ONESHOT;
        }

        if (item->object_number == O_MERCENARY1 && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_UZI_ITEM)) {
                SpawnItem(item, O_UZI_ITEM);
            }
        }

        if (item->object_number == O_MERCENARY2 && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
                SpawnItem(item, O_MAGNUM_ITEM);
            }
            g_MusicTrackFlags[52] |= IF_ONESHOT;
        }

        if (item->object_number == O_MERCENARY3 && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
                SpawnItem(item, O_SHOTGUN_ITEM);
            }
            g_MusicTrackFlags[51] |= IF_ONESHOT;
        }

        if (item->object_number == O_LARSON && item->hit_points <= 0) {
            g_MusicTrackFlags[51] |= IF_ONESHOT;
        }
    }

    BOX_NODE *node = g_Lara.LOT.node;
    ReadSGLara(&g_Lara);
    g_Lara.LOT.node = node;
    g_Lara.LOT.target_box = NO_BOX;

    ReadSG(&g_FlipEffect, sizeof(int32_t));
    ReadSG(&g_FlipTimer, sizeof(int32_t));
}

void ResetSG()
{
    SGCount = 0;
    SGPoint = g_SaveGame.buffer;
}

void SkipSG(int size)
{
    SGPoint += size;
    SGCount += size; // missing from OG
}

void WriteSG(void *pointer, int size)
{
    SGCount += size;
    if (SGCount >= MAX_SAVEGAME_BUFFER) {
        S_Shell_ExitSystem("FATAL: Savegame is too big to fit in buffer");
    }

    char *data = (char *)pointer;
    for (int i = 0; i < size; i++) {
        *SGPoint++ = *data++;
    }
}

void WriteSGLara(LARA_INFO *lara)
{
    int32_t tmp32 = 0;

    WriteSG(&lara->item_number, sizeof(int16_t));
    WriteSG(&lara->gun_status, sizeof(int16_t));
    WriteSG(&lara->gun_type, sizeof(int16_t));
    WriteSG(&lara->request_gun_type, sizeof(int16_t));
    WriteSG(&lara->calc_fall_speed, sizeof(int16_t));
    WriteSG(&lara->water_status, sizeof(int16_t));
    WriteSG(&lara->pose_count, sizeof(int16_t));
    WriteSG(&lara->hit_frame, sizeof(int16_t));
    WriteSG(&lara->hit_direction, sizeof(int16_t));
    WriteSG(&lara->air, sizeof(int16_t));
    WriteSG(&lara->dive_count, sizeof(int16_t));
    WriteSG(&lara->death_count, sizeof(int16_t));
    WriteSG(&lara->current_active, sizeof(int16_t));
    WriteSG(&lara->spaz_effect_count, sizeof(int16_t));

    // OG just writes the pointer address (!)
    if (lara->spaz_effect) {
        tmp32 = (size_t)lara->spaz_effect - (size_t)g_Effects;
    }
    WriteSG(&tmp32, sizeof(int32_t));

    WriteSG(&lara->mesh_effects, sizeof(int32_t));

    for (int i = 0; i < LM_NUMBER_OF; i++) {
        tmp32 = (size_t)lara->mesh_ptrs[i] - (size_t)g_MeshBase;
        WriteSG(&tmp32, sizeof(int32_t));
    }

    // OG just writes the pointer address (!) assuming it's a non-existing mesh
    // 16 (!!) which happens to be g_Lara's current target. Just write NULL.
    tmp32 = 0;
    WriteSG(&tmp32, sizeof(int32_t));

    WriteSG(&lara->target_angles[0], sizeof(PHD_ANGLE));
    WriteSG(&lara->target_angles[1], sizeof(PHD_ANGLE));
    WriteSG(&lara->turn_rate, sizeof(int16_t));
    WriteSG(&lara->move_angle, sizeof(int16_t));
    WriteSG(&lara->head_y_rot, sizeof(int16_t));
    WriteSG(&lara->head_x_rot, sizeof(int16_t));
    WriteSG(&lara->head_z_rot, sizeof(int16_t));
    WriteSG(&lara->torso_y_rot, sizeof(int16_t));
    WriteSG(&lara->torso_x_rot, sizeof(int16_t));
    WriteSG(&lara->torso_z_rot, sizeof(int16_t));

    WriteSGARM(&lara->left_arm);
    WriteSGARM(&lara->right_arm);
    WriteSG(&lara->pistols, sizeof(AMMO_INFO));
    WriteSG(&lara->magnums, sizeof(AMMO_INFO));
    WriteSG(&lara->uzis, sizeof(AMMO_INFO));
    WriteSG(&lara->shotgun, sizeof(AMMO_INFO));
    WriteSGLOT(&lara->LOT);
}

void WriteSGARM(LARA_ARM *arm)
{
    int32_t frame_base = (size_t)arm->frame_base - (size_t)g_AnimFrames;
    WriteSG(&frame_base, sizeof(int32_t));
    WriteSG(&arm->frame_number, sizeof(int16_t));
    WriteSG(&arm->lock, sizeof(int16_t));
    WriteSG(&arm->y_rot, sizeof(PHD_ANGLE));
    WriteSG(&arm->x_rot, sizeof(PHD_ANGLE));
    WriteSG(&arm->z_rot, sizeof(PHD_ANGLE));
    WriteSG(&arm->flash_gun, sizeof(int16_t));
}

void WriteSGLOT(LOT_INFO *lot)
{
    // it casually saves a pointer again!
    WriteSG(&lot->node, sizeof(int32_t));

    WriteSG(&lot->head, sizeof(int16_t));
    WriteSG(&lot->tail, sizeof(int16_t));
    WriteSG(&lot->search_number, sizeof(uint16_t));
    WriteSG(&lot->block_mask, sizeof(uint16_t));
    WriteSG(&lot->step, sizeof(int16_t));
    WriteSG(&lot->drop, sizeof(int16_t));
    WriteSG(&lot->fly, sizeof(int16_t));
    WriteSG(&lot->zone_count, sizeof(int16_t));
    WriteSG(&lot->target_box, sizeof(int16_t));
    WriteSG(&lot->required_box, sizeof(int16_t));
    WriteSG(&lot->target, sizeof(PHD_VECTOR));
}

void ReadSG(void *pointer, int size)
{
    SGCount += size;
    char *data = (char *)pointer;
    for (int i = 0; i < size; i++)
        *data++ = *SGPoint++;
}

void ReadSGLara(LARA_INFO *lara)
{
    int32_t tmp32 = 0;

    ReadSG(&lara->item_number, sizeof(int16_t));
    ReadSG(&lara->gun_status, sizeof(int16_t));
    ReadSG(&lara->gun_type, sizeof(int16_t));
    ReadSG(&lara->request_gun_type, sizeof(int16_t));
    ReadSG(&lara->calc_fall_speed, sizeof(int16_t));
    ReadSG(&lara->water_status, sizeof(int16_t));
    ReadSG(&lara->pose_count, sizeof(int16_t));
    ReadSG(&lara->hit_frame, sizeof(int16_t));
    ReadSG(&lara->hit_direction, sizeof(int16_t));
    ReadSG(&lara->air, sizeof(int16_t));
    ReadSG(&lara->dive_count, sizeof(int16_t));
    ReadSG(&lara->death_count, sizeof(int16_t));
    ReadSG(&lara->current_active, sizeof(int16_t));
    ReadSG(&lara->spaz_effect_count, sizeof(int16_t));

    lara->spaz_effect = NULL;
    SkipSG(sizeof(FX_INFO *));

    ReadSG(&lara->mesh_effects, sizeof(int32_t));
    for (int i = 0; i < LM_NUMBER_OF; i++) {
        ReadSG(&tmp32, sizeof(int32_t));
        lara->mesh_ptrs[i] = (int16_t *)((size_t)g_MeshBase + (size_t)tmp32);
    }

    lara->target = NULL;
    SkipSG(sizeof(ITEM_INFO *));

    ReadSG(&lara->target_angles[0], sizeof(PHD_ANGLE));
    ReadSG(&lara->target_angles[1], sizeof(PHD_ANGLE));
    ReadSG(&lara->turn_rate, sizeof(int16_t));
    ReadSG(&lara->move_angle, sizeof(int16_t));
    ReadSG(&lara->head_y_rot, sizeof(int16_t));
    ReadSG(&lara->head_x_rot, sizeof(int16_t));
    ReadSG(&lara->head_z_rot, sizeof(int16_t));
    ReadSG(&lara->torso_y_rot, sizeof(int16_t));
    ReadSG(&lara->torso_x_rot, sizeof(int16_t));
    ReadSG(&lara->torso_z_rot, sizeof(int16_t));

    ReadSGARM(&lara->left_arm);
    ReadSGARM(&lara->right_arm);
    ReadSG(&lara->pistols, sizeof(AMMO_INFO));
    ReadSG(&lara->magnums, sizeof(AMMO_INFO));
    ReadSG(&lara->uzis, sizeof(AMMO_INFO));
    ReadSG(&lara->shotgun, sizeof(AMMO_INFO));
    ReadSGLOT(&lara->LOT);
}

void ReadSGARM(LARA_ARM *arm)
{
    int32_t frame_base;
    ReadSG(&frame_base, sizeof(int32_t));
    arm->frame_base = (int16_t *)((size_t)g_AnimFrames + (size_t)frame_base);

    ReadSG(&arm->frame_number, sizeof(int16_t));
    ReadSG(&arm->lock, sizeof(int16_t));
    ReadSG(&arm->y_rot, sizeof(PHD_ANGLE));
    ReadSG(&arm->x_rot, sizeof(PHD_ANGLE));
    ReadSG(&arm->z_rot, sizeof(PHD_ANGLE));
    ReadSG(&arm->flash_gun, sizeof(int16_t));
}

void ReadSGLOT(LOT_INFO *lot)
{
    lot->node = NULL;
    SkipSG(4);

    ReadSG(&lot->head, sizeof(int16_t));
    ReadSG(&lot->tail, sizeof(int16_t));
    ReadSG(&lot->search_number, sizeof(uint16_t));
    ReadSG(&lot->block_mask, sizeof(uint16_t));
    ReadSG(&lot->step, sizeof(int16_t));
    ReadSG(&lot->drop, sizeof(int16_t));
    ReadSG(&lot->fly, sizeof(int16_t));
    ReadSG(&lot->zone_count, sizeof(int16_t));
    ReadSG(&lot->target_box, sizeof(int16_t));
    ReadSG(&lot->required_box, sizeof(int16_t));
    ReadSG(&lot->target, sizeof(PHD_VECTOR));
}
