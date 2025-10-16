#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "global/types_decomp.h"

#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/carrier.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/music.h>
#include <libtrx/game/objects/general/lift.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/game/objects/traps/sliding_pillar.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/pathing.h>
#include <libtrx/game/stats.h>
#include <libtrx/memory.h>

#include <stdio.h>
#include <string.h>

#define M_SAVE_CREATURE (1 << 7)
#define M_SAVEGAME_LEGACY_TOTAL_SIZE (1170 + 6272) // header + OG buffer size
#define M_SAVEGAME_LEGACY_TITLE_SIZE 75
#define M_LEGACY_MAX_MUSIC_TRACKS 64

#define M_SPECIAL_READ_WRITES                                                  \
    X_SPECIAL_READ_WRITE(S8, int8_t)                                           \
    X_SPECIAL_READ_WRITE(S16, int16_t)                                         \
    X_SPECIAL_READ_WRITE(S32, int32_t)                                         \
    X_SPECIAL_READ_WRITE(U8, uint8_t)                                          \
    X_SPECIAL_READ_WRITE(U16, uint16_t)                                        \
    X_SPECIAL_READ_WRITE(U32, uint32_t)

#pragma pack(push, 1)
typedef struct {
    uint8_t num_pickup[2];
    uint8_t num_puzzle[4];
    uint8_t num_key[4];
    uint16_t reserved;
} SAVEGAME_LEGACY_ITEM_STATS;
#pragma pack(pop)

static int32_t m_BufPos = 0;
static char *m_BufPtr = nullptr;

static const char *M_GetSaveFilePattern(void);
static bool M_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
static void M_SaveToFile(MYFILE *fp, SAVEGAME_INFO *info);
static bool M_LoadFromFile(MYFILE *fp);

static SAVEGAME_STRATEGY m_Strategy = {
    // clang-format off
    .allow_load = true,
    .allow_save = false,
    .format = SAVEGAME_FORMAT_LEGACY,
    .get_save_file_pattern_func = M_GetSaveFilePattern,
    .fill_info_func = M_FillInfo,
    .load_from_file_func = M_LoadFromFile,
    .save_to_file_func = M_SaveToFile,
    .load_only_resume_info_func = nullptr,
    .update_death_counters_func = nullptr,
    // clang-format on
};

static bool M_ItemHasSaveFlags(const OBJECT *const obj, const ITEM *const item)
{
    return obj->save_flags && item->object_id != O_WATERFALL
        && item->object_id != O_DART;
}

static bool M_ItemHasSavePosition(
    const OBJECT *const obj, const ITEM *const item)
{
    return obj->save_position && item->object_id != O_GONDOLA;
}

static void M_Reset(char *const buffer)
{
    m_BufPos = 0;
    m_BufPtr = buffer;
}

static void M_Read(void *const ptr, const size_t size)
{
    ASSERT(m_BufPos + size <= M_SAVEGAME_LEGACY_TOTAL_SIZE);
    m_BufPos += size;
    memcpy(ptr, m_BufPtr, size);
    m_BufPtr += size;
}

#define X_SPECIAL_READ_WRITE(name, type)                                       \
    static type M_Read##name(void)                                             \
    {                                                                          \
        type result;                                                           \
        M_Read(&result, sizeof(type));                                         \
        return result;                                                         \
    }
M_SPECIAL_READ_WRITES
#undef X_SPECIAL_READ_WRITE

static void M_Write(const void *ptr, const size_t size)
{
    m_BufPos += size;
    if (m_BufPos >= M_SAVEGAME_LEGACY_TOTAL_SIZE) {
        Shell_ExitSystem("Savegame is too big to fit in buffer");
    }

    memcpy(m_BufPtr, ptr, size);
    m_BufPtr += size;
}

#define X_SPECIAL_READ_WRITE(name, type)                                       \
    static void M_Write##name(type value)                                      \
    {                                                                          \
        M_Write(&value, sizeof(type));                                         \
    }
M_SPECIAL_READ_WRITES
#undef X_SPECIAL_READ_WRITE

static void M_Skip(const size_t size)
{
    m_BufPos += size;
    m_BufPtr += size;
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

        if (M_ItemHasSavePosition(obj, item)) {
            item->pos.x = M_ReadS32();
            item->pos.y = M_ReadS32();
            item->pos.z = M_ReadS32();
            item->rot.x = M_ReadS16();
            item->rot.y = M_ReadS16();
            item->rot.z = M_ReadS16();
            int16_t room_num = M_ReadS16();
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
        case O_BOAT:
            M_Read(item->data, sizeof(BOAT_INFO));
            break;

        case O_SKIDOO_FAST:
            M_Read(item->data, sizeof(SKIDOO_INFO));
            break;

        case O_LIFT:
            M_Read(item->data, sizeof(LIFT_INFO));
            break;

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
        item->room_num = M_ReadS16();
        item->speed = M_ReadS16();
        item->fall_speed = M_ReadS16();
        Item_Initialise(item_num);
        Item_AddActive(item_num);
        const int32_t flare_age = M_ReadS32();
        item->data = (void *)(intptr_t)flare_age;
    }
}

static void M_WriteStats(const LEVEL_STATS *const stats)
{
    M_WriteU32(stats->timer);
    M_WriteU32(stats->ammo_used);
    M_WriteU32(stats->ammo_hits);
    M_WriteU32(stats->distance_travelled);
    M_WriteU16(stats->kill_count);
    M_WriteU8(stats->secret_flags & 0xFF);
    M_WriteU8(stats->medipacks_used * 2);
}

static void M_WriteResumeInfo(const RESUME_INFO *const resume)
{
    ASSERT(resume != nullptr);
    M_WriteU16(resume->pistol_ammo);
    M_WriteU16(resume->magnum_ammo);
    M_WriteU16(resume->uzi_ammo);
    M_WriteU16(resume->shotgun_ammo);
    M_WriteU16(resume->m16_ammo);
    M_WriteU16(resume->grenade_ammo);
    M_WriteU16(resume->harpoon_ammo);
    M_WriteU8(resume->small_medipacks);
    M_WriteU8(resume->large_medipacks);
    M_WriteU8(0); // legacy reserved value
    M_WriteU8(resume->flares);
    M_WriteU8(resume->gun_status);
    M_WriteU8(resume->equipped_gun_type);

    uint16_t flags = 0;
    // clang-format off
    if (resume->flags.available)   { flags |= 0x01; }
    if (resume->flags.has_pistols) { flags |= 0x02; }
    if (resume->flags.has_magnums) { flags |= 0x04; }
    if (resume->flags.has_uzis)    { flags |= 0x08; }
    if (resume->flags.has_shotgun) { flags |= 0x10; }
    if (resume->flags.has_m16)     { flags |= 0x20; }
    if (resume->flags.has_grenade) { flags |= 0x40; }
    if (resume->flags.has_harpoon) { flags |= 0x80; }
    // clang-format on
    M_WriteU16(flags);

    M_WriteU16(0);
    M_WriteStats(&resume->stats);
}

static void M_WriteResumeInfos(void)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < 24; i++) {
        if (i < level_table->count) {
            const GF_LEVEL *const level = &level_table->levels[i];
            M_WriteResumeInfo(Savegame_GetCurrentInfo(level));
        } else {
            const RESUME_INFO null_resume_info = {};
            M_WriteResumeInfo(&null_resume_info);
        }
    }
}

static void M_WriteItems(void)
{
    Savegame_ProcessItemsBeforeSave();

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        const ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (M_ItemHasSavePosition(obj, item)) {
            M_WriteS32(item->pos.x);
            M_WriteS32(item->pos.y);
            M_WriteS32(item->pos.z);
            M_WriteS16(item->rot.x);
            M_WriteS16(item->rot.y);
            M_WriteS16(item->rot.z);
            M_WriteS16(item->room_num);
            M_WriteS16(item->speed);
            M_WriteS16(item->fall_speed);
        }

        if (obj->save_anim) {
            M_WriteS16(item->current_anim_state);
            M_WriteS16(item->goal_anim_state);
            M_WriteS16(item->required_anim_state);
            M_WriteS16(item->anim_num);
            M_WriteS16(item->frame_num);
        }

        if (obj->save_hitpoints) {
            M_WriteS16(item->hit_points);
        }

        if (M_ItemHasSaveFlags(obj, item)) {
            uint16_t flags = item->flags + item->active + (item->status << 1)
                + (item->gravity << 3) + (item->collidable << 4);
            if (obj->intelligent && item->data != nullptr) {
                flags |= M_SAVE_CREATURE;
            }
            M_WriteU16(flags);

            const CARRIED_ITEM *const carried_item = item->carried_item;
            if (carried_item != nullptr) {
                M_WriteS16(carried_item->spawn_num);
            }

            M_WriteS16(item->timer);
            if ((flags & M_SAVE_CREATURE) != 0) {
                const CREATURE *const creature = item->data;
                M_WriteS16(creature->head_rotation);
                M_WriteS16(creature->neck_rotation);
                M_WriteS16(creature->maximum_turn);
                M_WriteS16(creature->flags);
                M_WriteS32(creature->mood);
            }
        }

        switch (item->object_id) {
        case O_BOAT:
            M_Write(item->data, sizeof(BOAT_INFO));
            break;

        case O_SKIDOO_FAST:
            M_Write(item->data, sizeof(SKIDOO_INFO));
            break;

        case O_LIFT:
            M_Write(item->data, sizeof(LIFT_INFO));
            break;

        default:
            break;
        }
    }
}

static void M_WriteLaraArm(const LARA_ARM *const arm)
{
    const int32_t frame_base = 0; // not required
    M_WriteS32(frame_base);
    M_WriteS16(arm->frame_num);
    M_WriteS16(arm->anim_num);
    M_WriteS16(arm->lock);
    M_WriteS16(arm->rot.y);
    M_WriteS16(arm->rot.x);
    M_WriteS16(arm->rot.z);
    M_WriteS16(arm->flash_gun);
}

static void M_WriteAmmoInfo(const AMMO_INFO *const ammo_info)
{
    M_WriteS32(ammo_info->ammo);
}

static void M_WriteLara(const LARA_INFO *const lara)
{
    M_WriteS16(lara->item_num);
    M_WriteS16(lara->gun_status);
    M_WriteS16(lara->gun_type);
    M_WriteS16(lara->request_gun_type);
    M_WriteS16(lara->last_gun_type);
    M_WriteS16(lara->calc_fall_speed);
    M_WriteS16(lara->water_status);
    M_WriteS16(lara->climb_status);
    M_WriteS16(lara->pose_count);
    M_WriteS16(lara->hit_frame);
    M_WriteS16(lara->hit_direction);
    M_WriteS16(lara->air);
    M_WriteS16(lara->dive_timer);
    M_WriteS16(lara->death_timer);
    M_WriteS16(lara->current_active);
    M_WriteS16(lara->hit_effect_count);
    M_WriteS16(lara->flare.age);
    M_WriteS16(Lara_Vehicle_GetIndex());
    M_WriteS16(lara->gun_item_num);
    M_WriteS16(Object_ToGameID(lara->back_gun_obj_id));
    M_WriteS16(lara->flare.frame_num);

    uint16_t flags = 0;
    // clang-format off
    if (lara->flare.control) { flags |= 1 << 0; }
    if (lara->extra_anim)    { flags |= 1 << 2; }
    if (lara->burn)          { flags |= 1 << 4; }
    // clang-format on
    M_WriteU16(flags);

    M_WriteS32(lara->water_surface_dist);
    M_WriteS32(lara->last_pos.x);
    M_WriteS32(lara->last_pos.y);
    M_WriteS32(lara->last_pos.z);
    M_Skip(4);
    M_WriteU32(lara->mesh_effects);

    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        const int32_t mesh_offset = Object_GetMeshOffset(Lara_Mesh_Get(i));
        M_WriteS32(mesh_offset * 2);
    }

    M_Skip(4);
    M_WriteS16(lara->target_angles[0]);
    M_WriteS16(lara->target_angles[1]);

    M_WriteS16(lara->turn_rate);
    M_WriteS16(lara->move_angle);
    M_WriteS16(lara->head_rot.y);
    M_WriteS16(lara->head_rot.x);
    M_WriteS16(lara->head_rot.z);
    M_WriteS16(lara->torso_rot.y);
    M_WriteS16(lara->torso_rot.x);
    M_WriteS16(lara->torso_rot.z);

    M_WriteLaraArm(&lara->left_arm);
    M_WriteLaraArm(&lara->right_arm);
    M_WriteAmmoInfo(&lara->pistol_ammo);
    M_WriteAmmoInfo(&lara->magnum_ammo);
    M_WriteAmmoInfo(&lara->uzi_ammo);
    M_WriteAmmoInfo(&lara->shotgun_ammo);
    M_WriteAmmoInfo(&lara->harpoon_ammo);
    M_WriteAmmoInfo(&lara->grenade_ammo);
    M_WriteAmmoInfo(&lara->m16_ammo);
    M_Skip(4);
}

static void M_WriteFlares(void)
{
    int32_t num_flares = 0;
    for (int32_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (item->active && item->object_id == O_FLARE_ITEM) {
            num_flares++;
        }
    }

    M_WriteS32(num_flares);
    for (int32_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (item->active && item->object_id == O_FLARE_ITEM) {
            M_WriteS32(item->pos.x);
            M_WriteS32(item->pos.y);
            M_WriteS32(item->pos.z);
            M_WriteS16(item->rot.x);
            M_WriteS16(item->rot.y);
            M_WriteS16(item->rot.z);
            M_WriteS16(item->room_num);
            M_WriteS16(item->speed);
            M_WriteS16(item->fall_speed);
            const int32_t flare_age = (intptr_t)item->data;
            M_WriteS32(flare_age);
        }
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

static void M_SaveToFile(MYFILE *const fp, SAVEGAME_INFO *const info)
{
    char *buffer = Memory_Alloc(M_SAVEGAME_LEGACY_TOTAL_SIZE);
    M_Reset(buffer);

    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    const RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(current_level);

    char title[M_SAVEGAME_LEGACY_TITLE_SIZE];
    snprintf(title, M_SAVEGAME_LEGACY_TITLE_SIZE, "%s", current_level->title);
    M_Write(title, M_SAVEGAME_LEGACY_TITLE_SIZE);
    M_WriteS32(Savegame_GetCounter());

    M_WriteResumeInfos();
    M_WriteStats(&current_info->stats);
    M_WriteS16(current_level->num);
    M_WriteU8(Game_IsBonusFlagSet(GBF_NGPLUS) ? 1 : 0);

    const SAVEGAME_LEGACY_ITEM_STATS item_stats = {
        .num_pickup[0] = Inv_RequestItem(O_PICKUP_ITEM_1),
        .num_pickup[1] = Inv_RequestItem(O_PICKUP_ITEM_2),
        .num_puzzle[0] = Inv_RequestItem(O_PUZZLE_ITEM_1),
        .num_puzzle[1] = Inv_RequestItem(O_PUZZLE_ITEM_2),
        .num_puzzle[2] = Inv_RequestItem(O_PUZZLE_ITEM_3),
        .num_puzzle[3] = Inv_RequestItem(O_PUZZLE_ITEM_4),
        .num_key[0] = Inv_RequestItem(O_KEY_ITEM_1),
        .num_key[1] = Inv_RequestItem(O_KEY_ITEM_2),
        .num_key[2] = Inv_RequestItem(O_KEY_ITEM_3),
        .num_key[3] = Inv_RequestItem(O_KEY_ITEM_4),
        .reserved = 0,
    };
    M_Write(&item_stats, sizeof(SAVEGAME_LEGACY_ITEM_STATS));

    M_WriteS32(Room_GetFlipStatus());
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        uint8_t tflag = Room_GetFlipSlotFlags(i) >> 8;
        M_WriteU8(tflag);
    }

    for (int32_t i = 0; i < M_LEGACY_MAX_MUSIC_TRACKS; i++) {
        M_WriteU16(Music_GetTrackFlags(i));
    }
    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        const OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        M_WriteS16(object->flags);
    }

    M_WriteItems();

    LARA_INFO *const lara = Lara_GetLaraInfo();
    M_WriteLara(lara);

    if (lara->gun_item_num != NO_ITEM) {
        const ITEM *const weapon_item = Item_Get(lara->gun_item_num);
        M_WriteS16(weapon_item->object_id);
        M_WriteS16(weapon_item->anim_num);
        M_WriteS16(weapon_item->frame_num);
        M_WriteS16(weapon_item->current_anim_state);
        M_WriteS16(weapon_item->goal_anim_state);
    }

    M_WriteS32(Room_GetFlipEffect());
    M_WriteS32(Room_GetFlipTimer());
    M_WriteS32(Creature_AreAlliesHostile());

    M_WriteFlares();

    File_WriteData(fp, buffer, M_SAVEGAME_LEGACY_TOTAL_SIZE);
    Memory_FreePointer(&buffer);
}

static bool M_LoadFromFile(MYFILE *const fp)
{
    char *buffer = Memory_Alloc(File_Size(fp));
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, File_Size(fp));

    M_Reset(buffer);
    M_Skip(M_SAVEGAME_LEGACY_TITLE_SIZE);
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

    SAVEGAME_LEGACY_ITEM_STATS item_stats = {};
    M_Read(&item_stats, sizeof(SAVEGAME_LEGACY_ITEM_STATS));

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

REGISTER_SAVEGAME_STRATEGY(m_Strategy)
