#include "game/level.h"

#include "game/hwr.h"
#include "game/items.h"
#include "game/shell.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"
#include "specific/s_audio_sample.h"

#include <libtrx/benchmark.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/virtual_file.h>

#include <assert.h>

static void __cdecl M_LoadTexturePages(VFILE *file);
static void __cdecl M_LoadRooms(VFILE *file);
static void __cdecl M_LoadMeshBase(VFILE *file);
static void __cdecl M_LoadMeshes(VFILE *file);
static int32_t __cdecl M_LoadAnims(VFILE *file, int32_t **frame_pointers);
static void __cdecl M_LoadAnimChanges(VFILE *file);
static void __cdecl M_LoadAnimRanges(VFILE *file);
static void __cdecl M_LoadAnimCommands(VFILE *file);
static void __cdecl M_LoadAnimBones(VFILE *file);
static void __cdecl M_LoadAnimFrames(VFILE *file);
static void __cdecl M_LoadObjects(VFILE *file);
static void __cdecl M_LoadStaticObjects(VFILE *file);
static void __cdecl M_LoadTextures(VFILE *file);
static void __cdecl M_LoadSprites(VFILE *file);
static void __cdecl M_LoadItems(VFILE *file);
static void __cdecl M_LoadDepthQ(VFILE *file);
static void __cdecl M_LoadPalettes(VFILE *file);
static void __cdecl M_LoadCameras(VFILE *file);
static void __cdecl M_LoadSoundEffects(VFILE *file);
static void __cdecl M_LoadBoxes(VFILE *file);
static void __cdecl M_LoadAnimatedTextures(VFILE *file);
static void __cdecl M_LoadCinematic(VFILE *file);
static void __cdecl M_LoadDemo(VFILE *file);
static void __cdecl M_LoadDemoExternal(const char *level_name);
static void __cdecl M_LoadSamples(VFILE *file);

static void __cdecl M_LoadTexturePages(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    char *base = NULL;

    const bool is_16_bit = g_TextureFormat.bpp >= 16;
    const int32_t texture_size = TEXTURE_PAGE_WIDTH * TEXTURE_PAGE_HEIGHT;
    const int32_t texture_size_8_bit = texture_size * sizeof(uint8_t);
    const int32_t texture_size_16_bit = texture_size * sizeof(uint16_t);

    const int32_t num_pages = VFile_ReadS32(file);
    LOG_INFO("texture pages: %d", num_pages);

    if (g_SavedAppSettings.render_mode == RM_SOFTWARE) {
        for (int32_t i = 0; i < num_pages; i++) {
            if (g_TexturePageBuffer8[i] == NULL) {
                g_TexturePageBuffer8[i] =
                    game_malloc(texture_size, GBUF_TEXTURE_PAGES);
            }
            VFile_Read(file, g_TexturePageBuffer8[i], texture_size);
        }
        // skip 16-bit texture pages
        VFile_Skip(file, num_pages * texture_size_16_bit);
        goto finish;
    }

    base = Memory_Alloc(
        num_pages * (is_16_bit ? texture_size_16_bit : texture_size_8_bit));

    if (is_16_bit) {
        VFile_Skip(file, num_pages * texture_size_8_bit);
        char *ptr = base;
        for (int32_t i = 0; i < num_pages; i++) {
            VFile_Read(file, ptr, texture_size_16_bit);
            ptr += texture_size_16_bit;
        }
        HWR_LoadTexturePages(num_pages, base, NULL);
    } else {
        char *ptr = base;
        for (int32_t i = 0; i < num_pages; i++) {
            VFile_Read(file, ptr, texture_size_8_bit);
            ptr += texture_size_8_bit;
        }
        VFile_Skip(file, num_pages * texture_size_16_bit);
        HWR_LoadTexturePages((int32_t)num_pages, base, g_GamePalette8);
    }

    g_TexturePageCount = num_pages;

finish:
    Memory_FreePointer(&base);
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadRooms(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    g_RoomCount = VFile_ReadS16(file);
    LOG_INFO("rooms: %d", g_RoomCount);
    if (g_RoomCount > MAX_ROOMS) {
        Shell_ExitSystem("Too many rooms");
        goto finish;
    }

    g_Rooms = game_malloc(sizeof(ROOM) * g_RoomCount, GBUF_ROOMS);
    assert(g_Rooms != NULL);

    for (int32_t i = 0; i < g_RoomCount; i++) {
        ROOM *const r = &g_Rooms[i];

        r->pos.x = VFile_ReadS32(file);
        r->pos.y = 0;
        r->pos.z = VFile_ReadS32(file);

        r->min_floor = VFile_ReadS32(file);
        r->max_ceiling = VFile_ReadS32(file);

        const int32_t data_size = VFile_ReadS32(file);
        r->data = game_malloc(sizeof(int16_t) * data_size, GBUF_ROOM_MESH);
        VFile_Read(file, r->data, sizeof(int16_t) * data_size);

        const int16_t num_doors = VFile_ReadS16(file);
        if (num_doors <= 0) {
            r->portals = NULL;
        } else {
            r->portals = game_malloc(
                sizeof(PORTAL) * num_doors + sizeof(PORTALS),
                GBUF_ROOM_PORTALS);
            r->portals->count = num_doors;
            VFile_Read(file, r->portals->portal, sizeof(PORTAL) * num_doors);
        }

        r->size.z = VFile_ReadS16(file);
        r->size.x = VFile_ReadS16(file);

        r->sectors = game_malloc(
            sizeof(SECTOR) * r->size.z * r->size.x, GBUF_ROOM_FLOOR);
        for (int32_t i = 0; i < r->size.z * r->size.x; i++) {
            SECTOR *const sector = &r->sectors[i];
            sector->idx = VFile_ReadU16(file);
            sector->box = VFile_ReadS16(file);
            sector->pit_room = VFile_ReadU8(file);
            sector->floor = VFile_ReadS8(file);
            sector->sky_room = VFile_ReadU8(file);
            sector->ceiling = VFile_ReadS8(file);
        }

        r->ambient_1 = VFile_ReadS16(file);
        r->ambient_2 = VFile_ReadS16(file);
        r->light_mode = VFile_ReadS16(file);

        r->num_lights = VFile_ReadS16(file);
        if (!r->num_lights) {
            r->lights = NULL;
        } else {
            r->lights =
                game_malloc(sizeof(LIGHT) * r->num_lights, GBUF_ROOM_LIGHTS);
            for (int32_t i = 0; i < r->num_lights; i++) {
                LIGHT *const light = &r->lights[i];
                light->pos.x = VFile_ReadS32(file);
                light->pos.y = VFile_ReadS32(file);
                light->pos.z = VFile_ReadS32(file);
                light->intensity_1 = VFile_ReadS16(file);
                light->intensity_2 = VFile_ReadS16(file);
                light->falloff_1 = VFile_ReadS32(file);
                light->falloff_2 = VFile_ReadS32(file);
            }
        }

        r->num_meshes = VFile_ReadS16(file);
        if (!r->num_meshes) {
            r->meshes = NULL;
        } else {
            r->meshes = game_malloc(
                sizeof(MESH) * r->num_meshes, GBUF_ROOM_STATIC_MESHES);
            for (int32_t i = 0; i < r->num_meshes; i++) {
                MESH *const mesh = &r->meshes[i];
                mesh->pos.x = VFile_ReadS32(file);
                mesh->pos.y = VFile_ReadS32(file);
                mesh->pos.z = VFile_ReadS32(file);
                mesh->rot.y = VFile_ReadS16(file);
                mesh->shade_1 = VFile_ReadS16(file);
                mesh->shade_2 = VFile_ReadS16(file);
                mesh->static_num = VFile_ReadS16(file);
            }
        }

        r->flipped_room = VFile_ReadS16(file);
        r->flags = VFile_ReadU16(file);

        r->bound_active = 0;
        r->bound_left = g_PhdWinMaxX;
        r->bound_top = g_PhdWinMaxY;
        r->bound_bottom = 0;
        r->bound_right = 0;
        r->item_num = NO_ITEM;
        r->fx_num = NO_ITEM;
    }

    const int32_t floor_data_size = VFile_ReadS32(file);
    g_FloorData =
        game_malloc(sizeof(int16_t) * floor_data_size, GBUF_FLOOR_DATA);
    VFile_Read(file, g_FloorData, sizeof(int16_t) * floor_data_size);

finish:
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadMeshBase(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_meshes = VFile_ReadS32(file);
    LOG_INFO("meshes: %d", num_meshes);
    g_MeshBase = game_malloc(sizeof(int16_t) * num_meshes, GBUF_MESHES);
    VFile_Read(file, g_MeshBase, sizeof(int16_t) * num_meshes);
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadMeshes(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_mesh_ptrs = VFile_ReadS32(file);
    LOG_INFO("mesh pointers: %d", num_mesh_ptrs);
    int32_t *const mesh_indices =
        (int32_t *)Memory_Alloc(sizeof(int32_t) * num_mesh_ptrs);
    VFile_Read(file, mesh_indices, sizeof(int32_t) * num_mesh_ptrs);

    g_Meshes =
        game_malloc(sizeof(int16_t *) * num_mesh_ptrs, GBUF_MESH_POINTERS);
    for (int32_t i = 0; i < num_mesh_ptrs; i++) {
        g_Meshes[i] = &g_MeshBase[mesh_indices[i] / 2];
    }

    Memory_Free(mesh_indices);
    Benchmark_End(benchmark, NULL);
}

static int32_t __cdecl M_LoadAnims(VFILE *const file, int32_t **frame_pointers)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anims = VFile_ReadS32(file);
    LOG_INFO("anims: %d", num_anims);
    g_Anims = game_malloc(sizeof(ANIM) * num_anims, GBUF_ANIMS);
    if (frame_pointers != NULL) {
        *frame_pointers = Memory_Alloc(sizeof(int32_t) * num_anims);
    }

    for (int32_t i = 0; i < num_anims; i++) {
        ANIM *const anim = &g_Anims[i];
        const int32_t frame_idx = VFile_ReadS32(file);
        if (frame_pointers != NULL) {
            (*frame_pointers)[i] = frame_idx;
        }
        anim->frame_ptr = NULL; // filled later by the animation frame loader
        anim->interpolation = VFile_ReadS16(file);
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
    Benchmark_End(benchmark, NULL);
    return num_anims;
}

static void __cdecl M_LoadAnimChanges(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_changes = VFile_ReadS32(file);
    LOG_INFO("anim changes: %d", num_anim_changes);
    g_AnimChanges =
        game_malloc(sizeof(ANIM_CHANGE) * num_anim_changes, GBUF_STRUCTS);
    for (int32_t i = 0; i < num_anim_changes; i++) {
        ANIM_CHANGE *const change = &g_AnimChanges[i];
        change->goal_anim_state = VFile_ReadS16(file);
        change->num_ranges = VFile_ReadS16(file);
        change->range_idx = VFile_ReadS16(file);
    }
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadAnimRanges(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_ranges = VFile_ReadS32(file);
    LOG_INFO("anim ranges: %d", num_anim_ranges);
    g_AnimRanges =
        game_malloc(sizeof(ANIM_RANGE) * num_anim_ranges, GBUF_ANIM_RANGES);
    for (int32_t i = 0; i < num_anim_ranges; i++) {
        ANIM_RANGE *const range = &g_AnimRanges[i];
        range->start_frame = VFile_ReadS16(file);
        range->end_frame = VFile_ReadS16(file);
        range->link_anim_num = VFile_ReadS16(file);
        range->link_frame_num = VFile_ReadS16(file);
    }
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadAnimCommands(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_commands = VFile_ReadS32(file);
    LOG_INFO("anim commands: %d", num_anim_commands);
    g_AnimCommands =
        game_malloc(sizeof(int16_t) * num_anim_commands, GBUF_ANIM_COMMANDS);
    VFile_Read(file, g_AnimCommands, sizeof(int16_t) * num_anim_commands);
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadAnimBones(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_bones = VFile_ReadS32(file);
    LOG_INFO("anim bones: %d", num_anim_bones);
    g_AnimBones =
        game_malloc(sizeof(int32_t) * num_anim_bones, GBUF_ANIM_BONES);
    VFile_Read(file, g_AnimBones, sizeof(int32_t) * num_anim_bones);
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadAnimFrames(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t anim_frame_data_size = VFile_ReadS32(file);
    LOG_INFO("anim frame data size: %d", anim_frame_data_size);
    g_AnimFrames =
        game_malloc(sizeof(int16_t) * anim_frame_data_size, GBUF_ANIM_FRAMES);
    // TODO: make me FRAME_INFO
    int16_t *ptr = (int16_t *)&g_AnimFrames[0];
    VFile_Read(file, ptr, sizeof(int16_t) * anim_frame_data_size);
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadObjects(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_objects = VFile_ReadS32(file);
    LOG_INFO("objects: %d", num_objects);
    for (int32_t i = 0; i < num_objects; i++) {
        const GAME_OBJECT_ID object_id = VFile_ReadS32(file);
        OBJECT *const object = &g_Objects[object_id];
        object->mesh_count = VFile_ReadS16(file);
        object->mesh_idx = VFile_ReadS16(file);
        object->bone_idx = VFile_ReadS32(file);
        const int32_t frame_idx = VFile_ReadS32(file);
        object->frame_base = ((int16_t *)g_AnimFrames) + frame_idx / 2;
        object->anim_idx = VFile_ReadS16(file);
        object->loaded = 1;
    }
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadStaticObjects(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_static_objects = VFile_ReadS32(file);
    LOG_INFO("static objects: %d", num_static_objects);
    for (int32_t i = 0; i < num_static_objects; i++) {
        const int32_t static_num = VFile_ReadS32(file);
        STATIC_INFO *static_obj = &g_StaticObjects[static_num];
        static_obj->mesh_idx = VFile_ReadS16(file);
        static_obj->draw_bounds.min_x = VFile_ReadS16(file);
        static_obj->draw_bounds.max_x = VFile_ReadS16(file);
        static_obj->draw_bounds.min_y = VFile_ReadS16(file);
        static_obj->draw_bounds.max_y = VFile_ReadS16(file);
        static_obj->draw_bounds.min_z = VFile_ReadS16(file);
        static_obj->draw_bounds.max_z = VFile_ReadS16(file);
        static_obj->collision_bounds.min_x = VFile_ReadS16(file);
        static_obj->collision_bounds.max_x = VFile_ReadS16(file);
        static_obj->collision_bounds.min_y = VFile_ReadS16(file);
        static_obj->collision_bounds.max_y = VFile_ReadS16(file);
        static_obj->collision_bounds.min_z = VFile_ReadS16(file);
        static_obj->collision_bounds.max_z = VFile_ReadS16(file);
        static_obj->flags = VFile_ReadU16(file);
    }
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadTextures(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_textures = VFile_ReadS32(file);
    LOG_INFO("textures: %d", num_textures);
    if (num_textures > MAX_TEXTURES) {
        Shell_ExitSystem("Too many textures");
        return;
    }

    g_TextureInfoCount = num_textures;
    for (int32_t i = 0; i < num_textures; i++) {
        PHD_TEXTURE *texture = &g_TextureInfo[i];
        texture->draw_type = VFile_ReadU16(file);
        texture->tex_page = VFile_ReadU16(file);
        for (int32_t j = 0; j < 4; j++) {
            texture->uv[j].u = VFile_ReadU16(file);
            texture->uv[j].v = VFile_ReadU16(file);
        }
    }

    for (int32_t i = 0; i < num_textures; i++) {
        uint16_t *const uv = &g_TextureInfo[i].uv[0].u;
        uint8_t byte = 0;
        for (int32_t j = 0; j < 8; j++) {
            if ((uv[j] & 0x80) != 0) {
                uv[j] |= 0xFF;
                byte |= 1 << j;
            } else {
                uv[j] &= 0xFF00;
            }
        }
        g_LabTextureUVFlag[i] = byte;
    }

    AdjustTextureUVs(true);
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadSprites(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_sprites = VFile_ReadS32(file);
    LOG_DEBUG("sprites: %d", num_sprites);
    for (int32_t i = 0; i < num_sprites; i++) {
        PHD_SPRITE *const sprite = &g_PhdSprites[i];
        sprite->tex_page = VFile_ReadU16(file);
        sprite->offset = VFile_ReadU16(file);
        sprite->width = VFile_ReadU16(file);
        sprite->height = VFile_ReadU16(file);
        sprite->x0 = VFile_ReadS16(file);
        sprite->y0 = VFile_ReadS16(file);
        sprite->x1 = VFile_ReadS16(file);
        sprite->y1 = VFile_ReadS16(file);
    }

    const int32_t num_statics = VFile_ReadS32(file);
    LOG_DEBUG("statics: %d", num_statics);
    for (int32_t i = 0; i < num_statics; i++) {
        int32_t object_id = VFile_ReadS32(file);
        if (object_id >= O_NUMBER_OF) {
            object_id -= O_NUMBER_OF;
            STATIC_INFO *const static_object = &g_StaticObjects[object_id];
            VFile_Skip(file, sizeof(int16_t));
            static_object->mesh_idx = VFile_ReadS16(file);
        } else {
            OBJECT *const object = &g_Objects[object_id];
            object->mesh_count = VFile_ReadS16(file);
            object->mesh_idx = VFile_ReadS16(file);
            object->loaded = 1;
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadItems(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    const int32_t num_items = VFile_ReadS32(file);
    LOG_DEBUG("items: %d", num_items);
    if (!num_items) {
        goto finish;
    }

    if (num_items > MAX_ITEMS) {
        Shell_ExitSystem("Too many items");
        goto finish;
    }

    g_Items = game_malloc(sizeof(ITEM) * MAX_ITEMS, GBUF_ITEMS);
    g_LevelItemCount = num_items;

    Item_InitialiseArray(MAX_ITEMS);

    for (int32_t i = 0; i < num_items; i++) {
        ITEM *const item = &g_Items[i];
        item->object_id = VFile_ReadS16(file);
        item->room_num = VFile_ReadS16(file);
        item->pos.x = VFile_ReadS32(file);
        item->pos.y = VFile_ReadS32(file);
        item->pos.z = VFile_ReadS32(file);
        item->rot.y = VFile_ReadS16(file);
        item->shade_1 = VFile_ReadS16(file);
        item->shade_2 = VFile_ReadS16(file);
        item->flags = VFile_ReadS16(file);
        if (item->object_id < 0 || item->object_id >= O_NUMBER_OF) {
            Shell_ExitSystemFmt(
                "Bad object number (%d) on item %d", item->object_id, i);
            goto finish;
        }
        Item_Initialise(i);
    }

finish:
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadDepthQ(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    for (int32_t i = 0; i < 32; i++) {
        VFile_Read(file, g_DepthQTable[i].index, sizeof(uint8_t) * 256);
        g_DepthQTable[i].index[0] = 0;
    }

    if (g_GameVid_IsWindowedVGA) {
        RGB_888 palette[256];
        CopyBitmapPalette(
            g_GamePalette8, g_DepthQTable[0].index, 32 * sizeof(DEPTHQ_ENTRY),
            palette);
        SyncSurfacePalettes(
            g_DepthQTable, 256, 32, 256, g_GamePalette8, g_DepthQTable, 256,
            palette, true);
        memcpy(g_GamePalette8, palette, sizeof(g_GamePalette8));

        for (int32_t i = 0; i < 256; i++) {
            g_DepthQIndex[i] = S_COLOR(
                g_GamePalette8[i].red, g_GamePalette8[i].green,
                g_GamePalette8[i].blue);
        }
    } else {
        for (int32_t i = 0; i < 256; i++) {
            g_DepthQIndex[i] = g_DepthQTable[24].index[i];
        }
    }

    for (int32_t i = 0; i < 32; i++) {
        for (int32_t j = 0; j < 256; j++) {
            g_GouraudTable[j].index[i] = g_DepthQTable[i].index[j];
        }
    }

    g_IsWet = 0;
    for (int32_t i = 0; i < 256; i++) {
        g_WaterPalette[i].red = g_GamePalette8[i].red * 2 / 3;
        g_WaterPalette[i].green = g_GamePalette8[i].green * 2 / 3;
        g_WaterPalette[i].blue = g_GamePalette8[i].blue;
    }

    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadPalettes(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    VFile_Read(file, g_GamePalette8, sizeof(RGB_888) * 256);

    g_GamePalette8[0].red = 0;
    g_GamePalette8[0].green = 0;
    g_GamePalette8[0].blue = 0;

    for (int32_t i = 1; i < 256; i++) {
        RGB_888 *col = &g_GamePalette8[i];
        col->red = (col->red << 2) | (col->red >> 4);
        col->green = (col->green << 2) | (col->green >> 4);
        col->blue = (col->blue << 2) | (col->blue >> 4);
    }

    VFile_Read(file, g_GamePalette16, sizeof(PALETTEENTRY) * 256);
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadCameras(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_NumCameras = VFile_ReadS32(file);
    LOG_DEBUG("fixed cameras: %d", g_NumCameras);
    if (!g_NumCameras) {
        goto finish;
    }

    g_Camera.fixed =
        game_malloc(sizeof(OBJECT_VECTOR) * g_NumCameras, GBUF_CAMERAS);
    for (int32_t i = 0; i < g_NumCameras; i++) {
        OBJECT_VECTOR *const camera = &g_Camera.fixed[i];
        camera->x = VFile_ReadS32(file);
        camera->y = VFile_ReadS32(file);
        camera->z = VFile_ReadS32(file);
        camera->data = VFile_ReadS16(file);
        camera->flags = VFile_ReadS16(file);
    }

finish:
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadSoundEffects(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    g_SoundEffectCount = VFile_ReadS32(file);
    LOG_DEBUG("sound effects: %d", g_SoundEffectCount);
    if (!g_SoundEffectCount) {
        goto finish;
    }

    g_SoundEffects =
        game_malloc(sizeof(OBJECT_VECTOR) * g_SoundEffectCount, GBUF_SOUND_FX);
    for (int32_t i = 0; i < g_SoundEffectCount; i++) {
        OBJECT_VECTOR *const effect = &g_SoundEffects[i];
        effect->x = VFile_ReadS32(file);
        effect->y = VFile_ReadS32(file);
        effect->z = VFile_ReadS32(file);
        effect->data = VFile_ReadS16(file);
        effect->flags = VFile_ReadS16(file);
    }

finish:
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadBoxes(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_BoxCount = VFile_ReadS32(file);
    g_Boxes = game_malloc(sizeof(BOX_INFO) * g_BoxCount, GBUF_BOXES);
    for (int32_t i = 0; i < g_BoxCount; i++) {
        BOX_INFO *const box = &g_Boxes[i];
        box->left = VFile_ReadU8(file);
        box->right = VFile_ReadU8(file);
        box->top = VFile_ReadU8(file);
        box->bottom = VFile_ReadU8(file);
        box->height = VFile_ReadS16(file);
        box->overlap_index = VFile_ReadS16(file);
    }

    const int32_t num_overlaps = VFile_ReadS32(file);
    g_Overlap = game_malloc(sizeof(uint16_t) * num_overlaps, GBUF_OVERLAPS);
    VFile_Read(file, g_Overlap, sizeof(uint16_t) * num_overlaps);

    for (int32_t i = 0; i < 2; i++) {
        for (int32_t j = 0; j < 4; j++) {
            const bool skip = j == 2
                || (j == 1 && !g_Objects[O_SPIDER].loaded
                    && !g_Objects[O_SKIDOO_ARMED].loaded)
                || (j == 3 && !g_Objects[O_YETI].loaded
                    && !g_Objects[O_WORKER_3].loaded);

            if (skip) {
                VFile_Skip(file, sizeof(int16_t) * g_BoxCount);
                continue;
            }

            g_GroundZone[j][i] =
                game_malloc(sizeof(int16_t) * g_BoxCount, GBUF_GROUND_ZONE);
            VFile_Read(file, g_GroundZone[j][i], sizeof(int16_t) * g_BoxCount);
        }

        g_FlyZone[i] = game_malloc(sizeof(int16_t) * g_BoxCount, GBUF_FLY_ZONE);
        VFile_Read(file, g_FlyZone[i], sizeof(int16_t) * g_BoxCount);
    }

    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadAnimatedTextures(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_ranges = VFile_ReadS32(file);
    g_AnimTextureRanges = game_malloc(
        sizeof(int16_t) * num_ranges, GBUF_ANIMATING_TEXTURE_RANGES);
    VFile_Read(file, g_AnimTextureRanges, sizeof(int16_t) * num_ranges);
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadCinematic(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_NumCineFrames = VFile_ReadS16(file);
    if (g_NumCineFrames <= 0) {
        g_CineLoaded = false;
        goto finish;
    }

    g_CineData = game_malloc(
        sizeof(CINE_FRAME) * g_NumCineFrames, GBUF_CINEMATIC_FRAMES);
    for (int32_t i = 0; i < g_NumCineFrames; i++) {
        CINE_FRAME *const frame = &g_CineData[i];
        frame->tx = VFile_ReadS16(file);
        frame->ty = VFile_ReadS16(file);
        frame->tz = VFile_ReadS16(file);
        frame->cx = VFile_ReadS16(file);
        frame->cy = VFile_ReadS16(file);
        frame->cz = VFile_ReadS16(file);
        frame->fov = VFile_ReadS16(file);
        frame->roll = VFile_ReadS16(file);
    }
    g_CineLoaded = true;

finish:
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadDemo(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_DemoCount = 0;

    // TODO: is the allocation necessary if there's no demo data?
    // TODO: do not hardcode the allocation size
    g_DemoPtr = game_malloc(36000, GBUF_LOAD_DEMO_BUFFER);

    const int32_t demo_size = VFile_ReadU16(file);
    LOG_DEBUG("demo input size: %d", demo_size);
    if (!demo_size) {
        g_IsDemoLoaded = false;
    } else {
        g_IsDemoLoaded = true;
        VFile_Read(file, g_DemoPtr, demo_size);
    }

    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadDemoExternal(const char *const level_name)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    char file_name[MAX_PATH];
    strcpy(file_name, level_name);
    ChangeFileNameExtension(file_name, "DEM");

    HANDLE handle = CreateFileA(
        file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }

    // TODO: do not hardcode the allocation size
    DWORD bytes_read;
    ReadFileSync(handle, g_DemoPtr, 36000, &bytes_read, 0);
    g_IsDemoLoaded = bytes_read != 0;
    CloseHandle(handle);
    Benchmark_End(benchmark, NULL);
}

static void __cdecl M_LoadSamples(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    int32_t *sample_offsets = NULL;

    g_SoundIsActive = false;
    if (!S_Audio_Sample_IsEnabled()) {
        goto finish;
    }

    S_Audio_Sample_CloseAllTracks();

    VFile_Read(file, g_SampleLUT, sizeof(int16_t) * SFX_NUMBER_OF);
    g_NumSampleInfos = VFile_ReadS32(file);
    LOG_DEBUG("sample infos: %d", g_NumSampleInfos);
    if (!g_NumSampleInfos) {
        goto finish;
    }

    g_SampleInfos =
        game_malloc(sizeof(SAMPLE_INFO) * g_NumSampleInfos, GBUF_SAMPLE_INFOS);
    for (int32_t i = 0; i < g_NumSampleInfos; i++) {
        SAMPLE_INFO *const sample_info = &g_SampleInfos[i];
        sample_info->number = VFile_ReadS16(file);
        sample_info->volume = VFile_ReadS16(file);
        sample_info->randomness = VFile_ReadS16(file);
        sample_info->flags = VFile_ReadS16(file);
    }

    const int32_t num_samples = VFile_ReadS32(file);
    LOG_DEBUG("samples: %d", num_samples);
    if (!num_samples) {
        goto finish;
    }

    sample_offsets = Memory_Alloc(sizeof(int32_t) * num_samples);
    VFile_Read(file, sample_offsets, sizeof(int32_t) * num_samples);

    const char *const file_name = "data\\main.sfx";
    const char *const full_path = GetFullPath(file_name);
    LOG_DEBUG("Loading samples from %s", full_path);
    HANDLE sfx_handle = CreateFileA(
        full_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (sfx_handle == INVALID_HANDLE_VALUE) {
        Shell_ExitSystemFmt(
            "Could not open %s file: 0x%x", file_name, GetLastError());
        goto finish;
    }

    // TODO: refactor these WAVE/RIFF shenanigans
    int32_t sample_id = 0;
    for (int32_t i = 0; sample_id < num_samples; i++) {
        char header[0x2C];
        ReadFileSync(sfx_handle, header, 0x2C, NULL, NULL);
        if (*(int32_t *)(header + 0) != 0x46464952
            || *(int32_t *)(header + 8) != 0x45564157
            || *(int32_t *)(header + 36) != 0x61746164) {
            LOG_ERROR("Unexpected sample header for sample %d", i);
            goto finish;
        }
        const int32_t data_size = *(int32_t *)(header + 0x28);
        const int32_t aligned_size = (data_size + 1) & ~1;

        *(int16_t *)(header + 16) = 0;
        if (sample_offsets[sample_id] != i) {
            SetFilePointer(sfx_handle, aligned_size, NULL, FILE_CURRENT);
            continue;
        }

        char *sample_data = Memory_Alloc(aligned_size);
        ReadFileSync(sfx_handle, sample_data, aligned_size, NULL, NULL);
        // TODO: do not reconstruct the header in S_Audio_Sample_Load, just
        // pass the entire sample directly
        const bool result = S_Audio_Sample_Load(
            sample_id, (LPWAVEFORMATEX)(header + 20), sample_data, data_size);
        Memory_FreePointer(&sample_data);
        if (!result) {
            goto finish;
        }

        sample_id++;
    }
    CloseHandle(sfx_handle);

    g_SoundIsActive = true;

finish:
    Memory_FreePointer(&sample_offsets);
    Benchmark_End(benchmark, NULL);
}

bool __cdecl Level_Load(const char *const file_name, const int32_t level_num)
{
    LOG_DEBUG("%s (num=%d)", g_GF_LevelNames[level_num], level_num);
    init_game_malloc();

    BENCHMARK *const benchmark = Benchmark_Start();
    bool result = false;

    const char *full_path = GetFullPath(file_name);
    strcpy(g_LevelFileName, full_path);

    VFILE *file = VFile_CreateFromPath(full_path);

    const int32_t version = VFile_ReadS32(file);
    if (version > 45) {
        Shell_ExitSystemFmt(
            "FATAL: Level %d (%s) requires a new TOMB2.EXE (version %d) to run",
            level_num, full_path, file_name);
        return false;
    }

    if (version < 45) {
        Shell_ExitSystemFmt(
            "FATAL: Level %d (%s) is OUT OF DATE (version %d). COPY NEW "
            "EDITORS AND REMAKE LEVEL",
            level_num, full_path, file_name);
        return false;
    }

    g_LevelFilePalettesOffset = VFile_GetPos(file);
    M_LoadPalettes(file);

    g_LevelFileTexPagesOffset = VFile_GetPos(file);
    M_LoadTexturePages(file);
    VFile_Skip(file, 4);

    M_LoadRooms(file);

    M_LoadMeshBase(file);
    M_LoadMeshes(file);

    int32_t *frame_pointers = NULL;
    const int32_t num_anims = M_LoadAnims(file, &frame_pointers);
    M_LoadAnimChanges(file);
    M_LoadAnimRanges(file);
    M_LoadAnimCommands(file);
    M_LoadAnimBones(file);
    M_LoadAnimFrames(file);

    for (int32_t i = 0; i < num_anims; i++) {
        ANIM *const anim = &g_Anims[i];
        // TODO: this is horrible
        anim->frame_ptr = ((int16_t *)g_AnimFrames) + frame_pointers[i] / 2;
    }
    Memory_FreePointer(&frame_pointers);

    M_LoadObjects(file);
    InitialiseObjects();
    M_LoadStaticObjects(file);
    M_LoadTextures(file);

    M_LoadSprites(file);
    M_LoadCameras(file);
    M_LoadSoundEffects(file);
    M_LoadBoxes(file);
    M_LoadAnimatedTextures(file);
    M_LoadItems(file);

    g_LevelFileDepthQOffset = VFile_GetPos(file);
    M_LoadDepthQ(file);
    M_LoadCinematic(file);
    M_LoadDemo(file);
    M_LoadSamples(file);

    M_LoadDemoExternal(full_path);
    VFile_Close(file);
    file = NULL;

    Benchmark_End(benchmark, NULL);
    return true;
}
