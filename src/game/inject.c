#include "game/inject.h"

#include "config.h"
#include "filesystem.h"
#include "game/gamebuf.h"
#include "game/output.h"
#include "game/packer.h"
#include "global/const.h"
#include "global/vars.h"
#include "items.h"
#include "log.h"
#include "memory.h"
#include "util.h"

#include <stdbool.h>
#include <stddef.h>

#define INJECTION_MAGIC MKTAG('T', '1', 'M', 'J')
#define INJECTION_CURRENT_VERSION 5

typedef enum INJECTION_VERSION {
    INJ_VERSION_1 = 1,
    INJ_VERSION_2 = 2,
    INJ_VERSION_3 = 3,
    INJ_VERSION_4 = 4,
    INJ_VERSION_5 = 5,
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
    RMET_MOVE_VERTEX = 2,
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
    INJECTION *injection, FLOOR_INFO *floor);
static void Inject_SetMusicOneShot(FLOOR_INFO *floor);
static void Inject_InsertFloorData(
    INJECTION *injection, FLOOR_INFO *floor, LEVEL_INFO *level_info);
static void Inject_RoomShift(INJECTION *injection, int16_t room_num);
static void Inject_TriggeredItem(INJECTION *injection, LEVEL_INFO *level_info);

static void Inject_RoomMeshEdits(INJECTION *injection);
static void Inject_TextureRoomFace(INJECTION *injection);
static void Inject_MoveRoomFace(INJECTION *injection);
static void Inject_MoveRoomVertex(INJECTION *injection);
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
    int num_injections, char *filenames[], INJECTION_INFO *aggregate)
{
    m_NumInjections = num_injections;
    if (!num_injections) {
        return;
    }

    m_Injections = Memory_Alloc(sizeof(INJECTION) * num_injections);
    m_Aggregate = aggregate;

    for (int i = 0; i < num_injections; i++) {
        Inject_LoadFromFile(&m_Injections[i], filenames[i]);
    }
}

static void Inject_LoadFromFile(INJECTION *injection, const char *filename)
{
    injection->relevant = false;
    injection->info = NULL;
    return;

    MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
    injection->fp = fp;
    if (!fp) {
        LOG_WARNING("Could not open %s", filename);
        return;
    }

    uint32_t magic;
    File_Read(&magic, sizeof(uint32_t), 1, fp);
    if (magic != INJECTION_MAGIC) {
        LOG_WARNING("Invalid injection magic in %s", filename);
        return;
    }

    File_Read(&injection->version, sizeof(int32_t), 1, fp);
    if (injection->version < INJ_VERSION_1
        || injection->version > INJECTION_CURRENT_VERSION) {
        LOG_WARNING(
            "%s uses unsupported version %d", filename, injection->version);
        return;
    }

    File_Read(&injection->type, sizeof(int32_t), 1, fp);

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
    default:
        LOG_WARNING("%s is of unknown type %d", filename, injection->type);
        break;
    }

    if (!injection->relevant) {
        return;
    }

    injection->info = Memory_Alloc(sizeof(INJECTION_INFO));
    INJECTION_INFO *info = injection->info;

    File_Read(&info->texture_page_count, sizeof(int32_t), 1, fp);
    File_Read(&info->texture_count, sizeof(int32_t), 1, fp);
    File_Read(&info->sprite_info_count, sizeof(int32_t), 1, fp);
    File_Read(&info->sprite_count, sizeof(int32_t), 1, fp);
    File_Read(&info->mesh_count, sizeof(int32_t), 1, fp);
    File_Read(&info->mesh_ptr_count, sizeof(int32_t), 1, fp);
    File_Read(&info->anim_change_count, sizeof(int32_t), 1, fp);
    File_Read(&info->anim_range_count, sizeof(int32_t), 1, fp);
    File_Read(&info->anim_cmd_count, sizeof(int32_t), 1, fp);
    File_Read(&info->anim_bone_count, sizeof(int32_t), 1, fp);
    File_Read(&info->anim_frame_data_count, sizeof(int32_t), 1, fp);
    File_Read(&info->anim_count, sizeof(int32_t), 1, fp);
    File_Read(&info->object_count, sizeof(int32_t), 1, fp);
    File_Read(&info->sfx_count, sizeof(int32_t), 1, fp);
    File_Read(&info->sfx_data_size, sizeof(int32_t), 1, fp);
    File_Read(&info->sample_count, sizeof(int32_t), 1, fp);
    File_Read(&info->mesh_edit_count, sizeof(int32_t), 1, fp);
    File_Read(&info->texture_overwrite_count, sizeof(int32_t), 1, fp);
    File_Read(&info->floor_edit_count, sizeof(int32_t), 1, fp);
    File_Read(&info->floor_data_size, sizeof(int32_t), 1, fp);

    if (injection->version > INJ_VERSION_1) {
        // room_mesh_count is a summary of the change in mesh size,
        // while room_mesh_edit_count indicates how many edits to
        // read and interpret (not all edits incur a size change).
        File_Read(&info->room_mesh_count, sizeof(uint32_t), 1, fp);
        info->room_meshes =
            Memory_Alloc(sizeof(INJECTION_ROOM_MESH) * info->room_mesh_count);
        for (int i = 0; i < info->room_mesh_count; i++) {
            INJECTION_ROOM_MESH *mesh = &info->room_meshes[i];
            File_Read(&mesh->room_index, sizeof(int16_t), 1, fp);
            File_Read(&mesh->extra_size, sizeof(uint32_t), 1, fp);
        }

        File_Read(&info->room_mesh_edit_count, sizeof(uint32_t), 1, fp);
        File_Read(&info->room_door_edit_count, sizeof(uint32_t), 1, fp);
    } else {
        info->room_meshes = NULL;
    }

    if (injection->version > INJ_VERSION_2) {
        File_Read(&info->anim_range_edit_count, sizeof(int32_t), 1, fp);
    } else {
        info->anim_range_edit_count = 0;
    }

    if (injection->version > INJ_VERSION_3) {
        File_Read(&info->item_position_count, sizeof(int32_t), 1, fp);
    } else {
        info->item_position_count = 0;
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
    m_Aggregate->anim_count += info->anim_count;
    m_Aggregate->object_count += info->object_count;
    m_Aggregate->sfx_count += info->sfx_count;
    m_Aggregate->sfx_data_size += info->sfx_data_size;
    m_Aggregate->sample_count += info->sample_count;
    m_Aggregate->floor_data_size += info->floor_data_size;

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
    int source_page_count = 0;
    int32_t tpage_base = level_info->texture_page_count;

    for (int i = 0; i < m_NumInjections; i++) {
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
        File_Read(&source_palette[i].r, sizeof(uint8_t), 1, fp);
        File_Read(&source_palette[i].g, sizeof(uint8_t), 1, fp);
        File_Read(&source_palette[i].b, sizeof(uint8_t), 1, fp);
    }
    for (int i = 1; i < 256; i++) {
        source_palette[i].r *= 4;
        source_palette[i].g *= 4;
        source_palette[i].b *= 4;

        palette_map[i] = Inject_RemapRGB(source_palette[i]);
    }

    // Read in each page for this injection and realign the pixels
    // to the level's palette.
    File_Read(page_ptr, PAGE_SIZE, inj_info->texture_page_count, fp);
    int pixel_count = PAGE_SIZE * inj_info->texture_page_count;
    for (int i = 0; i < pixel_count; i++, page_ptr++) {
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
        File_Read(&texture->drawtype, sizeof(uint16_t), 1, fp);
        File_Read(&texture->tpage, sizeof(uint16_t), 1, fp);
        for (int32_t j = 0; j < 4; j++) {
            File_Read(&texture->uv[j].u, sizeof(uint16_t), 1, fp);
            File_Read(&texture->uv[j].v, sizeof(uint16_t), 1, fp);
        }
        g_PhdTextureInfo[level_info->texture_count + i].tpage += page_base;
    }

    for (int32_t i = 0; i < inj_info->sprite_info_count; i++) {
        PHD_SPRITE *sprite =
            &g_PhdSpriteInfo[level_info->sprite_info_count + i];
        File_Read(&sprite->tpage, sizeof(uint16_t), 1, fp);
        File_Read(&sprite->offset, sizeof(uint16_t), 1, fp);
        File_Read(&sprite->width, sizeof(uint16_t), 1, fp);
        File_Read(&sprite->height, sizeof(uint16_t), 1, fp);
        File_Read(&sprite->x1, sizeof(int16_t), 1, fp);
        File_Read(&sprite->y1, sizeof(int16_t), 1, fp);
        File_Read(&sprite->x2, sizeof(int16_t), 1, fp);
        File_Read(&sprite->y2, sizeof(int16_t), 1, fp);
        g_PhdSpriteInfo[level_info->sprite_info_count + i].tpage += page_base;
    }

    for (int i = 0; i < inj_info->sprite_count; i++) {
        GAME_OBJECT_ID object_num;
        File_Read(&object_num, sizeof(int32_t), 1, fp);
        if (object_num < O_NUMBER_OF) {
            File_Read(&g_Objects[object_num], sizeof(int16_t), 1, fp);
            File_Read(
                &g_Objects[object_num].mesh_index, sizeof(int16_t), 1, fp);
            g_Objects[object_num].mesh_index += level_info->sprite_info_count;
            level_info->sprite_info_count -= g_Objects[object_num].nmeshes;
            g_Objects[object_num].loaded = 1;
        } else {
            int32_t static_num = object_num - O_NUMBER_OF;
            File_Skip(fp, 2);
            File_Read(
                &g_StaticObjects[static_num].mesh_number, sizeof(int16_t), 1,
                fp);
            g_StaticObjects[static_num].mesh_number +=
                level_info->sprite_info_count;
            level_info->sprite_info_count++;
        }
        level_info->sprite_count++;
    }
}

static void Inject_MeshData(INJECTION *injection, LEVEL_INFO *level_info)
{
    INJECTION_INFO *inj_info = injection->info;
    MYFILE *fp = injection->fp;

    int32_t mesh_base = level_info->mesh_count;
    File_Read(
        g_MeshBase + mesh_base, sizeof(int16_t), inj_info->mesh_count, fp);
    level_info->mesh_count += inj_info->mesh_count;

    uint32_t *mesh_indices = GameBuf_Alloc(
        sizeof(uint32_t) * inj_info->mesh_ptr_count, GBUF_MESH_POINTERS);
    File_Read(mesh_indices, sizeof(uint32_t), inj_info->mesh_ptr_count, fp);

    for (int i = 0; i < inj_info->mesh_ptr_count; i++) {
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
        File_Read(&anim_change->goal_anim_state, sizeof(int16_t), 1, fp);
        File_Read(&anim_change->number_ranges, sizeof(int16_t), 1, fp);
        File_Read(&anim_change->range_index, sizeof(int16_t), 1, fp);
    }
    for (int32_t i = 0; i < inj_info->anim_range_count; i++) {
        ANIM_RANGE_STRUCT *anim_range =
            &g_AnimRanges[level_info->anim_range_count + i];
        File_Read(&anim_range->start_frame, sizeof(int16_t), 1, fp);
        File_Read(&anim_range->end_frame, sizeof(int16_t), 1, fp);
        File_Read(&anim_range->link_anim_num, sizeof(int16_t), 1, fp);
        File_Read(&anim_range->link_frame_num, sizeof(int16_t), 1, fp);
    }
    File_Read(
        g_AnimCommands + level_info->anim_command_count, sizeof(int16_t),
        inj_info->anim_cmd_count, fp);
    File_Read(
        g_AnimBones + level_info->anim_bone_count, sizeof(int32_t),
        inj_info->anim_bone_count, fp);
    File_Read(
        g_AnimFrames + level_info->anim_frame_data_count, sizeof(int16_t),
        inj_info->anim_frame_data_count, fp);

    for (int i = 0; i < inj_info->anim_count; i++) {
        ANIM_STRUCT *anim = &g_Anims[level_info->anim_count + i];

        File_Read(&anim->frame_ofs, sizeof(uint32_t), 1, fp);
        File_Read(&anim->interpolation, sizeof(int16_t), 1, fp);
        File_Read(&anim->current_anim_state, sizeof(int16_t), 1, fp);
        File_Read(&anim->velocity, sizeof(int32_t), 1, fp);
        File_Read(&anim->acceleration, sizeof(int32_t), 1, fp);
        File_Read(&anim->frame_base, sizeof(int16_t), 1, fp);
        File_Read(&anim->frame_end, sizeof(int16_t), 1, fp);
        File_Read(&anim->jump_anim_num, sizeof(int16_t), 1, fp);
        File_Read(&anim->jump_frame_num, sizeof(int16_t), 1, fp);
        File_Read(&anim->number_changes, sizeof(int16_t), 1, fp);
        File_Read(&anim->change_index, sizeof(int16_t), 1, fp);
        File_Read(&anim->number_commands, sizeof(int16_t), 1, fp);
        File_Read(&anim->command_index, sizeof(int16_t), 1, fp);

        // Re-align to the level.
        anim->jump_anim_num += level_info->anim_count;
        anim->frame_ofs += level_info->anim_frame_data_count * 2;
        anim->frame_ptr = &g_AnimFrames[anim->frame_ofs / 2];
        if (anim->number_changes) {
            anim->change_index += level_info->anim_change_count;
        }
        if (anim->number_commands) {
            anim->command_index += level_info->anim_command_count;
        }
    }

    // Re-align to the level.
    for (int i = 0; i < inj_info->anim_change_count; i++) {
        g_AnimChanges[level_info->anim_change_count++].range_index +=
            level_info->anim_range_count;
    }

    for (int i = 0; i < inj_info->anim_range_count; i++) {
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

    GAME_OBJECT_ID object_id;
    int32_t edit_count;
    int16_t anim_index;
    int16_t change_index;
    int16_t range_index;
    int16_t low_frame;
    int16_t high_frame;

    for (int i = 0; i < inj_info->anim_range_edit_count; i++) {
        File_Read(&object_id, sizeof(int32_t), 1, fp);
        File_Read(&anim_index, sizeof(int16_t), 1, fp);
        File_Read(&edit_count, sizeof(int32_t), 1, fp);

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
        for (int j = 0; j < edit_count; j++) {
            File_Read(&change_index, sizeof(int16_t), 1, fp);
            File_Read(&range_index, sizeof(int16_t), 1, fp);
            File_Read(&low_frame, sizeof(int16_t), 1, fp);
            File_Read(&high_frame, sizeof(int16_t), 1, fp);

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
    for (int i = 0; i < inj_info->object_count; i++) {
        int32_t tmp;
        File_Read(&tmp, sizeof(int32_t), 1, fp);
        OBJECT_INFO *object = &g_Objects[tmp];

        int16_t num_meshes;
        int16_t mesh_index;
        int32_t bone_index;
        File_Read(&num_meshes, sizeof(int16_t), 1, fp);
        File_Read(&mesh_index, sizeof(int16_t), 1, fp);
        File_Read(&bone_index, sizeof(int32_t), 1, fp);

        // When mesh data has been omitted from the injection, this indicates
        // that we wish to retain what's already defined so to avoid duplicate
        // packing.
        if (!object->loaded || num_meshes) {
            object->nmeshes = num_meshes;
            object->mesh_index = mesh_index + level_info->mesh_ptr_count;
            object->bone_index = bone_index + level_info->anim_bone_count;
        }

        File_Read(&tmp, sizeof(int32_t), 1, fp);
        object->frame_base =
            &g_AnimFrames[(tmp + level_info->anim_frame_data_count * 2) / 2];
        File_Read(&object->anim_index, sizeof(int16_t), 1, fp);
        object->anim_index += level_info->anim_count;
        object->loaded = 1;

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

    for (int i = 0; i < inj_info->sfx_count; i++) {
        int16_t sfx_id;
        File_Read(&sfx_id, sizeof(int16_t), 1, fp);
        g_SampleLUT[sfx_id] = level_info->sample_info_count;

        SAMPLE_INFO *sample_info =
            &g_SampleInfos[level_info->sample_info_count];
        File_Read(&sample_info->volume, sizeof(int16_t), 1, fp);
        File_Read(&sample_info->randomness, sizeof(int16_t), 1, fp);
        File_Read(&sample_info->flags, sizeof(int16_t), 1, fp);
        sample_info->number = level_info->sample_count;

        int16_t num_samples = (sample_info->flags >> 2) & 15;
        for (int j = 0; j < num_samples; j++) {
            int32_t sample_length;
            File_Read(&sample_length, sizeof(int32_t), 1, fp);
            File_Read(
                level_info->sample_data + level_info->sample_data_size,
                sizeof(char), sample_length, fp);

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
    for (int i = 0; i < object->nmeshes; i++) {
        int16_t *data_ptr = *mesh++;
        data_ptr += 5; // Skip centre and collision radius
        int vertex_count = *data_ptr++;
        data_ptr += 3 * vertex_count; // Skip vertex info

        // Skip normals or lights
        int normal_count = *data_ptr++;
        data_ptr += normal_count > 0 ? 3 * normal_count : -normal_count;

        // Align the tex_info references on the textured quads and triangles.
        int num_faces = *data_ptr++;
        for (int j = 0; j < num_faces; j++) {
            data_ptr += 4; // Skip vertices
            *data_ptr++ += page_base;
        }

        num_faces = *data_ptr++;
        for (int j = 0; j < num_faces; j++) {
            data_ptr += 3;
            *data_ptr++ += page_base;
        }

        // Align coloured quads and triangles to the level palette.
        num_faces = *data_ptr++;
        for (int j = 0; j < num_faces; j++) {
            data_ptr += 4;
            *data_ptr = palette_map[*data_ptr];
            data_ptr++;
        }

        num_faces = *data_ptr++;
        for (int j = 0; j < num_faces; j++) {
            data_ptr += 3;
            *data_ptr = palette_map[*data_ptr];
            data_ptr++;
        }
    }
}

static uint8_t Inject_RemapRGB(RGB_888 rgb)
{
    // Find the index of the nearest match to the given RGB
    int best_match = 0x7fffffff, test_match;
    int r, g, b;
    int best_index = 0;
    RGB_888 test_rgb;
    for (int i = 1; i < 256; i++) {
        test_rgb = Output_GetPaletteColor(i);
        r = rgb.r - test_rgb.r;
        g = rgb.g - test_rgb.g;
        b = rgb.b - test_rgb.b;
        test_match = SQUARE(r) + SQUARE(g) + SQUARE(b);

        if (test_match < best_match) {
            best_match = test_match;
            best_index = i;
        }
    }
    return (uint8_t)best_index;
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

    for (int i = 0; i < inj_info->mesh_edit_count; i++) {
        MESH_EDIT *mesh_edit = &mesh_edits[i];
        File_Read(&mesh_edit->object_id, sizeof(int32_t), 1, fp);
        File_Read(&mesh_edit->mesh_index, sizeof(int16_t), 1, fp);

        File_Read(&mesh_edit->face_edit_count, sizeof(int32_t), 1, fp);
        mesh_edit->face_edits =
            Memory_Alloc(sizeof(FACE_EDIT) * mesh_edit->face_edit_count);
        for (int j = 0; j < mesh_edit->face_edit_count; j++) {
            FACE_EDIT *face_edit = &mesh_edit->face_edits[j];
            File_Read(&face_edit->object_id, sizeof(int32_t), 1, fp);
            File_Read(&face_edit->source_identifier, sizeof(int16_t), 1, fp);
            File_Read(&face_edit->face_type, sizeof(int32_t), 1, fp);
            File_Read(&face_edit->face_index, sizeof(int16_t), 1, fp);

            File_Read(&face_edit->target_count, sizeof(int32_t), 1, fp);
            face_edit->targets =
                Memory_Alloc(sizeof(int16_t) * face_edit->target_count);
            File_Read(
                face_edit->targets, sizeof(int16_t), face_edit->target_count,
                fp);
        }

        File_Read(&mesh_edit->vertex_edit_count, sizeof(int32_t), 1, fp);
        mesh_edit->vertex_edits =
            Memory_Alloc(sizeof(VERTEX_EDIT) * mesh_edit->vertex_edit_count);
        for (int32_t i = 0; i < mesh_edit->vertex_edit_count; i++) {
            VERTEX_EDIT *vertex_edit = &mesh_edit->vertex_edits[i];
            File_Read(&vertex_edit->vertex_index, sizeof(int16_t), 1, fp);
            File_Read(&vertex_edit->x_change, sizeof(int16_t), 1, fp);
            File_Read(&vertex_edit->y_change, sizeof(int16_t), 1, fp);
            File_Read(&vertex_edit->z_change, sizeof(int16_t), 1, fp);
        }

        Inject_ApplyMeshEdit(mesh_edit, palette_map);

        for (int j = 0; j < mesh_edit->face_edit_count; j++) {
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
    data_ptr += 5; // Skip centre and collision radius

    int vertex_count = *data_ptr++;
    for (int i = 0; i < mesh_edit->vertex_edit_count; i++) {
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
    int normal_count = *data_ptr++;
    data_ptr += normal_count > 0 ? 3 * normal_count : -normal_count;

    // Find each face we are interested in and replace its texture
    // or palette reference with the one selected from each edit's
    // instructions.
    int16_t *data_start = data_ptr;
    for (int i = 0; i < mesh_edit->face_edit_count; i++) {
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

        int num_faces = *data_ptr++;
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
    int vertex_count;
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
    for (int i = 0; i < face_edit->target_count; i++) {
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
    int vertex_count = *data_ptr++;
    data_ptr += 3 * vertex_count;
    int normal_count = *data_ptr++;
    data_ptr += normal_count > 0 ? 3 * normal_count : -normal_count;

    int num_faces = *data_ptr++;
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

    uint16_t target_page, source_width, source_height;
    uint8_t target_x, target_y;
    for (int i = 0; i < inj_info->texture_overwrite_count; i++) {
        File_Read(&target_page, sizeof(uint16_t), 1, fp);
        File_Read(&target_x, sizeof(uint8_t), 1, fp);
        File_Read(&target_y, sizeof(uint8_t), 1, fp);
        File_Read(&source_width, sizeof(uint16_t), 1, fp);
        File_Read(&source_height, sizeof(uint16_t), 1, fp);

        uint8_t *source_img = Memory_Alloc(source_width * source_height);
        File_Read(source_img, source_width * source_height, 1, fp);

        // Copy the source image pixels directly into the target page.
        uint8_t *page = level_info->texture_page_ptrs + target_page * PAGE_SIZE;
        int pal_idx, target_pixel;
        for (int y = 0; y < source_height; y++) {
            for (int x = 0; x < source_width; x++) {
                pal_idx = source_img[y * source_width + x];
                target_pixel = (y + target_y) * PAGE_WIDTH + x + target_x;
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

    int32_t fd_edit_count;
    FLOOR_EDIT_TYPE edit_type;
    int16_t room;
    uint16_t y, x;
    for (int i = 0; i < inj_info->floor_edit_count; i++) {
        File_Read(&room, sizeof(int16_t), 1, fp);
        File_Read(&y, sizeof(uint16_t), 1, fp);
        File_Read(&x, sizeof(uint16_t), 1, fp);
        File_Read(&fd_edit_count, sizeof(int32_t), 1, fp);

        // Verify that the given room and coordinates are accurate.
        // Individual FD functions must check that floor is actually set.
        ROOM_INFO *r = NULL;
        FLOOR_INFO *floor = NULL;
        if (room < 0 || room >= g_RoomCount) {
            LOG_WARNING("Room index %d is invalid", room);
        } else {
            r = &g_RoomInfo[room];
            if (y >= r->y_size || x >= r->x_size) {
                LOG_WARNING(
                    "Sector [%d,%d] is invalid for room %d", y, x, room);
            } else {
                floor = &r->floor[r->x_size * y + x];
            }
        }

        for (int j = 0; j < fd_edit_count; j++) {
            File_Read(&edit_type, sizeof(int32_t), 1, fp);
            switch (edit_type) {
            case FET_TRIGGER_PARAM:
                Inject_TriggerParameterChange(injection, floor);
                break;
            case FET_MUSIC_ONESHOT:
                Inject_SetMusicOneShot(floor);
                break;
            case FET_FD_INSERT:
                Inject_InsertFloorData(injection, floor, level_info);
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
    INJECTION *injection, FLOOR_INFO *floor)
{
    MYFILE *fp = injection->fp;

    uint8_t cmd_type;
    int16_t old_param, new_param;
    File_Read(&cmd_type, sizeof(uint8_t), 1, fp);
    File_Read(&old_param, sizeof(int16_t), 1, fp);
    File_Read(&new_param, sizeof(int16_t), 1, fp);

    if (!floor) {
        return;
    }

    // If we can find an action item for the given floor that matches
    // the command type and old (current) parameter, change it to the
    // new parameter.

    uint16_t fd_index = floor->index;
    if (!fd_index) {
        return;
    }

    while (1) {
        uint16_t data = g_FloorData[fd_index++];
        switch (data & DATA_TYPE) {
        case FT_DOOR:
        case FT_ROOF:
        case FT_TILT:
            fd_index++;
            break;

        case FT_LAVA:
            break;

        case FT_TRIGGER: {
            uint16_t trig_type = (data & 0x3F00) >> 8;
            fd_index++; // skip trigger setup

            if (trig_type == TT_SWITCH || trig_type == TT_KEY
                || trig_type == TT_PICKUP) {
                fd_index++; // skip entity reference
            }

            while (1) {
                int16_t *command = &g_FloorData[fd_index++];

                if (TRIG_BITS(*command) == cmd_type) {
                    int16_t param = *command & VALUE_BITS;
                    if (param == old_param) {
                        *command =
                            (*command & ~VALUE_BITS) | (new_param & VALUE_BITS);
                        return;
                    }
                }

                if (TRIG_BITS(*command) == TO_CAMERA) {
                    fd_index++; // skip camera setup
                }

                if (*command & END_BIT) {
                    break;
                }
            }
            break;
        }
        }

        if (data & END_BIT) {
            break;
        }
    }
}

static void Inject_SetMusicOneShot(FLOOR_INFO *floor)
{
    if (!floor) {
        return;
    }

    uint16_t fd_index = floor->index;
    if (!fd_index) {
        return;
    }

    while (1) {
        uint16_t data = g_FloorData[fd_index++];
        switch (data & DATA_TYPE) {
        case FT_DOOR:
        case FT_ROOF:
        case FT_TILT:
            fd_index++;
            break;

        case FT_LAVA:
            break;

        case FT_TRIGGER: {
            uint16_t trig_type = TRIG_TYPE(data);
            int16_t *flags = &g_FloorData[fd_index++];

            if (trig_type == TT_SWITCH || trig_type == TT_KEY
                || trig_type == TT_PICKUP) {
                fd_index++; // skip entity reference
            }

            while (1) {
                int16_t *command = &g_FloorData[fd_index++];
                if (TRIG_BITS(*command) == TO_CD) {
                    *flags |= IF_ONESHOT;
                    return;
                }

                if (TRIG_BITS(*command) == TO_CAMERA) {
                    fd_index++; // skip camera setup
                }

                if (*command & END_BIT) {
                    break;
                }
            }
            return;
        }
        }

        if (data & END_BIT) {
            break;
        }
    }
}

static void Inject_InsertFloorData(
    INJECTION *injection, FLOOR_INFO *floor, LEVEL_INFO *level_info)
{
    MYFILE *fp = injection->fp;

    int32_t data_length;
    File_Read(&data_length, sizeof(int32_t), 1, fp);

    int16_t data[data_length];
    File_Read(&data, sizeof(int16_t), data_length, fp);

    if (!floor) {
        return;
    }

    floor->index = level_info->floor_data_size;
    for (int i = 0; i < data_length; i++) {
        g_FloorData[level_info->floor_data_size + i] = data[i];
    }

    level_info->floor_data_size += data_length;
}

static void Inject_RoomShift(INJECTION *injection, int16_t room_num)
{
    MYFILE *fp = injection->fp;

    uint32_t x_shift;
    uint32_t z_shift;
    int32_t y_shift;

    File_Read(&x_shift, sizeof(uint32_t), 1, fp);
    File_Read(&z_shift, sizeof(uint32_t), 1, fp);
    File_Read(&y_shift, sizeof(int32_t), 1, fp);

    ROOM_INFO *room = &g_RoomInfo[room_num];
    room->x += x_shift;
    room->z += z_shift;
    room->min_floor += y_shift;
    room->max_ceiling += y_shift;

    // Move any items in the room to match.
    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        if (item->room_number != room_num) {
            continue;
        }

        item->pos.x += x_shift;
        item->pos.y += y_shift;
        item->pos.z += z_shift;
    }

    if (!y_shift) {
        return;
    }

    // Update the sector floor and ceiling clicks to match.
    const int8_t click_shift = y_shift / STEP_L;
    const int8_t wall_height = NO_HEIGHT / STEP_L;
    for (int i = 0; i < room->x_size * room->y_size; i++) {
        FLOOR_INFO *floor = &room->floor[i];
        if (floor->floor == wall_height || floor->ceiling == wall_height) {
            continue;
        }

        floor->floor += click_shift;
        floor->ceiling += click_shift;
    }

    // Update vertex Y values to match; x and z are room-relative.
    int16_t *data_ptr = room->data;
    int16_t vertex_count = *data_ptr++;
    for (int i = 0; i < vertex_count; i++) {
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

    int16_t item_number = Item_Create();
    ITEM_INFO *item = &g_Items[item_number];

    File_Read(&item->object_number, sizeof(int16_t), 1, fp);
    File_Read(&item->room_number, sizeof(int16_t), 1, fp);
    File_Read(&item->pos.x, sizeof(int32_t), 1, fp);
    File_Read(&item->pos.y, sizeof(int32_t), 1, fp);
    File_Read(&item->pos.z, sizeof(int32_t), 1, fp);
    File_Read(&item->rot.y, sizeof(int16_t), 1, fp);
    File_Read(&item->shade, sizeof(int16_t), 1, fp);
    File_Read(&item->flags, sizeof(uint16_t), 1, fp);

    level_info->item_count++;
    g_LevelItemCount++;
}

uint32_t Inject_GetExtraRoomMeshSize(int32_t room_index)
{
    uint32_t size = 0;
    if (!m_Injections) {
        return size;
    }

    for (int i = 0; i < m_NumInjections; i++) {
        INJECTION *injection = &m_Injections[i];
        if (!injection->relevant || injection->version < INJ_VERSION_2) {
            continue;
        }

        INJECTION_INFO *inj_info = injection->info;
        for (int j = 0; j < inj_info->room_mesh_count; j++) {
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
    for (int i = 0; i < inj_info->room_mesh_edit_count; i++) {
        File_Read(&edit_type, sizeof(int32_t), 1, fp);

        switch (edit_type) {
        case RMET_TEXTURE_FACE:
            Inject_TextureRoomFace(injection);
            break;
        case RMET_MOVE_FACE:
            Inject_MoveRoomFace(injection);
            break;
        case RMET_MOVE_VERTEX:
            Inject_MoveRoomVertex(injection);
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

    int16_t target_room;
    FACE_TYPE target_face_type;
    int16_t target_face;
    int16_t source_room;
    FACE_TYPE source_face_type;
    int16_t source_face;

    File_Read(&target_room, sizeof(int16_t), 1, fp);
    File_Read(&target_face_type, sizeof(int32_t), 1, fp);
    File_Read(&target_face, sizeof(int16_t), 1, fp);
    File_Read(&source_room, sizeof(int16_t), 1, fp);
    File_Read(&source_face_type, sizeof(int32_t), 1, fp);
    File_Read(&source_face, sizeof(int16_t), 1, fp);

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

    int16_t target_room;
    FACE_TYPE face_type;
    int16_t target_face;
    int32_t vertex_count;
    int16_t vertex_index;
    int16_t new_vertex;

    File_Read(&target_room, sizeof(int16_t), 1, fp);
    File_Read(&face_type, sizeof(int32_t), 1, fp);
    File_Read(&target_face, sizeof(int16_t), 1, fp);
    File_Read(&vertex_count, sizeof(int32_t), 1, fp);

    for (int j = 0; j < vertex_count; j++) {
        File_Read(&vertex_index, sizeof(int16_t), 1, fp);
        File_Read(&new_vertex, sizeof(int16_t), 1, fp);

        int16_t *target =
            Inject_GetRoomFace(target_room, face_type, target_face);
        if (target) {
            target += vertex_index;
            *target = new_vertex;
        }
    }
}

static void Inject_MoveRoomVertex(INJECTION *injection)
{
    MYFILE *fp = injection->fp;

    int16_t target_room;
    int16_t target_vertex;
    int16_t x_change;
    int16_t y_change;
    int16_t z_change;

    File_Read(&target_room, sizeof(int16_t), 1, fp);
    File_Skip(fp, sizeof(int32_t));
    File_Read(&target_vertex, sizeof(int16_t), 1, fp);
    File_Read(&x_change, sizeof(int16_t), 1, fp);
    File_Read(&y_change, sizeof(int16_t), 1, fp);
    File_Read(&z_change, sizeof(int16_t), 1, fp);

    if (target_room < 0 || target_room >= g_RoomCount) {
        LOG_WARNING("Room index %d is invalid", target_room);
        return;
    }

    ROOM_INFO *r = &g_RoomInfo[target_room];
    int16_t vertex_count = *r->data;
    if (target_vertex < 0 || target_vertex >= vertex_count) {
        LOG_WARNING(
            "Vertex index %d, room %d is invalid", target_vertex, target_room);
        return;
    }

    int16_t *data_ptr = r->data + target_vertex * 4;
    *(data_ptr + 1) += x_change;
    *(data_ptr + 2) += y_change;
    *(data_ptr + 3) += z_change;
}

static void Inject_RotateRoomFace(INJECTION *injection)
{
    MYFILE *fp = injection->fp;

    int16_t target_room;
    FACE_TYPE face_type;
    int16_t target_face;
    uint8_t num_rotations;

    File_Read(&target_room, sizeof(int16_t), 1, fp);
    File_Read(&face_type, sizeof(int32_t), 1, fp);
    File_Read(&target_face, sizeof(int16_t), 1, fp);
    File_Read(&num_rotations, sizeof(uint8_t), 1, fp);

    int16_t *target = Inject_GetRoomFace(target_room, face_type, target_face);
    if (!target) {
        return;
    }

    int num_vertices = face_type == FT_TEXTURED_QUAD ? 4 : 3;
    int16_t *vertices[num_vertices];
    for (int i = 0; i < num_vertices; i++) {
        vertices[i] = target + i;
    }

    for (int i = 0; i < num_rotations; i++) {
        int16_t first = *vertices[0];
        for (int j = 0; j < num_vertices - 1; j++) {
            *vertices[j] = *vertices[j + 1];
        }
        *vertices[num_vertices - 1] = first;
    }
}

static void Inject_AddRoomFace(INJECTION *injection)
{
    MYFILE *fp = injection->fp;

    int16_t target_room;
    FACE_TYPE face_type;
    int16_t source_room;
    int16_t source_face;

    File_Read(&target_room, sizeof(int16_t), 1, fp);
    File_Read(&face_type, sizeof(int32_t), 1, fp);
    File_Read(&source_room, sizeof(int16_t), 1, fp);
    File_Read(&source_face, sizeof(int16_t), 1, fp);

    int num_vertices = face_type == FT_TEXTURED_QUAD ? 4 : 3;
    int16_t vertices[num_vertices];
    for (int i = 0; i < num_vertices; i++) {
        File_Read(&vertices[i], sizeof(int16_t), 1, fp);
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
    int data_index = 0;

    int32_t vertex_count = r->data[data_index++];
    data_index += vertex_count * 4;

    // Increment the relevant number of faces and work out the
    // starting point in the mesh for the injection.
    int inject_pos = 0;
    int num_data = r->data[data_index]; // Quads
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
    int inject_length = num_vertices + 1;
    for (int i = data_index; i >= inject_pos; i--) {
        r->data[i + inject_length] = r->data[i];
    }

    // Inject the face data.
    for (int i = 0; i < num_vertices; i++) {
        r->data[inject_pos++] = vertices[i];
    }
    r->data[inject_pos] = *source_texture;
}

static void Inject_AddRoomVertex(INJECTION *injection)
{
    MYFILE *fp = injection->fp;

    int16_t target_room;
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t lighting;

    File_Read(&target_room, sizeof(int16_t), 1, fp);
    File_Skip(fp, sizeof(int32_t));
    File_Read(&x, sizeof(int16_t), 1, fp);
    File_Read(&y, sizeof(int16_t), 1, fp);
    File_Read(&z, sizeof(int16_t), 1, fp);
    File_Read(&lighting, sizeof(int16_t), 1, fp);

    ROOM_INFO *r = &g_RoomInfo[target_room];
    int data_index = 0;

    int32_t vertex_count = r->data[data_index];
    r->data[data_index++]++;
    data_index += vertex_count * 4;

    int inject_pos = data_index;
    int num_data = r->data[data_index]; // Quads
    data_index += 1 + num_data * 5;

    num_data = r->data[data_index]; // Triangles
    data_index += 1 + num_data * 4;

    num_data = r->data[data_index]; // Sprites
    data_index += num_data * 2;

    // Move everything at the end of the mesh forwards to make space
    // for the new vertex.
    for (int i = data_index; i >= inject_pos; i--) {
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

    int num_faces = *data_ptr++;
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

    int16_t base_room;
    int16_t link_room;
    int16_t door_index = -1;
    int16_t x_change;
    int16_t y_change;
    int16_t z_change;
    for (int i = 0; i < inj_info->room_door_edit_count; i++) {
        File_Read(&base_room, sizeof(int16_t), 1, fp);
        File_Read(&link_room, sizeof(int16_t), 1, fp);
        if (injection->version >= INJ_VERSION_4) {
            File_Read(&door_index, sizeof(int16_t), 1, fp);
        }

        if (base_room < 0 || base_room >= g_RoomCount) {
            File_Skip(fp, sizeof(int16_t) * 12);
            LOG_WARNING("Room index %d is invalid", base_room);
            continue;
        }

        ROOM_INFO *r = &g_RoomInfo[base_room];
        DOOR_INFO *door = NULL;
        for (int j = 0; j < r->doors->count; j++) {
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

        for (int j = 0; j < 4; j++) {
            File_Read(&x_change, sizeof(int16_t), 1, fp);
            File_Read(&y_change, sizeof(int16_t), 1, fp);
            File_Read(&z_change, sizeof(int16_t), 1, fp);

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

    int16_t item_num;
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t room_num;
    int16_t y_rot;
    for (int i = 0; i < inj_info->item_position_count; i++) {
        File_Read(&item_num, sizeof(int16_t), 1, fp);
        File_Read(&y_rot, sizeof(int16_t), 1, fp);
        if (injection->version > INJ_VERSION_4) {
            File_Read(&x, sizeof(int32_t), 1, fp);
            File_Read(&y, sizeof(int32_t), 1, fp);
            File_Read(&z, sizeof(int32_t), 1, fp);
            File_Read(&room_num, sizeof(int16_t), 1, fp);
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
            item->room_number = room_num;
        }
    }
}

void Inject_Cleanup(void)
{
    if (!m_NumInjections) {
        return;
    }

    for (int i = 0; i < m_NumInjections; i++) {
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
