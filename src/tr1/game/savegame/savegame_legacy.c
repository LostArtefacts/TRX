#include "game/savegame/savegame_legacy.h"

#include "game/camera.h"
#include "game/carrier.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/level.h"
#include "game/lot.h"
#include "game/room.h"
#include "game/shell.h"
#include "game/stats.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <stdio.h>
#include <string.h>

#define SAVE_CREATURE (1 << 7)
#define SAVEGAME_LEGACY_TITLE_SIZE 75
#define SAVEGAME_LEGACY_MAX_BUFFER_SIZE (20 * 1024)

#pragma pack(push, 1)
typedef struct {
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
#pragma pack(pop)

static int m_SGBufPos = 0;
static char *m_SGBufPtr = nullptr;

static bool M_ItemHasSaveFlags(const OBJECT *obj, ITEM *item);
static bool M_ItemHasSaveAnim(const ITEM *item);
static bool M_ItemHasHitPoints(const ITEM *item);
static bool M_NeedsBaconLaraFix(char *buffer);

static void M_Reset(char *buffer);
static void M_Skip(int size);

static void M_Read(void *pointer, int size);
static void M_ReadArm(LARA_ARM *arm);
static void M_ReadLara(LARA_INFO *lara);
static void M_ReadLOT(LOT_INFO *lot);
static void M_SetCurrentPosition(int32_t level_num);
static void M_ReadResumeInfo(MYFILE *fp, GAME_INFO *game_info);

static void M_Write(const void *pointer, int size);
static void M_WriteArm(LARA_ARM *arm);
static void M_WriteLara(LARA_INFO *lara);
static void M_WriteLOT(LOT_INFO *lot);

static bool M_ItemHasSaveFlags(const OBJECT *const obj, ITEM *const item)
{
    // TR1X savegame files are enhanced to store more information by having
    // changed the save_flags bit for certain item types. However, legacy
    // TombATI saves do not contain the information that's associated with
    // these flags for these enhanced items. The way they are structured,
    // whether this information exists or not, cannot be figured out from the
    // save file alone. So the object IDs that got changed are listed here
    // to make sure the legacy savegame reader doesn't try to reach out for
    // information that's not there.
    return (
        obj->save_flags && item->object_id != O_EMBER_EMITTER
        && item->object_id != O_FLAME_EMITTER && item->object_id != O_WATERFALL
        && item->object_id != O_SCION_ITEM_1
        && item->object_id != O_DART_EMITTER);
}

static bool M_ItemHasSaveAnim(const ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    return obj->save_anim && item->object_id != O_BACON_LARA;
}

static bool M_ItemHasHitPoints(const ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    return obj->save_hitpoints && item->object_id != O_SCION_ITEM_3;
}

static bool M_NeedsBaconLaraFix(char *buffer)
{
    // Heuristic for issue #261.
    // TR1X enables save_flags for Bacon Lara, but OG TombATI does not. As
    // a consequence, Atlantis saves made with OG TombATI (which includes the
    // ones available for download on Stella's website) have different layout
    // than the saves made with TR1X. This was discovered after it was too
    // late to make a backwards incompatible change. At the same time, enabling
    // save_flags for Bacon Lara is desirable, as not doing this causes her to
    // freeze when the player reloads a save made in her room. This function is
    // used to determine whether the save about to be loaded includes
    // save_flags for Bacon Lara or not. Since savegames only contain very
    // concise information, we must make an educated guess here.

    ASSERT(buffer != nullptr);

    bool result = false;
    if (Game_GetCurrentLevel()->num != 14) {
        return result;
    }

    M_Reset(buffer);
    M_Skip(SAVEGAME_LEGACY_TITLE_SIZE); // level title
    M_Skip(sizeof(int32_t)); // save counter
    for (int i = 0; i < GF_GetLevelTable(GFLT_MAIN)->count; i++) {
        M_Skip(sizeof(uint16_t)); // pistol ammo
        M_Skip(sizeof(uint16_t)); // magnum ammo
        M_Skip(sizeof(uint16_t)); // uzi ammo
        M_Skip(sizeof(uint16_t)); // shotgun ammo
        M_Skip(sizeof(uint8_t)); // small medis
        M_Skip(sizeof(uint8_t)); // big medis
        M_Skip(sizeof(uint8_t)); // scions
        M_Skip(sizeof(int8_t)); // gun status
        M_Skip(sizeof(int8_t)); // gun type
        M_Skip(sizeof(uint16_t)); // flags
    }
    M_Skip(sizeof(uint32_t)); // timer
    M_Skip(sizeof(uint32_t)); // kills
    M_Skip(sizeof(uint16_t)); // secrets
    M_Skip(sizeof(uint16_t)); // current level
    M_Skip(sizeof(uint8_t)); // pickups
    M_Skip(sizeof(uint8_t)); // bonus_flag
    M_Skip(sizeof(SAVEGAME_LEGACY_ITEM_STATS)); // item stats
    M_Skip(sizeof(int32_t)); // flipmap status
    M_Skip(MAX_FLIP_MAPS * sizeof(int8_t)); // flipmap table
    M_Skip(Camera_GetFixedObjectCount() * sizeof(int16_t)); // cameras

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        ITEM tmp_item;

        if (obj->save_position) {
            M_Read(&tmp_item.pos.x, sizeof(int32_t));
            M_Read(&tmp_item.pos.y, sizeof(int32_t));
            M_Read(&tmp_item.pos.z, sizeof(int32_t));
            M_Read(&tmp_item.rot.x, sizeof(int16_t));
            M_Read(&tmp_item.rot.y, sizeof(int16_t));
            M_Read(&tmp_item.rot.z, sizeof(int16_t));
            M_Skip(sizeof(int16_t));
            M_Read(&tmp_item.speed, sizeof(int16_t));
            M_Read(&tmp_item.fall_speed, sizeof(int16_t));
        }
        if (M_ItemHasSaveAnim(item)) {
            M_Read(&tmp_item.current_anim_state, sizeof(int16_t));
            M_Read(&tmp_item.goal_anim_state, sizeof(int16_t));
            M_Read(&tmp_item.required_anim_state, sizeof(int16_t));
            M_Read(&tmp_item.anim_num, sizeof(int16_t));
            M_Read(&tmp_item.frame_num, sizeof(int16_t));
        }
        if (M_ItemHasHitPoints(item)) {
            M_Read(&tmp_item.hit_points, sizeof(int16_t));
        }
        if (M_ItemHasSaveFlags(obj, item)) {
            M_Read(&tmp_item.flags, sizeof(int16_t));
            M_Read(&tmp_item.timer, sizeof(int16_t));
            if (tmp_item.flags & SAVE_CREATURE) {
                CREATURE tmp_creature;
                M_Read(&tmp_creature.head_rotation, sizeof(int16_t));
                M_Read(&tmp_creature.neck_rotation, sizeof(int16_t));
                M_Read(&tmp_creature.maximum_turn, sizeof(int16_t));
                M_Read(&tmp_creature.flags, sizeof(int16_t));
                M_Read(&tmp_creature.mood, sizeof(int32_t));
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

static void M_Reset(char *buffer)
{
    m_SGBufPos = 0;
    m_SGBufPtr = buffer;
}

static void M_Skip(int size)
{
    m_SGBufPtr += size;
    m_SGBufPos += size; // missing from OG
}

static void M_Write(const void *pointer, int size)
{
    m_SGBufPos += size;
    if (m_SGBufPos >= SAVEGAME_LEGACY_MAX_BUFFER_SIZE) {
        Shell_ExitSystem("FATAL: Savegame is too big to fit in buffer");
    }

    const char *data = (const char *)pointer;
    for (int i = 0; i < size; i++) {
        *m_SGBufPtr++ = *data++;
    }
}

static void M_WriteLara(LARA_INFO *lara)
{
    int32_t tmp32 = 0;

    M_Write(&lara->item_num, sizeof(int16_t));
    M_Write(&lara->gun_status, sizeof(int16_t));
    M_Write(&lara->gun_type, sizeof(int16_t));
    M_Write(&lara->request_gun_type, sizeof(int16_t));
    M_Write(&lara->calc_fall_speed, sizeof(int16_t));
    M_Write(&lara->water_status, sizeof(int16_t));
    M_Write(&lara->pose_count, sizeof(int16_t));
    M_Write(&lara->hit_frame, sizeof(int16_t));
    M_Write(&lara->hit_direction, sizeof(int16_t));
    M_Write(&lara->air, sizeof(int16_t));
    M_Write(&lara->dive_timer, sizeof(int16_t));
    M_Write(&lara->death_timer, sizeof(int16_t));
    M_Write(&lara->current_active, sizeof(int16_t));
    M_Write(&lara->hit_effect_count, sizeof(int16_t));

    // OG just writes the pointer address (!).
    if (lara->hit_effect) {
        tmp32 = Effect_GetNum(lara->hit_effect);
    }
    M_Write(&tmp32, sizeof(int32_t));

    M_Write(&lara->mesh_effects, sizeof(int32_t));

    for (int i = 0; i < LM_NUMBER_OF; i++) {
        tmp32 = Object_GetMeshOffset(lara->mesh_ptrs[i]) * 2;
        M_Write(&tmp32, sizeof(int32_t));
    }

    // OG just writes the pointer address (!) assuming it's a non-existing mesh
    // 16 (!!) which happens to be g_Lara's current target. Just write nullptr.
    tmp32 = 0;
    M_Write(&tmp32, sizeof(int32_t));

    M_Write(&lara->target_angles[0], sizeof(PHD_ANGLE));
    M_Write(&lara->target_angles[1], sizeof(PHD_ANGLE));
    M_Write(&lara->turn_rate, sizeof(int16_t));
    M_Write(&lara->move_angle, sizeof(int16_t));
    M_Write(&lara->head_rot.y, sizeof(int16_t));
    M_Write(&lara->head_rot.x, sizeof(int16_t));
    M_Write(&lara->head_rot.z, sizeof(int16_t));
    M_Write(&lara->torso_rot.y, sizeof(int16_t));
    M_Write(&lara->torso_rot.x, sizeof(int16_t));
    M_Write(&lara->torso_rot.z, sizeof(int16_t));

    M_WriteArm(&lara->left_arm);
    M_WriteArm(&lara->right_arm);
    M_Write(&lara->pistols.ammo, sizeof(int32_t));
    M_Write(&lara->pistols.hit, sizeof(int32_t));
    M_Write(&lara->pistols.miss, sizeof(int32_t));
    M_Write(&lara->magnums.ammo, sizeof(int32_t));
    M_Write(&lara->magnums.hit, sizeof(int32_t));
    M_Write(&lara->magnums.miss, sizeof(int32_t));
    M_Write(&lara->uzis.ammo, sizeof(int32_t));
    M_Write(&lara->uzis.hit, sizeof(int32_t));
    M_Write(&lara->uzis.miss, sizeof(int32_t));
    M_Write(&lara->shotgun.ammo, sizeof(int32_t));
    M_Write(&lara->shotgun.hit, sizeof(int32_t));
    M_Write(&lara->shotgun.miss, sizeof(int32_t));
    M_WriteLOT(&lara->lot);
}

static void M_WriteArm(LARA_ARM *arm)
{
    // frame_base is not required
    const int32_t frame_base = 0;
    M_Write(&frame_base, sizeof(int32_t));
    M_Write(&arm->frame_num, sizeof(int16_t));
    M_Write(&arm->lock, sizeof(int16_t));
    M_Write(&arm->rot.y, sizeof(PHD_ANGLE));
    M_Write(&arm->rot.x, sizeof(PHD_ANGLE));
    M_Write(&arm->rot.z, sizeof(PHD_ANGLE));
    M_Write(&arm->flash_gun, sizeof(int16_t));
}

static void M_WriteLOT(LOT_INFO *lot)
{
    // it casually saves a pointer again!
    M_Write(&lot->node, sizeof(int32_t));

    M_Write(&lot->head, sizeof(int16_t));
    M_Write(&lot->tail, sizeof(int16_t));
    M_Write(&lot->search_num, sizeof(uint16_t));
    M_Write(&lot->block_mask, sizeof(uint16_t));
    M_Write(&lot->step, sizeof(int16_t));
    M_Write(&lot->drop, sizeof(int16_t));
    M_Write(&lot->fly, sizeof(int16_t));
    M_Write(&lot->zone_count, sizeof(int16_t));
    M_Write(&lot->target_box, sizeof(int16_t));
    M_Write(&lot->required_box, sizeof(int16_t));
    M_Write(&lot->target.x, sizeof(int32_t));
    M_Write(&lot->target.y, sizeof(int32_t));
    M_Write(&lot->target.z, sizeof(int32_t));
}

static void M_Read(void *pointer, int size)
{
    m_SGBufPos += size;
    char *data = (char *)pointer;
    for (int i = 0; i < size; i++)
        *data++ = *m_SGBufPtr++;
}

static void M_ReadLara(LARA_INFO *lara)
{
    int32_t tmp32 = 0;

    M_Read(&lara->item_num, sizeof(int16_t));
    M_Read(&lara->gun_status, sizeof(int16_t));
    M_Read(&lara->gun_type, sizeof(int16_t));
    M_Read(&lara->request_gun_type, sizeof(int16_t));
    M_Read(&lara->calc_fall_speed, sizeof(int16_t));
    M_Read(&lara->water_status, sizeof(int16_t));
    M_Read(&lara->pose_count, sizeof(int16_t));
    M_Read(&lara->hit_frame, sizeof(int16_t));
    M_Read(&lara->hit_direction, sizeof(int16_t));
    M_Read(&lara->air, sizeof(int16_t));
    M_Read(&lara->dive_timer, sizeof(int16_t));
    M_Read(&lara->death_timer, sizeof(int16_t));
    M_Read(&lara->current_active, sizeof(int16_t));
    M_Read(&lara->hit_effect_count, sizeof(int16_t));

    lara->hit_effect = nullptr;
    M_Skip(4); // pointer to EFFECT

    M_Read(&lara->mesh_effects, sizeof(int32_t));
    for (int i = 0; i < LM_NUMBER_OF; i++) {
        M_Read(&tmp32, sizeof(int32_t));
        OBJECT_MESH *const mesh = Object_FindMesh(tmp32 / 2);
        if (mesh != nullptr) {
            lara->mesh_ptrs[i] = mesh;
        }
    }

    lara->target = nullptr;
    M_Skip(4); // pointer to ITEM

    M_Read(&lara->target_angles[0], sizeof(PHD_ANGLE));
    M_Read(&lara->target_angles[1], sizeof(PHD_ANGLE));
    M_Read(&lara->turn_rate, sizeof(int16_t));
    M_Read(&lara->move_angle, sizeof(int16_t));
    M_Read(&lara->head_rot.y, sizeof(int16_t));
    M_Read(&lara->head_rot.x, sizeof(int16_t));
    M_Read(&lara->head_rot.z, sizeof(int16_t));
    M_Read(&lara->torso_rot.y, sizeof(int16_t));
    M_Read(&lara->torso_rot.x, sizeof(int16_t));
    M_Read(&lara->torso_rot.z, sizeof(int16_t));

    M_ReadArm(&lara->left_arm);
    M_ReadArm(&lara->right_arm);
    M_Read(&lara->pistols.ammo, sizeof(int32_t));
    M_Read(&lara->pistols.hit, sizeof(int32_t));
    M_Read(&lara->pistols.miss, sizeof(int32_t));
    M_Read(&lara->magnums.ammo, sizeof(int32_t));
    M_Read(&lara->magnums.hit, sizeof(int32_t));
    M_Read(&lara->magnums.miss, sizeof(int32_t));
    M_Read(&lara->uzis.ammo, sizeof(int32_t));
    M_Read(&lara->uzis.hit, sizeof(int32_t));
    M_Read(&lara->uzis.miss, sizeof(int32_t));
    M_Read(&lara->shotgun.ammo, sizeof(int32_t));
    M_Read(&lara->shotgun.hit, sizeof(int32_t));
    M_Read(&lara->shotgun.miss, sizeof(int32_t));
    M_ReadLOT(&lara->lot);
}

static void M_ReadArm(LARA_ARM *arm)
{
    // frame_base is superfluous
    M_Skip(sizeof(int32_t));

    M_Read(&arm->frame_num, sizeof(int16_t));
    M_Read(&arm->lock, sizeof(int16_t));
    M_Read(&arm->rot.y, sizeof(PHD_ANGLE));
    M_Read(&arm->rot.x, sizeof(PHD_ANGLE));
    M_Read(&arm->rot.z, sizeof(PHD_ANGLE));
    M_Read(&arm->flash_gun, sizeof(int16_t));
}

static void M_ReadLOT(LOT_INFO *lot)
{
    M_Skip(4); // pointer to BOX_NODE

    M_Read(&lot->head, sizeof(int16_t));
    M_Read(&lot->tail, sizeof(int16_t));
    M_Read(&lot->search_num, sizeof(uint16_t));
    M_Read(&lot->block_mask, sizeof(uint16_t));
    M_Read(&lot->step, sizeof(int16_t));
    M_Read(&lot->drop, sizeof(int16_t));
    M_Read(&lot->fly, sizeof(int16_t));
    M_Read(&lot->zone_count, sizeof(int16_t));
    M_Read(&lot->target_box, sizeof(int16_t));
    M_Read(&lot->required_box, sizeof(int16_t));
    M_Read(&lot->target.x, sizeof(int32_t));
    M_Read(&lot->target.y, sizeof(int32_t));
    M_Read(&lot->target.z, sizeof(int32_t));
}

static void M_SetCurrentPosition(const int32_t level_num)
{
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type == GFL_CURRENT) {
            g_GameInfo.current[current_level->num] = g_GameInfo.current[i];
        }
    }
}

static void M_ReadResumeInfo(MYFILE *fp, GAME_INFO *game_info)
{
    ASSERT(game_info->current != nullptr);
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        RESUME_INFO *current = Savegame_GetCurrentInfo(level);
        M_Read(&current->pistol_ammo, sizeof(uint16_t));
        M_Read(&current->magnum_ammo, sizeof(uint16_t));
        M_Read(&current->uzi_ammo, sizeof(uint16_t));
        M_Read(&current->shotgun_ammo, sizeof(uint16_t));
        M_Read(&current->num_medis, sizeof(uint8_t));
        M_Read(&current->num_big_medis, sizeof(uint8_t));
        M_Read(&current->num_scions, sizeof(uint8_t));
        M_Read(&current->gun_status, sizeof(int8_t));
        M_Read(&current->equipped_gun_type, sizeof(int8_t));
        current->holsters_gun_type = LGT_UNKNOWN;
        current->back_gun_type = LGT_UNKNOWN;
        uint16_t flags;
        M_Read(&flags, sizeof(uint16_t));
        current->flags.available = flags & 1 ? 1 : 0;
        current->flags.got_pistols = flags & 2 ? 1 : 0;
        current->flags.got_magnums = flags & 4 ? 1 : 0;
        current->flags.got_uzis = flags & 8 ? 1 : 0;
        current->flags.got_shotgun = flags & 16 ? 1 : 0;
        current->flags.costume = flags & 32 ? 1 : 0;
        // Gym and first level have special starting items.
        if (level == GF_GetFirstLevel() || level == GF_GetGymLevel()) {
            Savegame_ApplyLogicToCurrentInfo(level);
        }
    }

    uint32_t temp_timer = 0;
    uint32_t temp_kill_count = 0;
    uint16_t temp_secret_flags = 0;
    uint16_t current_level;
    M_Read(&temp_timer, sizeof(uint32_t));
    M_Read(&temp_kill_count, sizeof(uint32_t));
    M_Read(&temp_secret_flags, sizeof(uint16_t));
    M_Read(&current_level, sizeof(uint16_t));
    M_SetCurrentPosition(current_level);
    RESUME_INFO *const resume_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    resume_info->stats.timer = temp_timer;
    resume_info->stats.kill_count = temp_kill_count;
    resume_info->stats.secret_flags = temp_secret_flags;
    Stats_UpdateSecrets(&resume_info->stats);
    M_Read(&resume_info->stats.pickup_count, sizeof(uint8_t));
    M_Read(&game_info->bonus_flag, sizeof(uint8_t));
    game_info->death_count = -1;
}

char *Savegame_Legacy_GetSaveFileName(int32_t slot)
{
    size_t out_size =
        snprintf(nullptr, 0, g_GameFlow.savegame_fmt_legacy, slot) + 1;
    char *out = Memory_Alloc(out_size);
    snprintf(out, out_size, g_GameFlow.savegame_fmt_legacy, slot);
    return out;
}

bool Savegame_Legacy_FillInfo(MYFILE *fp, SAVEGAME_INFO *info)
{
    File_Seek(fp, 0, SEEK_SET);

    char title[SAVEGAME_LEGACY_TITLE_SIZE];
    File_ReadItems(fp, title, sizeof(char), SAVEGAME_LEGACY_TITLE_SIZE);
    info->level_title = Memory_DupStr(title);

    int32_t counter;
    counter = File_ReadS32(fp);
    info->counter = counter;

    for (int i = 0; i < GF_GetLevelTable(GFLT_MAIN)->count; i++) {
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

    const uint16_t level_num = File_ReadS16(fp);
    info->level_num = level_num;

    info->initial_version = VERSION_LEGACY;
    info->features.restart = false;
    info->features.select_level = false;

    return true;
}

bool Savegame_Legacy_LoadFromFile(MYFILE *fp, GAME_INFO *game_info)
{
    ASSERT(game_info != nullptr);

    int8_t tmp8;
    int16_t tmp16;
    int32_t tmp32;

    char *buffer = Memory_Alloc(File_Size(fp));
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, File_Size(fp));

    bool skip_reading_bacon_lara = M_NeedsBaconLaraFix(buffer);
    if (skip_reading_bacon_lara) {
        LOG_INFO("Enabling Bacon Lara savegame fix");
    }

    M_Reset(buffer);
    M_Skip(SAVEGAME_LEGACY_TITLE_SIZE); // level title
    M_Skip(sizeof(int32_t)); // save counter

    M_ReadResumeInfo(fp, game_info);
    g_Lara.holsters_gun_type = LGT_UNKNOWN;
    g_Lara.back_gun_type = LGT_UNKNOWN;

    Lara_InitialiseInventory(Game_GetCurrentLevel());
    SAVEGAME_LEGACY_ITEM_STATS item_stats = {};
    M_Read(&item_stats, sizeof(SAVEGAME_LEGACY_ITEM_STATS));
    Inv_AddItemNTimes(O_PICKUP_ITEM_1, item_stats.num_pickup1);
    Inv_AddItemNTimes(O_PICKUP_ITEM_2, item_stats.num_pickup2);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_1, item_stats.num_puzzle1);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_2, item_stats.num_puzzle2);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_3, item_stats.num_puzzle3);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_4, item_stats.num_puzzle4);
    Inv_AddItemNTimes(O_KEY_ITEM_1, item_stats.num_key1);
    Inv_AddItemNTimes(O_KEY_ITEM_2, item_stats.num_key2);
    Inv_AddItemNTimes(O_KEY_ITEM_3, item_stats.num_key3);
    Inv_AddItemNTimes(O_KEY_ITEM_4, item_stats.num_key4);
    Inv_AddItemNTimes(O_LEADBAR_ITEM, item_stats.num_leadbar);

    M_Read(&tmp32, sizeof(int32_t));
    if (tmp32) {
        Room_FlipMap();
    }

    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        M_Read(&tmp8, sizeof(int8_t));
        g_FlipMapTable[i] = tmp8 << 8;
    }

    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        M_Read(&object->flags, sizeof(int16_t));
    }

    Savegame_ProcessItemsBeforeLoad();

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->save_position) {
            M_Read(&item->pos.x, sizeof(int32_t));
            M_Read(&item->pos.y, sizeof(int32_t));
            M_Read(&item->pos.z, sizeof(int32_t));
            M_Read(&item->rot.x, sizeof(int16_t));
            M_Read(&item->rot.y, sizeof(int16_t));
            M_Read(&item->rot.z, sizeof(int16_t));
            M_Read(&tmp16, sizeof(int16_t));
            M_Read(&item->speed, sizeof(int16_t));
            M_Read(&item->fall_speed, sizeof(int16_t));

            if (item->room_num != tmp16) {
                Item_NewRoom(i, tmp16);
            }
        }

        if (M_ItemHasSaveAnim(item)) {
            M_Read(&item->current_anim_state, sizeof(int16_t));
            M_Read(&item->goal_anim_state, sizeof(int16_t));
            M_Read(&item->required_anim_state, sizeof(int16_t));
            M_Read(&item->anim_num, sizeof(int16_t));
            M_Read(&item->frame_num, sizeof(int16_t));

            if (item->object_id == O_LARA && item->anim_num < obj->anim_idx) {
                item->anim_num += obj->anim_idx;
            }
        }

        if (M_ItemHasHitPoints(item)) {
            M_Read(&item->hit_points, sizeof(int16_t));
        }

        if ((item->object_id != O_BACON_LARA || !skip_reading_bacon_lara)
            && M_ItemHasSaveFlags(obj, item)) {
            M_Read(&item->flags, sizeof(int16_t));
            M_Read(&item->timer, sizeof(int16_t));

            if (item->flags & IF_KILLED) {
                Item_Kill(i);
                item->status = IS_DEACTIVATED;
            } else {
                if ((item->flags & 1) && !item->active) {
                    Item_AddActive(i);
                }
                item->status = (item->flags & 6) >> 1;
                if (item->flags & 8) {
                    item->gravity = 1;
                }
                if (!(item->flags & 16)) {
                    item->collidable = 0;
                }
            }

            if (item->flags & SAVE_CREATURE) {
                LOT_EnableBaddieAI(i, 1);
                CREATURE *creature = item->data;
                if (creature) {
                    M_Read(&creature->head_rotation, sizeof(int16_t));
                    M_Read(&creature->neck_rotation, sizeof(int16_t));
                    M_Read(&creature->maximum_turn, sizeof(int16_t));
                    M_Read(&creature->flags, sizeof(int16_t));
                    M_Read(&creature->mood, sizeof(int32_t));
                } else {
                    M_Skip(4 * 2 + 4);
                }
            } else if (obj->intelligent) {
                item->data = nullptr;
            }
        }

        Carrier_TestLegacyDrops(i);
    }

    M_ReadLara(&g_Lara);
    M_Read(&g_FlipEffect, sizeof(int32_t));
    M_Read(&g_FlipTimer, sizeof(int32_t));
    Memory_FreePointer(&buffer);
    return true;
}

bool Savegame_Legacy_LoadOnlyResumeInfo(MYFILE *fp, GAME_INFO *game_info)
{
    ASSERT(game_info != nullptr);

    char *buffer = Memory_Alloc(File_Size(fp));
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, File_Size(fp));

    M_Skip(SAVEGAME_LEGACY_TITLE_SIZE); // level title
    M_Skip(sizeof(int32_t)); // save counter

    M_ReadResumeInfo(fp, game_info);

    Memory_FreePointer(&buffer);
    return true;
}

void Savegame_Legacy_SaveToFile(MYFILE *fp, GAME_INFO *game_info)
{
    ASSERT(game_info != nullptr);

    char *buffer = Memory_Alloc(SAVEGAME_LEGACY_MAX_BUFFER_SIZE);
    M_Reset(buffer);
    memset(m_SGBufPtr, 0, SAVEGAME_LEGACY_MAX_BUFFER_SIZE);

    const GF_LEVEL *const current_level = Game_GetCurrentLevel();

    char title[SAVEGAME_LEGACY_TITLE_SIZE];
    snprintf(title, SAVEGAME_LEGACY_TITLE_SIZE, "%s", current_level->title);
    M_Write(title, SAVEGAME_LEGACY_TITLE_SIZE);
    M_Write(&g_SaveCounter, sizeof(int32_t));

    ASSERT(game_info->current != nullptr);
    for (int i = 0; i < GF_GetLevelTable(GFLT_MAIN)->count; i++) {
        RESUME_INFO *current = &game_info->current[i];
        M_Write(&current->pistol_ammo, sizeof(uint16_t));
        M_Write(&current->magnum_ammo, sizeof(uint16_t));
        M_Write(&current->uzi_ammo, sizeof(uint16_t));
        M_Write(&current->shotgun_ammo, sizeof(uint16_t));
        M_Write(&current->num_medis, sizeof(uint8_t));
        M_Write(&current->num_big_medis, sizeof(uint8_t));
        M_Write(&current->num_scions, sizeof(uint8_t));
        M_Write(&current->gun_status, sizeof(int8_t));
        M_Write(&current->equipped_gun_type, sizeof(int8_t));
        uint16_t flags = 0;
        flags |= current->flags.available ? 1 : 0;
        flags |= current->flags.got_pistols ? 2 : 0;
        flags |= current->flags.got_magnums ? 4 : 0;
        flags |= current->flags.got_uzis ? 8 : 0;
        flags |= current->flags.got_shotgun ? 16 : 0;
        flags |= current->flags.costume ? 32 : 0;
        M_Write(&flags, sizeof(uint16_t));
    }

    M_Write(
        &game_info->current[current_level->num].stats.timer, sizeof(uint32_t));
    M_Write(
        &game_info->current[current_level->num].stats.kill_count,
        sizeof(uint32_t));
    M_Write(
        &game_info->current[current_level->num].stats.secret_flags,
        sizeof(uint16_t));
    M_Write(&current_level->num, sizeof(uint16_t));
    M_Write(
        &game_info->current[current_level->num].stats.pickup_count,
        sizeof(uint8_t));
    M_Write(&game_info->bonus_flag, sizeof(uint8_t));

    SAVEGAME_LEGACY_ITEM_STATS item_stats = {
        .num_pickup1 = Inv_RequestItem(O_PICKUP_ITEM_1),
        .num_pickup2 = Inv_RequestItem(O_PICKUP_ITEM_2),
        .num_puzzle1 = Inv_RequestItem(O_PUZZLE_ITEM_1),
        .num_puzzle2 = Inv_RequestItem(O_PUZZLE_ITEM_2),
        .num_puzzle3 = Inv_RequestItem(O_PUZZLE_ITEM_3),
        .num_puzzle4 = Inv_RequestItem(O_PUZZLE_ITEM_4),
        .num_key1 = Inv_RequestItem(O_KEY_ITEM_1),
        .num_key2 = Inv_RequestItem(O_KEY_ITEM_2),
        .num_key3 = Inv_RequestItem(O_KEY_ITEM_3),
        .num_key4 = Inv_RequestItem(O_KEY_ITEM_4),
        .num_leadbar = Inv_RequestItem(O_LEADBAR_ITEM),
        0
    };

    M_Write(&item_stats, sizeof(item_stats));

    M_Write(&g_FlipStatus, sizeof(int32_t));
    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        int8_t flag = g_FlipMapTable[i] >> 8;
        M_Write(&flag, sizeof(int8_t));
    }

    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        const OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        M_Write(&object->flags, sizeof(int16_t));
    }

    Savegame_ProcessItemsBeforeSave();

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->save_position) {
            M_Write(&item->pos.x, sizeof(int32_t));
            M_Write(&item->pos.y, sizeof(int32_t));
            M_Write(&item->pos.z, sizeof(int32_t));
            M_Write(&item->rot.x, sizeof(int16_t));
            M_Write(&item->rot.y, sizeof(int16_t));
            M_Write(&item->rot.z, sizeof(int16_t));
            M_Write(&item->room_num, sizeof(int16_t));
            M_Write(&item->speed, sizeof(int16_t));
            M_Write(&item->fall_speed, sizeof(int16_t));
        }

        if (M_ItemHasSaveAnim(item)) {
            M_Write(&item->current_anim_state, sizeof(int16_t));
            M_Write(&item->goal_anim_state, sizeof(int16_t));
            M_Write(&item->required_anim_state, sizeof(int16_t));
            M_Write(&item->anim_num, sizeof(int16_t));
            M_Write(&item->frame_num, sizeof(int16_t));
        }

        if (M_ItemHasHitPoints(item)) {
            M_Write(&item->hit_points, sizeof(int16_t));
        }

        if (M_ItemHasSaveFlags(obj, item)) {
            uint16_t flags = item->flags + item->active + (item->status << 1)
                + (item->gravity << 3) + (item->collidable << 4);
            if (obj->intelligent && item->data) {
                flags |= SAVE_CREATURE;
            }
            M_Write(&flags, sizeof(uint16_t));
            M_Write(&item->timer, sizeof(int16_t));
            if (flags & SAVE_CREATURE) {
                CREATURE *creature = item->data;
                M_Write(&creature->head_rotation, sizeof(int16_t));
                M_Write(&creature->neck_rotation, sizeof(int16_t));
                M_Write(&creature->maximum_turn, sizeof(int16_t));
                M_Write(&creature->flags, sizeof(int16_t));
                M_Write(&creature->mood, sizeof(int32_t));
            }
        }
    }

    M_WriteLara(&g_Lara);

    M_Write(&g_FlipEffect, sizeof(int32_t));
    M_Write(&g_FlipTimer, sizeof(int32_t));

    File_WriteData(fp, buffer, m_SGBufPos);
    Memory_FreePointer(&buffer);
}

bool Savegame_Legacy_UpdateDeathCounters(MYFILE *fp, GAME_INFO *game_info)
{
    return false;
}
