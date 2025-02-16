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

static int32_t m_SGBufPos = 0;
static char *m_SGBufPtr = nullptr;

static bool M_ItemHasSaveFlags(const OBJECT *obj, ITEM *item);
static bool M_ItemHasSaveAnim(const ITEM *item);
static bool M_ItemHasHitPoints(const ITEM *item);
static bool M_NeedsBaconLaraFix(char *buffer);

static void M_Reset(char *buffer);
static void M_Skip(size_t size);

static void M_Read(void *pointer, size_t size);

#define SPECIAL_READ(name, type)                                               \
    static type M_Read##name(void)                                             \
    {                                                                          \
        type result;                                                           \
        M_Read(&result, sizeof(type));                                         \
        return result;                                                         \
    }

#define SPECIAL_READS                                                          \
    SPECIAL_READ(S8, int8_t)                                                   \
    SPECIAL_READ(S16, int16_t)                                                 \
    SPECIAL_READ(S32, int32_t)                                                 \
    SPECIAL_READ(U8, uint8_t)                                                  \
    SPECIAL_READ(U16, uint16_t)                                                \
    SPECIAL_READ(U32, uint32_t)

SPECIAL_READS
#undef SPECIAL_READ
#undef SPECIAL_READS

static void M_ReadArm(LARA_ARM *arm);
static void M_ReadAmmoInfo(AMMO_INFO *ammo_info);
static void M_ReadLara(LARA_INFO *lara);
static void M_ReadLOT(LOT_INFO *lot);
static void M_SetCurrentPosition(int32_t level_num);
static void M_ReadResumeInfo(MYFILE *fp, GAME_INFO *game_info);

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

        ITEM tmp_item;

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

static void M_Read(void *const ptr, const size_t size)
{
    ASSERT(m_SGBufPos + size <= SAVEGAME_LEGACY_MAX_BUFFER_SIZE);
    m_SGBufPos += size;
    memcpy(ptr, m_SGBufPtr, size);
    m_SGBufPtr += size;
}

static void M_ReadLara(LARA_INFO *const lara)
{
    lara->item_num = M_ReadS16();
    lara->gun_status = M_ReadS16();
    lara->gun_type = M_ReadS16();
    lara->request_gun_type = M_ReadS16();
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
            Lara_SetMesh(i, mesh);
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
    M_ReadAmmoInfo(&lara->pistols);
    M_ReadAmmoInfo(&lara->magnums);
    M_ReadAmmoInfo(&lara->uzis);
    M_ReadAmmoInfo(&lara->shotgun);
    M_ReadLOT(&lara->lot);
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
    ammo_info->hit = M_ReadS32();
    ammo_info->miss = M_ReadS32();
}

static void M_ReadLOT(LOT_INFO *const lot)
{
    M_Skip(4); // pointer to BOX_NODE
    lot->head = M_ReadS16();
    lot->tail = M_ReadS16();
    lot->search_num = M_ReadU16();
    lot->block_mask = M_ReadU16();
    lot->step = M_ReadS16();
    lot->drop = M_ReadS16();
    lot->fly = M_ReadS16();
    lot->zone_count = M_ReadS16();
    lot->target_box = M_ReadS16();
    lot->required_box = M_ReadS16();
    lot->target.x = M_ReadS32();
    lot->target.y = M_ReadS32();
    lot->target.z = M_ReadS32();
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

static void M_ReadResumeInfo(MYFILE *const fp, GAME_INFO *const game_info)
{
    ASSERT(game_info->current != nullptr);
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        RESUME_INFO *const current = Savegame_GetCurrentInfo(level);
        current->pistol_ammo = M_ReadU16();
        current->magnum_ammo = M_ReadU16();
        current->uzi_ammo = M_ReadU16();
        current->shotgun_ammo = M_ReadU16();
        current->num_medis = M_ReadU8();
        current->num_big_medis = M_ReadU8();
        current->num_scions = M_ReadU8();
        current->gun_status = M_ReadS8();
        current->equipped_gun_type = M_ReadS8();
        current->holsters_gun_type = LGT_UNKNOWN;
        current->back_gun_type = LGT_UNKNOWN;

        const uint16_t flags = M_ReadU16();
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

    const uint32_t temp_timer = M_ReadU32();
    const uint32_t temp_kill_count = M_ReadU32();
    const uint16_t temp_secret_flags = M_ReadU16();
    const uint16_t current_level = M_ReadU16();
    M_SetCurrentPosition(current_level);

    RESUME_INFO *const resume_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    resume_info->stats.timer = temp_timer;
    resume_info->stats.kill_count = temp_kill_count;
    resume_info->stats.secret_flags = temp_secret_flags;
    Stats_UpdateSecrets(&resume_info->stats);
    resume_info->stats.pickup_count = M_ReadU8();
    game_info->bonus_flag = M_ReadU8();
    game_info->death_count = -1;
}

char *Savegame_Legacy_GetSaveFileName(const int32_t slot)
{
    const size_t out_size =
        snprintf(nullptr, 0, g_GameFlow.savegame_fmt_legacy, slot) + 1;
    char *out = Memory_Alloc(out_size);
    snprintf(out, out_size, g_GameFlow.savegame_fmt_legacy, slot);
    return out;
}

bool Savegame_Legacy_FillInfo(MYFILE *const fp, SAVEGAME_INFO *const info)
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

bool Savegame_Legacy_LoadFromFile(MYFILE *const fp, GAME_INFO *const game_info)
{
    ASSERT(game_info != nullptr);

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

            if (item->room_num != room_num) {
                Item_NewRoom(i, room_num);
            }
        }

        if (M_ItemHasSaveAnim(item)) {
            item->current_anim_state = M_ReadS16();
            item->goal_anim_state = M_ReadS16();
            item->required_anim_state = M_ReadS16();
            item->anim_num = M_ReadS16();
            item->frame_num = M_ReadS16();

            if (item->object_id == O_LARA && item->anim_num < obj->anim_idx) {
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
                    item->gravity = 1;
                }
                if (!(item->flags & 16)) {
                    item->collidable = 0;
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

        Carrier_TestLegacyDrops(i);
    }

    M_ReadLara(&g_Lara);
    Room_SetFlipEffect(M_ReadS32());
    Room_SetFlipTimer(M_ReadS32());
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

bool Savegame_Legacy_UpdateDeathCounters(
    MYFILE *const fp, GAME_INFO *const game_info)
{
    return false;
}
