#include "game/inject.h"

#include "config.h"
#include "game/gamebuf.h"
#include "game/output.h"
#include "game/packer.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"
#include "items.h"

#include <libtrx/filesystem.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#define INJECTION_MAGIC MKTAG('T', '1', 'M', 'J')
#define INJECTION_CURRENT_VERSION 8
#define NULL_FD_INDEX ((uint16_t)(-1))

typedef enum INJECTION_VERSION {
    INJ_VERSION_1 = 1,
    INJ_VERSION_2 = 2,
    INJ_VERSION_3 = 3,
    INJ_VERSION_4 = 4,
    INJ_VERSION_5 = 5,
    INJ_VERSION_6 = 6,
    INJ_VERSION_7 = 7,
    INJ_VERSION_8 = 8,
} INJECTION_VERSION;

typedef enum INJECTION_TYPE {
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
} INJECTION_TYPE;

typedef struct INJECTION {
    MYFILE *fp;
    INJECTION_VERSION version;
    INJECTION_TYPE type;
    INJECTION_INFO *info;
    bool relevant;
} INJECTION;

typedef enum FACE_TYPE {
    FT_TEXTURED_QUAD = 0,
    FT_TEXTURED_TRIANGLE = 1,
    FT_COLOURED_QUAD = 2,
    FT_COLOURED_TRIANGLE = 3
} FACE_TYPE;

typedef struct FACE_EDIT {
    GAME_OBJECT_ID object_id;
    int16_t source_identifier;
    FACE_TYPE face_type;
    int16_t face_index;
    int32_t target_count;
    int16_t *targets;
} FACE_EDIT;

typedef struct VERTEX_EDIT {
    int16_t vertex_index;
    int16_t x_change;
    int16_t y_change;
    int16_t z_change;
} VERTEX_EDIT;

typedef struct MESH_EDIT {
    GAME_OBJECT_ID object_id;
    int16_t mesh_index;
    XYZ_16 centre_shift;
    int32_t radius_shift;
    int32_t face_edit_count;
    int32_t vertex_edit_count;
    FACE_EDIT *face_edits;
    VERTEX_EDIT *vertex_edits;
} MESH_EDIT;

typedef enum FLOOR_EDIT_TYPE {
    FET_TRIGGER_PARAM = 0,
    FET_MUSIC_ONESHOT = 1,
    FET_FD_INSERT = 2,
    FET_ROOM_SHIFT = 3,
    FET_TRIGGER_ITEM = 4,
} FLOOR_EDIT_TYPE;

typedef enum ROOM_MESH_EDIT_TYPE {
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

static void Inject_LoadFromFile(INJECTION *injection, const char *filename);

static uint8_t Inject_RemapRGB(RGB_888 rgb);
static void Inject_AlignTextureReferences(
    OBJECT_INFO *object, uint8_t *palette_map, int32_t page_base);

static void Inject_LoadTexturePages(
    INJECTION *injection, uint8_t *palette_map, uint8_t *page_ptr);
static void Inject_TextureData(
    INJECTION *injection, LEVEL_INFO *level_info, int32_t page_base);
static void Inject_MeshData(INJECTION *injection, LEVEL_INFO *level_info);
static void Inject_AnimData(INJECTION *injection, LEVEL_INFO *level_info);
static void Inject_AnimRangeEdits(INJECTION *injection);
static void Inject_ObjectData(
    INJECTION *injection, LEVEL_INFO *level_info, uint8_t *palette_map);
static void Inject_SFXData(INJECTION *injection, LEVEL_INFO *level_info);

static int16_t *Inject_GetMeshTexture(FACE_EDIT *face_edit);

static void Inject_ApplyFaceEdit(
    FACE_EDIT *face_edit, int16_t *data_ptr, int16_t texture);
static void Inject_ApplyMeshEdit(MESH_EDIT *mesh_edit, uint8_t *palette_map);
static void Inject_MeshEdits(INJECTION *injection, uint8_t *palette_map);
static void Inject_TextureOverwrites(
    INJECTION *injection, LEVEL_INFO *level_info, uint8_t *palette_map);

static void Inject_FloorDataEdits(INJECTION *injection, LEVEL_INFO *level_info);
static void Inject_TriggerParameterChange(
    INJECTION *injection, SECTOR_INFO *sector);
static void Inject_SetMusicOneShot(SECTOR_INFO *sector);
static void Inject_InsertFloorData(
    INJECTION *injection, SECTOR_INFO *sector, LEVEL_INFO *level_info);
static void Inject_RoomShift(INJECTION *injection, int16_t room_num);
static void Inject_TriggeredItem(INJECTION *injection, LEVEL_INFO *level_info);

static void Inject_RoomMeshEdits(INJECTION *injection);
static void Inject_TextureRoomFace(INJECTION *injection);
static void Inject_MoveRoomFace(INJECTION *injection);
static void Inject_AlterRoomVertex(INJECTION *injection);
static void Inject_RotateRoomFace(INJECTION *injection);
static void Inject_AddRoomFace(INJECTION *injection);
static void Inject_AddRoomVertex(INJECTION *injection);

static int16_t *Inject_GetRoomTexture(
    int16_t room, FACE_TYPE face_type, int16_t face_index);
static int16_t *Inject_GetRoomFace(
    int16_t room, FACE_TYPE face_type, int16_t face_index);

static void Inject_RoomDoorEdits(INJECTION *injection);

static void Inject_ItemPositions(INJECTION *injection);

void Inject_Init(
    int32_t num_injections, char *filenames[], INJECTION_INFO *aggregate)
{
    m_NumInjections = num_injections;
    if (!num_injections) {
        return;
    }

    m_Injections = Memory_Alloc(sizeof(INJECTION) * num_injections);
    m_Aggregate = aggregate;

    for (int32_t i = 0; i < num_injections; i++) {
        Inject_LoadFromFile(&m_Injections[i], filenames[i]);
    }
}

static void Inject_LoadFromFile(INJECTION *injection, const char *filename)
{
    injection->relevant = false;
    injection->info = NULL;

    MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
    injection->fp = fp;
    if (!fp) {
        LOG_WARNING("Could not open %s", filename);
        return;
    }

    const uint32_t magic = File_ReadU32(fp);
    if (magic != INJECTION_MAGIC) {
        LOG_WARNING("Invalid injection magic in %s", filename);
        return;
    }

    injection->version = File_ReadS32(fp);
    if (injection->version < INJ_VERSION_1
        || injection->version > INJECTION_CURRENT_VERSION) {
        LOG_WARNING(
            "%s uses unsupported version %d", filename, injection->version);
        return;
    }

    injection->type = File_ReadS32(fp);

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
    default:
        LOG_WARNING("%s is of unknown type %d", filename, injection->type);
        break;
    }

    if (!injection->relevant) {
        return;
    }

    injection->info = Memory_Alloc(sizeof(INJECTION_INFO));
    INJECTION_INFO *info = injection->info;

    info->texture_page_count = File_ReadS32(fp);
    info->texture_count = File_ReadS32(fp);
    info->sprite_info_count = File_ReadS32(fp);
    info->sprite_count = File_ReadS32(fp);
    info->mesh_count = File_ReadS32(fp);
    info->mesh_ptr_count = File_ReadS32(fp);
    info->anim_change_count = File_ReadS32(fp);
    info->anim_range_count = File_ReadS32(fp);
    info->anim_cmd_count = File_ReadS32(fp);
    info->anim_bone_count = File_ReadS32(fp);
    info->anim_frame_data_count = File_ReadS32(fp);
    info->anim_count = File_ReadS32(fp);
    info->object_count = File_ReadS32(fp);
    info->sfx_count = File_ReadS32(fp);
    info->sfx_data_size = File_ReadS32(fp);
    info->sample_count = File_ReadS32(fp);
    info->mesh_edit_count = File_ReadS32(fp);
    info->texture_overwrite_count = File_ReadS32(fp);
    info->floor_edit_count = File_ReadS32(fp);

    if (injection->version < INJ_VERSION_8) {
        // Legacy value that stored the total injected floor data length.
        File_Skip(fp, sizeof(int32_t));
    }

    if (injection->version > INJ_VERSION_1) {
        // room_mesh_count is a summary of the change in mesh size,
        // while room_mesh_edit_count indicates how many edits to
        // read and interpret (not all edits incur a size change).
        info->room_mesh_count = File_ReadU32(fp);
        info->room_meshes =
            Memory_Alloc(sizeof(INJECTION_ROOM_MESH) * info->room_mesh_count);
        for (int32_t i = 0; i < info->room_mesh_count; i++) {
            INJECTION_ROOM_MESH *mesh = &info->room_meshes[i];
            mesh->room_index = File_ReadS16(fp);
            mesh->extra_size = File_ReadU32(fp);
        }

        info->room_mesh_edit_count = File_ReadU32(fp);
        info->room_door_edit_count = File_ReadU32(fp);
    } else {
        info->room_meshes = NULL;
    }

    if (injection->version > INJ_VERSION_2) {
        info->anim_range_edit_count = File_ReadS32(fp);
    } else {
        info->anim_range_edit_count = 0;
    }

    if (injection->version > INJ_VERSION_3) {
        info->item_position_count = File_ReadS32(fp);
    } else {
        info->item_position_count = 0;
    }

    // Get detailed frame counts
    {
        const size_t prev_pos = File_Pos(fp);

        // texture pages
        File_Skip(fp, 256 * 3);
        File_Skip(fp, info->texture_page_count * PAGE_SIZE);

        // textures
        File_Skip(fp, info->texture_count * 20);

        // sprites
        File_Skip(fp, info->sprite_info_count * 16);
        File_Skip(fp, info->sprite_count * 8);

        // meshes
        File_Skip(fp, info->mesh_count * 2);
        File_Skip(fp, info->mesh_ptr_count * 4);

        File_Skip(fp, info->anim_change_count * 6);
        File_Skip(fp, info->anim_range_count * 8);
        File_Skip(fp, info->anim_cmd_count * 2);
        File_Skip(fp, info->anim_bone_count * 4);

        info->anim_frame_count = 0;
        info->anim_frame_mesh_rot_count = 0;
        const size_t frame_data_start = File_Pos(fp);
        const size_t frame_data_end =
            frame_data_start + info->anim_frame_data_count * sizeof(int16_t);
        while (File_Pos(fp) < frame_data_end) {
            File_Skip(fp, 9 * sizeof(int16_t));
            const int16_t num_meshes = File_ReadS16(fp);
            File_Skip(fp, num_meshes * sizeof(int32_t));
            info->anim_frame_count++;
            info->anim_frame_mesh_rot_count += num_meshes;
        }
        assert(File_Pos(fp) == frame_data_end);

        File_Seek(fp, prev_pos, FILE_SEEK_SET);
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

void Inject_AllInjections(LEVEL_INFO *level_info)
{
    if (!m_Injections) {
        return;
    }

    uint8_t palette_map[256];
    uint8_t *source_pages =
        Memory_Alloc(m_Aggregate->texture_page_count * PAGE_SIZE);
    int32_t source_page_count = 0;
    int32_t tpage_base = level_info->texture_page_count;

    for (int32_t i = 0; i < m_NumInjections; i++) {
        INJECTION *injection = &m_Injections[i];
        if (!injection->relevant) {
            continue;
        }

        Inject_LoadTexturePages(
            injection, palette_map,
            source_pages + (source_page_count * PAGE_SIZE));

        Inject_TextureData(injection, level_info, tpage_base);
        Inject_MeshData(injection, level_info);
        Inject_AnimData(injection, level_info);
        Inject_ObjectData(injection, level_info, palette_map);
        Inject_SFXData(injection, level_info);

        Inject_MeshEdits(injection, palette_map);
        Inject_TextureOverwrites(injection, level_info, palette_map);
        Inject_FloorDataEdits(injection, level_info);
        Inject_RoomMeshEdits(injection);
        Inject_RoomDoorEdits(injection);
        Inject_AnimRangeEdits(injection);

        Inject_ItemPositions(injection);

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
        data->level_pages = level_info->texture_page_ptrs;
        data->object_count = level_info->texture_count;
        data->sprite_count = level_info->sprite_info_count;

        if (Packer_Pack(data)) {
            level_info->texture_page_count += Packer_GetAddedPageCount();
            level_info->texture_page_ptrs = data->level_pages;
        }

        Memory_FreePointer(&source_pages);
        Memory_FreePointer(&data);
    }
}

static void Inject_LoadTexturePages(
    INJECTION *injection, uint8_t *palette_map, uint8_t *page_ptr)
{
    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    palette_map[0] = 0;
    RGB_888 source_palette[256];
    for (int32_t i = 0; i < 256; i++) {
        source_palette[i].r = File_ReadU8(fp);
        source_palette[i].g = File_ReadU8(fp);
        source_palette[i].b = File_ReadU8(fp);
    }
    for (int32_t i = 1; i < 256; i++) {
        source_palette[i].r *= 4;
        source_palette[i].g *= 4;
        source_palette[i].b *= 4;

        palette_map[i] = Inject_RemapRGB(source_palette[i]);
    }

    // Read in each page for this injection and realign the pixels
    // to the level's palette.
    File_ReadItems(fp, page_ptr, PAGE_SIZE, inj_info->texture_page_count);
    int32_t pixel_count = PAGE_SIZE * inj_info->texture_page_count;
    for (int32_t i = 0; i < pixel_count; i++, page_ptr++) {
        *page_ptr = palette_map[*page_ptr];
    }
}

static void Inject_TextureData(
    INJECTION *injection, LEVEL_INFO *level_info, int32_t page_base)
{
    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    // Read the tex_infos and align them to the end of the page list.
    for (int32_t i = 0; i < inj_info->texture_count; i++) {
        PHD_TEXTURE *texture = &g_PhdTextureInfo[level_info->texture_count + i];
        texture->drawtype = File_ReadU16(fp);
        texture->tpage = File_ReadU16(fp);
        for (int32_t j = 0; j < 4; j++) {
            texture->uv[j].u = File_ReadU16(fp);
            texture->uv[j].v = File_ReadU16(fp);
        }
        g_PhdTextureInfo[level_info->texture_count + i].tpage += page_base;
    }

    for (int32_t i = 0; i < inj_info->sprite_info_count; i++) {
        PHD_SPRITE *sprite =
            &g_PhdSpriteInfo[level_info->sprite_info_count + i];
        sprite->tpage = File_ReadU16(fp);
        sprite->offset = File_ReadU16(fp);
        sprite->width = File_ReadU16(fp);
        sprite->height = File_ReadU16(fp);
        sprite->x1 = File_ReadS16(fp);
        sprite->y1 = File_ReadS16(fp);
        sprite->x2 = File_ReadS16(fp);
        sprite->y2 = File_ReadS16(fp);
        g_PhdSpriteInfo[level_info->sprite_info_count + i].tpage += page_base;
    }

    for (int32_t i = 0; i < inj_info->sprite_count; i++) {
        const GAME_OBJECT_ID object_id = File_ReadS32(fp);
        const int16_t num_meshes = File_ReadS16(fp);
        const int16_t mesh_index = File_ReadS16(fp);

        if (object_id < O_NUMBER_OF) {
            OBJECT_INFO *object = &g_Objects[object_id];
            object->nmeshes = num_meshes;
            object->mesh_index = mesh_index + level_info->sprite_info_count;
            object->loaded = 1;
        } else if (object_id - O_NUMBER_OF < STATIC_NUMBER_OF) {
            STATIC_INFO *object = &g_StaticObjects[object_id - O_NUMBER_OF];
            object->nmeshes = num_meshes;
            object->mesh_num = mesh_index + level_info->sprite_info_count;
            object->loaded = true;
        }
        level_info->sprite_info_count -= num_meshes;
        level_info->sprite_count++;
    }
}

static void Inject_MeshData(INJECTION *injection, LEVEL_INFO *level_info)
{
    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    int32_t mesh_base = level_info->mesh_count;
    File_ReadItems(
        fp, g_MeshBase + mesh_base, sizeof(int16_t), inj_info->mesh_count);
    level_info->mesh_count += inj_info->mesh_count;

    uint32_t *mesh_indices = GameBuf_Alloc(
        sizeof(uint32_t) * inj_info->mesh_ptr_count, GBUF_MESH_POINTERS);
    File_ReadItems(
        fp, mesh_indices, sizeof(uint32_t), inj_info->mesh_ptr_count);

    for (int32_t i = 0; i < inj_info->mesh_ptr_count; i++) {
        g_Meshes[level_info->mesh_ptr_count + i] =
            &g_MeshBase[mesh_base + mesh_indices[i] / 2];
    }
}

static void Inject_AnimData(INJECTION *injection, LEVEL_INFO *level_info)
{
    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    for (int32_t i = 0; i < inj_info->anim_change_count; i++) {
        ANIM_CHANGE_STRUCT *anim_change =
            &g_AnimChanges[level_info->anim_change_count + i];
        anim_change->goal_anim_state = File_ReadS16(fp);
        anim_change->number_ranges = File_ReadS16(fp);
        anim_change->range_index = File_ReadS16(fp);
    }
    for (int32_t i = 0; i < inj_info->anim_range_count; i++) {
        ANIM_RANGE_STRUCT *anim_range =
            &g_AnimRanges[level_info->anim_range_count + i];
        anim_range->start_frame = File_ReadS16(fp);
        anim_range->end_frame = File_ReadS16(fp);
        anim_range->link_anim_num = File_ReadS16(fp);
        anim_range->link_frame_num = File_ReadS16(fp);
    }
    File_ReadItems(
        fp, g_AnimCommands + level_info->anim_command_count, sizeof(int16_t),
        inj_info->anim_cmd_count);
    File_ReadItems(
        fp, g_AnimBones + level_info->anim_bone_count, sizeof(int32_t),
        inj_info->anim_bone_count);
    const size_t frame_data_start = File_Pos(fp);
    File_Skip(fp, inj_info->anim_frame_data_count * sizeof(int16_t));
    const size_t frame_data_end = File_Pos(fp);

    File_Seek(fp, frame_data_start, FILE_SEEK_SET);
    int32_t *mesh_rots =
        &g_AnimFrameMeshRots[level_info->anim_frame_mesh_rot_count];
    for (int32_t i = 0; i < inj_info->anim_frame_count; i++) {
        level_info->anim_frame_offsets[level_info->anim_frame_count + i] =
            File_Pos(fp) - frame_data_start;
        FRAME_INFO *const frame =
            &g_AnimFrames[level_info->anim_frame_count + i];
        frame->bounds.min.x = File_ReadS16(fp);
        frame->bounds.max.x = File_ReadS16(fp);
        frame->bounds.min.y = File_ReadS16(fp);
        frame->bounds.max.y = File_ReadS16(fp);
        frame->bounds.min.z = File_ReadS16(fp);
        frame->bounds.max.z = File_ReadS16(fp);
        frame->offset.x = File_ReadS16(fp);
        frame->offset.y = File_ReadS16(fp);
        frame->offset.z = File_ReadS16(fp);
        frame->nmeshes = File_ReadS16(fp);
        frame->mesh_rots = mesh_rots;
        File_ReadItems(fp, mesh_rots, sizeof(int32_t), frame->nmeshes);
        mesh_rots += frame->nmeshes;
    }
    assert(File_Pos(fp) == frame_data_end);

    for (int32_t i = 0; i < inj_info->anim_count; i++) {
        ANIM_STRUCT *anim = &g_Anims[level_info->anim_count + i];

        anim->frame_ofs = File_ReadU32(fp);
        anim->interpolation = File_ReadS16(fp);
        anim->current_anim_state = File_ReadS16(fp);
        anim->velocity = File_ReadS32(fp);
        anim->acceleration = File_ReadS32(fp);
        anim->frame_base = File_ReadS16(fp);
        anim->frame_end = File_ReadS16(fp);
        anim->jump_anim_num = File_ReadS16(fp);
        anim->jump_frame_num = File_ReadS16(fp);
        anim->number_changes = File_ReadS16(fp);
        anim->change_index = File_ReadS16(fp);
        anim->number_commands = File_ReadS16(fp);
        anim->command_index = File_ReadS16(fp);

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
        if (anim->number_changes) {
            anim->change_index += level_info->anim_change_count;
        }
        if (anim->number_commands) {
            anim->command_index += level_info->anim_command_count;
        }
    }

    // Re-align to the level.
    for (int32_t i = 0; i < inj_info->anim_change_count; i++) {
        g_AnimChanges[level_info->anim_change_count++].range_index +=
            level_info->anim_range_count;
    }

    for (int32_t i = 0; i < inj_info->anim_range_count; i++) {
        g_AnimRanges[level_info->anim_range_count++].link_anim_num +=
            level_info->anim_count;
    }
}

static void Inject_AnimRangeEdits(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_3) {
        return;
    }

    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    for (int32_t i = 0; i < inj_info->anim_range_edit_count; i++) {
        const GAME_OBJECT_ID object_id = File_ReadS32(fp);
        const int16_t anim_index = File_ReadS16(fp);
        const int32_t edit_count = File_ReadS32(fp);

        if (object_id < 0 || object_id >= O_NUMBER_OF) {
            LOG_WARNING("Object %d is not recognised", object_id);
            File_Skip(fp, edit_count * sizeof(int16_t) * 4);
            continue;
        }

        OBJECT_INFO *object = &g_Objects[object_id];
        if (!object->loaded) {
            LOG_WARNING("Object %d is not loaded", object_id);
            File_Skip(fp, edit_count * sizeof(int16_t) * 4);
            continue;
        }

        ANIM_STRUCT *anim = &g_Anims[object->anim_index + anim_index];
        for (int32_t j = 0; j < edit_count; j++) {
            const int16_t change_index = File_ReadS16(fp);
            const int16_t range_index = File_ReadS16(fp);
            const int16_t low_frame = File_ReadS16(fp);
            const int16_t high_frame = File_ReadS16(fp);

            if (change_index >= anim->number_changes) {
                LOG_WARNING(
                    "Change %d is invalid for animation %d", change_index,
                    anim_index);
                continue;
            }
            ANIM_CHANGE_STRUCT *change =
                &g_AnimChanges[anim->change_index + change_index];

            if (range_index >= change->number_ranges) {
                LOG_WARNING(
                    "Range %d is invalid for change %d, animation %d",
                    range_index, change_index, anim_index);
                continue;
            }
            ANIM_RANGE_STRUCT *range =
                &g_AnimRanges[change->range_index + range_index];

            range->start_frame = low_frame;
            range->end_frame = high_frame;
        }
    }
}

static void Inject_ObjectData(
    INJECTION *injection, LEVEL_INFO *level_info, uint8_t *palette_map)
{
    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    // TODO: consider refactoring once we have more injection
    // use cases.
    for (int32_t i = 0; i < inj_info->object_count; i++) {
        const GAME_OBJECT_ID object_id = File_ReadS32(fp);
        OBJECT_INFO *object = &g_Objects[object_id];

        const int16_t num_meshes = File_ReadS16(fp);
        const int16_t mesh_index = File_ReadS16(fp);
        const int32_t bone_index = File_ReadS32(fp);

        // When mesh data has been omitted from the injection, this indicates
        // that we wish to retain what's already defined so to avoid duplicate
        // packing.
        if (!object->loaded || num_meshes) {
            object->nmeshes = num_meshes;
            object->mesh_index = mesh_index + level_info->mesh_ptr_count;
            object->bone_index = bone_index + level_info->anim_bone_count;
        }

        const int32_t frame_offset = File_ReadS32(fp);
        object->anim_index = File_ReadS16(fp);
        object->anim_index += level_info->anim_count;
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
            Inject_AlignTextureReferences(
                object, palette_map, level_info->texture_count);
        }
    }
}

static void Inject_SFXData(INJECTION *injection, LEVEL_INFO *level_info)
{
    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    for (int32_t i = 0; i < inj_info->sfx_count; i++) {
        const int16_t sfx_id = File_ReadS16(fp);
        g_SampleLUT[sfx_id] = level_info->sample_info_count;

        SAMPLE_INFO *sample_info =
            &g_SampleInfos[level_info->sample_info_count];
        sample_info->volume = File_ReadS16(fp);
        sample_info->randomness = File_ReadS16(fp);
        sample_info->flags = File_ReadS16(fp);
        sample_info->number = level_info->sample_count;

        int16_t num_samples = (sample_info->flags >> 2) & 15;
        for (int32_t j = 0; j < num_samples; j++) {
            const int32_t sample_length = File_ReadS32(fp);
            File_ReadItems(
                fp, level_info->sample_data + level_info->sample_data_size,
                sizeof(char), sample_length);

            level_info->sample_offsets[level_info->sample_count] =
                level_info->sample_data_size;
            level_info->sample_data_size += sample_length;
            level_info->sample_count++;
        }

        level_info->sample_info_count++;
    }
}

static void Inject_AlignTextureReferences(
    OBJECT_INFO *object, uint8_t *palette_map, int32_t page_base)
{
    int16_t **mesh = &g_Meshes[object->mesh_index];
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

static uint8_t Inject_RemapRGB(RGB_888 rgb)
{
    // Find the index of the nearest match to the given RGB
    int32_t best_match = 0x7fffffff;
    int32_t test_match;
    int32_t best_index = 0;
    for (int32_t i = 1; i < 256; i++) {
        const RGB_888 test_rgb = Output_GetPaletteColor(i);
        const int32_t r = rgb.r - test_rgb.r;
        const int32_t g = rgb.g - test_rgb.g;
        const int32_t b = rgb.b - test_rgb.b;
        test_match = SQUARE(r) + SQUARE(g) + SQUARE(b);

        if (test_match < best_match) {
            best_match = test_match;
            best_index = i;
        }
    }
    return best_index;
}

static void Inject_MeshEdits(INJECTION *injection, uint8_t *palette_map)
{
    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    if (!inj_info->mesh_edit_count) {
        return;
    }

    MESH_EDIT *mesh_edits =
        Memory_Alloc(sizeof(MESH_EDIT) * inj_info->mesh_edit_count);

    for (int32_t i = 0; i < inj_info->mesh_edit_count; i++) {
        MESH_EDIT *mesh_edit = &mesh_edits[i];
        mesh_edit->object_id = File_ReadS32(fp);
        mesh_edit->mesh_index = File_ReadS16(fp);

        if (injection->version >= INJ_VERSION_6) {
            mesh_edit->centre_shift.x = File_ReadS16(fp);
            mesh_edit->centre_shift.y = File_ReadS16(fp);
            mesh_edit->centre_shift.z = File_ReadS16(fp);
            mesh_edit->radius_shift = File_ReadS32(fp);
        }

        mesh_edit->face_edit_count = File_ReadS32(fp);
        mesh_edit->face_edits =
            Memory_Alloc(sizeof(FACE_EDIT) * mesh_edit->face_edit_count);
        for (int32_t j = 0; j < mesh_edit->face_edit_count; j++) {
            FACE_EDIT *face_edit = &mesh_edit->face_edits[j];
            face_edit->object_id = File_ReadS32(fp);
            face_edit->source_identifier = File_ReadS16(fp);
            face_edit->face_type = File_ReadS32(fp);
            face_edit->face_index = File_ReadS16(fp);

            face_edit->target_count = File_ReadS32(fp);
            face_edit->targets =
                Memory_Alloc(sizeof(int16_t) * face_edit->target_count);
            File_ReadItems(
                fp, face_edit->targets, sizeof(int16_t),
                face_edit->target_count);
        }

        mesh_edit->vertex_edit_count = File_ReadS32(fp);
        mesh_edit->vertex_edits =
            Memory_Alloc(sizeof(VERTEX_EDIT) * mesh_edit->vertex_edit_count);
        for (int32_t i = 0; i < mesh_edit->vertex_edit_count; i++) {
            VERTEX_EDIT *vertex_edit = &mesh_edit->vertex_edits[i];
            vertex_edit->vertex_index = File_ReadS16(fp);
            vertex_edit->x_change = File_ReadS16(fp);
            vertex_edit->y_change = File_ReadS16(fp);
            vertex_edit->z_change = File_ReadS16(fp);
        }

        Inject_ApplyMeshEdit(mesh_edit, palette_map);

        for (int32_t j = 0; j < mesh_edit->face_edit_count; j++) {
            FACE_EDIT *face_edit = &mesh_edit->face_edits[j];
            Memory_FreePointer(&face_edit->targets);
        }

        Memory_FreePointer(&mesh_edit->face_edits);
        Memory_FreePointer(&mesh_edit->vertex_edits);
    }

    Memory_FreePointer(&mesh_edits);
}

static void Inject_ApplyMeshEdit(MESH_EDIT *mesh_edit, uint8_t *palette_map)
{
    OBJECT_INFO object = g_Objects[mesh_edit->object_id];
    if (!object.loaded) {
        return;
    }

    int16_t **mesh = &g_Meshes[object.mesh_index];
    int16_t *data_ptr = *(mesh + mesh_edit->mesh_index);

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
            int16_t *tex_ptr = Inject_GetMeshTexture(face_edit);
            if (!tex_ptr) {
                continue;
            }
            texture = *tex_ptr;
        }

        data_ptr = data_start;

        int32_t num_faces = *data_ptr++;
        if (face_edit->face_type == FT_TEXTURED_QUAD) {
            Inject_ApplyFaceEdit(face_edit, data_ptr, texture);
        }

        data_ptr += 5 * num_faces;
        num_faces = *data_ptr++;
        if (face_edit->face_type == FT_TEXTURED_TRIANGLE) {
            Inject_ApplyFaceEdit(face_edit, data_ptr, texture);
        }

        data_ptr += 4 * num_faces;
        num_faces = *data_ptr++;
        if (face_edit->face_type == FT_COLOURED_QUAD) {
            Inject_ApplyFaceEdit(face_edit, data_ptr, texture);
        }

        data_ptr += 5 * num_faces;
        num_faces = *data_ptr++;
        if (face_edit->face_type == FT_COLOURED_TRIANGLE) {
            Inject_ApplyFaceEdit(face_edit, data_ptr, texture);
        }
    }
}

static void Inject_ApplyFaceEdit(
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

static int16_t *Inject_GetMeshTexture(FACE_EDIT *face_edit)
{
    OBJECT_INFO object = g_Objects[face_edit->object_id];
    if (!object.loaded) {
        return NULL;
    }

    int16_t **mesh = &g_Meshes[object.mesh_index];
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

static void Inject_TextureOverwrites(
    INJECTION *injection, LEVEL_INFO *level_info, uint8_t *palette_map)
{
    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    for (int32_t i = 0; i < inj_info->texture_overwrite_count; i++) {
        const uint16_t target_page = File_ReadU16(fp);
        const uint8_t target_x = File_ReadU8(fp);
        const uint8_t target_y = File_ReadU8(fp);
        const uint16_t source_width = File_ReadU16(fp);
        const uint16_t source_height = File_ReadU16(fp);

        uint8_t *source_img = Memory_Alloc(source_width * source_height);
        File_ReadData(fp, source_img, source_width * source_height);

        // Copy the source image pixels directly into the target page.
        uint8_t *page = level_info->texture_page_ptrs + target_page * PAGE_SIZE;
        for (int32_t y = 0; y < source_height; y++) {
            for (int32_t x = 0; x < source_width; x++) {
                const int32_t pal_idx = source_img[y * source_width + x];
                const int32_t target_pixel =
                    (y + target_y) * PAGE_WIDTH + x + target_x;
                *(page + target_pixel) = palette_map[pal_idx];
            }
        }

        Memory_FreePointer(&source_img);
    }
}

static void Inject_FloorDataEdits(INJECTION *injection, LEVEL_INFO *level_info)
{
    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    for (int32_t i = 0; i < inj_info->floor_edit_count; i++) {
        const int16_t room = File_ReadS16(fp);
        const uint16_t x = File_ReadU16(fp);
        const uint16_t z = File_ReadU16(fp);
        const int32_t fd_edit_count = File_ReadS32(fp);

        // Verify that the given room and coordinates are accurate.
        // Individual FD functions must check that sector is actually set.
        ROOM_INFO *r = NULL;
        SECTOR_INFO *sector = NULL;
        if (room < 0 || room >= g_RoomCount) {
            LOG_WARNING("Room index %d is invalid", room);
        } else {
            r = &g_RoomInfo[room];
            if (x >= r->x_size || z >= r->z_size) {
                LOG_WARNING(
                    "Sector [%d,%d] is invalid for room %d", x, z, room);
            } else {
                sector = &r->sectors[r->z_size * x + z];
            }
        }

        for (int32_t j = 0; j < fd_edit_count; j++) {
            const FLOOR_EDIT_TYPE edit_type = File_ReadS32(fp);
            switch (edit_type) {
            case FET_TRIGGER_PARAM:
                Inject_TriggerParameterChange(injection, sector);
                break;
            case FET_MUSIC_ONESHOT:
                Inject_SetMusicOneShot(sector);
                break;
            case FET_FD_INSERT:
                Inject_InsertFloorData(injection, sector, level_info);
                break;
            case FET_ROOM_SHIFT:
                Inject_RoomShift(injection, room);
                break;
            case FET_TRIGGER_ITEM:
                Inject_TriggeredItem(injection, level_info);
                break;
            default:
                LOG_WARNING("Unknown floor data edit type: %d", edit_type);
                break;
            }
        }
    }
}

static void Inject_TriggerParameterChange(
    INJECTION *injection, SECTOR_INFO *sector)
{
    MYFILE *fp = injection->fp;

    const uint8_t cmd_type = File_ReadU8(fp);
    const int16_t old_param = File_ReadS16(fp);
    const int16_t new_param = File_ReadS16(fp);

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

static void Inject_SetMusicOneShot(SECTOR_INFO *sector)
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

static void Inject_InsertFloorData(
    INJECTION *injection, SECTOR_INFO *sector, LEVEL_INFO *level_info)
{
    MYFILE *fp = injection->fp;

    const int32_t data_length = File_ReadS32(fp);

    int16_t data[data_length];
    File_ReadItems(fp, data, sizeof(int16_t), data_length);

    if (sector == NULL) {
        return;
    }

    // This will reset all FD properties in the sector based on the raw data
    // imported. We pass a dummy null index to allow it to read from the
    // beginning of the array.
    Room_PopulateSectorData(sector, data, 0, NULL_FD_INDEX);
}

static void Inject_RoomShift(INJECTION *injection, int16_t room_num)
{
    MYFILE *fp = injection->fp;

    const uint32_t x_shift = ROUND_TO_SECTOR(File_ReadU32(fp));
    const uint32_t z_shift = ROUND_TO_SECTOR(File_ReadU32(fp));
    const int32_t y_shift = ROUND_TO_CLICK(File_ReadS32(fp));

    ROOM_INFO *room = &g_RoomInfo[room_num];
    room->x += x_shift;
    room->z += z_shift;
    room->min_floor += y_shift;
    room->max_ceiling += y_shift;

    // Move any items in the room to match.
    for (int32_t i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
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
    for (int32_t i = 0; i < room->z_size * room->x_size; i++) {
        SECTOR_INFO *const sector = &room->sectors[i];
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

static void Inject_TriggeredItem(INJECTION *injection, LEVEL_INFO *level_info)
{
    MYFILE *fp = injection->fp;

    if (g_LevelItemCount == MAX_ITEMS) {
        File_Skip(
            fp, sizeof(int16_t) * 4 + sizeof(int32_t) * 3 + sizeof(uint16_t));
        LOG_WARNING("Cannot add more than %d items", MAX_ITEMS);
        return;
    }

    int16_t item_num = Item_Create();
    ITEM_INFO *item = &g_Items[item_num];

    item->object_id = File_ReadS16(fp);
    item->room_num = File_ReadS16(fp);
    item->pos.x = File_ReadS32(fp);
    item->pos.y = File_ReadS32(fp);
    item->pos.z = File_ReadS32(fp);
    item->rot.y = File_ReadS16(fp);
    item->shade = File_ReadS16(fp);
    item->flags = File_ReadU16(fp);

    level_info->item_count++;
    g_LevelItemCount++;
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

static void Inject_RoomMeshEdits(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_2) {
        return;
    }

    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    ROOM_MESH_EDIT_TYPE edit_type;
    for (int32_t i = 0; i < inj_info->room_mesh_edit_count; i++) {
        edit_type = File_ReadS32(fp);

        switch (edit_type) {
        case RMET_TEXTURE_FACE:
            Inject_TextureRoomFace(injection);
            break;
        case RMET_MOVE_FACE:
            Inject_MoveRoomFace(injection);
            break;
        case RMET_ALTER_VERTEX:
            Inject_AlterRoomVertex(injection);
            break;
        case RMET_ROTATE_FACE:
            Inject_RotateRoomFace(injection);
            break;
        case RMET_ADD_FACE:
            Inject_AddRoomFace(injection);
            break;
        case RMET_ADD_VERTEX:
            Inject_AddRoomVertex(injection);
            break;
        default:
            LOG_WARNING("Unknown room mesh edit type: %d", edit_type);
            break;
        }
    }
}

static void Inject_TextureRoomFace(INJECTION *injection)
{
    MYFILE *fp = injection->fp;

    const int16_t target_room = File_ReadS16(fp);
    const FACE_TYPE target_face_type = File_ReadS32(fp);
    const int16_t target_face = File_ReadS16(fp);
    const int16_t source_room = File_ReadS16(fp);
    const FACE_TYPE source_face_type = File_ReadS32(fp);
    const int16_t source_face = File_ReadS16(fp);

    int16_t *source_texture =
        Inject_GetRoomTexture(source_room, source_face_type, source_face);
    int16_t *target_texture =
        Inject_GetRoomTexture(target_room, target_face_type, target_face);
    if (source_texture && target_texture) {
        *target_texture = *source_texture;
    }
}

static void Inject_MoveRoomFace(INJECTION *injection)
{
    MYFILE *fp = injection->fp;

    const int16_t target_room = File_ReadS16(fp);
    const FACE_TYPE face_type = File_ReadS32(fp);
    const int16_t target_face = File_ReadS16(fp);
    const int32_t vertex_count = File_ReadS32(fp);

    for (int32_t j = 0; j < vertex_count; j++) {
        const int16_t vertex_index = File_ReadS16(fp);
        const int16_t new_vertex = File_ReadS16(fp);

        int16_t *target =
            Inject_GetRoomFace(target_room, face_type, target_face);
        if (target) {
            target += vertex_index;
            *target = new_vertex;
        }
    }
}

static void Inject_AlterRoomVertex(INJECTION *injection)
{
    MYFILE *fp = injection->fp;

    const int16_t target_room = File_ReadS16(fp);
    File_Skip(fp, sizeof(int32_t));
    const int16_t target_vertex = File_ReadS16(fp);
    const int16_t x_change = File_ReadS16(fp);
    const int16_t y_change = File_ReadS16(fp);
    const int16_t z_change = File_ReadS16(fp);
    int16_t shade_change = 0;
    if (injection->version >= INJ_VERSION_7) {
        shade_change = File_ReadS16(fp);
    }

    if (target_room < 0 || target_room >= g_RoomCount) {
        LOG_WARNING("Room index %d is invalid", target_room);
        return;
    }

    const ROOM_INFO *const room = &g_RoomInfo[target_room];
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

static void Inject_RotateRoomFace(INJECTION *injection)
{
    MYFILE *fp = injection->fp;

    const int16_t target_room = File_ReadS16(fp);
    const FACE_TYPE face_type = File_ReadS32(fp);
    const int16_t target_face = File_ReadS16(fp);
    const uint8_t num_rotations = File_ReadU8(fp);

    int16_t *target = Inject_GetRoomFace(target_room, face_type, target_face);
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

static void Inject_AddRoomFace(INJECTION *injection)
{
    MYFILE *fp = injection->fp;

    const int16_t target_room = File_ReadS16(fp);
    const FACE_TYPE face_type = File_ReadS32(fp);
    const int16_t source_room = File_ReadS16(fp);
    const int16_t source_face = File_ReadS16(fp);

    int32_t num_vertices = face_type == FT_TEXTURED_QUAD ? 4 : 3;
    int16_t vertices[num_vertices];
    for (int32_t i = 0; i < num_vertices; i++) {
        vertices[i] = File_ReadS16(fp);
    }

    if (target_room < 0 || target_room >= g_RoomCount) {
        LOG_WARNING("Room index %d is invalid", target_room);
        return;
    }

    int16_t *source_texture =
        Inject_GetRoomTexture(source_room, face_type, source_face);
    if (!source_texture) {
        return;
    }

    ROOM_INFO *r = &g_RoomInfo[target_room];
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

static void Inject_AddRoomVertex(INJECTION *injection)
{
    MYFILE *fp = injection->fp;

    const int16_t target_room = File_ReadS16(fp);
    File_Skip(fp, sizeof(int32_t));
    const int16_t x = File_ReadS16(fp);
    const int16_t y = File_ReadS16(fp);
    const int16_t z = File_ReadS16(fp);
    const int16_t lighting = File_ReadS16(fp);

    ROOM_INFO *r = &g_RoomInfo[target_room];
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

static int16_t *Inject_GetRoomTexture(
    int16_t room, FACE_TYPE face_type, int16_t face_index)
{
    int16_t *face = Inject_GetRoomFace(room, face_type, face_index);
    if (face) {
        face += face_type == FT_TEXTURED_QUAD ? 4 : 3;
    }
    return face;
}

static int16_t *Inject_GetRoomFace(
    int16_t room, FACE_TYPE face_type, int16_t face_index)
{
    ROOM_INFO *r = NULL;
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

static void Inject_RoomDoorEdits(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_2) {
        return;
    }

    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    for (int32_t i = 0; i < inj_info->room_door_edit_count; i++) {
        const int16_t base_room = File_ReadS16(fp);
        const int16_t link_room = File_ReadS16(fp);
        int16_t door_index = -1;
        if (injection->version >= INJ_VERSION_4) {
            door_index = File_ReadS16(fp);
        }

        if (base_room < 0 || base_room >= g_RoomCount) {
            File_Skip(fp, sizeof(int16_t) * 12);
            LOG_WARNING("Room index %d is invalid", base_room);
            continue;
        }

        ROOM_INFO *r = &g_RoomInfo[base_room];
        DOOR_INFO *door = NULL;
        for (int32_t j = 0; j < r->doors->count; j++) {
            DOOR_INFO d = r->doors->door[j];
            if (d.room_num == link_room
                && (j == door_index || door_index == -1)) {
                door = &r->doors->door[j];
                break;
            }
        }

        if (!door) {
            File_Skip(fp, sizeof(int16_t) * 12);
            LOG_WARNING(
                "Room index %d has no matching door to %d", base_room,
                link_room);
            continue;
        }

        for (int32_t j = 0; j < 4; j++) {
            const int16_t x_change = File_ReadS16(fp);
            const int16_t y_change = File_ReadS16(fp);
            const int16_t z_change = File_ReadS16(fp);

            door->vertex[j].x += x_change;
            door->vertex[j].y += y_change;
            door->vertex[j].z += z_change;
        }
    }
}

static void Inject_ItemPositions(INJECTION *injection)
{
    if (injection->version < INJ_VERSION_4) {
        return;
    }

    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    for (int32_t i = 0; i < inj_info->item_position_count; i++) {
        const int16_t item_num = File_ReadS16(fp);
        const int16_t y_rot = File_ReadS16(fp);

        int32_t x;
        int32_t y;
        int32_t z;
        int16_t room_num;
        if (injection->version > INJ_VERSION_4) {
            x = File_ReadS32(fp);
            y = File_ReadS32(fp);
            z = File_ReadS32(fp);
            room_num = File_ReadS16(fp);
        }

        if (item_num < 0 || item_num >= g_LevelItemCount) {
            LOG_WARNING("Item number %d is out of level item range", item_num);
            continue;
        }

        ITEM_INFO *item = &g_Items[item_num];
        item->rot.y = y_rot;
        if (injection->version > INJ_VERSION_4) {
            item->pos.x = x;
            item->pos.y = y;
            item->pos.z = z;
            item->room_num = room_num;
        }
    }
}

void Inject_Cleanup(void)
{
    if (!m_NumInjections) {
        return;
    }

    for (int32_t i = 0; i < m_NumInjections; i++) {
        INJECTION *injection = &m_Injections[i];
        if (injection->fp) {
            File_Close(injection->fp);
        }
        if (injection->info) {
            if (injection->info->room_meshes) {
                Memory_FreePointer(&injection->info->room_meshes);
            }
            Memory_FreePointer(&injection->info);
        }
    }

    Memory_FreePointer(&m_Injections);
}
