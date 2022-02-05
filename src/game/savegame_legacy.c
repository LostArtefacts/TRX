#include "game/savegame_legacy.h"

#include "game/ai/pod.h"
#include "game/control.h"
#include "game/gameflow.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/objects/pickup.h"
#include "game/objects/puzzle_hole.h"
#include "game/shell.h"
#include "game/traps/movable_block.h"
#include "game/traps/rolling_block.h"
#include "global/vars.h"
#include "log.h"
#include "memory.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SAVE_CREATURE (1 << 7)
#define SAVEGAME_LEGACY_TITLE_SIZE 75

typedef struct SAVEGAME_LEGACY_ITEM_STATS {
    uint8_t num_pickup1;
    uint8_t num_pickup2;
    uint8_t num_puzzle1;
    uint8_t num_puzzle2;
    uint8_t num_puzzle3;
    uint8_t num_puzzle4;
    uint8_t num_key1;
    uint8_t num_key2;
    uint8_t num_key3;
    uint8_t num_key4;
    uint8_t num_leadbar;
    uint8_t dummy;
} SAVEGAME_LEGACY_ITEM_STATS;

static int m_SGBufPos = 0;
static char *m_SGBufPtr = NULL;

static bool SaveGame_Legacy_NeedsEvilLaraFix();

static void SaveGame_Legacy_Reset(GAME_INFO *game_info);
static void SaveGame_Legacy_Skip(int size);

static void SaveGame_Legacy_Read(void *pointer, int size);
static void SaveGame_Legacy_ReadArm(LARA_ARM *arm);
static void SaveGame_Legacy_ReadLara(LARA_INFO *lara);
static void SaveGame_Legacy_ReadLOT(LOT_INFO *lot);

static void SaveGame_Legacy_Write(void *pointer, int size);
static void SaveGame_Legacy_WriteArm(LARA_ARM *arm);
static void SaveGame_Legacy_WriteLara(LARA_INFO *lara);
static void SaveGame_Legacy_WriteLOT(LOT_INFO *lot);

static bool SaveGame_Legacy_NeedsEvilLaraFix(GAME_INFO *game_info)
{
    // Heuristic for issue #261.
    // Tomb1Main enables save_flags for Evil Lara, but OG TombATI does not. As
    // a consequence, Atlantis saves made with OG TombATI (which includes the
    // ones available for download on Stella's website) have different layout
    // than the saves made with Tomb1Main. This was discovered after it was too
    // late to make a backwards incompatible change. At the same time, enabling
    // save_flags for Evil Lara is desirable, as not doing this causes her to
    // freeze when the player reloads a save made in her room. This function is
    // used to determine whether the save about to be loaded includes
    // save_flags for Evil Lara or not. Since savegames only contain very
    // concise information, we must make an educated guess here.

    assert(game_info);

    bool result = false;
    if (g_CurrentLevel != 14) {
        return result;
    }

    SaveGame_Legacy_Reset(game_info);
    SaveGame_Legacy_Skip(SAVEGAME_LEGACY_TITLE_SIZE); // level title
    SaveGame_Legacy_Skip(sizeof(int32_t)); // save counter
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        SaveGame_Legacy_Skip(sizeof(uint16_t)); // pistol ammo
        SaveGame_Legacy_Skip(sizeof(uint16_t)); // magnum ammo
        SaveGame_Legacy_Skip(sizeof(uint16_t)); // uzi ammo
        SaveGame_Legacy_Skip(sizeof(uint16_t)); // shotgun ammo
        SaveGame_Legacy_Skip(sizeof(uint8_t)); // small medis
        SaveGame_Legacy_Skip(sizeof(uint8_t)); // big medis
        SaveGame_Legacy_Skip(sizeof(uint8_t)); // scions
        SaveGame_Legacy_Skip(sizeof(int8_t)); // gun status
        SaveGame_Legacy_Skip(sizeof(int8_t)); // gun type
        SaveGame_Legacy_Skip(sizeof(uint16_t)); // flags
    }
    SaveGame_Legacy_Skip(sizeof(uint32_t)); // timer
    SaveGame_Legacy_Skip(sizeof(uint32_t)); // kills
    SaveGame_Legacy_Skip(sizeof(uint16_t)); // secrets
    SaveGame_Legacy_Skip(sizeof(uint16_t)); // current level
    SaveGame_Legacy_Skip(sizeof(uint8_t)); // pickups
    SaveGame_Legacy_Skip(sizeof(uint8_t)); // bonus_flag
    SaveGame_Legacy_Skip(sizeof(SAVEGAME_LEGACY_ITEM_STATS)); // item stats
    SaveGame_Legacy_Skip(sizeof(int32_t)); // flipmap status
    SaveGame_Legacy_Skip(MAX_FLIP_MAPS * sizeof(int8_t)); // flipmap table
    SaveGame_Legacy_Skip(g_NumberCameras * sizeof(int16_t)); // cameras

    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        ITEM_INFO tmp_item;

        if (obj->save_position) {
            SaveGame_Legacy_Read(&tmp_item.pos, sizeof(PHD_3DPOS));
            SaveGame_Legacy_Skip(sizeof(int16_t));
            SaveGame_Legacy_Read(&tmp_item.speed, sizeof(int16_t));
            SaveGame_Legacy_Read(&tmp_item.fall_speed, sizeof(int16_t));
        }
        if (obj->save_anim) {
            SaveGame_Legacy_Read(&tmp_item.current_anim_state, sizeof(int16_t));
            SaveGame_Legacy_Read(&tmp_item.goal_anim_state, sizeof(int16_t));
            SaveGame_Legacy_Read(
                &tmp_item.required_anim_state, sizeof(int16_t));
            SaveGame_Legacy_Read(&tmp_item.anim_number, sizeof(int16_t));
            SaveGame_Legacy_Read(&tmp_item.frame_number, sizeof(int16_t));
        }
        if (obj->save_hitpoints) {
            SaveGame_Legacy_Read(&tmp_item.hit_points, sizeof(int16_t));
        }
        if (obj->save_flags) {
            SaveGame_Legacy_Read(&tmp_item.flags, sizeof(int16_t));
            SaveGame_Legacy_Read(&tmp_item.timer, sizeof(int16_t));
            if (tmp_item.flags & SAVE_CREATURE) {
                CREATURE_INFO tmp_creature;
                SaveGame_Legacy_Read(
                    &tmp_creature.head_rotation, sizeof(int16_t));
                SaveGame_Legacy_Read(
                    &tmp_creature.neck_rotation, sizeof(int16_t));
                SaveGame_Legacy_Read(
                    &tmp_creature.maximum_turn, sizeof(int16_t));
                SaveGame_Legacy_Read(&tmp_creature.flags, sizeof(int16_t));
                SaveGame_Legacy_Read(&tmp_creature.mood, sizeof(int32_t));
            }
        }

        // check for exceptionally high item positions.
        if ((ABS(tmp_item.pos.x) | ABS(tmp_item.pos.y) | ABS(tmp_item.pos.z))
            & 0xFF000000) {
            result = true;
        }
    }

    return result;
}

static void SaveGame_Legacy_Reset(GAME_INFO *game_info)
{
    assert(game_info);
    m_SGBufPos = 0;
    m_SGBufPtr = game_info->savegame_buffer;
}

static void SaveGame_Legacy_Skip(int size)
{
    m_SGBufPtr += size;
    m_SGBufPos += size; // missing from OG
}

static void SaveGame_Legacy_Write(void *pointer, int size)
{
    m_SGBufPos += size;
    if (m_SGBufPos >= MAX_SAVEGAME_BUFFER) {
        Shell_ExitSystem("FATAL: Savegame is too big to fit in buffer");
    }

    char *data = (char *)pointer;
    for (int i = 0; i < size; i++) {
        *m_SGBufPtr++ = *data++;
    }
}

static void SaveGame_Legacy_WriteLara(LARA_INFO *lara)
{
    int32_t tmp32 = 0;

    SaveGame_Legacy_Write(&lara->item_number, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->gun_status, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->gun_type, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->request_gun_type, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->calc_fall_speed, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->water_status, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->pose_count, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->hit_frame, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->hit_direction, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->air, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->dive_count, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->death_count, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->current_active, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->spaz_effect_count, sizeof(int16_t));

    // OG just writes the pointer address (!)
    if (lara->spaz_effect) {
        tmp32 = (size_t)lara->spaz_effect - (size_t)g_Effects;
    }
    SaveGame_Legacy_Write(&tmp32, sizeof(int32_t));

    SaveGame_Legacy_Write(&lara->mesh_effects, sizeof(int32_t));

    for (int i = 0; i < LM_NUMBER_OF; i++) {
        tmp32 = (size_t)lara->mesh_ptrs[i] - (size_t)g_MeshBase;
        SaveGame_Legacy_Write(&tmp32, sizeof(int32_t));
    }

    // OG just writes the pointer address (!) assuming it's a non-existing mesh
    // 16 (!!) which happens to be g_Lara's current target. Just write NULL.
    tmp32 = 0;
    SaveGame_Legacy_Write(&tmp32, sizeof(int32_t));

    SaveGame_Legacy_Write(&lara->target_angles[0], sizeof(PHD_ANGLE));
    SaveGame_Legacy_Write(&lara->target_angles[1], sizeof(PHD_ANGLE));
    SaveGame_Legacy_Write(&lara->turn_rate, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->move_angle, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->head_y_rot, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->head_x_rot, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->head_z_rot, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->torso_y_rot, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->torso_x_rot, sizeof(int16_t));
    SaveGame_Legacy_Write(&lara->torso_z_rot, sizeof(int16_t));

    SaveGame_Legacy_WriteArm(&lara->left_arm);
    SaveGame_Legacy_WriteArm(&lara->right_arm);
    SaveGame_Legacy_Write(&lara->pistols, sizeof(AMMO_INFO));
    SaveGame_Legacy_Write(&lara->magnums, sizeof(AMMO_INFO));
    SaveGame_Legacy_Write(&lara->uzis, sizeof(AMMO_INFO));
    SaveGame_Legacy_Write(&lara->shotgun, sizeof(AMMO_INFO));
    SaveGame_Legacy_WriteLOT(&lara->LOT);
}

static void SaveGame_Legacy_WriteArm(LARA_ARM *arm)
{
    int32_t frame_base = (size_t)arm->frame_base - (size_t)g_AnimFrames;
    SaveGame_Legacy_Write(&frame_base, sizeof(int32_t));
    SaveGame_Legacy_Write(&arm->frame_number, sizeof(int16_t));
    SaveGame_Legacy_Write(&arm->lock, sizeof(int16_t));
    SaveGame_Legacy_Write(&arm->y_rot, sizeof(PHD_ANGLE));
    SaveGame_Legacy_Write(&arm->x_rot, sizeof(PHD_ANGLE));
    SaveGame_Legacy_Write(&arm->z_rot, sizeof(PHD_ANGLE));
    SaveGame_Legacy_Write(&arm->flash_gun, sizeof(int16_t));
}

static void SaveGame_Legacy_WriteLOT(LOT_INFO *lot)
{
    // it casually saves a pointer again!
    SaveGame_Legacy_Write(&lot->node, sizeof(int32_t));

    SaveGame_Legacy_Write(&lot->head, sizeof(int16_t));
    SaveGame_Legacy_Write(&lot->tail, sizeof(int16_t));
    SaveGame_Legacy_Write(&lot->search_number, sizeof(uint16_t));
    SaveGame_Legacy_Write(&lot->block_mask, sizeof(uint16_t));
    SaveGame_Legacy_Write(&lot->step, sizeof(int16_t));
    SaveGame_Legacy_Write(&lot->drop, sizeof(int16_t));
    SaveGame_Legacy_Write(&lot->fly, sizeof(int16_t));
    SaveGame_Legacy_Write(&lot->zone_count, sizeof(int16_t));
    SaveGame_Legacy_Write(&lot->target_box, sizeof(int16_t));
    SaveGame_Legacy_Write(&lot->required_box, sizeof(int16_t));
    SaveGame_Legacy_Write(&lot->target, sizeof(PHD_VECTOR));
}

static void SaveGame_Legacy_Read(void *pointer, int size)
{
    m_SGBufPos += size;
    char *data = (char *)pointer;
    for (int i = 0; i < size; i++)
        *data++ = *m_SGBufPtr++;
}

static void SaveGame_Legacy_ReadLara(LARA_INFO *lara)
{
    int32_t tmp32 = 0;

    SaveGame_Legacy_Read(&lara->item_number, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->gun_status, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->gun_type, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->request_gun_type, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->calc_fall_speed, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->water_status, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->pose_count, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->hit_frame, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->hit_direction, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->air, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->dive_count, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->death_count, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->current_active, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->spaz_effect_count, sizeof(int16_t));

    lara->spaz_effect = NULL;
    SaveGame_Legacy_Skip(sizeof(FX_INFO *));

    SaveGame_Legacy_Read(&lara->mesh_effects, sizeof(int32_t));
    for (int i = 0; i < LM_NUMBER_OF; i++) {
        SaveGame_Legacy_Read(&tmp32, sizeof(int32_t));
        lara->mesh_ptrs[i] = (int16_t *)((size_t)g_MeshBase + (size_t)tmp32);
    }

    lara->target = NULL;
    SaveGame_Legacy_Skip(sizeof(ITEM_INFO *));

    SaveGame_Legacy_Read(&lara->target_angles[0], sizeof(PHD_ANGLE));
    SaveGame_Legacy_Read(&lara->target_angles[1], sizeof(PHD_ANGLE));
    SaveGame_Legacy_Read(&lara->turn_rate, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->move_angle, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->head_y_rot, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->head_x_rot, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->head_z_rot, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->torso_y_rot, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->torso_x_rot, sizeof(int16_t));
    SaveGame_Legacy_Read(&lara->torso_z_rot, sizeof(int16_t));

    SaveGame_Legacy_ReadArm(&lara->left_arm);
    SaveGame_Legacy_ReadArm(&lara->right_arm);
    SaveGame_Legacy_Read(&lara->pistols, sizeof(AMMO_INFO));
    SaveGame_Legacy_Read(&lara->magnums, sizeof(AMMO_INFO));
    SaveGame_Legacy_Read(&lara->uzis, sizeof(AMMO_INFO));
    SaveGame_Legacy_Read(&lara->shotgun, sizeof(AMMO_INFO));
    SaveGame_Legacy_ReadLOT(&lara->LOT);
}

static void SaveGame_Legacy_ReadArm(LARA_ARM *arm)
{
    int32_t frame_base;
    SaveGame_Legacy_Read(&frame_base, sizeof(int32_t));
    arm->frame_base = (int16_t *)((size_t)g_AnimFrames + (size_t)frame_base);

    SaveGame_Legacy_Read(&arm->frame_number, sizeof(int16_t));
    SaveGame_Legacy_Read(&arm->lock, sizeof(int16_t));
    SaveGame_Legacy_Read(&arm->y_rot, sizeof(PHD_ANGLE));
    SaveGame_Legacy_Read(&arm->x_rot, sizeof(PHD_ANGLE));
    SaveGame_Legacy_Read(&arm->z_rot, sizeof(PHD_ANGLE));
    SaveGame_Legacy_Read(&arm->flash_gun, sizeof(int16_t));
}

static void SaveGame_Legacy_ReadLOT(LOT_INFO *lot)
{
    lot->node = NULL;
    SaveGame_Legacy_Skip(sizeof(BOX_NODE *));

    SaveGame_Legacy_Read(&lot->head, sizeof(int16_t));
    SaveGame_Legacy_Read(&lot->tail, sizeof(int16_t));
    SaveGame_Legacy_Read(&lot->search_number, sizeof(uint16_t));
    SaveGame_Legacy_Read(&lot->block_mask, sizeof(uint16_t));
    SaveGame_Legacy_Read(&lot->step, sizeof(int16_t));
    SaveGame_Legacy_Read(&lot->drop, sizeof(int16_t));
    SaveGame_Legacy_Read(&lot->fly, sizeof(int16_t));
    SaveGame_Legacy_Read(&lot->zone_count, sizeof(int16_t));
    SaveGame_Legacy_Read(&lot->target_box, sizeof(int16_t));
    SaveGame_Legacy_Read(&lot->required_box, sizeof(int16_t));
    SaveGame_Legacy_Read(&lot->target, sizeof(PHD_VECTOR));
}

char *SaveGame_Legacy_GetSavePath(int32_t slot)
{
    size_t out_size =
        snprintf(NULL, 0, g_GameFlow.savegame_fmt_legacy, slot) + 1;
    char *out = Memory_Alloc(out_size);
    snprintf(out, out_size, g_GameFlow.savegame_fmt_legacy, slot);
    return out;
}

int16_t SaveGame_Legacy_GetLevelNumber(MYFILE *fp)
{
    File_Seek(fp, 0, SEEK_SET);
    File_Skip(fp, SAVEGAME_LEGACY_TITLE_SIZE); // level title
    File_Skip(fp, sizeof(int32_t)); // save counter
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        File_Skip(fp, sizeof(uint16_t)); // pistol ammo
        File_Skip(fp, sizeof(uint16_t)); // magnum ammo
        File_Skip(fp, sizeof(uint16_t)); // uzi ammo
        File_Skip(fp, sizeof(uint16_t)); // shotgun ammo
        File_Skip(fp, sizeof(uint8_t)); // small medis
        File_Skip(fp, sizeof(uint8_t)); // big medis
        File_Skip(fp, sizeof(uint8_t)); // scions
        File_Skip(fp, sizeof(int8_t)); // gun status
        File_Skip(fp, sizeof(int8_t)); // gun type
        File_Skip(fp, sizeof(uint16_t)); // flags
    }
    File_Skip(fp, sizeof(uint32_t)); // timer
    File_Skip(fp, sizeof(uint32_t)); // kills
    File_Skip(fp, sizeof(uint16_t)); // secrets

    uint16_t level_num;
    File_Read(&level_num, sizeof(int16_t), 1, fp);
    return level_num;
}

char *SaveGame_Legacy_GetLevelTitle(MYFILE *fp)
{
    File_Seek(fp, 0, SEEK_SET);
    char title[SAVEGAME_LEGACY_TITLE_SIZE];
    File_Read(title, sizeof(char), SAVEGAME_LEGACY_TITLE_SIZE, fp);
    return Memory_Dup(title);
}

int32_t SaveGame_Legacy_GetSaveCounter(MYFILE *fp)
{
    File_Seek(fp, 0, SEEK_SET);
    File_Skip(fp, SAVEGAME_LEGACY_TITLE_SIZE);
    int32_t counter;
    File_Read(&counter, sizeof(int32_t), 1, fp);
    return counter;
}

bool SaveGame_Legacy_ApplySaveBuffer(GAME_INFO *game_info)
{
    assert(game_info);

    int8_t tmp8;
    int16_t tmp16;
    int32_t tmp32;

    bool skip_reading_evil_lara = SaveGame_Legacy_NeedsEvilLaraFix(game_info);
    if (skip_reading_evil_lara) {
        LOG_INFO("Enabling Evil Lara savegame fix");
    }

    SaveGame_Legacy_Reset(game_info);
    SaveGame_Legacy_Skip(SAVEGAME_LEGACY_TITLE_SIZE); // level title
    SaveGame_Legacy_Skip(sizeof(int32_t)); // save counter

    assert(game_info->start);
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        START_INFO *start = &game_info->start[i];
        SaveGame_Legacy_Read(&start->pistol_ammo, sizeof(uint16_t));
        SaveGame_Legacy_Read(&start->magnum_ammo, sizeof(uint16_t));
        SaveGame_Legacy_Read(&start->uzi_ammo, sizeof(uint16_t));
        SaveGame_Legacy_Read(&start->shotgun_ammo, sizeof(uint16_t));
        SaveGame_Legacy_Read(&start->num_medis, sizeof(uint8_t));
        SaveGame_Legacy_Read(&start->num_big_medis, sizeof(uint8_t));
        SaveGame_Legacy_Read(&start->num_scions, sizeof(uint8_t));
        SaveGame_Legacy_Read(&start->gun_status, sizeof(int8_t));
        SaveGame_Legacy_Read(&start->gun_type, sizeof(int8_t));
        SaveGame_Legacy_Read(&start->flags, sizeof(uint16_t));
    }

    SaveGame_Legacy_Read(&game_info->timer, sizeof(uint32_t));
    SaveGame_Legacy_Read(&game_info->kills, sizeof(uint32_t));
    SaveGame_Legacy_Read(&game_info->secrets, sizeof(uint16_t));
    SaveGame_Legacy_Read(&g_CurrentLevel, sizeof(uint16_t));
    SaveGame_Legacy_Read(&game_info->pickups, sizeof(uint8_t));
    SaveGame_Legacy_Read(&game_info->bonus_flag, sizeof(uint8_t));

    InitialiseLaraInventory(g_CurrentLevel);
    SAVEGAME_LEGACY_ITEM_STATS item_stats = { 0 };
    SaveGame_Legacy_Read(&item_stats, sizeof(item_stats));
    Inv_AddItemNTimes(O_PICKUP_ITEM1, item_stats.num_pickup1);
    Inv_AddItemNTimes(O_PICKUP_ITEM2, item_stats.num_pickup2);
    Inv_AddItemNTimes(O_PUZZLE_ITEM1, item_stats.num_puzzle1);
    Inv_AddItemNTimes(O_PUZZLE_ITEM2, item_stats.num_puzzle2);
    Inv_AddItemNTimes(O_PUZZLE_ITEM3, item_stats.num_puzzle3);
    Inv_AddItemNTimes(O_PUZZLE_ITEM4, item_stats.num_puzzle4);
    Inv_AddItemNTimes(O_KEY_ITEM1, item_stats.num_key1);
    Inv_AddItemNTimes(O_KEY_ITEM2, item_stats.num_key2);
    Inv_AddItemNTimes(O_KEY_ITEM3, item_stats.num_key3);
    Inv_AddItemNTimes(O_KEY_ITEM4, item_stats.num_key4);
    Inv_AddItemNTimes(O_LEADBAR_ITEM, item_stats.num_leadbar);

    SaveGame_Legacy_Read(&tmp32, sizeof(int32_t));
    if (tmp32) {
        FlipMap();
    }

    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        SaveGame_Legacy_Read(&tmp8, sizeof(int8_t));
        g_FlipMapTable[i] = tmp8 << 8;
    }

    for (int i = 0; i < g_NumberCameras; i++) {
        SaveGame_Legacy_Read(&g_Camera.fixed[i].flags, sizeof(int16_t));
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
            SaveGame_Legacy_Read(&item->pos, sizeof(PHD_3DPOS));
            SaveGame_Legacy_Read(&tmp16, sizeof(int16_t));
            SaveGame_Legacy_Read(&item->speed, sizeof(int16_t));
            SaveGame_Legacy_Read(&item->fall_speed, sizeof(int16_t));

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
            SaveGame_Legacy_Read(&item->current_anim_state, sizeof(int16_t));
            SaveGame_Legacy_Read(&item->goal_anim_state, sizeof(int16_t));
            SaveGame_Legacy_Read(&item->required_anim_state, sizeof(int16_t));
            SaveGame_Legacy_Read(&item->anim_number, sizeof(int16_t));
            SaveGame_Legacy_Read(&item->frame_number, sizeof(int16_t));
        }

        if (obj->save_hitpoints) {
            SaveGame_Legacy_Read(&item->hit_points, sizeof(int16_t));
        }

        if (obj->save_flags
            && (item->object_number != O_EVIL_LARA
                || !skip_reading_evil_lara)) {
            SaveGame_Legacy_Read(&item->flags, sizeof(int16_t));
            SaveGame_Legacy_Read(&item->timer, sizeof(int16_t));

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
                    SaveGame_Legacy_Read(
                        &creature->head_rotation, sizeof(int16_t));
                    SaveGame_Legacy_Read(
                        &creature->neck_rotation, sizeof(int16_t));
                    SaveGame_Legacy_Read(
                        &creature->maximum_turn, sizeof(int16_t));
                    SaveGame_Legacy_Read(&creature->flags, sizeof(int16_t));
                    SaveGame_Legacy_Read(&creature->mood, sizeof(int32_t));
                } else {
                    SaveGame_Legacy_Skip(4 * 2 + 4);
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
            g_MusicTrackFlags[MX_PIERRE_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_SKATEKID && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_UZI_ITEM)) {
                SpawnItem(item, O_UZI_ITEM);
            }
        }

        if (item->object_number == O_COWBOY && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
                SpawnItem(item, O_MAGNUM_ITEM);
            }
            g_MusicTrackFlags[MX_COWBOY_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_BALDY && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
                SpawnItem(item, O_SHOTGUN_ITEM);
            }
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_LARSON && item->hit_points <= 0) {
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONESHOT;
        }
    }

    BOX_NODE *node = g_Lara.LOT.node;
    SaveGame_Legacy_ReadLara(&g_Lara);
    g_Lara.LOT.node = node;
    g_Lara.LOT.target_box = NO_BOX;

    SaveGame_Legacy_Read(&g_FlipEffect, sizeof(int32_t));
    SaveGame_Legacy_Read(&g_FlipTimer, sizeof(int32_t));
    return true;
}

void SaveGame_Legacy_FillSaveBuffer(GAME_INFO *game_info)
{
    assert(game_info);

    SaveGame_Legacy_Reset(game_info);
    memset(m_SGBufPtr, 0, MAX_SAVEGAME_BUFFER);

    char title[SAVEGAME_LEGACY_TITLE_SIZE];
    snprintf(
        title, SAVEGAME_LEGACY_TITLE_SIZE, "%s",
        g_GameFlow.levels[g_CurrentLevel].level_title);
    SaveGame_Legacy_Write(title, SAVEGAME_LEGACY_TITLE_SIZE);
    SaveGame_Legacy_Write(&g_SaveCounter, sizeof(int32_t));

    assert(game_info->start);
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        START_INFO *start = &game_info->start[i];
        SaveGame_Legacy_Write(&start->pistol_ammo, sizeof(uint16_t));
        SaveGame_Legacy_Write(&start->magnum_ammo, sizeof(uint16_t));
        SaveGame_Legacy_Write(&start->uzi_ammo, sizeof(uint16_t));
        SaveGame_Legacy_Write(&start->shotgun_ammo, sizeof(uint16_t));
        SaveGame_Legacy_Write(&start->num_medis, sizeof(uint8_t));
        SaveGame_Legacy_Write(&start->num_big_medis, sizeof(uint8_t));
        SaveGame_Legacy_Write(&start->num_scions, sizeof(uint8_t));
        SaveGame_Legacy_Write(&start->gun_status, sizeof(int8_t));
        SaveGame_Legacy_Write(&start->gun_type, sizeof(int8_t));
        SaveGame_Legacy_Write(&start->flags, sizeof(uint16_t));
    }

    SaveGame_Legacy_Write(&game_info->timer, sizeof(uint32_t));
    SaveGame_Legacy_Write(&game_info->kills, sizeof(uint32_t));
    SaveGame_Legacy_Write(&game_info->secrets, sizeof(uint16_t));
    SaveGame_Legacy_Write(&g_CurrentLevel, sizeof(uint16_t));
    SaveGame_Legacy_Write(&game_info->pickups, sizeof(uint8_t));
    SaveGame_Legacy_Write(&game_info->bonus_flag, sizeof(uint8_t));

    SAVEGAME_LEGACY_ITEM_STATS item_stats = {
        .num_pickup1 = Inv_RequestItem(O_PICKUP_ITEM1),
        .num_pickup2 = Inv_RequestItem(O_PICKUP_ITEM2),
        .num_puzzle1 = Inv_RequestItem(O_PUZZLE_ITEM1),
        .num_puzzle2 = Inv_RequestItem(O_PUZZLE_ITEM2),
        .num_puzzle3 = Inv_RequestItem(O_PUZZLE_ITEM3),
        .num_puzzle4 = Inv_RequestItem(O_PUZZLE_ITEM4),
        .num_key1 = Inv_RequestItem(O_KEY_ITEM1),
        .num_key2 = Inv_RequestItem(O_KEY_ITEM2),
        .num_key3 = Inv_RequestItem(O_KEY_ITEM3),
        .num_key4 = Inv_RequestItem(O_KEY_ITEM4),
        .num_leadbar = Inv_RequestItem(O_LEADBAR_ITEM),
        0
    };

    SaveGame_Legacy_Write(&item_stats, sizeof(item_stats));

    SaveGame_Legacy_Write(&g_FlipStatus, sizeof(int32_t));
    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        int8_t flag = g_FlipMapTable[i] >> 8;
        SaveGame_Legacy_Write(&flag, sizeof(int8_t));
    }

    for (int i = 0; i < g_NumberCameras; i++) {
        SaveGame_Legacy_Write(&g_Camera.fixed[i].flags, sizeof(int16_t));
    }

    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        if (obj->save_position) {
            SaveGame_Legacy_Write(&item->pos, sizeof(PHD_3DPOS));
            SaveGame_Legacy_Write(&item->room_number, sizeof(int16_t));
            SaveGame_Legacy_Write(&item->speed, sizeof(int16_t));
            SaveGame_Legacy_Write(&item->fall_speed, sizeof(int16_t));
        }

        if (obj->save_anim) {
            SaveGame_Legacy_Write(&item->current_anim_state, sizeof(int16_t));
            SaveGame_Legacy_Write(&item->goal_anim_state, sizeof(int16_t));
            SaveGame_Legacy_Write(&item->required_anim_state, sizeof(int16_t));
            SaveGame_Legacy_Write(&item->anim_number, sizeof(int16_t));
            SaveGame_Legacy_Write(&item->frame_number, sizeof(int16_t));
        }

        if (obj->save_hitpoints) {
            SaveGame_Legacy_Write(&item->hit_points, sizeof(int16_t));
        }

        if (obj->save_flags) {
            uint16_t flags = item->flags + item->active + (item->status << 1)
                + (item->gravity_status << 3) + (item->collidable << 4);
            if (obj->intelligent && item->data) {
                flags |= SAVE_CREATURE;
            }
            SaveGame_Legacy_Write(&flags, sizeof(uint16_t));
            SaveGame_Legacy_Write(&item->timer, sizeof(int16_t));
            if (flags & SAVE_CREATURE) {
                CREATURE_INFO *creature = item->data;
                SaveGame_Legacy_Write(
                    &creature->head_rotation, sizeof(int16_t));
                SaveGame_Legacy_Write(
                    &creature->neck_rotation, sizeof(int16_t));
                SaveGame_Legacy_Write(&creature->maximum_turn, sizeof(int16_t));
                SaveGame_Legacy_Write(&creature->flags, sizeof(int16_t));
                SaveGame_Legacy_Write(&creature->mood, sizeof(int32_t));
            }
        }
    }

    SaveGame_Legacy_WriteLara(&g_Lara);

    SaveGame_Legacy_Write(&g_FlipEffect, sizeof(int32_t));
    SaveGame_Legacy_Write(&g_FlipTimer, sizeof(int32_t));

    game_info->savegame_buffer_size = m_SGBufPos;
}
