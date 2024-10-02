#include "game/inject.h"

#include "config.h"
#include "game/gamebuf.h"
#include "game/output.h"
#include "game/packer.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"
#include "items.h"

#include <libtrx/benchmark.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>
#include <libtrx/virtual_file.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#define INJECTION_MAGIC MKTAG('T', '1', 'M', 'J')
#define INJECTION_CURRENT_VERSION 8
#define NULL_FD_INDEX ((uint16_t)(-1))

typedef enum {
    INJ_VERSION_1 = 1,
    INJ_VERSION_2 = 2,
    INJ_VERSION_3 = 3,
    INJ_VERSION_4 = 4,
    INJ_VERSION_5 = 5,
    INJ_VERSION_6 = 6,
    INJ_VERSION_7 = 7,
    INJ_VERSION_8 = 8,
} INJECTION_VERSION;

typedef enum {
    INJ_GENERAL = 0,
    INJ_BRAID = 1,
    INJ_TEXTURE_FIX = 2,
    INJ_UZI_SFX = 3,
    INJ_FLOOR_DATA = 4,
    INJ_LARA_ANIMS = 5,
    INJ_LARA_JUMPS = 6,
    INJ_ITEM_POSITION = 7,
    INJ_PS1_ENEMY = 8,
    INJ_DISABLE_ANIM_SPRITE = 9,
    INJ_SKYBOX = 10,
    INJ_PS1_CRYSTAL = 11,
} INJECTION_TYPE;

typedef struct {
    VFILE *fp;
    INJECTION_VERSION version;
    INJECTION_TYPE type;
    INJECTION_INFO *info;
    bool relevant;
} INJECTION;

typedef enum {
    FT_TEXTURED_QUAD = 0,
    FT_TEXTURED_TRIANGLE = 1,
    FT_COLOURED_QUAD = 2,
    FT_COLOURED_TRIANGLE = 3
} FACE_TYPE;

typedef struct {
    GAME_OBJECT_ID object_id;
    int16_t source_identifier;
    FACE_TYPE face_type;
    int16_t face_index;
    int32_t target_count;
    int16_t *targets;
} FACE_EDIT;

typedef struct {
    int16_t vertex_index;
    int16_t x_change;
    int16_t y_change;
    int16_t z_change;
} VERTEX_EDIT;

typedef struct {
    GAME_OBJECT_ID object_id;
    int16_t mesh_idx;
    XYZ_16 centre_shift;
    int32_t radius_shift;
    int32_t face_edit_count;
    int32_t vertex_edit_count;
    FACE_EDIT *face_edits;
    VERTEX_EDIT *vertex_edits;
} MESH_EDIT;

typedef enum {
    FET_TRIGGER_PARAM = 0,
    FET_MUSIC_ONESHOT = 1,
    FET_FD_INSERT = 2,
    FET_ROOM_SHIFT = 3,
    FET_TRIGGER_ITEM = 4,
} FLOOR_EDIT_TYPE;

typedef enum {
    RMET_TEXTURE_FACE = 0,
    RMET_MOVE_FACE = 1,
    RMET_ALTER_VERTEX = 2,
    RMET_ROTATE_FACE = 3,
    RMET_ADD_FACE = 4,
    RMET_ADD_VERTEX = 5,
} ROOM_MESH_EDIT_TYPE;

static int32_t m_NumInjections = 0;
static INJECTION *m_Injections = NULL;
static INJECTION_INFO *m_Aggregate = NULL;

static void M_LoadFromFile(INJECTION *injection, const char *filename);

static uint16_t M_RemapRGB(LEVEL_INFO *level_info, RGB_888 rgb);
static void M_AlignTextureReferences(
    OBJECT *object, uint16_t *palette_map, int32_t page_base);

static void M_LoadTexturePages(
    INJECTION *injection, LEVEL_INFO *level_info, uint16_t *palette_map,
    RGBA_8888 *page_ptr);
static void M_TextureData(
    INJECTION *injection, LEVEL_INFO *level_info, int32_t page_base);
static void M_MeshData(INJECTION *injection, LEVEL_INFO *level_info);
static void M_AnimData(INJECTION *injection, LEVEL_INFO *level_info);
static void M_AnimRangeEdits(INJECTION *injection);
static void M_ObjectData(
    INJECTION *injection, LEVEL_INFO *level_info, uint16_t *palette_map);
static void M_SFXData(INJECTION *injection, LEVEL_INFO *level_info);

static int16_t *M_GetMeshTexture(FACE_EDIT *face_edit);

static void M_ApplyFaceEdit(
    FACE_EDIT *face_edit, int16_t *data_ptr, int16_t texture);
static void M_ApplyMeshEdit(MESH_EDIT *mesh_edit, uint16_t *palette_map);
static void M_MeshEdits(INJECTION *injection, uint16_t *palette_map);
static void M_TextureOverwrites(
    INJECTION *injection, LEVEL_INFO *level_info, uint16_t *palette_map);

static void M_FloorDataEdits(INJECTION *injection, LEVEL_INFO *level_info);
static void M_TriggerParameterChange(INJECTION *injection, SECTOR *sector);
static void M_SetMusicOneShot(SECTOR *sector);
static void M_InsertFloorData(
    INJECTION *injection, SECTOR *sector, LEVEL_INFO *level_info);
static void M_RoomShift(INJECTION *injection, int16_t room_num);
static void M_TriggeredItem(INJECTION *injection, LEVEL_INFO *level_info);

static void M_RoomMeshEdits(INJECTION *injection);
static void M_TextureRoomFace(INJECTION *injection);
static void M_MoveRoomFace(INJECTION *injection);
static void M_AlterRoomVertex(INJECTION *injection);
static void M_RotateRoomFace(INJECTION *injection);
static void M_AddRoomFace(INJECTION *injection);
static void M_AddRoomVertex(INJECTION *injection);

static int16_t *M_GetRoomTexture(
    int16_t room, FACE_TYPE face_type, int16_t face_index);
static int16_t *M_GetRoomFace(
    int16_t room, FACE_TYPE face_type, int16_t face_index);

static void M_RoomDoorEdits(INJECTION *injection);

static void M_ItemPositions(INJECTION *injection);

static void M_LoadFromFile(INJECTION *injection, const char *filename)
{
    injection->relevant = false;
    injection->info = NULL;

    VFILE *const fp = VFile_CreateFromPath(filename);
    injection->fp = fp;
    if (!fp) {
        LOG_WARNING("Could not open %s", filename);
        return;
    }

    const uint32_t magic = VFile_ReadU32(fp);
    if (magic != INJECTION_MAGIC) {
        LOG_WARNING("Invalid injection magic in %s", filename);
        return;
    }

    injection->version = VFile_ReadS32(fp);
    if (injection->version < INJ_VERSION_1
        || injection->version > INJECTION_CURRENT_VERSION) {
        LOG_WARNING(
            "%s uses unsupported version %d", filename, injection->version);
        return;
    }

    injection->type = VFile_ReadS32(fp);

    switch (injection->type) {
    case INJ_GENERAL:
    case INJ_LARA_ANIMS:
        injection->relevant = true;
        break;
    case INJ_BRAID:
        injection->relevant = g_Config.enable_braid;
        break;
    case INJ_UZI_SFX:
        injection->relevant = g_Config.enable_ps_uzi_sfx;
        break;
    case INJ_FLOOR_DATA:
        injection->relevant = g_Config.fix_floor_data_issues;
        break;
    case INJ_TEXTURE_FIX:
        injection->relevant = g_Config.fix_texture_issues;
        break;
    case INJ_LARA_JUMPS:
        injection->relevant = g_Config.enable_tr2_jumping;
        break;
    case INJ_ITEM_POSITION:
        injection->relevant = g_Config.fix_item_rots;
        break;
    case INJ_PS1_ENEMY:
        injection->relevant = g_Config.restore_ps1_enemies;
        break;
    case INJ_DISABLE_ANIM_SPRITE:
        injection->relevant = !g_Config.fix_animated_sprites;
        break;
    case INJ_SKYBOX:
        injection->relevant = g_Config.enable_skybox;
        break;
    case INJ_PS1_CRYSTAL:
        injection->relevant =
            g_Config.enable_save_crystals && g_Config.enable_ps1_crystals;
        break;
    default:
        LOG_WARNING("%s is of unknown type %d", filename, injection->type);
        break;
    }

    if (!injection->relevant) {
        return;
    }

    injection->info = Memory_Alloc(sizeof(INJECTION_INFO));
    INJECTION_INFO *info = injection->info;

    info->texture_page_count = VFile_ReadS32(fp);
    info->texture_count = VFile_ReadS32(fp);
    info->sprite_info_count = VFile_ReadS32(fp);
    info->sprite_count = VFile_ReadS32(fp);
    info->mesh_count = VFile_ReadS32(fp);
    info->mesh_ptr_count = VFile_ReadS32(fp);
    info->anim_change_count = VFile_ReadS32(fp);
    info->anim_range_count = VFile_ReadS32(fp);
    info->anim_cmd_count = VFile_ReadS32(fp);
    info->anim_bone_count = VFile_ReadS32(fp);
    info->anim_frame_data_count = VFile_ReadS32(fp);
    info->anim_count = VFile_ReadS32(fp);
    info->object_count = VFile_ReadS32(fp);
    info->sfx_count = VFile_ReadS32(fp);
    info->sfx_data_size = VFile_ReadS32(fp);
    info->sample_count = VFile_ReadS32(fp);
    info->mesh_edit_count = VFile_ReadS32(fp);
    info->texture_overwrite_count = VFile_ReadS32(fp);
    info->floor_edit_count = VFile_ReadS32(fp);

    if (injection->version < INJ_VERSION_8) {
        // Legacy value that stored the total injected floor data length.
        VFile_Skip(fp, sizeof(int32_t));
    }

    if (injection->version > INJ_VERSION_1) {
        // room_mesh_count is a summary of the change in mesh size,
        // while room_mesh_edit_count indicates how many edits to
        // read and interpret (not all edits incur a size change).
        info->room_mesh_count = VFile_ReadU32(fp);
        info->room_meshes =
            Memory_Alloc(sizeof(INJECTION_ROOM_MESH) * info->room_mesh_count);
        for (int32_t i = 0; i < info->room_mesh_count; i++) {
            INJECTION_ROOM_MESH *mesh = &info->room_meshes[i];
            mesh->room_index = VFile_ReadS16(fp);
            mesh->extra_size = VFile_ReadU32(fp);
        }

        info->room_mesh_edit_count = VFile_ReadU32(fp);
        info->room_door_edit_count = VFile_ReadU32(fp);
    } else {
        info->room_meshes = NULL;
    }

    if (injection->version > INJ_VERSION_2) {
        info->anim_range_edit_count = VFile_ReadS32(fp);
    } else {
        info->anim_range_edit_count = 0;
    }

    if (injection->version > INJ_VERSION_3) {
        info->item_position_count = VFile_ReadS32(fp);
    } else {
        info->item_position_count = 0;
    }

    // Get detailed frame counts
    {
        const size_t prev_pos = VFile_GetPos(fp);

        // texture pages
        VFile_Skip(fp, 256 * 3);
        VFile_Skip(fp, info->texture_page_count * PAGE_SIZE);

        // textures
        VFile_Skip(fp, info->texture_count * 20);

        // sprites
        VFile_Skip(fp, info->sprite_info_count * 16);
        VFile_Skip(fp, info->sprite_count * 8);

        // meshes
        VFile_Skip(fp, info->mesh_count * 2);
        VFile_Skip(fp, info->mesh_ptr_count * 4);

        VFile_Skip(fp, info->anim_change_count * 6);
        VFile_Skip(fp, info->anim_range_count * 8);
        VFile_Skip(fp, info->anim_cmd_count * 2);
        VFile_Skip(fp, info->anim_bone_count * 4);

        info->anim_frame_count = 0;
        info->anim_frame_mesh_rot_count = 0;
        const size_t frame_data_start = VFile_GetPos(fp);
        const size_t frame_data_end =
            frame_data_start + info->anim_frame_data_count * sizeof(int16_t);
        while (VFile_GetPos(fp) < frame_data_end) {
            VFile_Skip(fp, 9 * sizeof(int16_t));
            const int16_t num_meshes = VFile_ReadS16(fp);
            VFile_Skip(fp, num_meshes * sizeof(int32_t));
            info->anim_frame_count++;
            info->anim_frame_mesh_rot_count += num_meshes;
        }
        assert(VFile_GetPos(fp) == frame_data_end);

        VFile_SetPos(fp, prev_pos);
    }

    m_Aggregate->texture_page_count += info->texture_page_count;
    m_Aggregate->texture_count += info->texture_count;
    m_Aggregate->sprite_info_count += info->sprite_info_count;
    m_Aggregate->sprite_count += info->sprite_count;
    m_Aggregate->mesh_count += info->mesh_count;
    m_Aggregate->mesh_ptr_count += info->mesh_ptr_count;
    m_Aggregate->anim_change_count += info->anim_change_count;
    m_Aggregate->anim_range_count += info->anim_range_count;
    m_Aggregate->anim_cmd_count += info->anim_cmd_count;
    m_Aggregate->anim_bone_count += info->anim_bone_count;
    m_Aggregate->anim_frame_data_count += info->anim_frame_data_count;
    m_Aggregate->anim_frame_count += info->anim_frame_count;
    m_Aggregate->anim_frame_mesh_rot_count += info->anim_frame_mesh_rot_count;
    m_Aggregate->anim_count += info->anim_count;
    m_Aggregate->object_count += info->object_count;
    m_Aggregate->sfx_count += info->sfx_count;
    m_Aggregate->sfx_data_size += info->sfx_data_size;
    m_Aggregate->sample_count += info->sample_count;

    LOG_INFO("%s queued for injection", filename);
}

static void M_LoadTexturePages(
    INJECTION *injection, LEVEL_INFO *level_info, uint16_t *palette_map,
    RGBA_8888 *page_ptr)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    palette_map[0] = 0;
    RGB_888 source_palette[256];
    for (int32_t i = 0; i < 256; i++) {
        source_palette[i].r = VFile_ReadU8(fp);
        source_palette[i].g = VFile_ReadU8(fp);
        source_palette[i].b = VFile_ReadU8(fp);
    }
    for (int32_t i = 1; i < 256; i++) {
        source_palette[i].r *= 4;
        source_palette[i].g *= 4;
        source_palette[i].b *= 4;
    }
    for (int32_t i = 0; i < 256; i++) {
        palette_map[i] = M_RemapRGB(level_info, source_palette[i]);
    }

    // Read in each page for this injection and realign the pixels
    // to the level's palette.
    const size_t pixel_count = PAGE_SIZE * inj_info->texture_page_count;
    uint8_t *indices = Memory_Alloc(pixel_count);
    VFile_Read(fp, indices, pixel_count);
    uint8_t *input = indices;
    RGBA_8888 *output = page_ptr;
    for (size_t i = 0; i < pixel_count; i++) {
        const uint8_t index = *input++;
        if (index == 0) {
            output->a = 0;
        } else {
            output->r = source_palette[index].r;
            output->g = source_palette[index].g;
            output->b = source_palette[index].b;
            output->a = 255;
        }
        output++;
    }
    Memory_FreePointer(&indices);

    Benchmark_End(benchmark, NULL);
}

static void M_TextureData(
    INJECTION *injection, LEVEL_INFO *level_info, int32_t page_base)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    // Read the tex_infos and align them to the end of the page list.
    for (int32_t i = 0; i < inj_info->texture_count; i++) {
        PHD_TEXTURE *texture = &g_PhdTextureInfo[level_info->texture_count + i];
        texture->drawtype = VFile_ReadU16(fp);
        texture->tpage = VFile_ReadU16(fp);
        for (int32_t j = 0; j < 4; j++) {
            texture->uv[j].u = VFile_ReadU16(fp);
            texture->uv[j].v = VFile_ReadU16(fp);
        }
        g_PhdTextureInfo[level_info->texture_count + i].tpage += page_base;
    }

    for (int32_t i = 0; i < inj_info->sprite_info_count; i++) {
        PHD_SPRITE *sprite =
            &g_PhdSpriteInfo[level_info->sprite_info_count + i];
        sprite->tpage = VFile_ReadU16(fp);
        sprite->offset = VFile_ReadU16(fp);
        sprite->width = VFile_ReadU16(fp);
        sprite->height = VFile_ReadU16(fp);
        sprite->x1 = VFile_ReadS16(fp);
        sprite->y1 = VFile_ReadS16(fp);
        sprite->x2 = VFile_ReadS16(fp);
        sprite->y2 = VFile_ReadS16(fp);
        g_PhdSpriteInfo[level_info->sprite_info_count + i].tpage += page_base;
    }

    for (int32_t i = 0; i < inj_info->sprite_count; i++) {
        const GAME_OBJECT_ID object_id = VFile_ReadS32(fp);
        const int16_t num_meshes = VFile_ReadS16(fp);
        const int16_t mesh_idx = VFile_ReadS16(fp);

        if (object_id < O_NUMBER_OF) {
            OBJECT *object = &g_Objects[object_id];
            object->nmeshes = num_meshes;
            object->mesh_idx = mesh_idx + level_info->sprite_info_count;
            object->loaded = 1;
        } else if (object_id - O_NUMBER_OF < STATIC_NUMBER_OF) {
            STATIC_INFO *object = &g_StaticObjects[object_id - O_NUMBER_OF];
            object->nmeshes = num_meshes;
            object->mesh_num = mesh_idx + level_info->sprite_info_count;
            object->loaded = true;
        }
        level_info->sprite_info_count -= num_meshes;
        level_info->sprite_count++;
    }

    Benchmark_End(benchmark, NULL);
}

static void M_MeshData(INJECTION *injection, LEVEL_INFO *level_info)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    int32_t mesh_base = level_info->mesh_count;
    VFile_Read(
        fp, g_MeshBase + mesh_base, sizeof(int16_t) * inj_info->mesh_count);
    level_info->mesh_count += inj_info->mesh_count;

    uint32_t *mesh_indices = GameBuf_Alloc(
        sizeof(uint32_t) * inj_info->mesh_ptr_count, GBUF_MESH_POINTERS);
    VFile_Read(fp, mesh_indices, sizeof(uint32_t) * inj_info->mesh_ptr_count);

    for (int32_t i = 0; i < inj_info->mesh_ptr_count; i++) {
        g_Meshes[level_info->mesh_ptr_count + i] =
            &g_MeshBase[mesh_base + mesh_indices[i] / 2];
    }

    Benchmark_End(benchmark, NULL);
}

static void M_AnimData(INJECTION *injection, LEVEL_INFO *level_info)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->anim_change_count; i++) {
        ANIM_CHANGE *anim_change =
            &g_AnimChanges[level_info->anim_change_count + i];
        anim_change->goal_anim_state = VFile_ReadS16(fp);
        anim_change->num_ranges = VFile_ReadS16(fp);
        anim_change->range_idx = VFile_ReadS16(fp);
    }
    for (int32_t i = 0; i < inj_info->anim_range_count; i++) {
        ANIM_RANGE *anim_range =
            &g_AnimRanges[level_info->anim_range_count + i];
        anim_range->start_frame = VFile_ReadS16(fp);
        anim_range->end_frame = VFile_ReadS16(fp);
        anim_range->link_anim_num = VFile_ReadS16(fp);
        anim_range->link_frame_num = VFile_ReadS16(fp);
    }
    VFile_Read(
        fp, g_AnimCommands + level_info->anim_command_count,
        sizeof(int16_t) * inj_info->anim_cmd_count);
    VFile_Read(
        fp, g_AnimBones + level_info->anim_bone_count,
        sizeof(int32_t) * inj_info->anim_bone_count);
    const size_t frame_data_start = VFile_GetPos(fp);
    VFile_Skip(fp, inj_info->anim_frame_data_count * sizeof(int16_t));
    const size_t frame_data_end = VFile_GetPos(fp);

    VFile_SetPos(fp, frame_data_start);
    int32_t *mesh_rots =
        &g_AnimFrameMeshRots[level_info->anim_frame_mesh_rot_count];
    for (int32_t i = 0; i < inj_info->anim_frame_count; i++) {
        level_info->anim_frame_offsets[level_info->anim_frame_count + i] =
            VFile_GetPos(fp) - frame_data_start;
        FRAME_INFO *const frame =
            &g_AnimFrames[level_info->anim_frame_count + i];
        frame->bounds.min.x = VFile_ReadS16(fp);
        frame->bounds.max.x = VFile_ReadS16(fp);
        frame->bounds.min.y = VFile_ReadS16(fp);
        frame->bounds.max.y = VFile_ReadS16(fp);
        frame->bounds.min.z = VFile_ReadS16(fp);
        frame->bounds.max.z = VFile_ReadS16(fp);
        frame->offset.x = VFile_ReadS16(fp);
        frame->offset.y = VFile_ReadS16(fp);
        frame->offset.z = VFile_ReadS16(fp);
        frame->nmeshes = VFile_ReadS16(fp);
        frame->mesh_rots = mesh_rots;
        VFile_Read(fp, mesh_rots, sizeof(int32_t) * frame->nmeshes);
        mesh_rots += frame->nmeshes;
    }
    assert(VFile_GetPos(fp) == frame_data_end);

    for (int32_t i = 0; i < inj_info->anim_count; i++) {
        ANIM *anim = &g_Anims[level_info->anim_count + i];

        anim->frame_ofs = VFile_ReadU32(fp);
        anim->interpolation = VFile_ReadS16(fp);
        anim->current_anim_state = VFile_ReadS16(fp);
        anim->velocity = VFile_ReadS32(fp);
        anim->acceleration = VFile_ReadS32(fp);
        anim->frame_base = VFile_ReadS16(fp);
        anim->frame_end = VFile_ReadS16(fp);
        anim->jump_anim_num = VFile_ReadS16(fp);
        anim->jump_frame_num = VFile_ReadS16(fp);
        anim->num_changes = VFile_ReadS16(fp);
        anim->change_idx = VFile_ReadS16(fp);
        anim->num_commands = VFile_ReadS16(fp);
        anim->command_idx = VFile_ReadS16(fp);

        // Re-align to the level.
        anim->jump_anim_num += level_info->anim_count;
        bool found = false;
        for (int32_t j = 0; j < inj_info->anim_frame_count; j++) {
            if (level_info->anim_frame_offsets[level_info->anim_frame_count + j]
                == (signed)anim->frame_ofs) {
                anim->frame_ptr =
                    &g_AnimFrames[level_info->anim_frame_count + j];
                found = true;
                break;
            }
        }
        anim->frame_ofs += level_info->anim_frame_data_count * 2;
        assert(found);
        if (anim->num_changes) {
            anim->change_idx += level_info->anim_change_count;
        }
        if (anim->num_commands) {
            anim->command_idx += level_info->anim_command_count;
        }
    }

    // Re-align to the level.
    for (int32_t i = 0; i < inj_info->anim_change_count; i++) {
        g_AnimChanges[level_info->anim_change_count++].range_idx +=
            level_info->anim_range_count;
    }

    for (int32_t i = 0; i < inj_info->anim_range_count; i++) {
        g_AnimRanges[level_info->anim_range_count++].link_anim_num +=
            level_info->anim_count;
    }

    Benchmark_End(benchmark, NULL);
}

static void M_AnimRangeEdits(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_3) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->anim_range_edit_count; i++) {
        const GAME_OBJECT_ID object_id = VFile_ReadS32(fp);
        const int16_t anim_idx = VFile_ReadS16(fp);
        const int32_t edit_count = VFile_ReadS32(fp);

        if (object_id < 0 || object_id >= O_NUMBER_OF) {
            LOG_WARNING("Object %d is not recognised", object_id);
            VFile_Skip(fp, edit_count * sizeof(int16_t) * 4);
            continue;
        }

        OBJECT *object = &g_Objects[object_id];
        if (!object->loaded) {
            LOG_WARNING("Object %d is not loaded", object_id);
            VFile_Skip(fp, edit_count * sizeof(int16_t) * 4);
            continue;
        }

        ANIM *anim = &g_Anims[object->anim_idx + anim_idx];
        for (int32_t j = 0; j < edit_count; j++) {
            const int16_t change_idx = VFile_ReadS16(fp);
            const int16_t range_idx = VFile_ReadS16(fp);
            const int16_t low_frame = VFile_ReadS16(fp);
            const int16_t high_frame = VFile_ReadS16(fp);

            if (change_idx >= anim->num_changes) {
                LOG_WARNING(
                    "Change %d is invalid for animation %d", change_idx,
                    anim_idx);
                continue;
            }
            ANIM_CHANGE *change = &g_AnimChanges[anim->change_idx + change_idx];

            if (range_idx >= change->num_ranges) {
                LOG_WARNING(
                    "Range %d is invalid for change %d, animation %d",
                    range_idx, change_idx, anim_idx);
                continue;
            }
            ANIM_RANGE *range = &g_AnimRanges[change->range_idx + range_idx];

            range->start_frame = low_frame;
            range->end_frame = high_frame;
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_ObjectData(
    INJECTION *injection, LEVEL_INFO *level_info, uint16_t *palette_map)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    // TODO: consider refactoring once we have more injection
    // use cases.
    for (int32_t i = 0; i < inj_info->object_count; i++) {
        const GAME_OBJECT_ID object_id = VFile_ReadS32(fp);
        OBJECT *object = &g_Objects[object_id];

        const int16_t num_meshes = VFile_ReadS16(fp);
        const int16_t mesh_idx = VFile_ReadS16(fp);
        const int32_t bone_idx = VFile_ReadS32(fp);

        // When mesh data has been omitted from the injection, this indicates
        // that we wish to retain what's already defined so to avoid duplicate
        // packing.
        if (!object->loaded || num_meshes) {
            object->nmeshes = num_meshes;
            object->mesh_idx = mesh_idx + level_info->mesh_ptr_count;
            object->bone_idx = bone_idx + level_info->anim_bone_count;
        }

        const int32_t frame_offset = VFile_ReadS32(fp);
        object->anim_idx = VFile_ReadS16(fp);
        object->anim_idx += level_info->anim_count;
        object->loaded = 1;

        if (inj_info->anim_frame_count > 0) {
            bool found = false;
            for (int32_t j = 0; j < inj_info->anim_frame_count; j++) {
                if (level_info
                        ->anim_frame_offsets[level_info->anim_frame_count + j]
                    == frame_offset) {
                    object->frame_base =
                        &g_AnimFrames[level_info->anim_frame_count + j];
                    found = true;
                    break;
                }
            }
            assert(found);
        }

        if (num_meshes) {
            M_AlignTextureReferences(
                object, palette_map, level_info->texture_count);
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_SFXData(INJECTION *injection, LEVEL_INFO *level_info)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->sfx_count; i++) {
        const int16_t sfx_id = VFile_ReadS16(fp);
        g_SampleLUT[sfx_id] = level_info->sample_info_count;

        SAMPLE_INFO *sample_info =
            &g_SampleInfos[level_info->sample_info_count];
        sample_info->volume = VFile_ReadS16(fp);
        sample_info->randomness = VFile_ReadS16(fp);
        sample_info->flags = VFile_ReadS16(fp);
        sample_info->number = level_info->sample_count;

        int16_t num_samples = (sample_info->flags >> 2) & 15;
        for (int32_t j = 0; j < num_samples; j++) {
            const int32_t sample_length = VFile_ReadS32(fp);
            VFile_Read(
                fp, level_info->sample_data + level_info->sample_data_size,
                sizeof(char) * sample_length);

            level_info->sample_offsets[level_info->sample_count] =
                level_info->sample_data_size;
            level_info->sample_data_size += sample_length;
            level_info->sample_count++;
        }

        level_info->sample_info_count++;
    }

    Benchmark_End(benchmark, NULL);
}

static void M_AlignTextureReferences(
    OBJECT *object, uint16_t *palette_map, int32_t page_base)
{
    int16_t **mesh = &g_Meshes[object->mesh_idx];
    for (int32_t i = 0; i < object->nmeshes; i++) {
        int16_t *data_ptr = *mesh++;
        data_ptr += 5; // Skip centre and collision radius
        int32_t vertex_count = *data_ptr++;
        data_ptr += 3 * vertex_count; // Skip vertex info

        // Skip normals or lights
        int32_t normal_count = *data_ptr++;
        data_ptr += normal_count > 0 ? 3 * normal_count : -normal_count;

        // Align the tex_info references on the textured quads and triangles.
        int32_t num_faces = *data_ptr++;
        for (int32_t j = 0; j < num_faces; j++) {
            data_ptr += 4; // Skip vertices
            *data_ptr++ += page_base;
        }

        num_faces = *data_ptr++;
        for (int32_t j = 0; j < num_faces; j++) {
            data_ptr += 3;
            *data_ptr++ += page_base;
        }

        // Align coloured quads and triangles to the level palette.
        num_faces = *data_ptr++;
        for (int32_t j = 0; j < num_faces; j++) {
            data_ptr += 4;
            *data_ptr = palette_map[*data_ptr];
            data_ptr++;
        }

        num_faces = *data_ptr++;
        for (int32_t j = 0; j < num_faces; j++) {
            data_ptr += 3;
            *data_ptr = palette_map[*data_ptr];
            data_ptr++;
        }
    }
}

static uint16_t M_RemapRGB(LEVEL_INFO *level_info, RGB_888 rgb)
{
    // Find the index of the exact match to the given RGB
    for (int32_t i = 0; i < level_info->palette_size; i++) {
        const RGB_888 test_rgb = level_info->palette[i];
        if (rgb.r == test_rgb.r && rgb.g == test_rgb.g && rgb.b == test_rgb.b) {
            return i;
        }
    }

    // Match not found - expand the game palette
    level_info->palette_size++;
    level_info->palette = Memory_Realloc(
        level_info->palette, level_info->palette_size * sizeof(RGB_888));
    level_info->palette[level_info->palette_size - 1] = rgb;
    return level_info->palette_size - 1;
}

static void M_MeshEdits(INJECTION *injection, uint16_t *palette_map)
{
    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    if (!inj_info->mesh_edit_count) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    MESH_EDIT *mesh_edits =
        Memory_Alloc(sizeof(MESH_EDIT) * inj_info->mesh_edit_count);

    for (int32_t i = 0; i < inj_info->mesh_edit_count; i++) {
        MESH_EDIT *mesh_edit = &mesh_edits[i];
        mesh_edit->object_id = VFile_ReadS32(fp);
        mesh_edit->mesh_idx = VFile_ReadS16(fp);

        if (injection->version >= INJ_VERSION_6) {
            mesh_edit->centre_shift.x = VFile_ReadS16(fp);
            mesh_edit->centre_shift.y = VFile_ReadS16(fp);
            mesh_edit->centre_shift.z = VFile_ReadS16(fp);
            mesh_edit->radius_shift = VFile_ReadS32(fp);
        }

        mesh_edit->face_edit_count = VFile_ReadS32(fp);
        mesh_edit->face_edits =
            Memory_Alloc(sizeof(FACE_EDIT) * mesh_edit->face_edit_count);
        for (int32_t j = 0; j < mesh_edit->face_edit_count; j++) {
            FACE_EDIT *face_edit = &mesh_edit->face_edits[j];
            face_edit->object_id = VFile_ReadS32(fp);
            face_edit->source_identifier = VFile_ReadS16(fp);
            face_edit->face_type = VFile_ReadS32(fp);
            face_edit->face_index = VFile_ReadS16(fp);

            face_edit->target_count = VFile_ReadS32(fp);
            face_edit->targets =
                Memory_Alloc(sizeof(int16_t) * face_edit->target_count);
            VFile_Read(
                fp, face_edit->targets,
                sizeof(int16_t) * face_edit->target_count);
        }

        mesh_edit->vertex_edit_count = VFile_ReadS32(fp);
        mesh_edit->vertex_edits =
            Memory_Alloc(sizeof(VERTEX_EDIT) * mesh_edit->vertex_edit_count);
        for (int32_t i = 0; i < mesh_edit->vertex_edit_count; i++) {
            VERTEX_EDIT *vertex_edit = &mesh_edit->vertex_edits[i];
            vertex_edit->vertex_index = VFile_ReadS16(fp);
            vertex_edit->x_change = VFile_ReadS16(fp);
            vertex_edit->y_change = VFile_ReadS16(fp);
            vertex_edit->z_change = VFile_ReadS16(fp);
        }

        M_ApplyMeshEdit(mesh_edit, palette_map);

        for (int32_t j = 0; j < mesh_edit->face_edit_count; j++) {
            FACE_EDIT *face_edit = &mesh_edit->face_edits[j];
            Memory_FreePointer(&face_edit->targets);
        }

        Memory_FreePointer(&mesh_edit->face_edits);
        Memory_FreePointer(&mesh_edit->vertex_edits);
    }

    Memory_FreePointer(&mesh_edits);
    Benchmark_End(benchmark, NULL);
}

static void M_ApplyMeshEdit(MESH_EDIT *mesh_edit, uint16_t *palette_map)
{
    OBJECT object = g_Objects[mesh_edit->object_id];
    if (!object.loaded) {
        return;
    }

    int16_t **mesh = &g_Meshes[object.mesh_idx];
    int16_t *data_ptr = *(mesh + mesh_edit->mesh_idx);

    *data_ptr++ += mesh_edit->centre_shift.x;
    *data_ptr++ += mesh_edit->centre_shift.y;
    *data_ptr++ += mesh_edit->centre_shift.z;

    int32_t *radius = (int32_t *)data_ptr;
    *radius += mesh_edit->radius_shift;
    data_ptr += 2;

    int32_t vertex_count = *data_ptr++;
    for (int32_t i = 0; i < mesh_edit->vertex_edit_count; i++) {
        VERTEX_EDIT *vertex_edit = mesh_edit->vertex_edits + i;
        int16_t *vertex_ptr = (data_ptr + 3 * vertex_edit->vertex_index);
        *vertex_ptr++ += vertex_edit->x_change;
        *vertex_ptr++ += vertex_edit->y_change;
        *vertex_ptr++ += vertex_edit->z_change;
    }

    if (!mesh_edit->face_edit_count) {
        return;
    }

    // Skip vertices and lights/normals.
    data_ptr += 3 * vertex_count;
    int32_t normal_count = *data_ptr++;
    data_ptr += normal_count > 0 ? 3 * normal_count : -normal_count;

    // Find each face we are interested in and replace its texture
    // or palette reference with the one selected from each edit's
    // instructions.
    int16_t *data_start = data_ptr;
    for (int32_t i = 0; i < mesh_edit->face_edit_count; i++) {
        FACE_EDIT *face_edit = &mesh_edit->face_edits[i];
        int16_t texture;
        if (face_edit->source_identifier < 0) {
            texture = palette_map[-face_edit->source_identifier];
        } else {
            int16_t *tex_ptr = M_GetMeshTexture(face_edit);
            if (!tex_ptr) {
                continue;
            }
            texture = *tex_ptr;
        }

        data_ptr = data_start;

        int32_t num_faces = *data_ptr++;
        if (face_edit->face_type == FT_TEXTURED_QUAD) {
            M_ApplyFaceEdit(face_edit, data_ptr, texture);
        }

        data_ptr += 5 * num_faces;
        num_faces = *data_ptr++;
        if (face_edit->face_type == FT_TEXTURED_TRIANGLE) {
            M_ApplyFaceEdit(face_edit, data_ptr, texture);
        }

        data_ptr += 4 * num_faces;
        num_faces = *data_ptr++;
        if (face_edit->face_type == FT_COLOURED_QUAD) {
            M_ApplyFaceEdit(face_edit, data_ptr, texture);
        }

        data_ptr += 5 * num_faces;
        num_faces = *data_ptr++;
        if (face_edit->face_type == FT_COLOURED_TRIANGLE) {
            M_ApplyFaceEdit(face_edit, data_ptr, texture);
        }
    }
}

static void M_ApplyFaceEdit(
    FACE_EDIT *face_edit, int16_t *data_ptr, int16_t texture)
{
    int32_t vertex_count;
    switch (face_edit->face_type) {
    case FT_TEXTURED_TRIANGLE:
    case FT_COLOURED_TRIANGLE:
        vertex_count = 3;
        break;
    default:
        vertex_count = 4;
        break;
    }

    int16_t *face_ptr;
    for (int32_t i = 0; i < face_edit->target_count; i++) {
        // Skip over the faces before this, plus this face's vertices.
        face_ptr =
            (data_ptr + (vertex_count + 1) * face_edit->targets[i]
             + vertex_count);
        *face_ptr = texture;
    }
}

static int16_t *M_GetMeshTexture(FACE_EDIT *face_edit)
{
    OBJECT object = g_Objects[face_edit->object_id];
    if (!object.loaded) {
        return NULL;
    }

    int16_t **mesh = &g_Meshes[object.mesh_idx];
    int16_t *data_ptr = *(mesh + face_edit->source_identifier);
    data_ptr += 5; // Skip centre and collision radius

    // Skip vertices and lights/normals.
    int32_t vertex_count = *data_ptr++;
    data_ptr += 3 * vertex_count;
    int32_t normal_count = *data_ptr++;
    data_ptr += normal_count > 0 ? 3 * normal_count : -normal_count;

    int32_t num_faces = *data_ptr++;
    if (face_edit->face_type == FT_TEXTURED_QUAD) {
        data_ptr += face_edit->face_index * 5 + 4;
        return data_ptr;
    }

    data_ptr += 5 * num_faces;
    num_faces = *data_ptr++;
    if (face_edit->face_type == FT_TEXTURED_TRIANGLE) {
        data_ptr += face_edit->face_index * 4 + 3;
        return data_ptr;
    }

    data_ptr += 4 * num_faces;
    num_faces = *data_ptr++;
    if (face_edit->face_type == FT_COLOURED_QUAD) {
        data_ptr += face_edit->face_index * 5 + 4;
        return data_ptr;
    }

    data_ptr += 5 * num_faces;
    num_faces = *data_ptr++;
    if (face_edit->face_type == FT_COLOURED_TRIANGLE) {
        data_ptr += face_edit->face_index * 4 + 3;
        return data_ptr;
    }

    return NULL;
}

static void M_TextureOverwrites(
    INJECTION *injection, LEVEL_INFO *level_info, uint16_t *palette_map)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->texture_overwrite_count; i++) {
        const uint16_t target_page = VFile_ReadU16(fp);
        const uint8_t target_x = VFile_ReadU8(fp);
        const uint8_t target_y = VFile_ReadU8(fp);
        const uint16_t source_width = VFile_ReadU16(fp);
        const uint16_t source_height = VFile_ReadU16(fp);

        uint8_t *source_img = Memory_Alloc(source_width * source_height);
        VFile_Read(fp, source_img, source_width * source_height);

        // Copy the source image pixels directly into the target page.
        RGBA_8888 *page =
            level_info->texture_rgb_page_ptrs + target_page * PAGE_SIZE;
        for (int32_t y = 0; y < source_height; y++) {
            for (int32_t x = 0; x < source_width; x++) {
                const uint8_t pal_idx = source_img[y * source_width + x];
                const int32_t target_pixel =
                    (y + target_y) * PAGE_WIDTH + x + target_x;
                if (pal_idx == 0) {
                    (*(page + target_pixel)).a = 0;
                } else {
                    const RGB_888 pix =
                        level_info->palette[palette_map[pal_idx]];
                    (*(page + target_pixel)).r = pix.r;
                    (*(page + target_pixel)).g = pix.g;
                    (*(page + target_pixel)).b = pix.b;
                    (*(page + target_pixel)).a = 255;
                }
            }
        }

        Memory_FreePointer(&source_img);
    }

    Benchmark_End(benchmark, NULL);
}

static void M_FloorDataEdits(INJECTION *injection, LEVEL_INFO *level_info)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->floor_edit_count; i++) {
        const int16_t room = VFile_ReadS16(fp);
        const uint16_t x = VFile_ReadU16(fp);
        const uint16_t z = VFile_ReadU16(fp);
        const int32_t fd_edit_count = VFile_ReadS32(fp);

        // Verify that the given room and coordinates are accurate.
        // Individual FD functions must check that sector is actually set.
        ROOM *r = NULL;
        SECTOR *sector = NULL;
        if (room < 0 || room >= g_RoomCount) {
            LOG_WARNING("Room index %d is invalid", room);
        } else {
            r = &g_RoomInfo[room];
            if (x >= r->size.x || z >= r->size.z) {
                LOG_WARNING(
                    "Sector [%d,%d] is invalid for room %d", x, z, room);
            } else {
                sector = &r->sectors[r->size.z * x + z];
            }
        }

        for (int32_t j = 0; j < fd_edit_count; j++) {
            const FLOOR_EDIT_TYPE edit_type = VFile_ReadS32(fp);
            switch (edit_type) {
            case FET_TRIGGER_PARAM:
                M_TriggerParameterChange(injection, sector);
                break;
            case FET_MUSIC_ONESHOT:
                M_SetMusicOneShot(sector);
                break;
            case FET_FD_INSERT:
                M_InsertFloorData(injection, sector, level_info);
                break;
            case FET_ROOM_SHIFT:
                M_RoomShift(injection, room);
                break;
            case FET_TRIGGER_ITEM:
                M_TriggeredItem(injection, level_info);
                break;
            default:
                LOG_WARNING("Unknown floor data edit type: %d", edit_type);
                break;
            }
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_TriggerParameterChange(INJECTION *injection, SECTOR *sector)
{
    VFILE *const fp = injection->fp;

    const uint8_t cmd_type = VFile_ReadU8(fp);
    const int16_t old_param = VFile_ReadS16(fp);
    const int16_t new_param = VFile_ReadS16(fp);

    if (sector == NULL || sector->trigger == NULL) {
        return;
    }

    // If we can find an action item for the given sector that matches
    // the command type and old (current) parameter, change it to the
    // new parameter.
    for (int32_t i = 0; i < sector->trigger->command_count; i++) {
        TRIGGER_CMD *const cmd = &sector->trigger->commands[i];
        if (cmd->type != cmd_type) {
            continue;
        }

        if (cmd->type == TO_CAMERA) {
            TRIGGER_CAMERA_DATA *const cam_data =
                (TRIGGER_CAMERA_DATA *)cmd->parameter;
            if (cam_data->camera_num == old_param) {
                cam_data->camera_num = new_param;
                break;
            }
        } else {
            if ((int16_t)(intptr_t)cmd->parameter == old_param) {
                cmd->parameter = (void *)(intptr_t)new_param;
                break;
            }
        }
    }
}

static void M_SetMusicOneShot(SECTOR *sector)
{
    if (sector == NULL || sector->trigger == NULL) {
        return;
    }

    for (int32_t i = 0; i < sector->trigger->command_count; i++) {
        const TRIGGER_CMD *const cmd = &sector->trigger->commands[i];
        if (cmd->type == TO_CD) {
            sector->trigger->one_shot = true;
        }
    }
}

static void M_InsertFloorData(
    INJECTION *injection, SECTOR *sector, LEVEL_INFO *level_info)
{
    VFILE *const fp = injection->fp;

    const int32_t data_length = VFile_ReadS32(fp);

    int16_t data[data_length];
    VFile_Read(fp, data, sizeof(int16_t) * data_length);

    if (sector == NULL) {
        return;
    }

    // This will reset all FD properties in the sector based on the raw data
    // imported. We pass a dummy null index to allow it to read from the
    // beginning of the array.
    Room_PopulateSectorData(sector, data, 0, NULL_FD_INDEX);
}

static void M_RoomShift(INJECTION *injection, int16_t room_num)
{
    VFILE *const fp = injection->fp;

    const uint32_t x_shift = ROUND_TO_SECTOR(VFile_ReadU32(fp));
    const uint32_t z_shift = ROUND_TO_SECTOR(VFile_ReadU32(fp));
    const int32_t y_shift = ROUND_TO_CLICK(VFile_ReadS32(fp));

    ROOM *room = &g_RoomInfo[room_num];
    room->pos.x += x_shift;
    room->pos.z += z_shift;
    room->min_floor += y_shift;
    room->max_ceiling += y_shift;

    // Move any items in the room to match.
    for (int32_t i = 0; i < g_LevelItemCount; i++) {
        ITEM *item = &g_Items[i];
        if (item->room_num != room_num) {
            continue;
        }

        item->pos.x += x_shift;
        item->pos.y += y_shift;
        item->pos.z += z_shift;
    }

    if (!y_shift) {
        return;
    }

    // Update the sector floor and ceiling heights to match.
    for (int32_t i = 0; i < room->size.z * room->size.x; i++) {
        SECTOR *const sector = &room->sectors[i];
        if (sector->floor.height == NO_HEIGHT
            || sector->ceiling.height == NO_HEIGHT) {
            continue;
        }

        sector->floor.height += y_shift;
        sector->ceiling.height += y_shift;
    }

    // Update vertex Y values to match; x and z are room-relative.
    int16_t *data_ptr = room->data;
    int16_t vertex_count = *data_ptr++;
    for (int32_t i = 0; i < vertex_count; i++) {
        *(data_ptr + (i * 4) + 1) += y_shift;
    }
}

static void M_TriggeredItem(INJECTION *injection, LEVEL_INFO *level_info)
{
    VFILE *const fp = injection->fp;

    if (g_LevelItemCount == MAX_ITEMS) {
        VFile_Skip(
            fp, sizeof(int16_t) * 4 + sizeof(int32_t) * 3 + sizeof(uint16_t));
        LOG_WARNING("Cannot add more than %d items", MAX_ITEMS);
        return;
    }

    int16_t item_num = Item_Create();
    ITEM *item = &g_Items[item_num];

    item->object_id = VFile_ReadS16(fp);
    item->room_num = VFile_ReadS16(fp);
    item->pos.x = VFile_ReadS32(fp);
    item->pos.y = VFile_ReadS32(fp);
    item->pos.z = VFile_ReadS32(fp);
    item->rot.y = VFile_ReadS16(fp);
    item->shade = VFile_ReadS16(fp);
    item->flags = VFile_ReadU16(fp);

    level_info->item_count++;
    g_LevelItemCount++;
}

static void M_RoomMeshEdits(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_2) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    ROOM_MESH_EDIT_TYPE edit_type;
    for (int32_t i = 0; i < inj_info->room_mesh_edit_count; i++) {
        edit_type = VFile_ReadS32(fp);

        switch (edit_type) {
        case RMET_TEXTURE_FACE:
            M_TextureRoomFace(injection);
            break;
        case RMET_MOVE_FACE:
            M_MoveRoomFace(injection);
            break;
        case RMET_ALTER_VERTEX:
            M_AlterRoomVertex(injection);
            break;
        case RMET_ROTATE_FACE:
            M_RotateRoomFace(injection);
            break;
        case RMET_ADD_FACE:
            M_AddRoomFace(injection);
            break;
        case RMET_ADD_VERTEX:
            M_AddRoomVertex(injection);
            break;
        default:
            LOG_WARNING("Unknown room mesh edit type: %d", edit_type);
            break;
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_TextureRoomFace(INJECTION *injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    const FACE_TYPE target_face_type = VFile_ReadS32(fp);
    const int16_t target_face = VFile_ReadS16(fp);
    const int16_t source_room = VFile_ReadS16(fp);
    const FACE_TYPE source_face_type = VFile_ReadS32(fp);
    const int16_t source_face = VFile_ReadS16(fp);

    int16_t *source_texture =
        M_GetRoomTexture(source_room, source_face_type, source_face);
    int16_t *target_texture =
        M_GetRoomTexture(target_room, target_face_type, target_face);
    if (source_texture && target_texture) {
        *target_texture = *source_texture;
    }
}

static void M_MoveRoomFace(INJECTION *injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    const FACE_TYPE face_type = VFile_ReadS32(fp);
    const int16_t target_face = VFile_ReadS16(fp);
    const int32_t vertex_count = VFile_ReadS32(fp);

    for (int32_t j = 0; j < vertex_count; j++) {
        const int16_t vertex_index = VFile_ReadS16(fp);
        const int16_t new_vertex = VFile_ReadS16(fp);

        int16_t *target = M_GetRoomFace(target_room, face_type, target_face);
        if (target) {
            target += vertex_index;
            *target = new_vertex;
        }
    }
}

static void M_AlterRoomVertex(INJECTION *injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    VFile_Skip(fp, sizeof(int32_t));
    const int16_t target_vertex = VFile_ReadS16(fp);
    const int16_t x_change = VFile_ReadS16(fp);
    const int16_t y_change = VFile_ReadS16(fp);
    const int16_t z_change = VFile_ReadS16(fp);
    int16_t shade_change = 0;
    if (injection->version >= INJ_VERSION_7) {
        shade_change = VFile_ReadS16(fp);
    }

    if (target_room < 0 || target_room >= g_RoomCount) {
        LOG_WARNING("Room index %d is invalid", target_room);
        return;
    }

    const ROOM *const room = &g_RoomInfo[target_room];
    const int16_t vertex_count = *room->data;
    if (target_vertex < 0 || target_vertex >= vertex_count) {
        LOG_WARNING(
            "Vertex index %d, room %d is invalid", target_vertex, target_room);
        return;
    }

    int16_t *const data_ptr = room->data + target_vertex * 4;
    *(data_ptr + 1) += x_change;
    *(data_ptr + 2) += y_change;
    *(data_ptr + 3) += z_change;
    *(data_ptr + 4) += shade_change;
    CLAMPG(*(data_ptr + 4), MAX_LIGHTING);
}

static void M_RotateRoomFace(INJECTION *injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    const FACE_TYPE face_type = VFile_ReadS32(fp);
    const int16_t target_face = VFile_ReadS16(fp);
    const uint8_t num_rotations = VFile_ReadU8(fp);

    int16_t *target = M_GetRoomFace(target_room, face_type, target_face);
    if (!target) {
        return;
    }

    int32_t num_vertices = face_type == FT_TEXTURED_QUAD ? 4 : 3;
    int16_t *vertices[num_vertices];
    for (int32_t i = 0; i < num_vertices; i++) {
        vertices[i] = target + i;
    }

    for (int32_t i = 0; i < num_rotations; i++) {
        int16_t first = *vertices[0];
        for (int32_t j = 0; j < num_vertices - 1; j++) {
            *vertices[j] = *vertices[j + 1];
        }
        *vertices[num_vertices - 1] = first;
    }
}

static void M_AddRoomFace(INJECTION *injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    const FACE_TYPE face_type = VFile_ReadS32(fp);
    const int16_t source_room = VFile_ReadS16(fp);
    const int16_t source_face = VFile_ReadS16(fp);

    int32_t num_vertices = face_type == FT_TEXTURED_QUAD ? 4 : 3;
    int16_t vertices[num_vertices];
    for (int32_t i = 0; i < num_vertices; i++) {
        vertices[i] = VFile_ReadS16(fp);
    }

    if (target_room < 0 || target_room >= g_RoomCount) {
        LOG_WARNING("Room index %d is invalid", target_room);
        return;
    }

    int16_t *source_texture =
        M_GetRoomTexture(source_room, face_type, source_face);
    if (!source_texture) {
        return;
    }

    ROOM *r = &g_RoomInfo[target_room];
    int32_t data_index = 0;

    int32_t vertex_count = r->data[data_index++];
    data_index += vertex_count * 4;

    // Increment the relevant number of faces and work out the
    // starting point in the mesh for the injection.
    int32_t inject_pos = 0;
    int32_t num_data = r->data[data_index]; // Quads
    if (face_type == FT_TEXTURED_QUAD) {
        r->data[data_index]++;
    }

    data_index += 1 + num_data * 5;
    if (face_type == FT_TEXTURED_QUAD) {
        inject_pos = data_index;
    }

    num_data = r->data[data_index]; // Triangles
    if (face_type == FT_TEXTURED_TRIANGLE) {
        r->data[data_index]++;
    }

    data_index += 1 + num_data * 4;
    if (face_type == FT_TEXTURED_TRIANGLE) {
        inject_pos = data_index;
    }

    num_data = r->data[data_index]; // Sprites
    data_index += num_data * 2;

    // Move everything at the end of the mesh forwards to make space
    // for the new face.
    int32_t inject_length = num_vertices + 1;
    for (int32_t i = data_index; i >= inject_pos; i--) {
        r->data[i + inject_length] = r->data[i];
    }

    // Inject the face data.
    for (int32_t i = 0; i < num_vertices; i++) {
        r->data[inject_pos++] = vertices[i];
    }
    r->data[inject_pos] = *source_texture;
}

static void M_AddRoomVertex(INJECTION *injection)
{
    VFILE *const fp = injection->fp;

    const int16_t target_room = VFile_ReadS16(fp);
    VFile_Skip(fp, sizeof(int32_t));
    const int16_t x = VFile_ReadS16(fp);
    const int16_t y = VFile_ReadS16(fp);
    const int16_t z = VFile_ReadS16(fp);
    const int16_t lighting = VFile_ReadS16(fp);

    ROOM *r = &g_RoomInfo[target_room];
    int32_t data_index = 0;

    int32_t vertex_count = r->data[data_index];
    r->data[data_index++]++;
    data_index += vertex_count * 4;

    int32_t inject_pos = data_index;
    int32_t num_data = r->data[data_index]; // Quads
    data_index += 1 + num_data * 5;

    num_data = r->data[data_index]; // Triangles
    data_index += 1 + num_data * 4;

    num_data = r->data[data_index]; // Sprites
    data_index += num_data * 2;

    // Move everything at the end of the mesh forwards to make space
    // for the new vertex.
    for (int32_t i = data_index; i >= inject_pos; i--) {
        r->data[i + 4] = r->data[i];
    }

    // Inject the vertex data.
    r->data[inject_pos++] = x;
    r->data[inject_pos++] = y;
    r->data[inject_pos++] = z;
    r->data[inject_pos] = lighting;
}

static int16_t *M_GetRoomTexture(
    int16_t room, FACE_TYPE face_type, int16_t face_index)
{
    int16_t *face = M_GetRoomFace(room, face_type, face_index);
    if (face) {
        face += face_type == FT_TEXTURED_QUAD ? 4 : 3;
    }
    return face;
}

static int16_t *M_GetRoomFace(
    int16_t room, FACE_TYPE face_type, int16_t face_index)
{
    ROOM *r = NULL;
    if (room < 0 || room >= g_RoomCount) {
        LOG_WARNING("Room index %d is invalid", room);
        return NULL;
    }

    r = &g_RoomInfo[room];
    int16_t *data_ptr = r->data;

    int32_t vertex_count = *data_ptr++;
    data_ptr += vertex_count * 4;

    int32_t num_faces = *data_ptr++;
    if (face_type == FT_TEXTURED_QUAD) {
        if (face_index < 0 || face_index >= num_faces) {
            LOG_WARNING("Quad index %d, room %d is invalid", face_index, room);
            return NULL;
        }
        data_ptr += face_index * 5;
        return data_ptr;
    }

    data_ptr += 5 * num_faces;
    num_faces = *data_ptr++;
    if (face_type == FT_TEXTURED_TRIANGLE) {
        if (face_index < 0 || face_index >= num_faces) {
            LOG_WARNING(
                "Triangle index %d, room %d is invalid", face_index, room);
            return NULL;
        }
        data_ptr += face_index * 4;
        return data_ptr;
    }

    return NULL;
}

static void M_RoomDoorEdits(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_2) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->room_door_edit_count; i++) {
        const int16_t base_room = VFile_ReadS16(fp);
        const int16_t link_room = VFile_ReadS16(fp);
        int16_t door_index = -1;
        if (injection->version >= INJ_VERSION_4) {
            door_index = VFile_ReadS16(fp);
        }

        if (base_room < 0 || base_room >= g_RoomCount) {
            VFile_Skip(fp, sizeof(int16_t) * 12);
            LOG_WARNING("Room index %d is invalid", base_room);
            continue;
        }

        ROOM *r = &g_RoomInfo[base_room];
        PORTAL *portal = NULL;
        for (int32_t j = 0; j < r->portals->count; j++) {
            PORTAL d = r->portals->portal[j];
            if (d.room_num == link_room
                && (j == door_index || door_index == -1)) {
                portal = &r->portals->portal[j];
                break;
            }
        }

        if (portal == NULL) {
            VFile_Skip(fp, sizeof(int16_t) * 12);
            LOG_WARNING(
                "Room index %d has no matching portal to %d", base_room,
                link_room);
            continue;
        }

        for (int32_t j = 0; j < 4; j++) {
            const int16_t x_change = VFile_ReadS16(fp);
            const int16_t y_change = VFile_ReadS16(fp);
            const int16_t z_change = VFile_ReadS16(fp);

            portal->vertex[j].x += x_change;
            portal->vertex[j].y += y_change;
            portal->vertex[j].z += z_change;
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void M_ItemPositions(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_4) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    INJECTION_INFO *inj_info = injection->info;
    VFILE *const fp = injection->fp;

    for (int32_t i = 0; i < inj_info->item_position_count; i++) {
        const int16_t item_num = VFile_ReadS16(fp);
        const int16_t y_rot = VFile_ReadS16(fp);

        int32_t x;
        int32_t y;
        int32_t z;
        int16_t room_num;
        if (injection->version > INJ_VERSION_4) {
            x = VFile_ReadS32(fp);
            y = VFile_ReadS32(fp);
            z = VFile_ReadS32(fp);
            room_num = VFile_ReadS16(fp);
        }

        if (item_num < 0 || item_num >= g_LevelItemCount) {
            LOG_WARNING("Item number %d is out of level item range", item_num);
            continue;
        }

        ITEM *item = &g_Items[item_num];
        item->rot.y = y_rot;
        if (injection->version > INJ_VERSION_4) {
            item->pos.x = x;
            item->pos.y = y;
            item->pos.z = z;
            item->room_num = room_num;
        }
    }

    Benchmark_End(benchmark, NULL);
}

void Inject_Init(
    int32_t num_injections, char *filenames[], INJECTION_INFO *aggregate)
{
    m_NumInjections = num_injections;
    if (!num_injections) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    m_Injections = Memory_Alloc(sizeof(INJECTION) * num_injections);
    m_Aggregate = aggregate;

    for (int32_t i = 0; i < num_injections; i++) {
        M_LoadFromFile(&m_Injections[i], filenames[i]);
    }

    Benchmark_End(benchmark, NULL);
}

void Inject_AllInjections(LEVEL_INFO *level_info)
{
    if (!m_Injections) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    uint16_t palette_map[256];
    RGBA_8888 *source_pages = Memory_Alloc(
        m_Aggregate->texture_page_count * PAGE_SIZE * sizeof(RGBA_8888));
    int32_t source_page_count = 0;
    int32_t tpage_base = level_info->texture_page_count;

    for (int32_t i = 0; i < m_NumInjections; i++) {
        INJECTION *injection = &m_Injections[i];
        if (!injection->relevant) {
            continue;
        }

        M_LoadTexturePages(
            injection, level_info, palette_map,
            source_pages + (source_page_count * PAGE_SIZE));

        M_TextureData(injection, level_info, tpage_base);
        M_MeshData(injection, level_info);
        M_AnimData(injection, level_info);
        M_ObjectData(injection, level_info, palette_map);
        M_SFXData(injection, level_info);

        M_MeshEdits(injection, palette_map);
        M_TextureOverwrites(injection, level_info, palette_map);
        M_FloorDataEdits(injection, level_info);
        M_RoomMeshEdits(injection);
        M_RoomDoorEdits(injection);
        M_AnimRangeEdits(injection);

        M_ItemPositions(injection);

        // Realign base indices for the next injection.
        INJECTION_INFO *inj_info = injection->info;
        level_info->anim_command_count += inj_info->anim_cmd_count;
        level_info->anim_bone_count += inj_info->anim_bone_count;
        level_info->anim_frame_data_count += inj_info->anim_frame_data_count;
        level_info->anim_frame_count += inj_info->anim_frame_count;
        level_info->anim_frame_mesh_rot_count +=
            inj_info->anim_frame_mesh_rot_count;
        level_info->anim_count += inj_info->anim_count;
        level_info->mesh_ptr_count += inj_info->mesh_ptr_count;
        level_info->texture_count += inj_info->texture_count;
        source_page_count += inj_info->texture_page_count;
        tpage_base += inj_info->texture_page_count;
    }

    if (source_page_count) {
        PACKER_DATA *data = Memory_Alloc(sizeof(PACKER_DATA));
        data->level_page_count = level_info->texture_page_count;
        data->source_page_count = source_page_count;
        data->source_pages = source_pages;
        data->level_pages = level_info->texture_rgb_page_ptrs;
        data->object_count = level_info->texture_count;
        data->sprite_count = level_info->sprite_info_count;

        if (Packer_Pack(data)) {
            level_info->texture_page_count += Packer_GetAddedPageCount();
            level_info->texture_rgb_page_ptrs = data->level_pages;
        }

        Memory_FreePointer(&source_pages);
        Memory_FreePointer(&data);
    }

    Benchmark_End(benchmark, NULL);
}

void Inject_Cleanup(void)
{
    if (!m_NumInjections) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();

    for (int32_t i = 0; i < m_NumInjections; i++) {
        INJECTION *injection = &m_Injections[i];
        if (injection->fp) {
            VFile_Close(injection->fp);
        }
        if (injection->info) {
            if (injection->info->room_meshes) {
                Memory_FreePointer(&injection->info->room_meshes);
            }
            Memory_FreePointer(&injection->info);
        }
    }

    Memory_FreePointer(&m_Injections);
    Benchmark_End(benchmark, NULL);
}

uint32_t Inject_GetExtraRoomMeshSize(int32_t room_index)
{
    uint32_t size = 0;
    if (!m_Injections) {
        return size;
    }

    for (int32_t i = 0; i < m_NumInjections; i++) {
        INJECTION *injection = &m_Injections[i];
        if (!injection->relevant || injection->version < INJ_VERSION_2) {
            continue;
        }

        INJECTION_INFO *inj_info = injection->info;
        for (int32_t j = 0; j < inj_info->room_mesh_count; j++) {
            INJECTION_ROOM_MESH *mesh = &inj_info->room_meshes[j];
            if (mesh->room_index == room_index) {
                size += mesh->extra_size;
            }
        }
    }

    return size;
}
