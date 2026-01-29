#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/carrier.h>
#include <trx/game/game.h>
#include <trx/game/gun/rifle.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects/general/pickup.h>
#include <trx/game/objects/traps/sliding_pillar.h>
#include <trx/game/objects/vehicles/skidoo_common.h>
#include <trx/game/pathing.h>
#include <trx/game/rooms.h>
#include <trx/game/savegame.h>
#include <trx/game/savegame/legacy_io.h>
#include <trx/game/stats.h>
#include <trx/memory.h>
#include <trx/version.h>

#include <stdio.h>
#include <string.h>

#define M_LEGACY_TITLE_SIZE 75
#define M_SAVE_CREATURE (1 << 7)
#define M_LEGACY_NO_ROOM 255
#define M_LEGACY_MAX_MUSIC_TRACKS 64
#define M_LEGACY_MAX_BUFFER_SIZE (20 * 1024)

typedef struct {
    int32_t buf_pos;
    char *buf_ptr;
    char *buffer;
} M_CONTEXT;

// =============================================================================
// Start of internal helpers
// =============================================================================

static M_CONTEXT *M_InitContext(MYFILE *const fp)
{
    const size_t size = File_Size(fp);
    M_CONTEXT *const ctx = Memory_Alloc(sizeof(*ctx) + size);
    ctx->buf_pos = 0;
    ctx->buffer = (char *)ctx + sizeof(*ctx);
    ctx->buf_ptr = ctx->buffer;
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, ctx->buffer, size);
    return ctx;
}

void M_FreeContext(M_CONTEXT *const ctx)
{
    Memory_Free(ctx);
}

static void M_Reset(M_CONTEXT *const ctx)
{
    ctx->buf_pos = 0;
    ctx->buf_ptr = ctx->buffer;
}

static void M_Read(M_CONTEXT *const ctx, void *const ptr, const size_t size)
{
    ASSERT(ctx->buf_pos + size <= M_LEGACY_MAX_BUFFER_SIZE);
    ASSERT(ctx->buf_ptr != nullptr);
    ctx->buf_pos += size;
    memcpy(ptr, ctx->buf_ptr, size);
    ctx->buf_ptr += size;
}

static void M_Skip(M_CONTEXT *const ctx, const size_t size)
{
    ctx->buf_pos += size;
    ctx->buf_ptr += size;
}

#define X_SPECIAL_READ(name, type)                                             \
    static type M_Read##name(M_CONTEXT *const ctx)                             \
    {                                                                          \
        type result;                                                           \
        M_Read(ctx, &result, sizeof(type));                                    \
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

static int32_t M_IOReadS32(const SAVEGAME_LEGACY_IO *const io)
{
    return M_ReadS32((M_CONTEXT *)io->priv);
}

static int16_t M_IOReadS16(const SAVEGAME_LEGACY_IO *const io)
{
    return M_ReadS16((M_CONTEXT *)io->priv);
}

static void M_IOSkip(const SAVEGAME_LEGACY_IO *const io, const size_t size)
{
    M_Skip((M_CONTEXT *)io->priv, size);
}

// =============================================================================
// End of internal helpers
// =============================================================================

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

static const char *M_GetSaveFilePattern(void)
{
    return g_GameFlow.savegame_fmt_legacy;
}

static bool M_ItemHasSaveFlags(const OBJECT *const obj, ITEM *const item)
{
    // TR1 TRX savegame files are enhanced to store more information by having
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
            && item->object_id != O_DART_EMITTER
            && item->object_id != O_BACON_LARA);
    } else {
        return obj->save_flags && item->object_id != O_WATERFALL
            && item->object_id != O_DART;
    }
}

static bool M_ItemHasSaveAnim(const ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    return obj->save_anim && item->object_id != O_BACON_LARA
        && item->object_id != O_PUZZLE_HOLE_1
        && item->object_id != O_PUZZLE_HOLE_2
        && item->object_id != O_PUZZLE_HOLE_3
        && item->object_id != O_PUZZLE_HOLE_4
        && item->object_id != O_PUZZLE_DONE_1
        && item->object_id != O_PUZZLE_DONE_2
        && item->object_id != O_PUZZLE_DONE_3
        && item->object_id != O_PUZZLE_DONE_4;
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

static int32_t M_GetLevelCount(void)
{
    if (g_TRVersion == 1) {
        return GF_GetLevelTable(GFLT_MAIN)->count;
    } else {
        return 24;
    }
}

static int16_t M_ReadRoomNum(M_CONTEXT *const ctx)
{
    const int16_t room_num = M_ReadS16(ctx);
    if (room_num == M_LEGACY_NO_ROOM) {
        return NO_ROOM;
    }
    return room_num;
}

static void M_ReadLaraArm(M_CONTEXT *const ctx, LARA_ARM *const arm)
{
    M_Skip(ctx, sizeof(int32_t)); // frame_base is superfluous
    arm->frame_num = M_ReadS16(ctx);
    if (g_TRVersion >= 2) {
        arm->anim_num = M_ReadS16(ctx);
    }
    arm->lock = M_ReadS16(ctx);
    arm->rot.y = M_ReadS16(ctx);
    arm->rot.x = M_ReadS16(ctx);
    arm->rot.z = M_ReadS16(ctx);
    arm->flash_gun = M_ReadS16(ctx);
}

static void M_ReadAmmoInfo(M_CONTEXT *const ctx, AMMO_INFO *const ammo_info)
{
    ammo_info->ammo = M_ReadS32(ctx);
    if (g_TRVersion == 1) {
        M_Skip(ctx, sizeof(int32_t)); // Legacy hits value
        M_Skip(ctx, sizeof(int32_t)); // Legacy miss value
    }
}

static void M_ReadLara(M_CONTEXT *const ctx)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->item_num = M_ReadS16(ctx);
    lara->gun_status = M_ReadS16(ctx);
    lara->gun_type = M_ReadS16(ctx);
    lara->request_gun_type = M_ReadS16(ctx);
    if (g_TRVersion == 1) {
        lara->last_gun_type = lara->request_gun_type;
    } else {
        lara->last_gun_type = M_ReadS16(ctx);
        if (lara->gun_type == LGT_MAGNUMS) {
            lara->gun_type = LGT_AUTOS;
        }
        if (lara->last_gun_type == LGT_MAGNUMS) {
            lara->last_gun_type = LGT_AUTOS;
        }
        if (lara->request_gun_type == LGT_MAGNUMS) {
            lara->request_gun_type = LGT_AUTOS;
        }
    }
    lara->calc_fall_speed = M_ReadS16(ctx);
    lara->water_status = M_ReadS16(ctx);
    if (g_TRVersion == 1) {
        lara->climb_status = false;
    } else {
        lara->climb_status = M_ReadS16(ctx);
    }
    lara->pose_count = M_ReadS16(ctx);
    lara->hit_frame = M_ReadS16(ctx);
    lara->hit_direction = M_ReadS16(ctx);
    lara->air = M_ReadS16(ctx);
    lara->dive_timer = M_ReadS16(ctx);
    lara->death_timer = M_ReadS16(ctx);
    lara->current.active = M_ReadS16(ctx);
    lara->hit_effect_count = M_ReadS16(ctx);

    if (g_TRVersion == 1) {
        M_Skip(ctx, 4); // pointer to EFFECT
    }
    lara->hit_effect = nullptr;

    if (g_TRVersion >= 2) {
        lara->flare.age = M_ReadS16(ctx);
        Lara_Vehicle_SetIndex(M_ReadS16(ctx));
        lara->gun_item_num = M_ReadS16(ctx);
        lara->back_gun_obj_id = Object_FromGameID(M_ReadS16(ctx));
        lara->flare.frame_num = M_ReadS16(ctx);

        const uint16_t flags = M_ReadU16(ctx);
        // clang-format off
        lara->flare.control = (flags & (1 << 0)) != 0;
        lara->extra_anim    = (flags & (1 << 2)) != 0;
        lara->burn          = (flags & (1 << 4)) != 0;
        // clang-format on

        lara->water_surface_dist = M_ReadS32(ctx);
        lara->last_pos.x = M_ReadS32(ctx);
        lara->last_pos.y = M_ReadS32(ctx);
        lara->last_pos.z = M_ReadS32(ctx);
        M_Skip(ctx, 4);
    }

    lara->mesh_effects = M_ReadS32(ctx);
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        OBJECT_MESH *const mesh = Object_FindMesh(M_ReadS32(ctx) / 2);
        if (mesh != nullptr) {
            Lara_Mesh_Set(i, mesh);
        }
    }

    M_Skip(ctx, 4); // target – raw pointer to ITEM
    lara->target = nullptr;
    lara->target_angles[0] = M_ReadS16(ctx);
    lara->target_angles[1] = M_ReadS16(ctx);

    lara->turn_rate = M_ReadS16(ctx);
    lara->move_angle = M_ReadS16(ctx);
    lara->head_rot.y = M_ReadS16(ctx);
    lara->head_rot.x = M_ReadS16(ctx);
    lara->head_rot.z = M_ReadS16(ctx);
    lara->torso_rot.y = M_ReadS16(ctx);
    lara->torso_rot.x = M_ReadS16(ctx);
    lara->torso_rot.z = M_ReadS16(ctx);

    M_ReadLaraArm(ctx, &lara->left_arm);
    M_ReadLaraArm(ctx, &lara->right_arm);
    M_ReadAmmoInfo(ctx, &lara->pistol_ammo);
    if (g_TRVersion == 1) {
        M_ReadAmmoInfo(ctx, &lara->magnum_ammo);
    } else {
        M_ReadAmmoInfo(ctx, &lara->autos_ammo);
    }
    M_ReadAmmoInfo(ctx, &lara->uzi_ammo);
    M_ReadAmmoInfo(ctx, &lara->shotgun_ammo);
    if (g_TRVersion >= 2) {
        M_ReadAmmoInfo(ctx, &lara->harpoon_ammo);
        M_ReadAmmoInfo(ctx, &lara->grenade_ammo);
        M_ReadAmmoInfo(ctx, &lara->m16_ammo);
    }

    if (g_TRVersion == 1) {
        M_Skip(ctx, 36); // Skip LOT for water currents
    } else {
        M_Skip(ctx, 4);
    }

    const bool has_rifle = Inv_RequestItem(O_SHOTGUN_ITEM) != 0;
    Gun_Rifle_LoadLegacy(has_rifle);
}

static void M_ReadStats(M_CONTEXT *const ctx, LEVEL_STATS *const stats)
{
    if (g_TRVersion == 1) {
        stats->timer = M_ReadU32(ctx);
        stats->kill_count = M_ReadU32(ctx);
        stats->secret_flags = M_ReadU16(ctx);
        M_Skip(ctx, 2); // current level num
        stats->pickup_count = M_ReadU8(ctx);
    } else {
        stats->timer = M_ReadU32(ctx);
        stats->ammo_used = M_ReadU32(ctx);
        stats->ammo_hits = M_ReadU32(ctx);
        stats->distance_travelled = M_ReadU32(ctx);
        stats->kill_count = M_ReadU16(ctx);
        stats->secret_flags = M_ReadU8(ctx);
        stats->medipacks_used = M_ReadU8(ctx) / 2.0f;
    }
    stats->death_count = -1;
    Stats_UpdateSecrets(stats);
}

static void M_ReadResumeInfo(M_CONTEXT *const ctx, RESUME_INFO *const resume)
{
    resume->level_completed = false;
    resume->prev_level = -1;
    resume->hurt_allies = false;

    resume->pistol_ammo = M_ReadU16(ctx);
    if (g_TRVersion == 1) {
        resume->magnum_ammo = M_ReadU16(ctx);
    } else {
        resume->autos_ammo = M_ReadU16(ctx);
    }
    resume->uzi_ammo = M_ReadU16(ctx);
    resume->shotgun_ammo = M_ReadU16(ctx);
    if (g_TRVersion >= 2) {
        resume->m16_ammo = M_ReadU16(ctx);
        resume->grenade_ammo = M_ReadU16(ctx);
        resume->harpoon_ammo = M_ReadU16(ctx);
    }
    resume->small_medipacks = M_ReadU8(ctx);
    resume->large_medipacks = M_ReadU8(ctx);
    if (g_TRVersion == 1) {
        resume->num_scions = M_ReadU8(ctx);
        resume->gun_status = M_ReadS8(ctx);
        resume->equipped_gun_type = M_ReadS8(ctx);
        resume->holsters_gun_type = LGT_UNKNOWN;
        resume->back_gun_type = LGT_UNKNOWN;
    } else {
        M_Skip(ctx, 1); // legacy reserved value
        resume->flares = M_ReadU8(ctx);
        resume->gun_status = M_ReadU8(ctx);
        resume->equipped_gun_type = M_ReadU8(ctx);
        if (resume->equipped_gun_type == LGT_MAGNUMS) {
            resume->equipped_gun_type = LGT_AUTOS;
        }
        resume->holsters_gun_type = LGT_UNKNOWN;
        resume->back_gun_type = LGT_UNKNOWN;
    }

    const uint16_t flags = M_ReadU16(ctx);
    // clang-format off
    resume->flags.available     = (flags & 0x01) != 0;
    resume->flags.has_pistols   = (flags & 0x02) != 0;
    resume->flags.has_uzis      = (flags & 0x08) != 0;
    resume->flags.has_shotgun   = (flags & 0x10) != 0;
    if (g_TRVersion == 1) {
        resume->flags.has_magnums   = (flags & 0x04) != 0;
        resume->flags.costume       = (flags & 0x20) != 0;
    } else {
        resume->flags.has_autos     = (flags & 0x04) != 0;
        resume->flags.has_m16       = (flags & 0x20) != 0;
        resume->flags.has_grenade   = (flags & 0x40) != 0;
        resume->flags.has_harpoon   = (flags & 0x80) != 0;
    }
    // clang-format on

    if (g_TRVersion >= 2) {
        M_Skip(ctx, sizeof(uint16_t));
        M_ReadStats(ctx, &resume->stats);
    }
}

static void M_ReadResumeInfos(M_CONTEXT *const ctx)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < M_GetLevelCount(); i++) {
        if (i < level_table->count) {
            const GF_LEVEL *const level = &level_table->levels[i];
            M_ReadResumeInfo(ctx, Savegame_GetCurrentInfo(level));
        } else {
            RESUME_INFO dummy_resume_info = {};
            M_ReadResumeInfo(ctx, &dummy_resume_info);
        }
    }
}

static void M_ReadCurrentStats(M_CONTEXT *const ctx)
{
    LEVEL_STATS current_stats = {};
    M_ReadStats(ctx, &current_stats);

    int16_t current_level_num;
    if (g_TRVersion == 1) {
        M_Skip(ctx, -3);
        current_level_num = M_ReadS16(ctx);
        M_Skip(ctx, 1);
    } else {
        current_level_num = M_ReadS16(ctx);
    }

    const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, current_level_num);
    if (level != nullptr) {
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        resume->stats = current_stats;
    }

    const bool is_ng_plus = M_ReadU8(ctx) != 0;
    if (is_ng_plus) {
        Game_SetBonusFlag(GBF_NGPLUS);
    }
}

static void M_ReadItem(M_CONTEXT *const ctx, const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    if (M_ItemHasSavePosition(item)) {
        item->pos.x = M_ReadS32(ctx);
        item->pos.y = M_ReadS32(ctx);
        item->pos.z = M_ReadS32(ctx);
        item->rot.x = M_ReadS16(ctx);
        item->rot.y = M_ReadS16(ctx);
        item->rot.z = M_ReadS16(ctx);
        const int16_t room_num = M_ReadRoomNum(ctx);
        item->speed = M_ReadS16(ctx);
        item->fall_speed = M_ReadS16(ctx);

        Item_UpdateRoom(item_num, room_num);
    }

    if (M_ItemHasSaveAnim(item)) {
        item->current_anim_state = M_ReadS16(ctx);
        item->goal_anim_state = M_ReadS16(ctx);
        item->required_anim_state = M_ReadS16(ctx);
        item->anim_num = M_ReadS16(ctx);
        item->frame_num = M_ReadS16(ctx);

        if (item->object_id == O_LARA
            && item->anim_num < LARA_ORIGINAL_ANIM_COUNT) {
            item->anim_num += obj->anim_idx;
        }
    }

    if (M_ItemHasHitPoints(item)) {
        item->hit_points = M_ReadS16(ctx);
    }

    if (M_ItemHasSaveFlags(obj, item)) {
        item->flags = M_ReadU16(ctx);
        if (obj->intelligent && g_TRVersion >= 2) {
            M_Skip(ctx, sizeof(int16_t)); // legacy carried item
        }
        item->timer = M_ReadS16(ctx);

        if (item->flags & IF_KILLED) {
            Item_Kill(item_num);
            item->status = IS_DEACTIVATED;
        } else {
            if ((item->flags & 0x01u) != 0 && !item->active) {
                Item_AddActive(item_num);
            }
            item->status = (item->flags & 0x06u) >> 1;
            if ((item->flags & 0x08u) != 0) {
                item->gravity = true;
            }
            if ((item->flags & 0x10u) == 0) {
                item->collidable = false;
            }
        }

        if ((item->flags & M_SAVE_CREATURE) != 0) {
            LOT_EnableBaddieAI(item_num, true);
            CREATURE *const creature = item->data;
            if (creature != nullptr) {
                creature->head_rotation = M_ReadS16(ctx);
                creature->neck_rotation = M_ReadS16(ctx);
                creature->maximum_turn = M_ReadS16(ctx);
                creature->flags = M_ReadS16(ctx);
                creature->mood = M_ReadS32(ctx);
            } else {
                M_Skip(ctx, 12);
            }
        } else if (obj->intelligent) {
            item->data = nullptr;
            if (item->clear_body && item->hit_points <= 0
                && (item->flags & IF_KILLED) == 0) {
                item->next_active = Item_GetPrevActive();
                Item_SetPrevActive(item_num);
            }
        }
    }

    const SAVEGAME_LEGACY_IO io = {
        .read_s32 = M_IOReadS32,
        .read_s16 = M_IOReadS16,
        .skip = M_IOSkip,
        .tr_version = g_TRVersion,
        .priv = ctx,
    };

    if (obj->priv_legacy_load_func != nullptr) {
        obj->priv_legacy_load_func(item, &io);
    }

    switch (item->object_id) {
    case O_SLIDING_PILLAR: {
        SLIDING_PILLAR_INFO *const data = item->data;
        data->linked.pos = item->pos;
        data->linked.room_num = item->room_num;
        break;
    }

    case O_SKIDOO_FAST: {
        SKIDOO_INFO *const data = item->data;
        data->track_mesh = M_ReadS16(ctx);
        data->skidoo_turn = M_ReadS32(ctx);
        data->left_fallspeed = M_ReadS32(ctx);
        data->right_fallspeed = M_ReadS32(ctx);
        data->momentum_angle = M_ReadS16(ctx);
        data->extra_rotation = M_ReadS16(ctx);
        data->pitch = M_ReadS32(ctx);
        break;
    }

    default:
        break;
    }

    if (g_TRVersion == 1) {
        Carrier_TestLegacyDrops(item_num);
    } else {
        // TODO: consolidate where this function is called
        if (obj->handle_save_func != nullptr) {
            obj->handle_save_func(item, SAVEGAME_STAGE_AFTER_LOAD);
        }
    }
}

static void M_ReadItems(M_CONTEXT *const ctx)
{
    Savegame_ProcessItemsBeforeLoad();

    for (int32_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        M_ReadItem(ctx, item_num);
    }
}

static void M_ReadFlares(M_CONTEXT *const ctx)
{
    const int32_t num_flares = M_ReadS32(ctx);
    for (int32_t i = 0; i < num_flares; i++) {
        const int16_t item_num = Item_Create();
        ITEM *const item = Item_Get(item_num);
        item->object_id = O_FLARE_ITEM;
        item->pos.x = M_ReadS32(ctx);
        item->pos.y = M_ReadS32(ctx);
        item->pos.z = M_ReadS32(ctx);
        item->rot.x = M_ReadS16(ctx);
        item->rot.y = M_ReadS16(ctx);
        item->rot.z = M_ReadS16(ctx);
        item->room_num = M_ReadRoomNum(ctx);
        item->speed = M_ReadS16(ctx);
        item->fall_speed = M_ReadS16(ctx);
        Item_Initialise(item_num);
        Item_AddActive(item_num);
        const int32_t flare_age = M_ReadS32(ctx);
        item->data = (void *)(intptr_t)flare_age;
    }
}

static bool M_FillInfo(MYFILE *const fp, SAVEGAME_INFO *const info)
{
    File_Seek(fp, 0, SEEK_SET);

    char level_title[M_LEGACY_TITLE_SIZE];
    File_ReadData(fp, level_title, M_LEGACY_TITLE_SIZE);
    info->level_title = Memory_DupStr(level_title);
    info->counter = File_ReadS32(fp);

    for (int32_t i = 0; i < M_GetLevelCount(); i++) {
        File_Skip(fp, sizeof(uint16_t)); // pistol ammo
        File_Skip(fp, sizeof(uint16_t)); // magnum ammo
        File_Skip(fp, sizeof(uint16_t)); // uzi ammo
        File_Skip(fp, sizeof(uint16_t)); // shotgun ammo
        if (g_TRVersion >= 2) {
            File_Skip(fp, sizeof(uint16_t)); // m16 ammo
            File_Skip(fp, sizeof(uint16_t)); // grenade ammo
            File_Skip(fp, sizeof(uint16_t)); // harpoon ammo
        }
        File_Skip(fp, sizeof(uint8_t)); // small medis
        File_Skip(fp, sizeof(uint8_t)); // big medis
        if (g_TRVersion == 1) {
            File_Skip(fp, sizeof(uint8_t)); // scions
            File_Skip(fp, sizeof(int8_t)); // gun status
            File_Skip(fp, sizeof(int8_t)); // gun type
            File_Skip(fp, sizeof(uint16_t)); // flags
        } else {
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
    }

    if (g_TRVersion == 1) {
        File_Skip(fp, sizeof(uint32_t)); // timer
        File_Skip(fp, sizeof(uint32_t)); // kills
        File_Skip(fp, sizeof(uint16_t)); // secrets
    } else {
        File_Skip(fp, sizeof(uint32_t)); // timer
        File_Skip(fp, sizeof(uint32_t)); // ammo used
        File_Skip(fp, sizeof(uint32_t)); // hits
        File_Skip(fp, sizeof(uint32_t)); // distance
        File_Skip(fp, sizeof(uint16_t)); // kills
        File_Skip(fp, sizeof(uint8_t)); // secret flags
        File_Skip(fp, sizeof(uint8_t)); // medis used
    }

    info->level_num = File_ReadS16(fp);
    info->initial_version = VERSION_LEGACY;
    info->features.restart = false;
    info->features.select_level = false;
    return true;
}

static bool M_LoadFromFile(MYFILE *const fp)
{
    M_CONTEXT *const ctx = M_InitContext(fp);

    M_Reset(ctx);
    M_Skip(ctx, M_LEGACY_TITLE_SIZE); // level title
    M_Skip(ctx, sizeof(int32_t)); // save counter

    M_ReadResumeInfos(ctx);
    M_ReadCurrentStats(ctx);

    // Copy RESUME_INFO of "current position" level to the target level
    if (g_TRVersion == 1) {
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
    Inv_AddItemNTimes(O_PICKUP_ITEM_1, M_ReadU8(ctx));
    Inv_AddItemNTimes(O_PICKUP_ITEM_2, M_ReadU8(ctx));
    Inv_AddItemNTimes(O_PUZZLE_ITEM_1, M_ReadU8(ctx));
    Inv_AddItemNTimes(O_PUZZLE_ITEM_2, M_ReadU8(ctx));
    Inv_AddItemNTimes(O_PUZZLE_ITEM_3, M_ReadU8(ctx));
    Inv_AddItemNTimes(O_PUZZLE_ITEM_4, M_ReadU8(ctx));
    Inv_AddItemNTimes(O_KEY_ITEM_1, M_ReadU8(ctx));
    Inv_AddItemNTimes(O_KEY_ITEM_2, M_ReadU8(ctx));
    Inv_AddItemNTimes(O_KEY_ITEM_3, M_ReadU8(ctx));
    Inv_AddItemNTimes(O_KEY_ITEM_4, M_ReadU8(ctx));
    if (g_TRVersion == 1) {
        Inv_AddItemNTimes(O_LEADBAR_ITEM, M_ReadU8(ctx));
        M_Skip(ctx, 1); // reserved
    } else {
        M_Skip(ctx, 2); // reserved
    }

    if (M_ReadS32(ctx) != 0) {
        Room_FlipMap();
    }
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        Room_SetFlipSlotFlags(i, M_ReadS8(ctx) << 8);
    }

    if (g_TRVersion >= 2) {
        for (int32_t i = 0; i < M_LEGACY_MAX_MUSIC_TRACKS; i++) {
            const int32_t track_id = Music_ConvertLegacyTrack(i);
            Music_SetTrackFlags(track_id, M_ReadU16(ctx));
        }
    }

    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        object->flags = M_ReadS16(ctx);
    }

    M_ReadItems(ctx);

    M_ReadLara(ctx);
    if (g_TRVersion >= 2) {
        LARA_INFO *const lara = Lara_GetLaraInfo();
        if (lara->gun_item_num != NO_ITEM) {
            lara->gun_item_num = Item_Create();
            ITEM *const weapon_item = Item_Get(lara->gun_item_num);
            weapon_item->object_id = Object_FromGameID(M_ReadS16(ctx));
            weapon_item->anim_num = M_ReadS16(ctx);
            weapon_item->frame_num = M_ReadS16(ctx);
            weapon_item->current_anim_state = M_ReadS16(ctx);
            weapon_item->goal_anim_state = M_ReadS16(ctx);
            weapon_item->status = IS_ACTIVE;
            weapon_item->room_num = NO_ROOM;
        }
    }

    Room_SetFlipEffect(M_ReadS32(ctx));
    Room_SetFlipTimer(M_ReadS32(ctx));

    if (g_TRVersion >= 2) {
        Creature_SetAlliesHostile(M_ReadS32(ctx) != 0);
        M_ReadFlares(ctx);
    }

    M_FreeContext(ctx);
    return true;
}

static bool M_LoadOnlyResumeInfo(MYFILE *const fp)
{
    M_CONTEXT *const ctx = M_InitContext(fp);
    M_Skip(ctx, M_LEGACY_TITLE_SIZE); // level title
    M_Skip(ctx, sizeof(int32_t)); // save counter
    M_ReadResumeInfos(ctx);
    M_FreeContext(ctx);
    return true;
}

REGISTER_SAVEGAME_STRATEGY(m_Strategy)
