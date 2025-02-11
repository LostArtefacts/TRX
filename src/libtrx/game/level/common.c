#include "game/level/common.h"

#include "benchmark.h"
#include "debug.h"
#include "game/anims.h"
#include "game/camera.h"
#include "game/const.h"
#include "game/demo.h"
#include "game/effects/const.h"
#include "game/game_buf.h"
#include "game/inject.h"
#include "game/objects/common.h"
#include "game/objects/setup.h"
#include "game/output.h"
#include "game/rooms.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "log.h"
#include "memory.h"
#include "utils.h"
#include "vector.h"

#include <string.h>

static int16_t *m_AnimCommands = nullptr;

static RGBA_8888 M_ARGB1555To8888(uint16_t argb1555);
static void M_ReadPosition(XYZ_32 *pos, VFILE *file);
static void M_ReadShade(SHADE *shade, VFILE *file);
static void M_ReadVertex(XYZ_16 *vertex, VFILE *file);
static void M_ReadFace4(FACE4 *face, VFILE *file);
static void M_ReadFace3(FACE3 *face, VFILE *file);
static void M_ReadRoomMesh(int32_t room_num, VFILE *file);
static void M_ReadObjectMesh(OBJECT_MESH *mesh, VFILE *file);
static void M_ReadBounds16(BOUNDS_16 *bounds, VFILE *file);
static void M_ReadObjectVector(OBJECT_VECTOR *obj, VFILE *file);

static RGBA_8888 M_ARGB1555To8888(const uint16_t argb1555)
{
    // Extract 5-bit values for each ARGB component
    uint8_t a1 = (argb1555 >> 15) & 0x01;
    uint8_t r5 = (argb1555 >> 10) & 0x1F;
    uint8_t g5 = (argb1555 >> 5) & 0x1F;
    uint8_t b5 = argb1555 & 0x1F;

    // Expand 5-bit color components to 8-bit
    uint8_t a8 = a1 * 255; // 1-bit alpha (either 0 or 255)
    uint8_t r8 = (r5 << 3) | (r5 >> 2);
    uint8_t g8 = (g5 << 3) | (g5 >> 2);
    uint8_t b8 = (b5 << 3) | (b5 >> 2);

    return (RGBA_8888) {
        .r = r8,
        .g = g8,
        .b = b8,
        .a = a8,
    };
}

static void M_ReadPosition(XYZ_32 *const pos, VFILE *const file)
{
    pos->x = VFile_ReadS32(file);
    pos->y = VFile_ReadS32(file);
    pos->z = VFile_ReadS32(file);
}

static void M_ReadShade(SHADE *const shade, VFILE *const file)
{
    shade->value_1 = VFile_ReadS16(file);
#if TR_VERSION == 1
    shade->value_2 = shade->value_1;
#else
    shade->value_2 = VFile_ReadS16(file);
#endif
}

static void M_ReadVertex(XYZ_16 *const vertex, VFILE *const file)
{
    vertex->x = VFile_ReadS16(file);
    vertex->y = VFile_ReadS16(file);
    vertex->z = VFile_ReadS16(file);
}

static void M_ReadFace4(FACE4 *const face, VFILE *const file)
{
    for (int32_t i = 0; i < 4; i++) {
        face->vertices[i] = VFile_ReadU16(file);
    }
    face->texture_idx = VFile_ReadU16(file);
    face->enable_reflections = false;
}

static void M_ReadFace3(FACE3 *const face, VFILE *const file)
{
    for (int32_t i = 0; i < 3; i++) {
        face->vertices[i] = VFile_ReadU16(file);
    }
    face->texture_idx = VFile_ReadU16(file);
    face->enable_reflections = false;
}

static void M_ReadRoomMesh(const int32_t room_num, VFILE *const file)
{
    ROOM *const room = Room_Get(room_num);
    const INJECTION_MESH_META inj_data = Inject_GetRoomMeshMeta(room_num);

    const uint32_t mesh_length = VFile_ReadU32(file);
    size_t start_pos = VFile_GetPos(file);

    {
        room->mesh.num_vertices = VFile_ReadS16(file);
        const int32_t alloc_count =
            room->mesh.num_vertices + inj_data.num_vertices;
        room->mesh.vertices =
            GameBuf_Alloc(sizeof(ROOM_VERTEX) * alloc_count, GBUF_ROOM_MESH);
        for (int32_t i = 0; i < room->mesh.num_vertices; i++) {
            ROOM_VERTEX *const vertex = &room->mesh.vertices[i];
            M_ReadVertex(&vertex->pos, file);
            vertex->light_base = VFile_ReadS16(file);
#if TR_VERSION == 1
            vertex->flags = 0;
            vertex->light_adder = vertex->light_base;
#elif TR_VERSION == 2
            vertex->light_table_value = VFile_ReadU8(file);
            vertex->flags = VFile_ReadU8(file);
            vertex->light_adder = VFile_ReadS16(file);
#endif
        }
    }

    {
        room->mesh.num_face4s = VFile_ReadS16(file);
        const int32_t alloc_count = room->mesh.num_face4s + inj_data.num_quads;
        room->mesh.face4s =
            GameBuf_Alloc(sizeof(FACE4) * alloc_count, GBUF_ROOM_MESH);
        for (int32_t i = 0; i < room->mesh.num_face4s; i++) {
            M_ReadFace4(&room->mesh.face4s[i], file);
        }
    }

    {
        room->mesh.num_face3s = VFile_ReadS16(file);
        const int32_t alloc_count =
            room->mesh.num_face3s + inj_data.num_triangles;
        room->mesh.face3s =
            GameBuf_Alloc(sizeof(FACE4) * alloc_count, GBUF_ROOM_MESH);
        for (int32_t i = 0; i < room->mesh.num_face3s; i++) {
            M_ReadFace3(&room->mesh.face3s[i], file);
        }
    }

    {
        room->mesh.num_sprites = VFile_ReadS16(file);
        const int32_t alloc_count =
            room->mesh.num_sprites + inj_data.num_sprites;
        room->mesh.sprites =
            GameBuf_Alloc(sizeof(ROOM_SPRITE) * alloc_count, GBUF_ROOM_MESH);
        for (int32_t i = 0; i < room->mesh.num_sprites; i++) {
            ROOM_SPRITE *const sprite = &room->mesh.sprites[i];
            sprite->vertex = VFile_ReadU16(file);
            sprite->texture = VFile_ReadU16(file);
        }
    }

    const size_t total_read =
        (VFile_GetPos(file) - start_pos) / sizeof(int16_t);
    ASSERT(total_read == mesh_length);
}

static void M_ReadObjectMesh(OBJECT_MESH *const mesh, VFILE *const file)
{
    M_ReadVertex(&mesh->center, file);
    mesh->radius = VFile_ReadS16(file);
    VFile_Skip(file, sizeof(int16_t));

    mesh->enable_reflections = false;

    {
        mesh->num_vertices = VFile_ReadS16(file);
        mesh->vertices =
            GameBuf_Alloc(sizeof(XYZ_16) * mesh->num_vertices, GBUF_MESHES);
        for (int32_t i = 0; i < mesh->num_vertices; i++) {
            M_ReadVertex(&mesh->vertices[i], file);
        }
    }

    {
        mesh->num_lights = VFile_ReadS16(file);
        if (mesh->num_lights > 0) {
            mesh->lighting.normals =
                GameBuf_Alloc(sizeof(XYZ_16) * mesh->num_lights, GBUF_MESHES);
            for (int32_t i = 0; i < mesh->num_lights; i++) {
                M_ReadVertex(&mesh->lighting.normals[i], file);
            }
        } else {
            mesh->lighting.lights = GameBuf_Alloc(
                sizeof(int16_t) * ABS(mesh->num_lights), GBUF_MESHES);
            for (int32_t i = 0; i < ABS(mesh->num_lights); i++) {
                mesh->lighting.lights[i] = VFile_ReadS16(file);
            }
        }
    }

    {
        mesh->num_tex_face4s = VFile_ReadS16(file);
        mesh->tex_face4s =
            GameBuf_Alloc(sizeof(FACE4) * mesh->num_tex_face4s, GBUF_MESHES);
        for (int32_t i = 0; i < mesh->num_tex_face4s; i++) {
            M_ReadFace4(&mesh->tex_face4s[i], file);
        }
    }

    {
        mesh->num_tex_face3s = VFile_ReadS16(file);
        mesh->tex_face3s =
            GameBuf_Alloc(sizeof(FACE3) * mesh->num_tex_face3s, GBUF_MESHES);
        for (int32_t i = 0; i < mesh->num_tex_face3s; i++) {
            M_ReadFace3(&mesh->tex_face3s[i], file);
        }
    }

    {
        mesh->num_flat_face4s = VFile_ReadS16(file);
        mesh->flat_face4s =
            GameBuf_Alloc(sizeof(FACE4) * mesh->num_flat_face4s, GBUF_MESHES);
        for (int32_t i = 0; i < mesh->num_flat_face4s; i++) {
            M_ReadFace4(&mesh->flat_face4s[i], file);
        }
    }

    {
        mesh->num_flat_face3s = VFile_ReadS16(file);
        mesh->flat_face3s =
            GameBuf_Alloc(sizeof(FACE3) * mesh->num_flat_face3s, GBUF_MESHES);
        for (int32_t i = 0; i < mesh->num_flat_face3s; i++) {
            M_ReadFace3(&mesh->flat_face3s[i], file);
        }
    }
}

static void M_ReadBounds16(BOUNDS_16 *const bounds, VFILE *const file)
{
    bounds->min.x = VFile_ReadS16(file);
    bounds->max.x = VFile_ReadS16(file);
    bounds->min.y = VFile_ReadS16(file);
    bounds->max.y = VFile_ReadS16(file);
    bounds->min.z = VFile_ReadS16(file);
    bounds->max.z = VFile_ReadS16(file);
}

static void M_ReadObjectVector(OBJECT_VECTOR *const obj, VFILE *const file)
{
    M_ReadPosition(&obj->pos, file);
    obj->data = VFile_ReadS16(file);
    obj->flags = VFile_ReadS16(file);
}

void Level_ReadPalettes(LEVEL_INFO *const info, VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    const int32_t palette_size = 256;
    info->palette.size = palette_size;

    info->palette.data_24 = Memory_Alloc(sizeof(RGB_888) * palette_size);
    VFile_Read(file, info->palette.data_24, sizeof(RGB_888) * palette_size);
    info->palette.data_24[0].r = 0;
    info->palette.data_24[0].g = 0;
    info->palette.data_24[0].b = 0;
    for (int32_t i = 1; i < palette_size; i++) {
        RGB_888 *const col = &info->palette.data_24[i];
        col->r = (col->r << 2) | (col->r >> 4);
        col->g = (col->g << 2) | (col->g >> 4);
        col->b = (col->b << 2) | (col->b >> 4);
    }

#if TR_VERSION == 1
    info->palette.data_32 = nullptr;
#else
    RGBA_8888 palette_16[palette_size];
    info->palette.data_32 = Memory_Alloc(sizeof(RGB_888) * palette_size);
    VFile_Read(file, palette_16, sizeof(RGBA_8888) * palette_size);
    for (int32_t i = 0; i < palette_size; i++) {
        info->palette.data_32[i].r = palette_16[i].r;
        info->palette.data_32[i].g = palette_16[i].g;
        info->palette.data_32[i].b = palette_16[i].b;
    }
#endif

    Benchmark_End(benchmark, nullptr);
}

// TODO: replace extra_pages with value from injection interface
void Level_ReadTexturePages(
    LEVEL_INFO *const info, const int32_t extra_pages, VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    const int32_t num_pages = VFile_ReadS32(file);
    info->textures.page_count = num_pages;
    LOG_INFO("texture pages: %d", num_pages);

    const int32_t texture_size_8_bit =
        num_pages * TEXTURE_PAGE_SIZE * sizeof(uint8_t);
    const int32_t texture_size_32_bit =
        (num_pages + extra_pages) * TEXTURE_PAGE_SIZE * sizeof(RGBA_8888);

    info->textures.pages_24 = Memory_Alloc(texture_size_8_bit);
    VFile_Read(file, info->textures.pages_24, texture_size_8_bit);

    info->textures.pages_32 = Memory_Alloc(texture_size_32_bit);
    RGBA_8888 *output = info->textures.pages_32;

#if TR_VERSION == 1
    const uint8_t *input = info->textures.pages_24;
    for (int32_t i = 0; i < texture_size_8_bit; i++) {
        const uint8_t index = *input++;
        const RGB_888 pix = info->palette.data_24[index];
        output->r = pix.r;
        output->g = pix.g;
        output->b = pix.b;
        output->a = index == 0 ? 0 : 0xFF;
        output++;
    }
#else
    const int32_t texture_size_16_bit =
        num_pages * TEXTURE_PAGE_SIZE * sizeof(uint16_t);
    uint16_t *input = Memory_Alloc(texture_size_16_bit);
    uint16_t *input_ptr = input;
    VFile_Read(file, input, texture_size_16_bit);
    for (int32_t i = 0; i < num_pages * TEXTURE_PAGE_SIZE; i++) {
        *output++ = M_ARGB1555To8888(*input_ptr++);
    }
    Memory_FreePointer(&input);
#endif

    Benchmark_End(benchmark, nullptr);
}

void Level_ReadRooms(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    const int32_t num_rooms = VFile_ReadS16(file);
    LOG_INFO("rooms: %d", num_rooms);
    if (num_rooms > MAX_ROOMS) {
        Shell_ExitSystem("Too many rooms");
        goto finish;
    }

    Room_InitialiseRooms(num_rooms);
    for (int32_t i = 0; i < num_rooms; i++) {
        ROOM *const room = Room_Get(i);

        room->pos.x = VFile_ReadS32(file);
        room->pos.y = 0;
        room->pos.z = VFile_ReadS32(file);

        room->min_floor = VFile_ReadS32(file);
        room->max_ceiling = VFile_ReadS32(file);

        M_ReadRoomMesh(i, file);

        const int16_t num_portals = VFile_ReadS16(file);
        if (num_portals <= 0) {
            room->portals = nullptr;
        } else {
            room->portals = GameBuf_Alloc(
                sizeof(PORTAL) * num_portals + sizeof(PORTALS),
                GBUF_ROOM_PORTALS);
            room->portals->count = num_portals;
            for (int32_t j = 0; j < num_portals; j++) {
                PORTAL *const portal = &room->portals->portal[j];
                portal->room_num = VFile_ReadS16(file);
                M_ReadVertex(&portal->normal, file);
                for (int32_t k = 0; k < 4; k++) {
                    M_ReadVertex(&portal->vertex[k], file);
                }
            }
        }

        room->size.z = VFile_ReadS16(file);
        room->size.x = VFile_ReadS16(file);

        const int32_t sector_count = room->size.x * room->size.z;
        room->sectors =
            GameBuf_Alloc(sizeof(SECTOR) * sector_count, GBUF_ROOM_SECTORS);
        for (int32_t i = 0; i < sector_count; i++) {
            SECTOR *const sector = &room->sectors[i];
            sector->idx = VFile_ReadU16(file);
            sector->box = VFile_ReadS16(file);
            sector->portal_room.pit = VFile_ReadU8(file);
            sector->floor.height = VFile_ReadS8(file) * STEP_L;
            sector->portal_room.sky = VFile_ReadU8(file);
            sector->ceiling.height = VFile_ReadS8(file) * STEP_L;
        }

        room->ambient = VFile_ReadS16(file);
#if TR_VERSION == 1
        room->light_mode = RLM_NORMAL;
#else
        VFile_Skip(file, sizeof(int16_t)); // Unused second ambient
        room->light_mode = VFile_ReadS16(file);
#endif

        room->num_lights = VFile_ReadS16(file);
        room->lights = room->num_lights == 0
            ? nullptr
            : GameBuf_Alloc(sizeof(LIGHT) * room->num_lights, GBUF_ROOM_LIGHTS);
        for (int32_t i = 0; i < room->num_lights; i++) {
            LIGHT *const light = &room->lights[i];
            M_ReadPosition(&light->pos, file);
            M_ReadShade(&light->shade, file);
            light->falloff.value_1 = VFile_ReadS32(file);
#if TR_VERSION == 2
            light->falloff.value_2 = VFile_ReadS32(file);
#endif
        }

        room->num_static_meshes = VFile_ReadS16(file);
        room->static_meshes = room->num_static_meshes == 0
            ? nullptr
            : GameBuf_Alloc(
                  sizeof(STATIC_MESH) * room->num_static_meshes,
                  GBUF_ROOM_STATIC_MESHES);
        for (int32_t i = 0; i < room->num_static_meshes; i++) {
            STATIC_MESH *const mesh = &room->static_meshes[i];
            M_ReadPosition(&mesh->pos, file);
            mesh->rot.y = VFile_ReadS16(file);
            M_ReadShade(&mesh->shade, file);
            mesh->static_num = VFile_ReadS16(file);
        }

        room->flipped_room = VFile_ReadS16(file);
        room->flags = VFile_ReadU16(file);

        room->bound_active = 0;
        room->bound_left = Viewport_GetMaxX();
        room->bound_top = Viewport_GetMaxY();
        room->bound_bottom = 0;
        room->bound_right = 0;
        room->item_num = NO_ITEM;
        room->effect_num = NO_EFFECT;
    }

    Room_InitialiseFlipStatus();

    const int32_t floor_data_size = VFile_ReadS32(file);
    int16_t *floor_data = Memory_Alloc(sizeof(int16_t) * floor_data_size);
    VFile_Read(file, floor_data, sizeof(int16_t) * floor_data_size);

    Room_ParseFloorData(floor_data);
    Memory_FreePointer(&floor_data);

finish:
    Benchmark_End(benchmark, nullptr);
}

void Level_ReadObjectMeshes(
    const int32_t num_indices, const int32_t *const indices, VFILE *const file)
{
    // Construct and store distinct meshes only e.g. Lara's hips are referenced
    // by several pointers as a dummy mesh.
    VECTOR *const unique_indices =
        Vector_CreateAtCapacity(sizeof(int32_t), num_indices);
    int32_t pointer_map[num_indices];
    for (int32_t i = 0; i < num_indices; i++) {
        const int32_t pointer = indices[i];
        const int32_t index = Vector_IndexOf(unique_indices, (void *)&pointer);
        if (index == -1) {
            pointer_map[i] = unique_indices->count;
            Vector_Add(unique_indices, (void *)&pointer);
        } else {
            pointer_map[i] = index;
        }
    }

    OBJECT_MESH *const meshes =
        GameBuf_Alloc(sizeof(OBJECT_MESH) * unique_indices->count, GBUF_MESHES);
    size_t start_pos = VFile_GetPos(file);
    for (int i = 0; i < unique_indices->count; i++) {
        const int32_t pointer = *(const int32_t *)Vector_Get(unique_indices, i);
        VFile_SetPos(file, start_pos + pointer);
        M_ReadObjectMesh(&meshes[i], file);

        // The original data position is required for backward compatibility
        // with savegame files, specifically for Lara's mesh pointers.
        Object_SetMeshOffset(&meshes[i], pointer / 2);
    }

    for (int32_t i = 0; i < num_indices; i++) {
        Object_StoreMesh(&meshes[pointer_map[i]]);
    }

    LOG_INFO("%d unique meshes constructed", unique_indices->count);

    Vector_Free(unique_indices);
}

void Level_ReadAnims(
    const int32_t base_idx, const int32_t num_anims, VFILE *const file)
{
    for (int32_t i = 0; i < num_anims; i++) {
        ANIM *const anim = Anim_GetAnim(base_idx + i);
        anim->frame_ofs = VFile_ReadU32(file);
        anim->frame_ptr = nullptr; // filled later by the animation frame loader
        anim->interpolation = VFile_ReadU8(file);
        anim->frame_size = VFile_ReadU8(file);
        anim->current_anim_state = VFile_ReadS16(file);
        anim->velocity = VFile_ReadS32(file);
        anim->acceleration = VFile_ReadS32(file);
        anim->frame_base = VFile_ReadS16(file);
        anim->frame_end = VFile_ReadS16(file);
        anim->jump_anim_num = VFile_ReadS16(file);
        anim->jump_frame_num = VFile_ReadS16(file);
        anim->num_changes = VFile_ReadS16(file);
        anim->change_idx = VFile_ReadS16(file);
        anim->num_commands = VFile_ReadS16(file);
        anim->command_idx = VFile_ReadS16(file);
    }
}

void Level_ReadAnimChanges(
    const int32_t base_idx, const int32_t num_changes, VFILE *const file)
{
    for (int32_t i = 0; i < num_changes; i++) {
        ANIM_CHANGE *const anim_change = Anim_GetChange(base_idx + i);
        anim_change->goal_anim_state = VFile_ReadS16(file);
        anim_change->num_ranges = VFile_ReadS16(file);
        anim_change->range_idx = VFile_ReadS16(file);
    }
}

void Level_ReadAnimRanges(
    const int32_t base_idx, const int32_t num_ranges, VFILE *const file)
{
    for (int32_t i = 0; i < num_ranges; i++) {
        ANIM_RANGE *const anim_range = Anim_GetRange(base_idx + i);
        anim_range->start_frame = VFile_ReadS16(file);
        anim_range->end_frame = VFile_ReadS16(file);
        anim_range->link_anim_num = VFile_ReadS16(file);
        anim_range->link_frame_num = VFile_ReadS16(file);
    }
}

void Level_InitialiseAnimCommands(const int32_t num_cmds)
{
    m_AnimCommands = Memory_Alloc(sizeof(int16_t) * num_cmds);
}

void Level_ReadAnimCommands(
    const int32_t base_idx, const int32_t num_cmds, VFILE *const file)
{
    VFile_Read(file, m_AnimCommands + base_idx, sizeof(int16_t) * num_cmds);
}

void Level_LoadAnimCommands(void)
{
    Anim_LoadCommands(m_AnimCommands);
    Memory_FreePointer(&m_AnimCommands);
}

void Level_ReadAnimBones(
    const int32_t base_idx, const int32_t num_bones, VFILE *const file)
{
    for (int32_t i = 0; i < num_bones; i++) {
        ANIM_BONE *const bone = Anim_GetBone(base_idx + i);
        const int32_t flags = VFile_ReadS32(file);
        bone->matrix_pop = (flags & 1) != 0;
        bone->matrix_push = (flags & 2) != 0;
        bone->rot_x = false;
        bone->rot_y = false;
        bone->rot_z = false;
        M_ReadPosition(&bone->pos, file);
    }
}

void Level_LoadAnimFrames(LEVEL_INFO *const info)
{
    const int32_t frame_count =
        Anim_GetTotalFrameCount(info->anims.frame_count);
    Anim_InitialiseFrames(frame_count);
    Anim_LoadFrames(info->anims.frames, info->anims.frame_count);
    Memory_FreePointer(&info->anims.frames);
}

void Level_ReadObjects(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_objects = VFile_ReadS32(file);
    LOG_INFO("objects: %d", num_objects);
    for (int32_t i = 0; i < num_objects; i++) {
        const GAME_OBJECT_ID obj_id = VFile_ReadS32(file);
        if (obj_id < 0 || obj_id >= O_NUMBER_OF) {
            Shell_ExitSystemFmt(
                "Invalid object ID: %d (max=%d)", obj_id, O_NUMBER_OF);
        }

        OBJECT *const obj = Object_Get(obj_id);
        obj->mesh_count = VFile_ReadS16(file);
        obj->mesh_idx = VFile_ReadS16(file);
        obj->bone_idx = VFile_ReadS32(file) / ANIM_BONE_SIZE;
        obj->frame_ofs = VFile_ReadU32(file);
        obj->frame_base = nullptr;
        obj->anim_idx = VFile_ReadS16(file);
        obj->loaded = true;
    }

    Benchmark_End(benchmark, nullptr);
}

void Level_ReadStaticObjects(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_objects = VFile_ReadS32(file);
    LOG_INFO("static objects: %d", num_objects);
    for (int32_t i = 0; i < num_objects; i++) {
        const int32_t static_id = VFile_ReadS32(file);
        if (static_id < 0 || static_id >= MAX_STATIC_OBJECTS) {
            Shell_ExitSystemFmt(
                "Invalid static ID: %d (max=%d)", static_id,
                MAX_STATIC_OBJECTS);
        }

        STATIC_OBJECT_3D *const obj = Object_Get3DStatic(static_id);
        obj->mesh_idx = VFile_ReadS16(file);
        obj->loaded = true;

        M_ReadBounds16(&obj->draw_bounds, file);
        M_ReadBounds16(&obj->collision_bounds, file);

        const uint16_t flags = VFile_ReadU16(file);
        obj->collidable = (flags & 1) == 0;
        obj->visible = (flags & 2) != 0;
    }

    Benchmark_End(benchmark, nullptr);
}

void Level_ReadObjectTextures(
    const int32_t base_idx, const int16_t base_page_idx,
    const int32_t num_textures, VFILE *const file)
{
    for (int32_t i = 0; i < num_textures; i++) {
        OBJECT_TEXTURE *const texture = Output_GetObjectTexture(base_idx + i);
        texture->draw_type = VFile_ReadU16(file);
        texture->tex_page = VFile_ReadU16(file) + base_page_idx;
        for (int32_t j = 0; j < 4; j++) {
            texture->uv[j].u = VFile_ReadU16(file);
            texture->uv[j].v = VFile_ReadU16(file);
        }
    }
}

void Level_ReadSpriteTextures(
    const int32_t base_idx, const int16_t base_page_idx,
    const int32_t num_textures, VFILE *const file)
{
    for (int32_t i = 0; i < num_textures; i++) {
        SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(base_idx + i);
        sprite->tex_page = VFile_ReadU16(file) + base_page_idx;
        sprite->offset = VFile_ReadU16(file);
        sprite->width = VFile_ReadU16(file);
        sprite->height = VFile_ReadU16(file);
        sprite->x0 = VFile_ReadS16(file);
        sprite->y0 = VFile_ReadS16(file);
        sprite->x1 = VFile_ReadS16(file);
        sprite->y1 = VFile_ReadS16(file);
    }
}

void Level_ReadSpriteSequences(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_sequences = VFile_ReadS32(file);
    LOG_DEBUG("sprite sequences: %d", num_sequences);
    for (int32_t i = 0; i < num_sequences; i++) {
        const int32_t object_id = VFile_ReadS32(file);
        const int16_t num_meshes = VFile_ReadS16(file);
        const int16_t mesh_idx = VFile_ReadS16(file);

        if (object_id >= 0 && object_id < O_NUMBER_OF) {
            OBJECT *const obj = Object_Get(object_id);
            obj->mesh_count = num_meshes;
            obj->mesh_idx = mesh_idx;
            obj->loaded = true;
        } else if (object_id - O_NUMBER_OF < MAX_STATIC_OBJECTS) {
            STATIC_OBJECT_2D *const obj =
                Object_Get2DStatic(object_id - O_NUMBER_OF);
            obj->frame_count = ABS(num_meshes);
            obj->texture_idx = mesh_idx;
            obj->loaded = true;
        } else {
            Shell_ExitSystemFmt("Invalid sprite slot (%d)", object_id);
        }
    }

    Benchmark_End(benchmark, nullptr);
}

void Level_ReadAnimatedTextureRanges(
    const int32_t num_ranges, VFILE *const file)
{
    for (int32_t i = 0; i < num_ranges; i++) {
        ANIMATED_TEXTURE_RANGE *const range = Output_GetAnimatedTextureRange(i);
        range->next_range = i == num_ranges - 1
            ? nullptr
            : Output_GetAnimatedTextureRange(i + 1);

        // Level data is tied to the original logic in Output_AnimateTextures
        // and hence stores one less than the actual count here.
        range->num_textures = VFile_ReadS16(file) + 1;
        range->textures = GameBuf_Alloc(
            sizeof(int16_t) * range->num_textures,
            GBUF_ANIMATED_TEXTURE_RANGES);
        VFile_Read(
            file, range->textures, sizeof(int16_t) * range->num_textures);
    }
}

void Level_ReadLightMap(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    for (int32_t i = 0; i < 32; i++) {
        LIGHT_MAP *const light_map = Output_GetLightMap(i);
        VFile_Read(file, light_map->index, sizeof(uint8_t) * 256);
        light_map->index[0] = 0;
    }

    for (int32_t i = 0; i < 32; i++) {
        const LIGHT_MAP *const light_map = Output_GetLightMap(i);
        for (int32_t j = 0; j < 256; j++) {
            SHADE_MAP *const shade_map = Output_GetShadeMap(j);
            shade_map->index[i] = light_map->index[j];
        }
    }

    Benchmark_End(benchmark, nullptr);
}

void Level_ReadCinematicFrames(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int16_t num_frames = VFile_ReadS16(file);
    LOG_INFO("cinematic frames: %d", num_frames);
    Camera_InitialiseCineFrames(num_frames);
    for (int32_t i = 0; i < num_frames; i++) {
        CINE_FRAME *const frame = Camera_GetCineFrame(i);
        frame->tx = VFile_ReadS16(file);
        frame->ty = VFile_ReadS16(file);
        frame->tz = VFile_ReadS16(file);
        frame->cx = VFile_ReadS16(file);
        frame->cy = VFile_ReadS16(file);
        frame->cz = VFile_ReadS16(file);
        frame->fov = VFile_ReadS16(file);
        frame->roll = VFile_ReadS16(file);
    }

    Benchmark_End(benchmark, nullptr);
}

void Level_ReadCamerasAndSinks(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_objects = VFile_ReadS32(file);
    LOG_DEBUG("fixed cameras/sinks: %d", num_objects);
    Camera_InitialiseFixedObjects(num_objects);
    for (int32_t i = 0; i < num_objects; i++) {
        M_ReadObjectVector(Camera_GetFixedObject(i), file);
    }

    Benchmark_End(benchmark, nullptr);
}

void Level_ReadItems(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_items = VFile_ReadS32(file);
    LOG_INFO("items: %d", num_items);
    if (num_items > MAX_ITEMS) {
        Shell_ExitSystem("Too many items");
        goto finish;
    }

    Item_InitialiseItems(num_items);
    for (int32_t i = 0; i < num_items; i++) {
        ITEM *const item = Item_Get(i);
        item->object_id = VFile_ReadS16(file);
        item->room_num = VFile_ReadS16(file);
        M_ReadPosition(&item->pos, file);
        item->rot.y = VFile_ReadS16(file);
        M_ReadShade(&item->shade, file);
        item->flags = VFile_ReadS16(file);
        if (item->object_id < 0 || item->object_id >= O_NUMBER_OF) {
            Shell_ExitSystemFmt(
                "Bad object number (%d) on item %d", item->object_id, i);
            goto finish;
        }
    }

finish:
    Benchmark_End(benchmark, nullptr);
}

void Level_ReadDemoData(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const uint16_t size = VFile_ReadU16(file);
    LOG_INFO("demo buffer size: %d", size);
    Demo_InitialiseData(size);
    if (size != 0) {
        uint32_t *const data = Demo_GetData();
        VFile_Read(file, data, size);
    }
    Benchmark_End(benchmark, nullptr);
}

void Level_ReadSoundSources(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_sources = VFile_ReadS32(file);
    LOG_INFO("sound sources: %d", num_sources);
    Sound_InitialiseSources(num_sources);
    for (int32_t i = 0; i < num_sources; i++) {
        M_ReadObjectVector(Sound_GetSource(i), file);
    }

    Benchmark_End(benchmark, nullptr);
}

// TODO: replace extra vars with values from injection interface
void Level_ReadSamples(
    LEVEL_INFO *const info, const int32_t extra_sfx_count,
    const int32_t extra_data_size, const int32_t extra_offset_count,
    VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    int16_t *const sample_lut = Sound_GetSampleLUT();
    VFile_Read(file, sample_lut, sizeof(int16_t) * SFX_NUMBER_OF);

    const int32_t num_sample_infos = VFile_ReadS32(file);
    info->samples.info_count = num_sample_infos;
    LOG_INFO("sample infos: %d", num_sample_infos);
    Sound_InitialiseSampleInfos(num_sample_infos + extra_sfx_count);
    for (int32_t i = 0; i < num_sample_infos; i++) {
        SAMPLE_INFO *const sample_info = Sound_GetSampleInfoByIdx(i);
        sample_info->number = VFile_ReadS16(file);
        sample_info->volume = VFile_ReadS16(file);
        sample_info->randomness = VFile_ReadS16(file);
        sample_info->flags = VFile_ReadS16(file);
    }

#if TR_VERSION == 1
    const int32_t data_size = VFile_ReadS32(file);
    info->samples.data_size = data_size;
    LOG_INFO("%d sample data size", data_size);

    info->samples.data =
        GameBuf_Alloc(data_size + extra_data_size, GBUF_SAMPLES);
    VFile_Read(file, info->samples.data, sizeof(char) * data_size);
#endif

    const int32_t num_offsets = VFile_ReadS32(file);
    LOG_INFO("samples: %d", num_offsets);
    info->samples.offset_count = num_offsets;

    info->samples.offsets =
        Memory_Alloc(sizeof(int32_t) * (num_offsets + extra_offset_count));
    VFile_Read(file, info->samples.offsets, sizeof(int32_t) * num_offsets);

finish:
    Benchmark_End(benchmark, nullptr);
}

void Level_LoadTexturePages(LEVEL_INFO *const info)
{
    const int32_t num_pages = info->textures.page_count;
    Output_InitialiseTexturePages(num_pages, TR_VERSION == 2);
    for (int32_t i = 0; i < num_pages; i++) {
#if TR_VERSION == 2
        uint8_t *const target_8 = Output_GetTexturePage8(i);
        const uint8_t *const source_8 =
            &info->textures.pages_24[i * TEXTURE_PAGE_SIZE];
        memcpy(target_8, source_8, TEXTURE_PAGE_SIZE * sizeof(uint8_t));
#endif

        RGBA_8888 *const target_32 = Output_GetTexturePage32(i);
        const RGBA_8888 *const source_32 =
            &info->textures.pages_32[i * TEXTURE_PAGE_SIZE];
        memcpy(target_32, source_32, TEXTURE_PAGE_SIZE * sizeof(RGBA_8888));
    }

    Memory_FreePointer(&info->textures.pages_24);
    Memory_FreePointer(&info->textures.pages_32);
}

void Level_LoadPalettes(LEVEL_INFO *const info)
{
    Output_InitialisePalettes(
        info->palette.size, info->palette.data_24, info->palette.data_32);
    Memory_FreePointer(&info->palette.data_24);
    Memory_FreePointer(&info->palette.data_32);
}

void Level_LoadObjectsAndItems(void)
{
    // Object and item setup/initialisation must take place after injections
    // have been processed. A cached item count must be used as individual
    // initialisations may increment the total item count.
    Object_SetupAllObjects();

    const int32_t item_count = Item_GetLevelCount();
    for (int32_t i = 0; i < item_count; i++) {
        Item_Initialise(i);
    }
}
