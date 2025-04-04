#include "decomp/savegame.h"

#include "game/camera.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/lot.h"
#include "game/objects/general/lift.h"
#include "game/requester.h"
#include "game/room.h"
#include "game/shell.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/music.h>
#include <libtrx/memory.h>

#include <stdio.h>
#include <string.h>

#define SAVE_CREATURE (1 << 7)
#define SAVEGAME_LEGACY_TOTAL_SIZE (1170 + 6272) // header + OG buffer size
#define SAVEGAME_LEGACY_TITLE_SIZE 75

#define SPECIAL_READ_WRITES                                                    \
    SPECIAL_READ_WRITE(S8, int8_t)                                             \
    SPECIAL_READ_WRITE(S16, int16_t)                                           \
    SPECIAL_READ_WRITE(S32, int32_t)                                           \
    SPECIAL_READ_WRITE(U8, uint8_t)                                            \
    SPECIAL_READ_WRITE(U16, uint16_t)                                          \
    SPECIAL_READ_WRITE(U32, uint32_t)

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
static uint32_t m_ReqFlags1[MAX_REQUESTER_ITEMS];
static uint32_t m_ReqFlags2[MAX_REQUESTER_ITEMS];

static void M_Reset(char *buffer);

static void M_Read(void *ptr, size_t size);
#undef SPECIAL_READ_WRITE
#define SPECIAL_READ_WRITE(name, type) static type M_Read##name(void);
SPECIAL_READ_WRITES
static void M_Skip(size_t size);
static void M_ReadResumeInfo(RESUME_INFO *resume);
static void M_ReadResumeInfos(void);
static void M_ReadStats(LEVEL_STATS *const stats);
static void M_ReadItems(void);
static void M_ReadLara(LARA_INFO *lara);
static void M_ReadLaraArm(LARA_ARM *arm);
static void M_ReadAmmoInfo(AMMO_INFO *ammo_info);
static void M_ReadFlares(void);

static void M_Write(const void *ptr, size_t size);
#undef SPECIAL_READ_WRITE
#define SPECIAL_READ_WRITE(name, type) static void M_Write##name(type value);
SPECIAL_READ_WRITES
static void M_WriteResumeInfo(const RESUME_INFO *resume);
static void M_WriteResumeInfos(void);
static void M_WriteStats(const LEVEL_STATS *stats);
static void M_WriteItems(void);
static void M_WriteLara(const LARA_INFO *lara);
static void M_WriteLaraArm(const LARA_ARM *arm);
static void M_WriteAmmoInfo(const AMMO_INFO *ammo_info);
static void M_WriteFlares(void);

static void M_Reset(char *const buffer)
{
    m_BufPos = 0;
    m_BufPtr = buffer;
}

static void M_Read(void *const ptr, const size_t size)
{
    ASSERT(m_BufPos + size <= SAVEGAME_LEGACY_TOTAL_SIZE);
    m_BufPos += size;
    memcpy(ptr, m_BufPtr, size);
    m_BufPtr += size;
}

#undef SPECIAL_READ_WRITE
#define SPECIAL_READ_WRITE(name, type)                                         \
    static type M_Read##name(void)                                             \
    {                                                                          \
        type result;                                                           \
        M_Read(&result, sizeof(type));                                         \
        return result;                                                         \
    }
SPECIAL_READ_WRITES

#undef SPECIAL_READ_WRITE
#define SPECIAL_READ_WRITE(name, type)                                         \
    static void M_Write##name(type value)                                      \
    {                                                                          \
        M_Write(&value, sizeof(type));                                         \
    }
SPECIAL_READ_WRITES

static void M_Skip(const size_t size)
{
    m_BufPos += size;
    m_BufPtr += size;
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
    resume->reserved1 = M_ReadU8();
    resume->flares = M_ReadU8();
    resume->gun_status = M_ReadU8();
    resume->gun_type = M_ReadU8();

    const uint16_t flags = M_ReadU16();
    // clang-format off
    resume->available     = (flags & 0x01) ? 1 : 0;
    resume->has_pistols   = (flags & 0x02) ? 1 : 0;
    resume->has_magnums   = (flags & 0x04) ? 1 : 0;
    resume->has_uzis      = (flags & 0x08) ? 1 : 0;
    resume->has_shotgun   = (flags & 0x10) ? 1 : 0;
    resume->has_m16       = (flags & 0x20) ? 1 : 0;
    resume->has_grenade   = (flags & 0x40) ? 1 : 0;
    resume->has_harpoon   = (flags & 0x80) ? 1 : 0;
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

static void M_ReadStats(LEVEL_STATS *const stats)
{
    stats->timer = M_ReadU32();
    stats->ammo_used = M_ReadU32();
    stats->ammo_hits = M_ReadU32();
    stats->distance = M_ReadU32();
    stats->kills = M_ReadU16();
    stats->secret_flags = M_ReadU8();
    stats->medipacks = M_ReadU8();
}

static void M_ReadItems(void)
{
    Savegame_ProcessItemsBeforeLoad();

    for (int32_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->save_position) {
            item->pos.x = M_ReadS32();
            item->pos.y = M_ReadS32();
            item->pos.z = M_ReadS32();
            item->rot.x = M_ReadS16();
            item->rot.y = M_ReadS16();
            item->rot.z = M_ReadS16();
            int16_t room_num = M_ReadS16();
            item->speed = M_ReadS16();
            item->fall_speed = M_ReadS16();

            if (item->room_num != room_num) {
                Item_NewRoom(item_num, room_num);
            }
        }

        if (obj->save_anim) {
            item->current_anim_state = M_ReadS16();
            item->goal_anim_state = M_ReadS16();
            item->required_anim_state = M_ReadS16();
            item->anim_num = M_ReadS16();
            item->frame_num = M_ReadS16();
        }

        if (obj->save_hitpoints) {
            item->hit_points = M_ReadS16();
        }

        if (obj->save_flags) {
            item->flags = M_ReadU16();

            if (obj->intelligent) {
                item->carried_item = M_ReadS16();
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
                    item->gravity = 1;
                }
                if (!(item->flags & 0x10)) {
                    item->collidable = 0;
                }
            }

            if (item->flags & SAVE_CREATURE) {
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
        }
    }
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
    lara->dive_count = M_ReadS16();
    lara->death_timer = M_ReadS16();
    lara->current_active = M_ReadS16();
    lara->hit_effect_count = M_ReadS16();
    lara->flare_age = M_ReadS16();
    lara->skidoo = M_ReadS16();
    lara->weapon_item = M_ReadS16();
    lara->back_gun = M_ReadS16();
    lara->flare_frame = M_ReadS16();

    const uint16_t flags = M_ReadU16();
    // clang-format off
    lara->flare_control_left  = flags >> 0;
    lara->flare_control_right = flags >> 1;
    lara->extra_anim          = flags >> 2;
    lara->look                = flags >> 3;
    lara->burn                = flags >> 4;
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
            Lara_SetMesh(i, mesh);
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
    lara->creature = nullptr;
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

static void M_Write(const void *ptr, const size_t size)
{
    m_BufPos += size;
    if (m_BufPos >= SAVEGAME_LEGACY_TOTAL_SIZE) {
        Shell_ExitSystem("Savegame is too big to fit in buffer");
    }

    memcpy(m_BufPtr, ptr, size);
    m_BufPtr += size;
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
    M_WriteU8(resume->reserved1);
    M_WriteU8(resume->flares);
    M_WriteU8(resume->gun_status);
    M_WriteU8(resume->gun_type);

    uint16_t flags = 0;
    // clang-format off
    if (resume->available)   { flags |= 0x01; }
    if (resume->has_pistols) { flags |= 0x02; }
    if (resume->has_magnums) { flags |= 0x04; }
    if (resume->has_uzis)    { flags |= 0x08; }
    if (resume->has_shotgun) { flags |= 0x10; }
    if (resume->has_m16)     { flags |= 0x20; }
    if (resume->has_grenade) { flags |= 0x40; }
    if (resume->has_harpoon) { flags |= 0x80; }
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

static void M_WriteStats(const LEVEL_STATS *const stats)
{
    M_WriteU32(stats->timer);
    M_WriteU32(stats->ammo_used);
    M_WriteU32(stats->ammo_hits);
    M_WriteU32(stats->distance);
    M_WriteU16(stats->kills);
    M_WriteU8(stats->secret_flags);
    M_WriteU8(stats->medipacks);
}

static void M_WriteItems(void)
{
    Savegame_ProcessItemsBeforeSave();

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        const ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->save_position) {
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

        if (obj->save_flags) {
            uint16_t flags = item->flags + item->active + (item->status << 1)
                + (item->gravity << 3) + (item->collidable << 4);
            if (obj->intelligent && item->data != nullptr) {
                flags |= SAVE_CREATURE;
            }
            M_WriteU16(flags);
            if (obj->intelligent) {
                M_WriteS16(item->carried_item);
            }

            M_WriteS16(item->timer);
            if (flags & SAVE_CREATURE) {
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
        }
    }
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
    M_WriteS16(lara->dive_count);
    M_WriteS16(lara->death_timer);
    M_WriteS16(lara->current_active);
    M_WriteS16(lara->hit_effect_count);
    M_WriteS16(lara->flare_age);
    M_WriteS16(lara->skidoo);
    M_WriteS16(lara->weapon_item);
    M_WriteS16(lara->back_gun);
    M_WriteS16(lara->flare_frame);

    uint16_t flags = 0;
    // clang-format off
    if (lara->flare_control_left)  { flags |= 1 << 0; }
    if (lara->flare_control_right) { flags |= 1 << 1; }
    if (lara->extra_anim)          { flags |= 1 << 2; }
    if (lara->look)                { flags |= 1 << 3; }
    if (lara->burn)                { flags |= 1 << 4; }
    // clang-format on
    M_WriteU16(flags);

    M_WriteS32(lara->water_surface_dist);
    M_WriteS32(lara->last_pos.x);
    M_WriteS32(lara->last_pos.y);
    M_WriteS32(lara->last_pos.z);
    M_Skip(4);
    M_WriteU32(lara->mesh_effects);

    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        const int32_t mesh_offset = Object_GetMeshOffset(Lara_GetMesh(i));
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

void Savegame_ResetCurrentInfo(const GF_LEVEL *const level)
{
    RESUME_INFO *const current = Savegame_GetCurrentInfo(level);
    memset(current, 0, sizeof(RESUME_INFO));
}

void Savegame_InitCurrentInfo(void)
{
    if (Game_IsBonusFlagSet(GBF_NGPLUS)) {
        return;
    }

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        Savegame_ResetCurrentInfo(level);
        Savegame_ApplyLogicToCurrentInfo(level);
        Savegame_GetCurrentInfo(level)->available = 0;
    }

    if (GF_GetGymLevel() != nullptr) {
        Savegame_GetCurrentInfo(GF_GetGymLevel())->available = 1;
    }
    if (GF_GetFirstLevel() != nullptr) {
        Savegame_GetCurrentInfo(GF_GetFirstLevel())->available = 1;
    }
    Game_SetBonusFlag(GBF_NONE);
}

void Savegame_CarryCurrentInfoToNextLevel(
    const GF_LEVEL *const src_level, const GF_LEVEL *const dst_level)
{
    LOG_INFO(
        "Copying resume info from level #%d to level #%d", src_level->num,
        dst_level->num);
    RESUME_INFO *const src_resume = Savegame_GetCurrentInfo(src_level);
    RESUME_INFO *const dst_resume = Savegame_GetCurrentInfo(dst_level);
    memcpy(dst_resume, src_resume, sizeof(RESUME_INFO));
}

void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *const level)
{
    RESUME_INFO *resume = Savegame_GetCurrentInfo(level);
    if (resume == nullptr) {
        return;
    }

    resume->has_pistols = 1;
    resume->gun_type = LGT_PISTOLS;
    resume->pistol_ammo = 1000;

    if (level == GF_GetGymLevel()) {
        resume->available = 1;

        resume->has_pistols = 0;
        resume->has_shotgun = 0;
        resume->has_magnums = 0;
        resume->has_uzis = 0;
        resume->has_harpoon = 0;
        resume->has_m16 = 0;
        resume->has_grenade = 0;

        resume->pistol_ammo = 0;
        resume->shotgun_ammo = 0;
        resume->magnum_ammo = 0;
        resume->uzi_ammo = 0;
        resume->harpoon_ammo = 0;
        resume->m16_ammo = 0;
        resume->grenade_ammo = 0;

        resume->flares = 0;
        resume->large_medipacks = 0;
        resume->small_medipacks = 0;
        resume->gun_type = LGT_UNARMED;
        resume->gun_status = LGS_ARMLESS;
    } else if (level == GF_GetFirstLevel()) {
        resume->available = 1;

        resume->has_pistols = 1;
        resume->has_shotgun = 1;
        resume->has_magnums = 0;
        resume->has_uzis = 0;
        resume->has_harpoon = 0;
        resume->has_m16 = 0;
        resume->has_grenade = 0;

        resume->shotgun_ammo = 2 * SHOTGUN_AMMO_CLIP;
        resume->magnum_ammo = 0;
        resume->uzi_ammo = 0;
        resume->harpoon_ammo = 0;
        resume->m16_ammo = 0;
        resume->grenade_ammo = 0;

        resume->flares = 2;
        resume->small_medipacks = 1;
        resume->large_medipacks = 1;
        resume->gun_status = LGS_ARMLESS;
    }

    if (Game_IsBonusFlagSet(GBF_NGPLUS) && level != GF_GetGymLevel()) {
        resume->has_pistols = 1;
        resume->has_shotgun = 1;
        resume->has_magnums = 1;
        resume->has_uzis = 1;
        resume->has_grenade = 1;
        resume->has_harpoon = 1;
        resume->has_m16 = 1;
        resume->has_grenade = 1;

        resume->shotgun_ammo = 10000;
        resume->magnum_ammo = 10000;
        resume->uzi_ammo = 10000;
        resume->harpoon_ammo = 10000;
        resume->m16_ammo = 10000;
        resume->grenade_ammo = 10000;

        resume->flares = -1;
        resume->gun_type = LGT_GRENADE;
    }

    if (g_GF_RemoveWeapons) {
        resume->has_pistols = 0;
        resume->has_magnums = 0;
        resume->has_uzis = 0;
        resume->has_shotgun = 0;
        resume->has_m16 = 0;
        resume->has_grenade = 0;
        resume->has_harpoon = 0;
        resume->gun_type = LGT_UNARMED;
        resume->gun_status = LGS_ARMLESS;
        g_GF_RemoveWeapons = false;
    }

    if (g_GF_RemoveAmmo) {
        resume->m16_ammo = 0;
        resume->grenade_ammo = 0;
        resume->harpoon_ammo = 0;
        resume->shotgun_ammo = 0;
        resume->uzi_ammo = 0;
        resume->magnum_ammo = 0;
        resume->pistol_ammo = 0;
        resume->flares = 0;
        resume->large_medipacks = 0;
        resume->small_medipacks = 0;
        g_GF_RemoveAmmo = false;
    }

    const STATS_COMMON default_stats = Savegame_GetDefaultStats(level);
    resume->stats.max_secret_count = default_stats.max_secret_count;
}

void Savegame_PersistGameToCurrentInfo(const GF_LEVEL *const level)
{
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    resume->available = 1;

    if (Inv_RequestItem(O_PISTOL_ITEM)) {
        resume->has_pistols = 1;
        resume->pistol_ammo = 1000;
    } else {
        resume->has_pistols = 0;
        resume->pistol_ammo = 1000;
    }

    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        resume->has_shotgun = 1;
        resume->shotgun_ammo = g_Lara.shotgun_ammo.ammo;
    } else {
        resume->has_shotgun = 0;
        resume->shotgun_ammo =
            Inv_RequestItem(O_SHOTGUN_AMMO_ITEM) * SHOTGUN_AMMO_QTY;
    }

    if (Inv_RequestItem(O_MAGNUM_ITEM)) {
        resume->has_magnums = 1;
        resume->magnum_ammo = g_Lara.magnum_ammo.ammo;
    } else {
        resume->has_magnums = 0;
        resume->magnum_ammo =
            Inv_RequestItem(O_MAGNUM_AMMO_ITEM) * MAGNUM_AMMO_QTY;
    }

    if (Inv_RequestItem(O_UZI_ITEM)) {
        resume->has_uzis = 1;
        resume->uzi_ammo = g_Lara.uzi_ammo.ammo;
    } else {
        resume->has_uzis = 0;
        resume->uzi_ammo = Inv_RequestItem(O_UZI_AMMO_ITEM) * UZI_AMMO_QTY;
    }

    if (Inv_RequestItem(O_M16_ITEM)) {
        resume->has_m16 = 1;
        resume->m16_ammo = g_Lara.m16_ammo.ammo;
    } else {
        resume->has_m16 = 0;
        resume->m16_ammo = Inv_RequestItem(O_M16_AMMO_ITEM) * M16_AMMO_QTY;
    }

    if (Inv_RequestItem(O_HARPOON_ITEM)) {
        resume->has_harpoon = 1;
        resume->harpoon_ammo = g_Lara.harpoon_ammo.ammo;
    } else {
        resume->has_harpoon = 0;
        resume->harpoon_ammo =
            Inv_RequestItem(O_HARPOON_AMMO_ITEM) * HARPOON_AMMO_QTY;
    }

    if (Inv_RequestItem(O_GRENADE_ITEM)) {
        resume->has_grenade = 1;
        resume->grenade_ammo = g_Lara.grenade_ammo.ammo;
    } else {
        resume->has_grenade = 0;
        resume->grenade_ammo =
            Inv_RequestItem(O_GRENADE_AMMO_ITEM) * GRENADE_AMMO_QTY;
    }

    resume->flares = Inv_RequestItem(O_FLARE_ITEM);
    resume->small_medipacks = Inv_RequestItem(O_SMALL_MEDIPACK_ITEM);
    resume->large_medipacks = Inv_RequestItem(O_LARGE_MEDIPACK_ITEM);

    if (g_Lara.gun_type == LGT_FLARE) {
        resume->gun_type = g_Lara.last_gun_type;
    } else {
        resume->gun_type = g_Lara.gun_type;
    }
    resume->gun_status = LGS_ARMLESS;
}

void S_SaveGame(MYFILE *const fp)
{
    char *buffer = Memory_Alloc(SAVEGAME_LEGACY_TOTAL_SIZE);
    M_Reset(buffer);

    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    const RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(current_level);

    char title[SAVEGAME_LEGACY_TITLE_SIZE];
    snprintf(title, SAVEGAME_LEGACY_TITLE_SIZE, "%s", current_level->title);
    M_Write(title, SAVEGAME_LEGACY_TITLE_SIZE);
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

    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        M_WriteU16(Music_GetTrackFlags(i));
    }
    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        const OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        M_WriteS16(object->flags);
    }

    M_WriteItems();
    M_WriteLara(&g_Lara);

    if (g_Lara.weapon_item != NO_ITEM) {
        const ITEM *const weapon_item = Item_Get(g_Lara.weapon_item);
        M_WriteS16(weapon_item->object_id);
        M_WriteS16(weapon_item->anim_num);
        M_WriteS16(weapon_item->frame_num);
        M_WriteS16(weapon_item->current_anim_state);
        M_WriteS16(weapon_item->goal_anim_state);
    }

    M_WriteS32(Room_GetFlipEffect());
    M_WriteS32(Room_GetFlipTimer());
    M_WriteS32(g_IsMonkAngry);

    M_WriteFlares();

    File_WriteData(fp, buffer, SAVEGAME_LEGACY_TOTAL_SIZE);
    Memory_FreePointer(&buffer);
}

void S_LoadGame(MYFILE *const fp)
{
    char *buffer = Memory_Alloc(File_Size(fp));
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, File_Size(fp));

    M_Reset(buffer);
    M_Skip(SAVEGAME_LEGACY_TITLE_SIZE);
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

    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        Music_SetTrackFlags(i, M_ReadU16());
    }

    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        object->flags = M_ReadS16();
    }

    M_ReadItems();
    M_ReadLara(&g_Lara);

    if (g_Lara.weapon_item != NO_ITEM) {
        g_Lara.weapon_item = Item_Create();

        ITEM *const weapon_item = Item_Get(g_Lara.weapon_item);
        weapon_item->object_id = M_ReadS16();
        weapon_item->anim_num = M_ReadS16();
        weapon_item->frame_num = M_ReadS16();
        weapon_item->current_anim_state = M_ReadS16();
        weapon_item->goal_anim_state = M_ReadS16();
        weapon_item->status = IS_ACTIVE;
        weapon_item->room_num = NO_ROOM;
    }

    if (g_Lara.burn) {
        g_Lara.burn = 0;
        Lara_CatchFire();
    }

    Room_SetFlipEffect(M_ReadS32());
    Room_SetFlipTimer(M_ReadS32());
    g_IsMonkAngry = M_ReadS32();

    M_ReadFlares();

    Memory_FreePointer(&buffer);
}
