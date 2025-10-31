#include "debug.h"
#include "game/camera.h"
#include "game/carrier.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/gun/rifle.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/level.h"
#include "game/music.h"
#include "game/objects/general/lift.h"
#include "game/objects/general/pickup.h"
#include "game/objects/traps/movable_block.h"
#include "game/objects/traps/sliding_pillar.h"
#include "game/objects/vars.h"
#include "game/objects/vehicles/boat.h"
#include "game/objects/vehicles/skidoo_common.h"
#include "game/pathing.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/stats.h"
#include "log.h"
#include "memory.h"
#include "utils.h"
#include "version.h"

#include <stdio.h>
#include <string.h>

#define M_LEGACY_TITLE_SIZE 75
#define M_SAVE_CREATURE (1 << 7)
#define M_LEGACY_NO_ROOM 255
#define M_LEGACY_MAX_MUSIC_TRACKS 64

#pragma pack(push, 1)
typedef struct {
    uint8_t num_pickup[2];
    uint8_t num_puzzle[4];
    uint8_t num_key[4];
#if TR_VERSION == 1
    uint8_t num_leadbar;
    uint8_t dummy;
#else
    uint16_t reserved;
#endif
} M_LEGACY_ITEM_STATS;
#pragma pack(pop)

static int32_t m_BufPos = 0;
static char *m_BufPtr = nullptr;

#if TR_VERSION == 1
    #define M_LEGACY_MAX_BUFFER_SIZE (20 * 1024)
#else
    #define M_LEGACY_MAX_BUFFER_SIZE (1170 + 6272) // header + OG buffer size
#endif

static void M_Reset(char *const buffer)
{
    m_BufPos = 0;
    m_BufPtr = buffer;
}

static void M_Read(void *const ptr, const size_t size)
{
    ASSERT(m_BufPos + size <= M_LEGACY_MAX_BUFFER_SIZE);
    ASSERT(m_BufPtr != nullptr);
    m_BufPos += size;
    memcpy(ptr, m_BufPtr, size);
    m_BufPtr += size;
}

static void M_Skip(const size_t size)
{
    m_BufPos += size;
    m_BufPtr += size;
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
    if (g_TRVersion == 1) {
        return (
            obj->save_flags && item->object_id != O_EMBER_EMITTER
            && item->object_id != O_FLAME_EMITTER
            && item->object_id != O_WATERFALL
            && item->object_id != O_SCION_ITEM_1
            && item->object_id != O_DART_EMITTER);
    } else {
        return obj->save_flags && item->object_id != O_WATERFALL
            && item->object_id != O_DART;
    }
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

static bool M_ItemHasSavePosition(const ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    if (g_TRVersion == 1) {
        return obj->save_position && obj->collision_func != Pickup_Collision;
    } else {
        return obj->save_position && item->object_id != O_GONDOLA;
    }
}

#if TR_VERSION == 1
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
    M_Skip(M_LEGACY_TITLE_SIZE); // level title
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
    M_Skip(sizeof(M_LEGACY_ITEM_STATS)); // item stats
    M_Skip(sizeof(int32_t)); // flipmap status
    M_Skip(MAX_FLIP_MAPS * sizeof(int8_t)); // flipmap table
    M_Skip(Camera_GetFixedObjectCount() * sizeof(int16_t)); // cameras

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        ITEM tmp_item = {};

        if (M_ItemHasSavePosition(item)) {
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
            if (tmp_item.flags & M_SAVE_CREATURE) {
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

    const bool has_rifle = Inv_RequestItem(O_SHOTGUN_ITEM) != 0;
    Gun_Rifle_LoadLegacy(has_rifle);
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

static void M_ReadResumeInfos(void)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < GF_GetLevelTable(GFLT_MAIN)->count; i++) {
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

    char title[M_LEGACY_TITLE_SIZE];
    File_ReadItems(fp, title, sizeof(char), M_LEGACY_TITLE_SIZE);
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
    M_Skip(M_LEGACY_TITLE_SIZE); // level title
    M_Skip(sizeof(int32_t)); // save counter

    M_ReadResumeInfos();

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
    M_LEGACY_ITEM_STATS item_stats = {};
    M_Read(&item_stats, sizeof(M_LEGACY_ITEM_STATS));
    Inv_AddItemNTimes(O_PICKUP_ITEM_1, item_stats.num_pickup[0]);
    Inv_AddItemNTimes(O_PICKUP_ITEM_2, item_stats.num_pickup[1]);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_1, item_stats.num_puzzle[0]);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_2, item_stats.num_puzzle[1]);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_3, item_stats.num_puzzle[2]);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_4, item_stats.num_puzzle[3]);
    Inv_AddItemNTimes(O_KEY_ITEM_1, item_stats.num_key[0]);
    Inv_AddItemNTimes(O_KEY_ITEM_2, item_stats.num_key[1]);
    Inv_AddItemNTimes(O_KEY_ITEM_3, item_stats.num_key[2]);
    Inv_AddItemNTimes(O_KEY_ITEM_4, item_stats.num_key[3]);
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

        if (M_ItemHasSavePosition(item)) {
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

            if (item->flags & M_SAVE_CREATURE) {
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

#else

static int16_t M_ReadRoomNum(void)
{
    const int16_t room_num = M_ReadS16();
    if (room_num == M_LEGACY_NO_ROOM) {
        return NO_ROOM;
    }
    return room_num;
}

static void M_ReadStats(LEVEL_STATS *const stats)
{
    stats->timer = M_ReadU32();
    stats->ammo_used = M_ReadU32();
    stats->ammo_hits = M_ReadU32();
    stats->distance_travelled = M_ReadU32();
    stats->kill_count = M_ReadU16();
    stats->secret_flags = M_ReadU8();
    stats->medipacks_used = M_ReadU8() / 2.0f;
    stats->death_count = -1;
    Stats_UpdateSecrets(stats);
}

static void M_ReadResumeInfo(RESUME_INFO *const resume)
{
    resume->pistol_ammo = M_ReadU16();
    resume->magnum_ammo = M_ReadU16();
    resume->uzi_ammo = M_ReadU16();
    resume->shotgun_ammo = M_ReadU16();
    resume->m16_ammo = M_ReadU16();
    resume->grenade_ammo = M_ReadU16();
    resume->harpoon_ammo = M_ReadU16();
    resume->small_medipacks = M_ReadU8();
    resume->large_medipacks = M_ReadU8();
    M_Skip(sizeof(uint8_t)); // legacy reserved value
    resume->flares = M_ReadU8();
    resume->gun_status = M_ReadU8();
    resume->equipped_gun_type = M_ReadU8();
    resume->holsters_gun_type = LGT_UNKNOWN;
    resume->back_gun_type = LGT_UNKNOWN;

    const uint16_t flags = M_ReadU16();
    // clang-format off
    resume->flags.available     = (flags & 0x01) != 0;
    resume->flags.has_pistols   = (flags & 0x02) != 0;
    resume->flags.has_magnums   = (flags & 0x04) != 0;
    resume->flags.has_uzis      = (flags & 0x08) != 0;
    resume->flags.has_shotgun   = (flags & 0x10) != 0;
    resume->flags.has_m16       = (flags & 0x20) != 0;
    resume->flags.has_grenade   = (flags & 0x40) != 0;
    resume->flags.has_harpoon   = (flags & 0x80) != 0;
    // clang-format on

    M_Skip(sizeof(uint16_t));
    M_ReadStats(&resume->stats);
}

static void M_ReadResumeInfos(void)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < 24; i++) {
        if (i < level_table->count) {
            const GF_LEVEL *const level = &level_table->levels[i];
            M_ReadResumeInfo(Savegame_GetCurrentInfo(level));
        } else {
            RESUME_INFO dummy_resume_info;
            M_ReadResumeInfo(&dummy_resume_info);
        }
    }
}

static void M_ReadItems(void)
{
    Savegame_ProcessItemsBeforeLoad();

    for (int32_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (M_ItemHasSavePosition(item)) {
            item->pos.x = M_ReadS32();
            item->pos.y = M_ReadS32();
            item->pos.z = M_ReadS32();
            item->rot.x = M_ReadS16();
            item->rot.y = M_ReadS16();
            item->rot.z = M_ReadS16();
            int16_t room_num = M_ReadRoomNum();
            item->speed = M_ReadS16();
            item->fall_speed = M_ReadS16();

            Item_UpdateRoom(item_num, room_num);
        }

        if (obj->save_anim) {
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

        if (obj->save_hitpoints) {
            item->hit_points = M_ReadS16();
        }

        if (M_ItemHasSaveFlags(obj, item)) {
            item->flags = M_ReadU16();

            if (obj->intelligent) {
                M_Skip(sizeof(int16_t)); // legacy carried item
            }
            item->timer = M_ReadS16();

            if (item->flags & IF_KILLED) {
                Item_Kill(item_num);
                item->status = IS_DEACTIVATED;
            } else {
                if ((item->flags & 1) && !item->active) {
                    Item_AddActive(item_num);
                }

                item->status = (item->flags & 6) >> 1;
                if (item->flags & 8) {
                    item->gravity = true;
                }
                if (!(item->flags & 0x10)) {
                    item->collidable = false;
                }
            }

            if ((item->flags & M_SAVE_CREATURE) != 0) {
                LOT_EnableBaddieAI(item_num, true);
                CREATURE *const creature = item->data;
                if (creature != nullptr) {
                    creature->head_rotation = M_ReadS16();
                    creature->neck_rotation = M_ReadS16();
                    creature->maximum_turn = M_ReadS16();
                    creature->flags = M_ReadS16();
                    creature->mood = M_ReadS32();
                } else {
                    M_Skip(12);
                }
            } else if (obj->intelligent) {
                item->data = nullptr;
                if (item->killed && item->hit_points <= 0
                    && !(item->flags & IF_KILLED)) {
                    item->next_active = Item_GetPrevActive();
                    Item_SetPrevActive(item_num);
                }
            }
        }

        switch (item->object_id) {
        case O_BOAT: {
            BOAT_INFO *const data = item->data;
            data->boat_turn = M_ReadS32();
            data->left_fallspeed = M_ReadS32();
            data->right_fallspeed = M_ReadS32();
            data->tilt_angle = M_ReadS16();
            data->extra_rotation = M_ReadS16();
            data->water = M_ReadS32();
            data->pitch = M_ReadS32();
            break;
        }

        case O_SKIDOO_FAST: {
            SKIDOO_INFO *const data = item->data;
            data->track_mesh = M_ReadS16();
            data->skidoo_turn = M_ReadS32();
            data->left_fallspeed = M_ReadS32();
            data->right_fallspeed = M_ReadS32();
            data->momentum_angle = M_ReadS16();
            data->extra_rotation = M_ReadS16();
            data->pitch = M_ReadS32();
            break;
        }

        case O_LIFT: {
            LIFT_INFO *const data = item->data;
            data->start_height = M_ReadS32();
            data->wait_time = M_ReadS32();
            break;
        }

        default:
            break;
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

        if (obj->handle_save_func != nullptr) {
            obj->handle_save_func(item, SAVEGAME_STAGE_AFTER_LOAD);
        }
    }
}

static void M_ReadLaraArm(LARA_ARM *const arm)
{
    M_ReadS32(); // arm frame_base is not required
    arm->frame_num = M_ReadS16();
    arm->anim_num = M_ReadS16();
    arm->lock = M_ReadS16();
    arm->rot.y = M_ReadS16();
    arm->rot.x = M_ReadS16();
    arm->rot.z = M_ReadS16();
    arm->flash_gun = M_ReadS16();
}

static void M_ReadAmmoInfo(AMMO_INFO *const ammo_info)
{
    ammo_info->ammo = M_ReadS32();
}

static void M_ReadLara(LARA_INFO *const lara)
{
    lara->item_num = M_ReadS16();
    lara->gun_status = M_ReadS16();
    lara->gun_type = M_ReadS16();
    lara->request_gun_type = M_ReadS16();
    lara->last_gun_type = M_ReadS16();
    lara->calc_fall_speed = M_ReadS16();
    lara->water_status = M_ReadS16();
    lara->climb_status = M_ReadS16();
    lara->pose_count = M_ReadS16();
    lara->hit_frame = M_ReadS16();
    lara->hit_direction = M_ReadS16();
    lara->air = M_ReadS16();
    lara->dive_timer = M_ReadS16();
    lara->death_timer = M_ReadS16();
    lara->current_active = M_ReadS16();
    lara->hit_effect_count = M_ReadS16();
    lara->flare.age = M_ReadS16();
    Lara_Vehicle_SetIndex(M_ReadS16());
    lara->gun_item_num = M_ReadS16();
    lara->back_gun_obj_id = Object_FromGameID(M_ReadS16());
    lara->flare.frame_num = M_ReadS16();

    const uint16_t flags = M_ReadU16();
    // clang-format off
    lara->flare.control = (flags & (1 << 0)) != 0;
    lara->extra_anim    = (flags & (1 << 2)) != 0;
    lara->burn          = (flags & (1 << 4)) != 0;
    // clang-format on

    lara->water_surface_dist = M_ReadS32();
    lara->last_pos.x = M_ReadS32();
    lara->last_pos.y = M_ReadS32();
    lara->last_pos.z = M_ReadS32();
    M_Skip(4);
    lara->hit_effect = nullptr;
    lara->mesh_effects = M_ReadU32();

    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        OBJECT_MESH *const mesh = Object_FindMesh(M_ReadS32() / 2);
        if (mesh != nullptr) {
            Lara_Mesh_Set(i, mesh);
        }
    }

    M_Skip(4);
    lara->target = nullptr;
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

    M_ReadLaraArm(&lara->left_arm);
    M_ReadLaraArm(&lara->right_arm);
    M_ReadAmmoInfo(&lara->pistol_ammo);
    M_ReadAmmoInfo(&lara->magnum_ammo);
    M_ReadAmmoInfo(&lara->uzi_ammo);
    M_ReadAmmoInfo(&lara->shotgun_ammo);
    M_ReadAmmoInfo(&lara->harpoon_ammo);
    M_ReadAmmoInfo(&lara->grenade_ammo);
    M_ReadAmmoInfo(&lara->m16_ammo);
    M_Skip(4);
}

static void M_ReadFlares(void)
{
    const int32_t num_flares = M_ReadS32();
    for (int32_t i = 0; i < num_flares; i++) {
        const int16_t item_num = Item_Create();
        ITEM *const item = Item_Get(item_num);
        item->object_id = O_FLARE_ITEM;
        item->pos.x = M_ReadS32();
        item->pos.y = M_ReadS32();
        item->pos.z = M_ReadS32();
        item->rot.x = M_ReadS16();
        item->rot.y = M_ReadS16();
        item->rot.z = M_ReadS16();
        item->room_num = M_ReadRoomNum();
        item->speed = M_ReadS16();
        item->fall_speed = M_ReadS16();
        Item_Initialise(item_num);
        Item_AddActive(item_num);
        const int32_t flare_age = M_ReadS32();
        item->data = (void *)(intptr_t)flare_age;
    }
}

static const char *M_GetSaveFilePattern(void)
{
    return g_GameFlow.savegame_fmt_legacy;
}

static bool M_FillInfo(MYFILE *const fp, SAVEGAME_INFO *const savegame_info)
{
    char level_title[75];
    File_ReadData(fp, level_title, 75);
    savegame_info->level_title = Memory_DupStr(level_title);
    savegame_info->counter = File_ReadS32(fp);

    for (int32_t i = 0; i < 24; i++) {
        File_Skip(fp, sizeof(uint16_t)); // pistol ammo
        File_Skip(fp, sizeof(uint16_t)); // magnum ammo
        File_Skip(fp, sizeof(uint16_t)); // uzi ammo
        File_Skip(fp, sizeof(uint16_t)); // shotgun ammo
        File_Skip(fp, sizeof(uint16_t)); // m16 ammo
        File_Skip(fp, sizeof(uint16_t)); // grenade ammo
        File_Skip(fp, sizeof(uint16_t)); // harpoon ammo
        File_Skip(fp, sizeof(uint8_t)); // small medis
        File_Skip(fp, sizeof(uint8_t)); // big medis
        File_Skip(fp, sizeof(uint8_t)); // reserved
        File_Skip(fp, sizeof(uint8_t)); // flares
        File_Skip(fp, sizeof(int8_t)); // gun status
        File_Skip(fp, sizeof(int8_t)); // gun type
        File_Skip(fp, sizeof(uint16_t)); // flags
        File_Skip(fp, sizeof(uint16_t)); // unused
        File_Skip(fp, sizeof(uint32_t)); // timer
        File_Skip(fp, sizeof(uint32_t)); // ammo used
        File_Skip(fp, sizeof(uint32_t)); // hits
        File_Skip(fp, sizeof(uint32_t)); // distance
        File_Skip(fp, sizeof(uint16_t)); // kills
        File_Skip(fp, sizeof(uint8_t)); // secret flags
        File_Skip(fp, sizeof(uint8_t)); // medis used
    }

    File_Skip(fp, sizeof(uint32_t)); // timer
    File_Skip(fp, sizeof(uint32_t)); // ammo used
    File_Skip(fp, sizeof(uint32_t)); // hits
    File_Skip(fp, sizeof(uint32_t)); // distance
    File_Skip(fp, sizeof(uint16_t)); // kills
    File_Skip(fp, sizeof(uint8_t)); // secret flags
    File_Skip(fp, sizeof(uint8_t)); // medis used

    savegame_info->level_num = File_ReadS16(fp);
    savegame_info->initial_version = VERSION_LEGACY;
    savegame_info->features.restart = false;
    savegame_info->features.select_level = false;

    return true;
}

static bool M_LoadFromFile(MYFILE *const fp)
{
    char *buffer = Memory_Alloc(File_Size(fp));
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, File_Size(fp));

    M_Reset(buffer);
    M_Skip(M_LEGACY_TITLE_SIZE);
    M_Skip(sizeof(int32_t)); // save counter

    M_ReadResumeInfos();
    {
        LEVEL_STATS current_stats = {};
        M_ReadStats(&current_stats);
        const int16_t current_level = M_ReadS16();
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, current_level);
        RESUME_INFO *const current_info = Savegame_GetCurrentInfo(level);
        current_info->stats = current_stats;
    }

    const bool is_ng_plus = M_ReadU8() != 0;
    if (is_ng_plus) {
        Game_SetBonusFlag(GBF_NGPLUS);
    }

    M_LEGACY_ITEM_STATS item_stats = {};
    M_Read(&item_stats, sizeof(M_LEGACY_ITEM_STATS));

    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    Lara_InitialiseInventory(current_level);
    Inv_AddItemNTimes(O_PICKUP_ITEM_1, item_stats.num_pickup[0]);
    Inv_AddItemNTimes(O_PICKUP_ITEM_2, item_stats.num_pickup[1]);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_1, item_stats.num_puzzle[0]);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_2, item_stats.num_puzzle[1]);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_3, item_stats.num_puzzle[2]);
    Inv_AddItemNTimes(O_PUZZLE_ITEM_4, item_stats.num_puzzle[3]);
    Inv_AddItemNTimes(O_KEY_ITEM_1, item_stats.num_key[0]);
    Inv_AddItemNTimes(O_KEY_ITEM_2, item_stats.num_key[1]);
    Inv_AddItemNTimes(O_KEY_ITEM_3, item_stats.num_key[2]);
    Inv_AddItemNTimes(O_KEY_ITEM_4, item_stats.num_key[3]);

    if (M_ReadS32()) {
        Room_FlipMap();
    }

    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        Room_SetFlipSlotFlags(i, M_ReadS8() << 8);
    }

    for (int32_t i = 0; i < M_LEGACY_MAX_MUSIC_TRACKS; i++) {
        const int32_t track_id = Music_ConvertLegacyTrack(i);
        Music_SetTrackFlags(track_id, M_ReadU16());
    }

    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        object->flags = M_ReadS16();
    }

    M_ReadItems();

    LARA_INFO *const lara = Lara_GetLaraInfo();
    M_ReadLara(lara);

    if (lara->gun_item_num != NO_ITEM) {
        lara->gun_item_num = Item_Create();

        ITEM *const weapon_item = Item_Get(lara->gun_item_num);
        weapon_item->object_id = Object_FromGameID(M_ReadS16());
        weapon_item->anim_num = M_ReadS16();
        weapon_item->frame_num = M_ReadS16();
        weapon_item->current_anim_state = M_ReadS16();
        weapon_item->goal_anim_state = M_ReadS16();
        weapon_item->status = IS_ACTIVE;
        weapon_item->room_num = NO_ROOM;
    }

    Room_SetFlipEffect(M_ReadS32());
    Room_SetFlipTimer(M_ReadS32());
    Creature_SetAlliesHostile(M_ReadS32() != 0);

    M_ReadFlares();

    Memory_FreePointer(&buffer);
    return true;
}

#endif

static bool M_LoadOnlyResumeInfo(MYFILE *const fp)
{
    char *buffer = Memory_Alloc(File_Size(fp));
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, File_Size(fp));
    M_Reset(buffer);
    M_Skip(M_LEGACY_TITLE_SIZE); // level title
    M_Skip(sizeof(int32_t)); // save counter
    M_ReadResumeInfos();
    Memory_FreePointer(&buffer);
    return true;
}

REGISTER_SAVEGAME_STRATEGY(m_Strategy)
