#include "game/inject.h"

#include "config.h"
#include "filesystem.h"
#include "game/gamebuf.h"
#include "game/output.h"
#include "game/packer.h"
#include "global/const.h"
#include "global/vars.h"
#include "log.h"
#include "memory.h"
#include "util.h"

#include <stddef.h>

#define BIN_VERSION 1

typedef enum INJECTION_TYPE {
    INJ_GENERAL = 0,
    INJ_BRAID = 1,
} INJECTION_TYPE;

typedef struct INJECTION {
    MYFILE *fp;
    INJECTION_TYPE type;
    INJECTION_INFO info;
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
    int16_t mesh_index;
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

static int32_t m_NumInjections = 0;
static INJECTION *m_Injections = NULL;
static INJECTION_INFO *m_Aggregate = NULL;

static bool Inject_LoadFromFile(INJECTION *injection, const char *filename);
static void Inject_Cleanup(void);

static uint8_t Inject_RemapRGB(RGB888 rgb);
static void Inject_AlignTextureReferences(
    OBJECT_INFO *object, RGB888 *palette, int32_t page_base);

static void Inject_LoadTexturePages(
    INJECTION *injection, RGB888 *palette, uint8_t *page_ptr);
static void Inject_TextureData(
    INJECTION *injection, LEVEL_INFO *level_info, int32_t page_base);
static void Inject_MeshData(INJECTION *injection, LEVEL_INFO *level_info);
static void Inject_AnimData(INJECTION *injection, LEVEL_INFO *level_info);
static void Inject_ObjectData(
    INJECTION *injection, LEVEL_INFO *level_info, RGB888 *palette);
static void Inject_SFXData(INJECTION *injection, LEVEL_INFO *level_info);

static int16_t *Inject_GetMeshTexture(FACE_EDIT *face_edit);

static void Inject_ApplyFaceEdit(
    FACE_EDIT *face_edit, int16_t *data_ptr, int16_t texture);
static void Inject_ApplyMeshEdit(MESH_EDIT *mesh_edit);
static void Inject_MeshEdits(INJECTION *injection);

bool Inject_Init(
    int num_injections, char *filenames[], INJECTION_INFO *aggregate)
{
    m_NumInjections = num_injections;
    m_Injections = Memory_Alloc(sizeof(INJECTION) * num_injections);
    m_Aggregate = aggregate;

    for (int i = 0; i < num_injections; i++) {
        INJECTION *injection = &m_Injections[i];
        if (!Inject_LoadFromFile(injection, filenames[i])) {
            Inject_Cleanup();
            return false;
        }
    }

    return true;
}

static bool Inject_LoadFromFile(INJECTION *injection, const char *filename)
{
    MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
    if (!fp) {
        LOG_ERROR("Could not open %s", filename);
        return false;
    }

    int32_t header_size;
    File_Read(&header_size, sizeof(int32_t), 1, fp);

    int32_t version;
    File_Read(&version, sizeof(int32_t), 1, fp);
    if (version != BIN_VERSION) {
        LOG_ERROR(
            "%s is version %d; expected %d", filename, version, BIN_VERSION);
        return false;
    }

    injection->fp = fp;
    File_Read(&injection->type, sizeof(int32_t), 1, fp);

    switch (injection->type) {
    case INJ_GENERAL:
        injection->relevant = true;
        break;
    case INJ_BRAID:
        injection->relevant = g_Config.enable_braid;
        break;
    default:
        injection->relevant = false;
        LOG_WARNING("%s is of unknown type %d", filename, injection->type);
        break;
    }

    if (injection->relevant) {
        File_Read(&injection->info, sizeof(INJECTION_INFO), 1, fp);
        INJECTION_INFO info = injection->info;

        m_Aggregate->texture_page_count += info.texture_page_count;
        m_Aggregate->texture_count += info.texture_count;
        m_Aggregate->sprite_info_count += info.sprite_info_count;
        m_Aggregate->sprite_count += info.sprite_count;
        m_Aggregate->mesh_count += info.mesh_count;
        m_Aggregate->mesh_ptr_count += info.mesh_ptr_count;
        m_Aggregate->anim_change_count += info.anim_change_count;
        m_Aggregate->anim_range_count += info.anim_range_count;
        m_Aggregate->anim_cmd_count += info.anim_cmd_count;
        m_Aggregate->anim_bone_count += info.anim_bone_count;
        m_Aggregate->anim_frame_count += info.anim_frame_count;
        m_Aggregate->anim_count += info.anim_count;
        m_Aggregate->object_count += info.object_count;
        m_Aggregate->sfx_count += info.sfx_count;
        m_Aggregate->sfx_data_size += info.sfx_data_size;
        m_Aggregate->sample_count += info.sample_count;

        LOG_INFO("%s queued for injection", filename);
    }

    return true;
}

bool Inject_AllInjections(LEVEL_INFO *level_info)
{
    if (!m_Injections) {
        return true;
    }

    RGB888 palette[256];
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
            injection, palette, source_pages + (source_page_count * PAGE_SIZE));

        Inject_TextureData(injection, level_info, tpage_base);
        Inject_MeshData(injection, level_info);
        Inject_AnimData(injection, level_info);
        Inject_ObjectData(injection, level_info, palette);
        Inject_SFXData(injection, level_info);

        Inject_MeshEdits(injection);

        // Realign base indices for the next injection.
        INJECTION_INFO inj_info = injection->info;
        level_info->anim_command_count += inj_info.anim_cmd_count;
        level_info->anim_bone_count += inj_info.anim_bone_count;
        level_info->anim_frame_count += inj_info.anim_frame_count;
        level_info->anim_count += inj_info.anim_count;
        level_info->mesh_ptr_count += inj_info.mesh_ptr_count;
        level_info->texture_count += inj_info.texture_count;
        source_page_count += inj_info.texture_page_count;
        tpage_base += inj_info.texture_page_count;
    }

    bool result = true;

    if (source_page_count) {
        PACKER_DATA *data = Memory_Alloc(sizeof(PACKER_DATA));
        data->level_page_count = level_info->texture_page_count;
        data->source_page_count = source_page_count;
        data->source_pages = source_pages;
        data->level_pages = level_info->texture_page_ptrs;
        data->object_count = level_info->texture_count;
        data->sprite_count = level_info->sprite_info_count;

        result = Packer_Pack(data);
        if (result) {
            level_info->texture_page_count += Packer_GetAddedPageCount();
            level_info->texture_page_ptrs = data->level_pages;
        }

        Memory_FreePointer(&source_pages);
        Memory_FreePointer(&data);
    }

    Inject_Cleanup();

    return result;
}

static void Inject_LoadTexturePages(
    INJECTION *injection, RGB888 *palette, uint8_t *page_ptr)
{
    INJECTION_INFO inj_info = injection->info;
    MYFILE *fp = injection->fp;

    File_Read(palette, sizeof(RGB888), 256, fp);
    for (int i = 1; i < 256; i++) {
        palette[i].r *= 4;
        palette[i].g *= 4;
        palette[i].b *= 4;
    }

    // Read in each page for this injection and realign the pixels
    // to the level's palette.
    File_Read(page_ptr, PAGE_SIZE, inj_info.texture_page_count, fp);
    int pixel_count = PAGE_SIZE * inj_info.texture_page_count;
    uint8_t pal_idx;
    for (int i = 0; i < pixel_count; i++, page_ptr++) {
        pal_idx = *page_ptr;
        if (pal_idx > 0) {
            *page_ptr = Inject_RemapRGB(palette[pal_idx]);
        }
    }
}

static void Inject_TextureData(
    INJECTION *injection, LEVEL_INFO *level_info, int32_t page_base)
{
    INJECTION_INFO inj_info = injection->info;
    MYFILE *fp = injection->fp;

    // Read the tex_infos and align them to the end of the page list.
    File_Read(
        g_PhdTextureInfo + level_info->texture_count, sizeof(PHD_TEXTURE),
        inj_info.texture_count, fp);
    for (int i = 0; i < inj_info.texture_count; i++) {
        g_PhdTextureInfo[level_info->texture_count + i].tpage += page_base;
    }

    File_Read(
        g_PhdSpriteInfo + level_info->sprite_info_count, sizeof(PHD_SPRITE),
        inj_info.sprite_info_count, fp);
    for (int i = 0; i < inj_info.sprite_info_count; i++) {
        g_PhdSpriteInfo[level_info->sprite_info_count + i].tpage += page_base;
    }

    for (int i = 0; i < inj_info.sprite_count; i++) {
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
    INJECTION_INFO inj_info = injection->info;
    MYFILE *fp = injection->fp;

    int32_t mesh_base = level_info->mesh_count;
    File_Read(g_MeshBase + mesh_base, sizeof(int16_t), inj_info.mesh_count, fp);
    level_info->mesh_count += inj_info.mesh_count;

    uint32_t *mesh_indices = GameBuf_Alloc(
        sizeof(uint32_t) * inj_info.mesh_ptr_count, GBUF_MESH_POINTERS);
    File_Read(mesh_indices, sizeof(uint32_t), inj_info.mesh_ptr_count, fp);

    for (int i = 0; i < inj_info.mesh_ptr_count; i++) {
        g_Meshes[level_info->mesh_ptr_count + i] =
            &g_MeshBase[mesh_base + mesh_indices[i] / 2];
    }
}

static void Inject_AnimData(INJECTION *injection, LEVEL_INFO *level_info)
{
    INJECTION_INFO inj_info = injection->info;
    MYFILE *fp = injection->fp;

    File_Read(
        g_AnimChanges + level_info->anim_change_count,
        sizeof(ANIM_CHANGE_STRUCT), inj_info.anim_change_count, fp);
    File_Read(
        g_AnimRanges + level_info->anim_range_count, sizeof(ANIM_RANGE_STRUCT),
        inj_info.anim_range_count, fp);
    File_Read(
        g_AnimCommands + level_info->anim_command_count, sizeof(int16_t),
        inj_info.anim_cmd_count, fp);
    File_Read(
        g_AnimBones + level_info->anim_bone_count, sizeof(int32_t),
        inj_info.anim_bone_count, fp);
    File_Read(
        g_AnimFrames + level_info->anim_frame_count, sizeof(int16_t),
        inj_info.anim_frame_count, fp);

    for (int i = 0; i < inj_info.anim_count; i++) {
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
        anim->frame_ofs += level_info->anim_frame_count * 2;
        anim->frame_ptr = &g_AnimFrames[anim->frame_ofs / 2];
        if (anim->number_changes) {
            anim->change_index += level_info->anim_change_count;
        }
        if (anim->number_commands) {
            anim->command_index += level_info->anim_command_count;
        }
    }

    // Re-align to the level.
    for (int i = 0; i < inj_info.anim_change_count; i++) {
        g_AnimChanges[level_info->anim_change_count++].range_index +=
            level_info->anim_range_count;
    }

    for (int i = 0; i < inj_info.anim_range_count; i++) {
        g_AnimRanges[level_info->anim_range_count++].link_anim_num +=
            level_info->anim_count;
    }
}

static void Inject_ObjectData(
    INJECTION *injection, LEVEL_INFO *level_info, RGB888 *palette)
{
    INJECTION_INFO inj_info = injection->info;
    MYFILE *fp = injection->fp;

    for (int i = 0; i < inj_info.object_count; i++) {
        int32_t tmp;
        File_Read(&tmp, sizeof(int32_t), 1, fp);
        OBJECT_INFO *object = &g_Objects[tmp];

        File_Read(&object->nmeshes, sizeof(int16_t), 1, fp);
        File_Read(&object->mesh_index, sizeof(int16_t), 1, fp);
        object->mesh_index += level_info->mesh_ptr_count;
        File_Read(&object->bone_index, sizeof(int32_t), 1, fp);
        object->bone_index += level_info->anim_bone_count;

        File_Read(&tmp, sizeof(int32_t), 1, fp);
        object->frame_base =
            &g_AnimFrames[(tmp + level_info->anim_frame_count * 2) / 2];
        File_Read(&object->anim_index, sizeof(int16_t), 1, fp);
        object->anim_index += level_info->anim_count;
        object->loaded = 1;

        Inject_AlignTextureReferences(
            object, palette, level_info->texture_count);
    }
}

static void Inject_SFXData(INJECTION *injection, LEVEL_INFO *level_info)
{
    INJECTION_INFO inj_info = injection->info;
    MYFILE *fp = injection->fp;

    for (int i = 0; i < inj_info.sfx_count; i++) {
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
    OBJECT_INFO *object, RGB888 *palette, int32_t page_base)
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
            *data_ptr = Inject_RemapRGB(palette[*data_ptr]);
            data_ptr++;
        }

        num_faces = *data_ptr++;
        for (int j = 0; j < num_faces; j++) {
            data_ptr += 3;
            *data_ptr = Inject_RemapRGB(palette[*data_ptr]);
            data_ptr++;
        }
    }
}

static uint8_t Inject_RemapRGB(RGB888 rgb)
{
    // Find the index of the nearest match to the given RGB
    int best_match = 0x7fffffff, test_match;
    int r, g, b, best_index;
    RGB888 test_rgb;
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

static void Inject_MeshEdits(INJECTION *injection)
{
    INJECTION_INFO inj_info = injection->info;
    MYFILE *fp = injection->fp;

    if (!inj_info.mesh_edit_count) {
        return;
    }

    MESH_EDIT *mesh_edits =
        Memory_Alloc(sizeof(MESH_EDIT) * inj_info.mesh_edit_count);

    for (int i = 0; i < inj_info.mesh_edit_count; i++) {
        MESH_EDIT *mesh_edit = &mesh_edits[i];
        File_Read(&mesh_edit->object_id, sizeof(int32_t), 1, fp);
        File_Read(&mesh_edit->mesh_index, sizeof(int16_t), 1, fp);

        File_Read(&mesh_edit->face_edit_count, sizeof(int32_t), 1, fp);
        mesh_edit->face_edits =
            Memory_Alloc(sizeof(FACE_EDIT) * mesh_edit->face_edit_count);
        for (int j = 0; j < mesh_edit->face_edit_count; j++) {
            FACE_EDIT *face_edit = &mesh_edit->face_edits[j];
            File_Read(&face_edit->object_id, sizeof(int32_t), 1, fp);
            File_Read(&face_edit->mesh_index, sizeof(int16_t), 1, fp);
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
        File_Read(
            mesh_edit->vertex_edits, sizeof(VERTEX_EDIT),
            mesh_edit->vertex_edit_count, fp);

        Inject_ApplyMeshEdit(mesh_edit);

        for (int j = 0; j < mesh_edit->face_edit_count; j++) {
            FACE_EDIT *face_edit = &mesh_edit->face_edits[j];
            Memory_FreePointer(&face_edit->targets);
        }

        Memory_FreePointer(&mesh_edit->face_edits);
        Memory_FreePointer(&mesh_edit->vertex_edits);
    }

    Memory_FreePointer(&mesh_edits);
}

static void Inject_ApplyMeshEdit(MESH_EDIT *mesh_edit)
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
    // with the one selected from each edit's instructions.
    int16_t *data_start = data_ptr;
    for (int i = 0; i < mesh_edit->face_edit_count; i++) {
        FACE_EDIT *face_edit = &mesh_edit->face_edits[i];
        int16_t *texture = Inject_GetMeshTexture(face_edit);
        if (!texture) {
            continue;
        }

        data_ptr = data_start;

        int num_faces = *data_ptr++;
        if (face_edit->face_type == FT_TEXTURED_QUAD) {
            Inject_ApplyFaceEdit(face_edit, data_ptr, *texture);
        }

        data_ptr += 5 * num_faces;
        num_faces = *data_ptr++;
        if (face_edit->face_type == FT_TEXTURED_TRIANGLE) {
            Inject_ApplyFaceEdit(face_edit, data_ptr, *texture);
        }

        data_ptr += 4 * num_faces;
        num_faces = *data_ptr++;
        if (face_edit->face_type == FT_COLOURED_QUAD) {
            Inject_ApplyFaceEdit(face_edit, data_ptr, *texture);
        }

        data_ptr += 5 * num_faces;
        num_faces = *data_ptr++;
        if (face_edit->face_type == FT_COLOURED_TRIANGLE) {
            Inject_ApplyFaceEdit(face_edit, data_ptr, *texture);
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
    int16_t *data_ptr = *(mesh + face_edit->mesh_index);
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

static void Inject_Cleanup(void)
{
    for (int i = 0; i < m_NumInjections; i++) {
        INJECTION *injection = &m_Injections[i];
        if (injection->fp) {
            File_Close(injection->fp);
        }
    }

    Memory_FreePointer(&m_Injections);
}
