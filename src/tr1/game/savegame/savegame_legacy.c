#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/stats.h"

#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/carrier.h>
#include <libtrx/game/level.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/game/objects/traps/sliding_pillar.h>
#include <libtrx/game/objects/vars.h>
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

static int32_t m_SGBufPos = 0;
static char *m_SGBufPtr = nullptr;

static void M_Read(void *const ptr, const size_t size)
{
    ASSERT(m_SGBufPos + size <= SAVEGAME_LEGACY_MAX_BUFFER_SIZE);
    ASSERT(m_SGBufPtr != nullptr);
    m_SGBufPos += size;
    memcpy(ptr, m_SGBufPtr, size);
    m_SGBufPtr += size;
}

#define X_SPECIAL_READ(name, type)                                             \
    static type M_Read##name(void)                                             \
    {                                                                          \
        type result;                                                           \
        M_Read(&result, sizeof(type));                                         \
        return result;                                                         \
    }

#define L_SPECIAL_READS                                                        \
    X_SPECIAL_READ(S8, int8_t)                                                 \
    X_SPECIAL_READ(S16, int16_t)                                               \
    X_SPECIAL_READ(S32, int32_t)                                               \
    X_SPECIAL_READ(U8, uint8_t)                                                \
    X_SPECIAL_READ(U16, uint16_t)                                              \
    X_SPECIAL_READ(U32, uint32_t)

L_SPECIAL_READS
#undef X_SPECIAL_READ
#undef L_SPECIAL_READS

static const char *M_GetSaveFilePattern(void);
static bool M_FillInfo(MYFILE *fp, SAVEGAME_INFO *savegame_info);
static bool M_LoadFromFile(MYFILE *fp);
static bool M_LoadOnlyResumeInfo(MYFILE *fp);

static SAVEGAME_STRATEGY m_Strategy = {
    // clang-format off
    .allow_load = true,
    .allow_save = false,
    .format = SAVEGAME_FORMAT_LEGACY,
    .get_save_file_pattern_func = M_GetSaveFilePattern,
    .fill_info_func = M_FillInfo,
    .load_from_file_func = M_LoadFromFile,
    .load_only_resume_info_func = M_LoadOnlyResumeInfo,
    .save_to_file_func = nullptr,
    .update_death_counters_func = nullptr,
    // clang-format on
};

static void M_Reset(char *buffer)
{
    m_SGBufPos = 0;
    m_SGBufPtr = buffer;
}

static void M_Skip(const size_t size)
{
    m_SGBufPtr += size;
    m_SGBufPos += size; // missing from OG
}

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
    for (int32_t i = 0; i < GF_GetLevelTable(GFLT_MAIN)->count; i++) {
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

        ITEM tmp_item = {};

        if (obj->save_position) {
            tmp_item.pos.x = M_ReadS32();
            tmp_item.pos.y = M_ReadS32();
            tmp_item.pos.z = M_ReadS32();
            tmp_item.rot.x = M_ReadS16();
            tmp_item.rot.y = M_ReadS16();
            tmp_item.rot.z = M_ReadS16();
            M_Skip(sizeof(int16_t));
            tmp_item.speed = M_ReadS16();
            tmp_item.fall_speed = M_ReadS16();
        }
        if (M_ItemHasSaveAnim(item)) {
            tmp_item.current_anim_state = M_ReadS16();
            tmp_item.goal_anim_state = M_ReadS16();
            tmp_item.required_anim_state = M_ReadS16();
            tmp_item.anim_num = M_ReadS16();
            tmp_item.frame_num = M_ReadS16();
        }
        if (M_ItemHasHitPoints(item)) {
            tmp_item.hit_points = M_ReadS16();
        }
        if (M_ItemHasSaveFlags(obj, item)) {
            tmp_item.flags = M_ReadS16();
            tmp_item.timer = M_ReadS16();
            if (tmp_item.flags & SAVE_CREATURE) {
                CREATURE tmp_creature;
                tmp_creature.head_rotation = M_ReadS16();
                tmp_creature.neck_rotation = M_ReadS16();
                tmp_creature.maximum_turn = M_ReadS16();
                tmp_creature.flags = M_ReadS16();
                tmp_creature.mood = M_ReadS32();
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

static void M_ReadArm(LARA_ARM *const arm)
{
    M_Skip(sizeof(int32_t)); // frame_base is superfluous
    arm->frame_num = M_ReadS16();
    arm->lock = M_ReadS16();
    arm->rot.y = M_ReadS16();
    arm->rot.x = M_ReadS16();
    arm->rot.z = M_ReadS16();
    arm->flash_gun = M_ReadS16();
}

static void M_ReadAmmoInfo(AMMO_INFO *const ammo_info)
{
    ammo_info->ammo = M_ReadS32();
    M_Skip(sizeof(int32_t)); // Legacy hits value
    M_Skip(sizeof(int32_t)); // Legacy miss value
}

static void M_ReadLOT(LOT_INFO *const lot)
{
    M_Skip(4); // pointer to BOX_NODE
    lot->head = M_ReadS16();
    lot->tail = M_ReadS16();
    lot->search_num = M_ReadU16();
    lot->setup.block_mask = M_ReadU16();
    lot->setup.step = M_ReadS16();
    lot->setup.drop = M_ReadS16();
    lot->setup.fly = M_ReadS16();
    lot->zone_count = M_ReadS16();
    lot->target_box = M_ReadS16();
    lot->required_box = M_ReadS16();
    lot->target.x = M_ReadS32();
    lot->target.y = M_ReadS32();
    lot->target.z = M_ReadS32();
}

static void M_ReadLara(LARA_INFO *const lara)
{
    lara->item_num = M_ReadS16();
    lara->gun_status = M_ReadS16();
    lara->gun_type = M_ReadS16();
    lara->request_gun_type = M_ReadS16();
    lara->last_gun_type = lara->request_gun_type;
    lara->calc_fall_speed = M_ReadS16();
    lara->water_status = M_ReadS16();
    lara->pose_count = M_ReadS16();
    lara->hit_frame = M_ReadS16();
    lara->hit_direction = M_ReadS16();
    lara->air = M_ReadS16();
    lara->dive_timer = M_ReadS16();
    lara->death_timer = M_ReadS16();
    lara->current_active = M_ReadS16();
    lara->hit_effect_count = M_ReadS16();

    lara->hit_effect = nullptr;
    M_Skip(4); // pointer to EFFECT

    lara->mesh_effects = M_ReadS32();
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        OBJECT_MESH *const mesh = Object_FindMesh(M_ReadS32() / 2);
        if (mesh != nullptr) {
            Lara_Mesh_Set(i, mesh);
        }
    }

    lara->target = nullptr;
    M_Skip(4); // pointer to ITEM

    lara->target_angles[0] = M_ReadS16();
    lara->target_angles[1] = M_ReadS16();
    lara->turn_rate = M_ReadS16();
    lara->move_angle = M_ReadS16();
    lara->head_rot.y = M_ReadS16();
    lara->head_rot.x = M_ReadS16();
    lara->head_rot.z = M_ReadS16();
    lara->torso_rot.y = M_ReadS16();
    lara->torso_rot.x = M_ReadS16();
    lara->torso_rot.z = M_ReadS16();

    M_ReadArm(&lara->left_arm);
    M_ReadArm(&lara->right_arm);
    M_ReadAmmoInfo(&lara->pistol_ammo);
    M_ReadAmmoInfo(&lara->magnum_ammo);
    M_ReadAmmoInfo(&lara->uzi_ammo);
    M_ReadAmmoInfo(&lara->shotgun_ammo);
    M_ReadLOT(&lara->lot);
}

static void M_ReadResumeInfo(RESUME_INFO *const resume)
{
    resume->pistol_ammo = M_ReadU16();
    resume->magnum_ammo = M_ReadU16();
    resume->uzi_ammo = M_ReadU16();
    resume->shotgun_ammo = M_ReadU16();
    resume->small_medipacks = M_ReadU8();
    resume->large_medipacks = M_ReadU8();
    resume->num_scions = M_ReadU8();
    resume->gun_status = M_ReadS8();
    resume->equipped_gun_type = M_ReadS8();
    resume->holsters_gun_type = LGT_UNKNOWN;
    resume->back_gun_type = LGT_UNKNOWN;

    const uint16_t flags = M_ReadU16();
    // clang-format off
    resume->flags.available     = (flags & 0x01) != 0;
    resume->flags.has_pistols   = (flags & 0x02) != 0;
    resume->flags.has_magnums   = (flags & 0x04) != 0;
    resume->flags.has_uzis      = (flags & 0x08) != 0;
    resume->flags.has_shotgun   = (flags & 0x10) != 0;
    resume->flags.costume       = (flags & 0x20) != 0;
    // clang-format on
}

static void M_ReadResumeInfos(MYFILE *const fp)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < 22; i++) {
        if (i < level_table->count) {
            const GF_LEVEL *const level = &level_table->levels[i];
            M_ReadResumeInfo(Savegame_GetCurrentInfo(level));

            // Gym and first level have special starting items.
            if (level == GF_GetFirstLevel() || level == GF_GetGymLevel()) {
                Savegame_ApplyLogicToCurrentInfo(level);
            }
        } else {
            RESUME_INFO dummy_resume_info;
            M_ReadResumeInfo(&dummy_resume_info);
        }
    }

    const uint32_t temp_timer = M_ReadU32();
    const uint32_t temp_kill_count = M_ReadU32();
    const uint16_t temp_secret_flags = M_ReadU16();
    const uint16_t current_level_num = M_ReadU16();
    const uint8_t temp_pickup_count = M_ReadU8();
    const uint8_t temp_flags = M_ReadU8();

    const GF_LEVEL *current_level = Game_GetCurrentLevel();
    if (current_level != nullptr) {
        RESUME_INFO *const resume_info = Savegame_GetCurrentInfo(current_level);
        resume_info->stats.timer = temp_timer;
        resume_info->stats.kill_count = temp_kill_count;
        resume_info->stats.secret_flags = temp_secret_flags;
        Stats_UpdateSecrets(&resume_info->stats);
        resume_info->stats.pickup_count = temp_pickup_count;
        resume_info->stats.death_count = -1;
    }

    const bool is_ng_plus = temp_flags != 0;
    if (is_ng_plus) {
        Game_SetBonusFlag(GBF_NGPLUS);
    }
}

static const char *M_GetSaveFilePattern(void)
{
    return g_GameFlow.savegame_fmt_legacy;
}

static bool M_FillInfo(MYFILE *const fp, SAVEGAME_INFO *const info)
{
    File_Seek(fp, 0, SEEK_SET);

    char title[SAVEGAME_LEGACY_TITLE_SIZE];
    File_ReadItems(fp, title, sizeof(char), SAVEGAME_LEGACY_TITLE_SIZE);
    info->level_title = Memory_DupStr(title);

    int32_t counter;
    counter = File_ReadS32(fp);
    info->counter = counter;

    for (int32_t i = 0; i < GF_GetLevelTable(GFLT_MAIN)->count; i++) {
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

static bool M_LoadFromFile(MYFILE *const fp)
{
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

    M_ReadResumeInfos(fp);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->holsters_gun_type = LGT_UNKNOWN;
    lara->back_gun_type = LGT_UNKNOWN;

    // Copy RESUME_INFO of "current position" level to the target level
    {
        const GF_LEVEL *const level = Game_GetCurrentLevel();
        const GF_LEVEL *current_position = nullptr;
        const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
        for (int32_t i = 0; i < level_table->count; i++) {
            const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
            if (level->type == GFL_CURRENT) {
                current_position = level;
            }
        }
        if (current_position != nullptr) {
            *Savegame_GetCurrentInfo(level) =
                *Savegame_GetCurrentInfo(current_position);
        }
    }

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

    if (M_ReadS32() != 0) {
        Room_FlipMap();
    }

    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        Room_SetFlipSlotFlags(i, M_ReadS8() << 8);
    }

    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        object->flags = M_ReadS16();
    }

    Savegame_ProcessItemsBeforeLoad();

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->save_position) {
            item->pos.x = M_ReadS32();
            item->pos.y = M_ReadS32();
            item->pos.z = M_ReadS32();
            item->rot.x = M_ReadS16();
            item->rot.y = M_ReadS16();
            item->rot.z = M_ReadS16();
            const int16_t room_num = M_ReadS16();
            item->speed = M_ReadS16();
            item->fall_speed = M_ReadS16();

            Item_UpdateRoom(i, room_num);
        }

        if (M_ItemHasSaveAnim(item)) {
            item->current_anim_state = M_ReadS16();
            item->goal_anim_state = M_ReadS16();
            item->required_anim_state = M_ReadS16();
            item->anim_num = M_ReadS16();
            item->frame_num = M_ReadS16();

            if (item->object_id == O_LARA
                && item->anim_num < LARA_ORIGINAL_ANIM_COUNT) {
                item->anim_num += obj->anim_idx;
            }
        }

        if (M_ItemHasHitPoints(item)) {
            item->hit_points = M_ReadS16();
        }

        if ((item->object_id != O_BACON_LARA || !skip_reading_bacon_lara)
            && M_ItemHasSaveFlags(obj, item)) {
            item->flags = M_ReadS16();
            item->timer = M_ReadS16();

            if (item->flags & IF_KILLED) {
                Item_Kill(i);
                item->status = IS_DEACTIVATED;
            } else {
                if ((item->flags & 1) && !item->active) {
                    Item_AddActive(i);
                }
                item->status = (item->flags & 6) >> 1;
                if (item->flags & 8) {
                    item->gravity = true;
                }
                if (!(item->flags & 16)) {
                    item->collidable = false;
                }
            }

            if (item->flags & SAVE_CREATURE) {
                LOT_EnableBaddieAI(i, 1);
                CREATURE *const creature = item->data;
                if (creature != nullptr) {
                    creature->head_rotation = M_ReadS16();
                    creature->neck_rotation = M_ReadS16();
                    creature->maximum_turn = M_ReadS16();
                    creature->flags = M_ReadS16();
                    creature->mood = M_ReadS32();
                } else {
                    M_Skip(4 * 2 + 4);
                }
            } else if (obj->intelligent) {
                item->data = nullptr;
            }
        }

        if (Object_IsType(item->object_id, g_MovableBlockObjects)) {
            MOVABLE_BLOCK_INFO *const data = item->data;
            data->linked.pos = item->pos;
            data->linked.room_num = item->room_num;
        } else if (item->object_id == O_SLIDING_PILLAR) {
            SLIDING_PILLAR_INFO *const data = item->data;
            data->linked.pos = item->pos;
            data->linked.room_num = item->room_num;
        }

        Carrier_TestLegacyDrops(i);
    }

    M_ReadLara(lara);
    Room_SetFlipEffect(M_ReadS32());
    Room_SetFlipTimer(M_ReadS32());
    Memory_FreePointer(&buffer);
    return true;
}

static bool M_LoadOnlyResumeInfo(MYFILE *const fp)
{
    char *buffer = Memory_Alloc(File_Size(fp));
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, File_Size(fp));

    M_Reset(buffer);
    M_Skip(SAVEGAME_LEGACY_TITLE_SIZE); // level title
    M_Skip(sizeof(int32_t)); // save counter

    M_ReadResumeInfos(fp);

    Memory_FreePointer(&buffer);
    return true;
}

REGISTER_SAVEGAME_STRATEGY(m_Strategy)
