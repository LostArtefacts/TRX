#include "game/level/common.h"

#include "benchmark.h"
#include "config.h"
#include "debug.h"
#include "game/anims.h"
#include "game/camera.h"
#include "game/const.h"
#include "game/demo.h"
#include "game/effects.h"
#include "game/effects/const.h"
#include "game/game.h"
#include "game/game_buf.h"
#include "game/game_string_table.h"
#include "game/gym.h"
#include "game/inject.h"
#include "game/lara.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/objects/setup.h"
#include "game/objects/vars.h"
#include "game/option.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/pathing.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "game/viewport.h"
#include "log.h"
#include "memory.h"
#include "thread_pool.h"
#include "utils.h"
#include "vector.h"

#include <SDL2/SDL_cpuinfo.h>
#include <string.h>

#define M_NO_ROOM_LEGACY 255

static LEVEL_INFO m_Info = {};

static size_t M_GetObjectCount(const LEVEL_LOADER *const loader)
{
    switch (loader->game_version) {
    case 1:
        return 191;
    case 2:
        return 265;
    default:
        ASSERT_FAIL();
    }
    return 0;
}

static size_t M_GetSampleCount(const LEVEL_LOADER *const loader)
{
    switch (loader->game_version) {
    case 1:
        return 256;
    case 2:
        return 370;
    default:
        ASSERT_FAIL();
    }
    return 0;
}

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

static void M_FixTrapezoidRatios(FACE4 *const face, const XYZ_16 vertices[4])
{
    // This function attempts to correct texture coordinate ratios for a
    // quadrilateral so the GPU, which typically renders a quad as two
    // triangles, does not warp the texture disproportionately across the quad's
    // diagonal divisions. By comparing the 3D edge lengths (in world space)
    // with the corresponding 2D UV edge lengths, it computes a scale factor
    // that preserves the trapezoidal shape in texture space and allows the
    // shader to warp the texture uniformly across all four corners.
    //
    // In many software rasterization or older GPU pipelines, triangles can get
    // rendered using affine interpolation of texture coordinates, causing
    // visible warping when a four-sided polygon is split internally.
    // The original approach (coded by XProger) handled only rectangular UV
    // maps by simply scaling the edges; this updated version takes into
    // account the actual UV trapezoid to achieve a more uniform texture
    // projection.

    // 1) Gather the coordinate and texture information
    const OBJECT_TEXTURE *const tex =
        Output_GetObjectTexture(face->texture_idx);
    const TEXTURE_UV *uvs[4] = {
        &tex->uv[0],
        &tex->uv[1],
        &tex->uv[2],
        &tex->uv[3],
    };
    TEXTURE_ZW_F *zws[4] = {
        &face->texture_zw[0],
        &face->texture_zw[1],
        &face->texture_zw[2],
        &face->texture_zw[3],
    };
    XYZ_F c0, c1, c2, c3, *coords[4] = { &c0, &c1, &c2, &c3 };
    for (int32_t i = 0; i < 4; i++) {
        coords[i]->x = vertices[i].x;
        coords[i]->y = vertices[i].y;
        coords[i]->z = vertices[i].z;
    }

    // 2) Compute geometric edges
    //    a = c0-c1, b = c3-c2, c = c0-c3, d = c1-c2
    const XYZ_F a = XYZ_F_Subtract(c0, c1);
    const XYZ_F b = XYZ_F_Subtract(c3, c2);
    const XYZ_F c = XYZ_F_Subtract(c0, c3);
    const XYZ_F d = XYZ_F_Subtract(c1, c2);

    const float a_l = XYZ_F_Length(a);
    const float b_l = XYZ_F_Length(b);
    const float c_l = XYZ_F_Length(c);
    const float d_l = XYZ_F_Length(d);

    // 3) Compute dot‐products in 3D to see which edges differ more
    const float ab = XYZ_F_DotProduct(a, b) / (a_l * b_l);
    const float cd = XYZ_F_DotProduct(c, d) / (c_l * d_l);

    // 4) Compute tx, ty in for orientation
    const float tx = ABS(uvs[0]->u - uvs[3]->u);
    const float ty = ABS(uvs[0]->v - uvs[3]->v);

    // 5) Measure the same edges in UV space so we know the “current” shape
    XYZ_F uv0 = { uvs[0]->u, uvs[0]->v, 0.0f };
    XYZ_F uv1 = { uvs[1]->u, uvs[1]->v, 0.0f };
    XYZ_F uv2 = { uvs[2]->u, uvs[2]->v, 0.0f };
    XYZ_F uv3 = { uvs[3]->u, uvs[3]->v, 0.0f };

    // au = uv0 - uv1, bu = uv3 - uv2, cu = uv0 - uv3, du = uv1 - uv2
    const XYZ_F au = XYZ_F_Subtract(uv0, uv1);
    const XYZ_F bu = XYZ_F_Subtract(uv3, uv2);
    const XYZ_F cu = XYZ_F_Subtract(uv0, uv3);
    const XYZ_F du = XYZ_F_Subtract(uv1, uv2);

    const float au_l = XYZ_F_Length(au);
    const float bu_l = XYZ_F_Length(bu);
    const float cu_l = XYZ_F_Length(cu);
    const float du_l = XYZ_F_Length(du);

    // We'll reuse the same ab/cd dot logic in UV if needed, but typically
    // we only need the lengths to find the ratio vs. geometry.

    // 6) Figure out the correction ratios per-corner, taking care of both
    //    geometry and UV mesh proportions
    if (ab > cd) {
        const int k = (tx > ty) ? 1 : 0; // pick axis
        if (a_l > b_l) {
            // geometry ratio = (b_l / a_l)
            // uv ratio       = (bu_l / au_l)  (avoid /0 check if needed)
            const float geom_ratio = (a_l > 1e-6f) ? (b_l / a_l) : 1.0f;
            const float uv_ratio = (au_l > 1e-6f) ? (bu_l / au_l) : 1.0f;
            const float fix = geom_ratio / uv_ratio; // final scale
            zws[2]->zw[k] = fix;
            zws[3]->zw[k] = fix;
        } else if (a_l < b_l) {
            const float geom_ratio = (b_l > 1e-6f) ? (a_l / b_l) : 1.0f;
            const float uv_ratio = (bu_l > 1e-6f) ? (au_l / bu_l) : 1.0f;
            const float fix = geom_ratio / uv_ratio;
            zws[0]->zw[k] = fix;
            zws[1]->zw[k] = fix;
        }
    } else if (ab < cd) {
        const int k = (tx > ty) ? 0 : 1; // pick axis
        if (c_l > d_l) {
            const float geom_ratio = (c_l > 1e-6f) ? (d_l / c_l) : 1.0f;
            const float uv_ratio = (cu_l > 1e-6f) ? (du_l / cu_l) : 1.0f;
            const float fix = geom_ratio / uv_ratio;
            zws[1]->zw[k] = fix;
            zws[2]->zw[k] = fix;
        } else if (c_l < d_l) {
            const float geom_ratio = (d_l > 1e-6f) ? (c_l / d_l) : 1.0f;
            const float uv_ratio = (du_l > 1e-6f) ? (cu_l / du_l) : 1.0f;
            const float fix = geom_ratio / uv_ratio;
            zws[0]->zw[k] = fix;
            zws[3]->zw[k] = fix;
        }
    }
}

static void M_PremultiplyTexturePage(void *userdata)
{
    const int32_t page = *(int32_t *)userdata;
    Output_LockTexturePage32(page);
    RGBA_8888 *ptr = Output_GetTexturePage32(page);
    const float inv255 = 1.0f / 255.0f;
    for (int32_t i = 0; i < TEXTURE_PAGE_SIZE; i++, ptr++) {
        ptr->r *= ptr->a * inv255;
        ptr->g *= ptr->a * inv255;
        ptr->b *= ptr->a * inv255;
    }
    Output_UnlockTexturePage32(page);
}

static void M_ReadPosition(XYZ_32 *const pos, VFILE *const file)
{
    pos->x = VFile_ReadS32(file);
    pos->y = VFile_ReadS32(file);
    pos->z = VFile_ReadS32(file);
}

static void M_ReadShade(
    const LEVEL_LOADER *const loader, SHADE *const shade, VFILE *const file)
{
    shade->value_1 = VFile_ReadS16(file);
    if (loader->game_version == 1) {
        shade->value_2 = shade->value_1;
    } else {
        shade->value_2 = VFile_ReadS16(file);
    }
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
        face->texture_zw[i].z = 1.0f;
        face->texture_zw[i].w = 1.0f;
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

static void M_ReadRoomMesh(
    const LEVEL_LOADER *const loader, const int32_t room_num, VFILE *const file,
    const INJECTION_MESH_META inj_data)
{
    ROOM *const room = Room_Get(room_num);
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
            if (loader->game_version == 1) {
                vertex->flags = 0;
                vertex->light_adder = vertex->light_base;
            } else {
                vertex->light_table_value = VFile_ReadU8(file);
                vertex->flags = VFile_ReadU8(file);
                vertex->light_adder = VFile_ReadS16(file);
            }
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
            room->mesh.num_sprites + inj_data.num_static_2ds;
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
    mesh->depth_adjustment = 0.005;

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

void Level_ReadPalettes(const LEVEL_LOADER *const loader, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();

    const int32_t palette_size = 256;
    m_Info.palette.size = palette_size;

    m_Info.palette.data_24 = Memory_Alloc(sizeof(RGB_888) * palette_size);
    VFile_Read(file, m_Info.palette.data_24, sizeof(RGB_888) * palette_size);
    m_Info.palette.data_24[0].r = 0;
    m_Info.palette.data_24[0].g = 0;
    m_Info.palette.data_24[0].b = 0;
    for (int32_t i = 1; i < palette_size; i++) {
        RGB_888 *const col = &m_Info.palette.data_24[i];
        col->r = (col->r << 2) | (col->r >> 4);
        col->g = (col->g << 2) | (col->g >> 4);
        col->b = (col->b << 2) | (col->b >> 4);
    }

    if (loader->game_version == 1) {
        m_Info.palette.data_32 = nullptr;
    } else {
        RGBA_8888 palette_16[palette_size];
        m_Info.palette.data_32 = Memory_Alloc(sizeof(RGB_888) * palette_size);
        VFile_Read(file, palette_16, sizeof(RGBA_8888) * palette_size);
        for (int32_t i = 0; i < palette_size; i++) {
            m_Info.palette.data_32[i].r = palette_16[i].r;
            m_Info.palette.data_32[i].g = palette_16[i].g;
            m_Info.palette.data_32[i].b = palette_16[i].b;
        }
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadTexturePages(const LEVEL_LOADER *const loader, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();

    const int32_t num_pages = VFile_ReadS32(file);
    m_Info.textures.page_count = num_pages;
    LOG_INFO("texture pages: %d", num_pages);

    const int32_t extra_pages = Inject_GetDataCount(IDT_TEXTURE_PAGES);
    const int32_t texture_size_8_bit =
        (num_pages + extra_pages) * TEXTURE_PAGE_SIZE * sizeof(uint8_t);
    const int32_t texture_size_32_bit =
        (num_pages + extra_pages) * TEXTURE_PAGE_SIZE * sizeof(RGBA_8888);

    m_Info.textures.pages_24 = Memory_Alloc(texture_size_8_bit);
    VFile_Read(file, m_Info.textures.pages_24, num_pages * TEXTURE_PAGE_SIZE);

    m_Info.textures.pages_32 = Memory_Alloc(texture_size_32_bit);
    RGBA_8888 *output = m_Info.textures.pages_32;

    if (loader->game_version == 1) {
        const uint8_t *input = m_Info.textures.pages_24;
        for (int32_t i = 0; i < num_pages * TEXTURE_PAGE_SIZE; i++) {
            const uint8_t index = *input++;
            const RGB_888 pix = m_Info.palette.data_24[index];
            output->r = pix.r;
            output->g = pix.g;
            output->b = pix.b;
            output->a = index == 0 ? 0 : 0xFF;
            output++;
        }
    } else {
        const int32_t texture_size_16_bit =
            num_pages * TEXTURE_PAGE_SIZE * sizeof(uint16_t);
        uint16_t *input = Memory_Alloc(texture_size_16_bit);
        uint16_t *input_ptr = input;
        VFile_Read(file, input, texture_size_16_bit);
        for (int32_t i = 0; i < num_pages * TEXTURE_PAGE_SIZE; i++) {
            *output++ = M_ARGB1555To8888(*input_ptr++);
        }
        Memory_FreePointer(&input);
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadRooms(const LEVEL_LOADER *const loader, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();

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

        const INJECTION_MESH_META inj_data = Inject_GetRoomMeshMeta(i);
        M_ReadRoomMesh(loader, i, file, inj_data);

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
            if (sector->portal_room.pit == M_NO_ROOM_LEGACY) {
                sector->portal_room.pit = NO_ROOM;
            }
            if (sector->portal_room.sky == M_NO_ROOM_LEGACY) {
                sector->portal_room.sky = NO_ROOM;
            }
        }

        room->ambient = VFile_ReadS16(file);
        if (loader->game_version == 1) {
            room->light_mode = RLM_NORMAL;
        } else {
            VFile_Skip(file, sizeof(int16_t)); // Unused second ambient
            room->light_mode = VFile_ReadS16(file);
        }

        room->num_lights = VFile_ReadS16(file);
        room->lights = room->num_lights == 0
            ? nullptr
            : GameBuf_Alloc(sizeof(LIGHT) * room->num_lights, GBUF_ROOM_LIGHTS);
        for (int32_t i = 0; i < room->num_lights; i++) {
            LIGHT *const light = &room->lights[i];
            M_ReadPosition(&light->pos, file);
            M_ReadShade(loader, &light->shade, file);
            light->falloff.value_1 = VFile_ReadS32(file);
            if (loader->game_version == 2) {
                light->falloff.value_2 = VFile_ReadS32(file);
            }
        }

        room->num_static_meshes = VFile_ReadS16(file);
        const int32_t static_count =
            room->num_static_meshes + inj_data.num_static_3ds;
        room->static_meshes = static_count == 0
            ? nullptr
            : GameBuf_Alloc(
                  sizeof(STATIC_MESH) * static_count, GBUF_ROOM_STATIC_MESHES);
        for (int32_t i = 0; i < room->num_static_meshes; i++) {
            STATIC_MESH *const mesh = &room->static_meshes[i];
            M_ReadPosition(&mesh->pos, file);
            mesh->rot.y = VFile_ReadS16(file);
            M_ReadShade(loader, &mesh->shade, file);
            mesh->static_num = VFile_ReadS16(file);
        }

        room->flipped_room = VFile_ReadS16(file);
        room->flags = VFile_ReadU16(file);

        room->bound_active = 0;
        room->bound_left = Viewport_GetMaxX(VIEWPORT_GAME);
        room->bound_top = Viewport_GetMaxY(VIEWPORT_GAME);
        room->bound_bottom = Viewport_GetMinY(VIEWPORT_GAME);
        room->bound_right = Viewport_GetMinX(VIEWPORT_GAME);
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
    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadObjectMeshes(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_meshes = VFile_ReadS32(file);
    LOG_INFO("object mesh data: %d", num_meshes);

    const size_t data_start_pos = VFile_GetPos(file);
    VFile_Skip(file, num_meshes * sizeof(int16_t));

    m_Info.mesh_ptr_count = VFile_ReadS32(file);
    LOG_INFO("object mesh offsets: %d", m_Info.mesh_ptr_count);
    const int32_t alloc_size = m_Info.mesh_ptr_count * sizeof(int32_t);
    int32_t *mesh_offsets = Memory_Alloc(alloc_size);
    VFile_Read(file, mesh_offsets, alloc_size);

    const size_t end_pos = VFile_GetPos(file);
    VFile_SetPos(file, data_start_pos);

    Object_InitialiseMeshes(
        m_Info.mesh_ptr_count + Inject_GetDataCount(IDT_MESH_POINTERS));
    Level_AppendObjectMeshes(m_Info.mesh_ptr_count, mesh_offsets, file);

    VFile_SetPos(file, end_pos);
    Memory_FreePointer(&mesh_offsets);

    Benchmark_End(&benchmark, nullptr);
}

void Level_AppendObjectMeshes(
    const int32_t num_offsets, const int32_t *const offsets, VFILE *const file)
{
#define L_ALIGN 2

    // Savegames identify meshes by their file pointer values divided by 2.
    // (Historically, meshes were stored in int16_t[] arrays, so the so-called
    // "pointers" are really just array indices into that layout.)
    //
    // Original level meshes work fine under this scheme, but injected meshes
    // are different, as they come from separate VFiles and bring their own
    // pointer values. To prevent conflicts, calling Level_AppendObjectMeshes()
    // for injected content must assign unique pseudo-pointers.
    //
    // Rules for injected meshes:
    // - Pointers do not need to match real file offsets.
    // - They only need to be unique and preserve ordering.
    //
    // Only the original level data requires true offset congruence so that old
    // savegames remain compatible. For everything else, simple linear indexing
    // is sufficient.
    int32_t base_index = 0;
    if (Object_GetMeshCount() > 0) {
        // NOTE(Dash): Not assuming offsets are strictly increasing, so we scan
        // all meshes and pick the max.
        for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
            base_index =
                MAX(base_index, Object_GetMeshOffset(Object_GetMesh(i)));
        }
        base_index += L_ALIGN;
    }

    // Construct and store distinct meshes only e.g. Lara's hips are referenced
    // by several pointers as a dummy mesh.
    VECTOR *const unique_offsets =
        Vector_CreateAtCapacity(sizeof(int32_t), num_offsets);
    int32_t pointer_map[num_offsets];
    for (int32_t i = 0; i < num_offsets; i++) {
        const int32_t pointer = offsets[i] + base_index;
        const int32_t index = Vector_IndexOf(unique_offsets, (void *)&pointer);
        if (index == -1) {
            pointer_map[i] = unique_offsets->count;
            Vector_Add(unique_offsets, (void *)&pointer);
        } else {
            pointer_map[i] = index;
        }
    }

    OBJECT_MESH *const meshes =
        GameBuf_Alloc(sizeof(OBJECT_MESH) * unique_offsets->count, GBUF_MESHES);
    size_t start_pos = VFile_GetPos(file);
    for (int i = 0; i < unique_offsets->count; i++) {
        const int32_t pointer = *(const int32_t *)Vector_Get(unique_offsets, i);
        VFile_SetPos(file, start_pos + pointer - base_index);
        M_ReadObjectMesh(&meshes[i], file);

        // The original data position is required for backward compatibility
        // with savegame files, specifically for Lara's mesh pointers.
        Object_SetMeshOffset(&meshes[i], pointer / L_ALIGN);
    }

    for (int32_t i = 0; i < num_offsets; i++) {
        Object_StoreMesh(&meshes[pointer_map[i]]);
    }
#undef L_ALIGN

    LOG_INFO("%d unique meshes constructed", unique_offsets->count);

    Vector_Free(unique_offsets);
}

void Level_ReadAnims(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_anims = VFile_ReadS32(file);
    m_Info.anims.anim_count = num_anims;
    LOG_INFO("anims: %d", num_anims);
    Anim_InitialiseAnims(num_anims + Inject_GetDataCount(IDT_ANIMS));
    Level_AppendAnims(0, num_anims, file);
    Benchmark_End(&benchmark, nullptr);
}

void Level_AppendAnims(
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

void Level_ReadAnimChanges(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_anim_changes = VFile_ReadS32(file);
    m_Info.anims.change_count = num_anim_changes;
    LOG_INFO("anim changes: %d", num_anim_changes);
    Anim_InitialiseChanges(
        num_anim_changes + Inject_GetDataCount(IDT_ANIM_CHANGES));
    Level_AppendAnimChanges(0, num_anim_changes, file);
    Benchmark_End(&benchmark, nullptr);
}

void Level_AppendAnimChanges(
    const int32_t base_idx, const int32_t num_changes, VFILE *const file)
{
    for (int32_t i = 0; i < num_changes; i++) {
        ANIM_CHANGE *const anim_change = Anim_GetChange(base_idx + i);
        anim_change->goal_anim_state = VFile_ReadS16(file);
        anim_change->num_ranges = VFile_ReadS16(file);
        anim_change->range_idx = VFile_ReadS16(file);
    }
}

void Level_ReadAnimRanges(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_anim_ranges = VFile_ReadS32(file);
    m_Info.anims.range_count = num_anim_ranges;
    LOG_INFO("anim ranges: %d", num_anim_ranges);
    Anim_InitialiseRanges(
        num_anim_ranges + Inject_GetDataCount(IDT_ANIM_RANGES));
    Level_AppendAnimRanges(0, num_anim_ranges, file);
    Benchmark_End(&benchmark, nullptr);
}

void Level_AppendAnimRanges(
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

void Level_ReadAnimCommands(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_commands = VFile_ReadS32(file);
    m_Info.anims.command_count = num_commands;
    LOG_INFO("anim commands: %d", num_commands);
    m_Info.anims.commands = Memory_Alloc(
        sizeof(int16_t)
        * (num_commands + Inject_GetDataCount(IDT_ANIM_COMMANDS)));
    Level_AppendAnimCommands(0, num_commands, file);
    Benchmark_End(&benchmark, nullptr);
}

void Level_AppendAnimCommands(
    const int32_t base_idx, const int32_t num_commands, VFILE *const file)
{
    VFile_Read(
        file, &m_Info.anims.commands[base_idx], sizeof(int16_t) * num_commands);
}

void Level_LoadAnimCommands(void)
{
    Anim_LoadCommands(m_Info.anims.commands);
    Memory_FreePointer(&m_Info.anims.commands);
}

void Level_ReadAnimBones(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_anim_bones = VFile_ReadS32(file) / ANIM_BONE_SIZE;
    m_Info.anims.bone_count = num_anim_bones;
    LOG_INFO("anim bones: %d", num_anim_bones);
    Anim_InitialiseBones(num_anim_bones + Inject_GetDataCount(IDT_ANIM_BONES));
    Level_AppendAnimBones(0, num_anim_bones, file);
    Benchmark_End(&benchmark, nullptr);
}

void Level_AppendAnimBones(
    const int32_t base_idx, const int32_t num_bones, VFILE *const file)
{
    for (int32_t i = 0; i < num_bones; i++) {
        ANIM_BONE *const bone = Anim_GetBone(base_idx + i);
        const int32_t flags = VFile_ReadS32(file);
        bone->matrix_pop = (flags & 1) != 0;
        bone->matrix_push = (flags & 2) != 0;
        bone->rot.x = false;
        bone->rot.y = false;
        bone->rot.z = false;
        M_ReadPosition(&bone->pos, file);
    }
}

void Level_ReadAnimFrames(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t raw_data_count = VFile_ReadS32(file);
    m_Info.anims.frame_count = raw_data_count;
    LOG_INFO("raw anim frames: %d", raw_data_count);
    m_Info.anims.frames = Memory_Alloc(
        sizeof(int16_t)
        * (raw_data_count + Inject_GetDataCount(IDT_ANIM_FRAMES)));
    Level_AppendAnimFrames(0, raw_data_count, file);
    Benchmark_End(&benchmark, nullptr);
}

void Level_AppendAnimFrames(
    const int32_t base_idx, const int32_t num_frames, VFILE *const file)
{
    VFile_Read(
        file, &m_Info.anims.frames[base_idx], sizeof(int16_t) * num_frames);
}

void Level_LoadAnimFrames(const LEVEL_LOADER *const loader)
{
    const int32_t frame_count =
        Anim_GetTotalFrameCount(loader, m_Info.anims.frame_count);
    Anim_InitialiseFrames(frame_count);
    Anim_LoadFrames(loader, m_Info.anims.frames, m_Info.anims.frame_count);
    Memory_FreePointer(&m_Info.anims.frames);
}

void Level_ReadObjects(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_objects = VFile_ReadS32(file);
    LOG_INFO("objects: %d", num_objects);
    for (int32_t i = 0; i < num_objects; i++) {
        const int32_t game_obj_id = VFile_ReadS32(file);
        OBJECT *const obj = Object_GetByGameID(game_obj_id);
        if (obj == nullptr) {
            Shell_ExitSystemFmt("Invalid object ID: %d", game_obj_id);
        }
        obj->mesh_count = VFile_ReadS16(file);
        obj->mesh_idx = VFile_ReadS16(file);
        obj->bone_idx = VFile_ReadS32(file) / ANIM_BONE_SIZE;
        obj->frame_ofs = VFile_ReadU32(file);
        obj->frame_base = nullptr;
        obj->anim_idx = VFile_ReadS16(file);
        obj->loaded = true;
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadStaticObjects(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_objects = VFile_ReadS32(file);
    LOG_INFO("static objects: %d", num_objects);
    for (int32_t i = 0; i < num_objects; i++) {
        const int32_t static_id = VFile_ReadS32(file);
        if (static_id < 0 || static_id >= MAX_STATIC_OBJECTS_3D) {
            Shell_ExitSystemFmt(
                "Invalid static ID: %d (max=%d)", static_id,
                MAX_STATIC_OBJECTS_3D - 1);
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

    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadObjectTextures(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_textures = VFile_ReadS32(file);
    m_Info.textures.object_count = num_textures;
    LOG_INFO("object textures: %d", num_textures);
    Output_InitialiseObjectTextures(
        num_textures + Inject_GetDataCount(IDT_OBJECT_TEXTURES));
    Level_AppendObjectTextures(0, 0, num_textures, file);
    Benchmark_End(&benchmark, nullptr);
}

void Level_AppendObjectTextures(
    const int32_t base_idx, const int16_t base_page_idx,
    const int32_t num_textures, VFILE *const file)
{
    for (int32_t i = 0; i < num_textures; i++) {
        OBJECT_TEXTURE *const texture = Output_GetObjectTexture(base_idx + i);
        texture->uv_count = 4; // Default to 4 vertices
        texture->draw_type = VFile_ReadU16(file);
        texture->tex_page = VFile_ReadU16(file) + base_page_idx;
        for (int32_t j = 0; j < 4; j++) {
            texture->uv[j].u = VFile_ReadU16(file);
            texture->uv[j].v = VFile_ReadU16(file);
        }
    }
}

void Level_ReadSpriteTextures(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_textures = VFile_ReadS32(file);
    m_Info.textures.sprite_count = num_textures;
    LOG_INFO("sprite textures: %d", num_textures);
    Output_InitialiseSpriteTextures(
        num_textures + Inject_GetDataCount(IDT_SPRITE_TEXTURES));
    Level_AppendSpriteTextures(0, 0, num_textures, file);

    Benchmark_End(&benchmark, nullptr);
}

void Level_AppendSpriteTextures(
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

void Level_ReadSpriteSequences(
    const LEVEL_LOADER *const loader, VFILE *const file)
{
    int32_t max_obj_id = -1;
    const int32_t object_count = M_GetObjectCount(loader);
    for (OBJECT_ID obj_id = O_FIRST; obj_id <= object_count; obj_id++) {
        max_obj_id = MAX(max_obj_id, Object_ToGameID(obj_id));
    }

    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_sequences = VFile_ReadS32(file);
    LOG_DEBUG("sprite sequences: %d", num_sequences);
    for (int32_t i = 0; i < num_sequences; i++) {
        const int32_t id = VFile_ReadS32(file);
        const int16_t num_meshes = VFile_ReadS16(file);
        const int16_t mesh_idx = VFile_ReadS16(file);

        if (id >= 0 && id < max_obj_id) {
            OBJECT *const obj = Object_GetByGameID(id);
            ASSERT(obj != nullptr);
            obj->mesh_count = num_meshes;
            obj->mesh_idx = mesh_idx;
            obj->anim_idx = NO_ANIM;
            obj->loaded = true;
        } else if (id - max_obj_id < MAX_STATIC_OBJECTS_2D) {
            STATIC_OBJECT_2D *const obj = Object_Get2DStatic(id - max_obj_id);
            obj->frame_count = ABS(num_meshes);
            obj->texture_idx = mesh_idx;
            obj->loaded = true;
        } else {
            Shell_ExitSystemFmt("Invalid sprite slot (%d)", id);
        }
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadPathingData(const LEVEL_LOADER *const loader, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_boxes = VFile_ReadS32(file);
    Box_InitialiseBoxes(num_boxes);
    for (int32_t i = 0; i < num_boxes; i++) {
        BOX_INFO *const box = Box_GetBox(i);
        if (loader->game_version == 1) {
            box->left = VFile_ReadS32(file);
            box->right = VFile_ReadS32(file);
            box->top = VFile_ReadS32(file);
            box->bottom = VFile_ReadS32(file);
        } else {
            box->left = VFile_ReadU8(file) << WALL_SHIFT;
            box->right = (VFile_ReadU8(file) << WALL_SHIFT) - 1;
            box->top = VFile_ReadU8(file) << WALL_SHIFT;
            box->bottom = (VFile_ReadU8(file) << WALL_SHIFT) - 1;
        }
        box->height = VFile_ReadS16(file);
        box->overlap_index = VFile_ReadS16(file);
    }

    const int32_t num_overlaps = VFile_ReadS32(file);
    int16_t *const overlaps = Box_InitialiseOverlaps(num_overlaps);
    VFile_Read(file, overlaps, sizeof(int16_t) * num_overlaps);

    for (int32_t flip_status = 0; flip_status < 2; flip_status++) {
        for (int32_t zone_idx = 0; zone_idx < Box_GetZoneCount(); zone_idx++) {
            int16_t *const ground_zone =
                Box_GetGroundZone(flip_status, zone_idx);
            VFile_Read(file, ground_zone, sizeof(int16_t) * num_boxes);
        }

        int16_t *const fly_zone = Box_GetFlyZone(flip_status);
        VFile_Read(file, fly_zone, sizeof(int16_t) * num_boxes);
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadAnimatedTextureRanges(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t data_size = VFile_ReadS32(file);
    const size_t end_position =
        VFile_GetPos(file) + data_size * sizeof(int16_t);

    const int16_t num_ranges = VFile_ReadS16(file);
    LOG_INFO("animated texture ranges: %d", num_ranges);
    Output_InitialiseAnimatedTextures(num_ranges);

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

    VFile_SetPos(file, end_position);
    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadLightMap(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
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

    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadCinematicFrames(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int16_t num_frames = VFile_ReadS16(file);
    const int32_t inj_frames = Inject_GetDataCount(IDT_CINEMATIC_FRAMES);
    LOG_INFO("cinematic frames: %d", num_frames);
    Camera_InitialiseCineFrames(MAX(num_frames, inj_frames));
    for (int32_t i = 0; i < num_frames; i++) {
        CINE_FRAME *const frame = Camera_GetCineFrame(i);
        M_ReadVertex(&frame->target.shift, file);
        M_ReadVertex(&frame->camera.shift, file);
        frame->fov = VFile_ReadS16(file);
        frame->roll = VFile_ReadS16(file);
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadCamerasAndSinks(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_objects = VFile_ReadS32(file);
    LOG_DEBUG("fixed cameras/sinks: %d", num_objects);
    Camera_InitialiseFixedObjects(num_objects);
    for (int32_t i = 0; i < num_objects; i++) {
        M_ReadObjectVector(Camera_GetFixedObject(i), file);
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadItems(const LEVEL_LOADER *const loader, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_items = VFile_ReadS32(file);
    LOG_INFO("items: %d", num_items);
    if (num_items > MAX_ITEMS) {
        Shell_ExitSystem("Too many items");
        goto finish;
    }

    Item_InitialiseItems(num_items);
    for (int32_t i = 0; i < num_items; i++) {
        ITEM *const item = Item_Get(i);
        const int16_t obj_id = VFile_ReadS16(file);
        item->object_id = Object_FromGameID(obj_id);
        if (item->object_id == NO_OBJECT) {
            Shell_ExitSystemFmt("Bad object number (%d) on item %d", obj_id, i);
            goto finish;
        }

        item->room_num = VFile_ReadS16(file);
        M_ReadPosition(&item->pos, file);
        item->rot.y = VFile_ReadS16(file);
        M_ReadShade(loader, &item->shade, file);
        item->flags = VFile_ReadS16(file);
    }

finish:
    Stats_ObserveItemsLoad();
    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadDemoData(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const uint16_t size = VFile_ReadU16(file);
    LOG_INFO("demo buffer size: %d", size);
    Demo_InitialiseData(size);
    if (size != 0) {
        uint32_t *const data = Demo_GetData();
        VFile_Read(file, data, size);
    }
    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadSoundSources(VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_sources = VFile_ReadS32(file);
    LOG_INFO("sound sources: %d", num_sources);
    Sound_InitialiseSources(num_sources);
    for (int32_t i = 0; i < num_sources; i++) {
        M_ReadObjectVector(Sound_GetSource(i), file);
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_ReadSamples(const LEVEL_LOADER *const loader, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();

    const int32_t sample_count = M_GetSampleCount(loader);
    int16_t *const sample_lut = Memory_Alloc(sizeof(int16_t) * sample_count);
    int16_t *const sample_lut_inv =
        Memory_Alloc(sizeof(int16_t) * sample_count);
    VFile_Read(file, sample_lut, sizeof(int16_t) * sample_count);
    for (int32_t i = 0; i < sample_count; i++) {
        if (sample_lut[i] != -1) {
            sample_lut_inv[sample_lut[i]] = i;
        }
    }

    const int32_t num_sample_infos = VFile_ReadS32(file);
    LOG_INFO("sample infos: %d", num_sample_infos);
    for (int32_t i = 0; i < num_sample_infos; i++) {
        SAMPLE_INFO *const sample_info =
            Sound_GetOrCreateSample(sample_lut_inv[i]);
        ASSERT(sample_info != nullptr);
        sample_info->number = VFile_ReadS16(file);
        sample_info->volume = VFile_ReadS16(file);
        sample_info->randomness = VFile_ReadS16(file);
        sample_info->flags.all = VFile_ReadU16(file);
        Sound_ReserveSampleData(
            sample_info->number, sample_info->flags.num_samples);
        if (loader->game_version == 1) {
            switch (sample_info->flags.mode_bits) {
            case 0:
                sample_info->mode = SAMPLE_MODE_WAIT;
                break;
            case 1:
                sample_info->mode = SAMPLE_MODE_RESTART;
                break;
            case 2:
                sample_info->mode = SAMPLE_MODE_LOOPED;
                break;
            case 3:
                LOG_WARNING(
                    "Unexpected sample mode for sample %d. flags=%0X", i,
                    sample_info->flags);
                break;
            }
        } else {
            switch (sample_info->flags.mode_bits) {
            case 0:
                sample_info->mode = SAMPLE_MODE_NORMAL;
                break;
            case 1:
                sample_info->mode = SAMPLE_MODE_WAIT;
                break;
            case 2:
                sample_info->mode = SAMPLE_MODE_RESTART;
                break;
            case 3:
                sample_info->mode = SAMPLE_MODE_LOOPED;
                break;
            }
        }
    }

    if (loader->game_version == 1) {
        const int32_t data_size = VFile_ReadS32(file);
        m_Info.samples.data_size = data_size;
        LOG_INFO("%d sample data size", data_size);

        m_Info.samples.data = GameBuf_Alloc(
            data_size + Inject_GetDataCount(IDT_SAMPLE_DATA), GBUF_SAMPLES);
        VFile_Read(file, m_Info.samples.data, sizeof(char) * data_size);
    }

    const int32_t num_offsets = VFile_ReadS32(file);
    LOG_INFO("samples: %d", num_offsets);
    m_Info.samples.offset_count = num_offsets;

    m_Info.samples.offsets = Memory_Alloc(
        sizeof(int32_t)
        * (num_offsets + Inject_GetDataCount(IDT_SAMPLE_INDICES)));
    VFile_Read(file, m_Info.samples.offsets, sizeof(int32_t) * num_offsets);

    Memory_Free(sample_lut);
    Memory_Free(sample_lut_inv);
    Benchmark_End(&benchmark, nullptr);
}

void Level_LoadTextures(void)
{
    for (int32_t room_num = 0; room_num < Room_GetCount(); room_num++) {
        const ROOM *const room = Room_Get(room_num);
        for (int32_t j = 0; j < room->mesh.num_face3s; j++) {
            const FACE3 *const face = &room->mesh.face3s[j];
            OBJECT_TEXTURE *const texture =
                Output_GetObjectTexture(face->texture_idx);
            texture->uv_count = 3;
        }
    }

    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        const OBJECT_MESH *const mesh = Object_GetMesh(i);
        for (int32_t j = 0; j < mesh->num_tex_face3s; j++) {
            const FACE3 *const face = &mesh->tex_face3s[j];
            OBJECT_TEXTURE *const texture =
                Output_GetObjectTexture(face->texture_idx);
            texture->uv_count = 3;
        }
    }

    for (int32_t room_num = 0; room_num < Room_GetCount(); room_num++) {
        ROOM *const room = Room_Get(room_num);
        for (int32_t j = 0; j < room->mesh.num_face4s; j++) {
            FACE4 *const face = &room->mesh.face4s[j];
            XYZ_16 vertices[4] = {
                room->mesh.vertices[face->vertices[0]].pos,
                room->mesh.vertices[face->vertices[1]].pos,
                room->mesh.vertices[face->vertices[2]].pos,
                room->mesh.vertices[face->vertices[3]].pos,
            };
            M_FixTrapezoidRatios(face, vertices);
        }
    }

    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        const OBJECT_MESH *const mesh = Object_GetMesh(i);
        for (int32_t j = 0; j < mesh->num_tex_face4s; j++) {
            FACE4 *const face = &mesh->tex_face4s[j];
            XYZ_16 vertices[4] = {
                mesh->vertices[face->vertices[0]],
                mesh->vertices[face->vertices[1]],
                mesh->vertices[face->vertices[2]],
                mesh->vertices[face->vertices[3]],
            };
            M_FixTrapezoidRatios(face, vertices);
        }
    }
}

void Level_LoadTexturePages(const LEVEL_LOADER *const loader)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_pages = m_Info.textures.page_count;
    Output_InitialiseTexturePages(num_pages, loader->game_version == 2);

    for (int32_t i = 0; i < num_pages; i++) {
        if (loader->game_version == 2) {
            uint8_t *const target_8 = Output_GetTexturePage8(i);
            const uint8_t *const source_8 =
                &m_Info.textures.pages_24[i * TEXTURE_PAGE_SIZE];
            memcpy(target_8, source_8, TEXTURE_PAGE_SIZE * sizeof(uint8_t));
        }

        const RGBA_8888 *const source_32 =
            &m_Info.textures.pages_32[i * TEXTURE_PAGE_SIZE];
        RGBA_8888 *const target_32 = Output_GetTexturePage32(i);
        memcpy(target_32, source_32, TEXTURE_PAGE_SIZE * sizeof(RGBA_8888));
    }
    Benchmark_End(&benchmark, "copied texture data");

    {
        int32_t *pages = Memory_Alloc(num_pages * sizeof(int32_t));
        const int num_threads = SDL_GetCPUCount();
        THREAD_POOL *const pool =
            ThreadPool_Create(num_threads > 0 ? num_threads : 1);
        for (int32_t i = 0; i < num_pages; i++) {
            pages[i] = i;
        }
        for (int32_t i = 0; i < num_pages; i++) {
            ThreadPool_AddJob(pool, M_PremultiplyTexturePage, &pages[i]);
        }
        ThreadPool_Wait(pool);
        ThreadPool_Destroy(pool);
        Memory_Free(pages);
    }
    Benchmark_End(&benchmark, "premultiplied alpha");

    Memory_FreePointer(&m_Info.textures.pages_24);
    Memory_FreePointer(&m_Info.textures.pages_32);
    Benchmark_End(&benchmark, nullptr);
}

void Level_LoadPalettes(void)
{
    Output_InitialisePalettes(
        m_Info.palette.size, m_Info.palette.data_24, m_Info.palette.data_32);
    Memory_FreePointer(&m_Info.palette.data_24);
    Memory_FreePointer(&m_Info.palette.data_32);
}

void Level_LoadObjectsAndItems(void)
{
    // Object and item setup/initialisation must take place after injections
    // have been processed. A cached item count must be used as individual
    // initialisations may increment the total item count.
    Object_SetupAllObjects();

    // Must take place after object setup but before item initialization.
    Level_LoadWalkables();

    const int32_t item_count = Item_GetLevelCount();
    for (int32_t i = 0; i < item_count; i++) {
        Item_Initialise(i);
    }

    Lara_State_Initialise();
}

void Level_LoadWalkables(void)
{
    for (int32_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->add_walkable_func != nullptr) {
            obj->add_walkable_func(item_num);
        }
    }
}

LEVEL_INFO *Level_GetInfo(void)
{
    return &m_Info;
}

void Level_Unload(void)
{
    Music_ResetTrackFlags();
    Sound_ResetSamples();

    Lara_InitialiseLoad(NO_ITEM);

#if TR_VERSION == 2
    Gym_ResetAssault();
    Creature_SetAlliesHostile(false);
#endif
    Object_Reset();
    Camera_Reset();
    Walkable_Reset();

#if TR_VERSION == 2
    Output_SetSunsetTimer(0);
#endif
    Output_DispatchLevelUnload();

    Music_SetVolume(g_Config.audio.music_volume);
    Sound_StopAll();
    Viewport_AlterFOV(-1);
}

bool Level_Initialise(
    const GF_LEVEL *const level, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    BENCHMARK benchmark = Benchmark_Start();
    LOG_DEBUG("num=%d (%s)", level->num, level->path);
    if (level->type == GFL_DEMO) {
        Random_SeedDraw(0xD371F947);
        Random_SeedControl(0xD371F947);
    }

    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    if (resume != nullptr) {
        resume->stats.timer = 0;
        resume->stats.secret_flags = 0;
        resume->stats.secret_count = 0;
#if TR_VERSION == 1
        resume->stats.pickup_count = 0;
#endif
        resume->stats.kill_count = 0;
        resume->stats.ammo_hits = 0;
        resume->stats.ammo_used = 0;
        resume->stats.medipacks_used = 0;
        resume->stats.distance_travelled = 0;
    }

    Game_SetIsLevelComplete(false);
    Game_FadeToBlack(-1);
    if (level->type != GFL_TITLE && level->type != GFL_DEMO) {
        Gym_SetInventoryOpenEnabled(false);
    }
    if (level->type != GFL_TITLE && level->type != GFL_CUTSCENE) {
        Game_SetCurrentLevel(level);
    }
    GF_SetCurrentLevel(level);

    if (level == nullptr) {
        return false;
    }

    Level_Unload();
    Level_Load(level);
    GameStringTable_Apply(level);

    Effect_InitialiseArray();
    LOT_InitialiseArray();

    Option_Reset();
    Overlay_Reset();
    Overlay_SetHealthBarTimer(100);

    Benchmark_End(&benchmark, nullptr);
    return true;
}
