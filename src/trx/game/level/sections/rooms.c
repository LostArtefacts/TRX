#include <trx/core/benchmark.h>
#include <trx/core/colors.h>
#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/const.h>
#include <trx/game/effects.h>
#include <trx/game/game_buf.h>
#include <trx/game/inject.h>
#include <trx/game/level/format/format.h>
#include <trx/game/level/sections/read.h>
#include <trx/game/pathing.h>
#include <trx/game/rooms.h>
#include <trx/game/shell.h>

#define M_NO_ROOM_LEGACY 255
#define M_NO_BOX_TR3_LEGACY 0x7FF
#define M_NO_HEIGHT_LEGACY (-32512)

static void M_ReadPosition(XYZ_32 *const pos, TRX_FILE *const file)
{
    pos->x = File_ReadS32(file);
    pos->y = File_ReadS32(file);
    pos->z = File_ReadS32(file);
}

static void M_ReadVertex(XYZ_16 *const vertex, TRX_FILE *const file)
{
    vertex->x = File_ReadS16(file);
    vertex->y = File_ReadS16(file);
    vertex->z = File_ReadS16(file);
}

static void M_ReadShade(
    const LEVEL_FORMAT_LOADER *const loader, SHADE *const shade,
    TRX_FILE *const file)
{
    shade->value_1 = File_ReadS16(file);
    if (loader->game_version == 1) {
        shade->value_2 = shade->value_1;
    } else {
        shade->value_2 = File_ReadS16(file);
    }
}

static LIGHT *M_InitialiseLegacyLight(LIGHT *const light)
{
    light->layout = LIGHT_LAYOUT_LEGACY;
    light->type = LIGHT_TYPE_POINT;
    light->color = COLOR_RGB_888_WHITE;
    light->u.legacy = (LIGHT_LEGACY_DATA) {};
    return light;
}

static RESULT M_ReadFace(
    FACE *const face, const size_t vertex_count, const int32_t num_vertices,
    TRX_FILE *const file)
{
    face->vertex_count = vertex_count;
    for (size_t i = 0; i < vertex_count; i++) {
        face->vertices[i] = File_ReadU16(file);
        FAIL_IF(
            face->vertices[i] >= num_vertices,
            "face names vertex %d of the %d the room holds", face->vertices[i],
            num_vertices);
        face->texture_zw[i].z = 1.0f;
        face->texture_zw[i].w = 1.0f;
    }
    const uint16_t texture_idx = File_ReadU16(file);
    face->texture_idx = texture_idx & 0x7FFF;
    face->double_sided = (texture_idx & 0x8000) != 0;
    face->effects = 0;
    face->enable_reflections = false;
    face->semi_transparent = false;
    return OK;
}

static void M_ReadRoomLightTR4(LIGHT *const light, TRX_FILE *const file)
{
    light->layout = LIGHT_LAYOUT_TR4;
    M_ReadPosition(&light->pos, file);
    light->color.r = File_ReadU8(file);
    light->color.g = File_ReadU8(file);
    light->color.b = File_ReadU8(file);
    light->type = (LIGHT_TYPE)File_ReadU8(file);
    // Signed on disk; the OG engine scales by |intensity| / 8191.
    light->u.tr4.intensity = (int16_t)File_ReadU16(file);
    File_ReadData(file, &light->u.tr4.inner_radius, sizeof(float));
    File_ReadData(file, &light->u.tr4.outer_radius, sizeof(float));
    File_ReadData(file, &light->u.tr4.length, sizeof(float));
    File_ReadData(file, &light->u.tr4.cutoff, sizeof(float));
    File_ReadData(file, &light->u.tr4.dir.x, sizeof(float));
    File_ReadData(file, &light->u.tr4.dir.y, sizeof(float));
    File_ReadData(file, &light->u.tr4.dir.z, sizeof(float));
}

// TR4 room vertex colors stay in the OG "128 = neutral" scale (5-bit
// channels expanded with << 3 like tomb4's ProcessRoomData); the shader's
// VERT_OVERBRIGHT path doubles them and turns the excess into an additive
// overbright term.
static RGBA_8888 M_ExpandTR4RoomVertexColor(const uint16_t color)
{
    return (RGBA_8888) {
        .r = ((color >> 10) & 0x1F) << 3,
        .g = ((color >> 5) & 0x1F) << 3,
        .b = (color & 0x1F) << 3,
        .a = 255,
    };
}

static RESULT M_ReadRoomMesh(
    const LEVEL_FORMAT_LOADER *const loader, const int32_t room_num,
    TRX_FILE *const file, const INJECTION_ROOM_META inj_data)
{
    ROOM *const room = Room_Get(room_num);
    const uint32_t mesh_length = File_ReadU32(file);
    const size_t start_pos = File_Pos(file);

    {
        room->mesh.num_vertices = File_ReadCountS16(file);
        const int32_t alloc_count =
            room->mesh.num_vertices + inj_data.num_vertices;
        room->mesh.vertices =
            GameBuf_Alloc(sizeof(ROOM_VERTEX) * alloc_count, GBUF_ROOM_MESH);
        for (int32_t i = 0; i < room->mesh.num_vertices; i++) {
            ROOM_VERTEX *const vertex = &room->mesh.vertices[i];
            M_ReadVertex(&vertex->pos, file);
            if (loader->game_version == 1) {
                vertex->light_base = File_ReadS16(file);
                vertex->flags.disable_wibble = false;
                vertex->flags.move = false;
                vertex->flags.glow = false;
                vertex->color = COLOR_RGBA_8888_WHITE;
            } else if (loader->game_version == 2) {
                vertex->light_base = File_ReadS16(file);
                vertex->light_table_value = File_ReadU8(file);
                const uint8_t flags = File_ReadU8(file);
                vertex->flags.disable_wibble = (flags & 0x80u) != 0u;
                vertex->flags.move = false;
                vertex->flags.glow = false;
                File_Skip(file, 2);
                vertex->color = COLOR_RGBA_8888_WHITE;
            } else if (loader->game_version == 3 || loader->game_version == 4) {
                File_Skip(file, 2); // lighting - unused
                const uint16_t flags = File_ReadU16(file);
                vertex->flags.disable_wibble = (flags & 0x8000u) != 0u;
                vertex->flags.move = (flags & 0x2000u) != 0u;
                vertex->flags.glow = (flags & 0x4000u) != 0u;
                const uint16_t raw_color = File_ReadU16(file);
                vertex->color = loader->game_version == 4
                    ? M_ExpandTR4RoomVertexColor(raw_color)
                    : Color_ARGB1555ToRGBA8888(raw_color);
                vertex->color.a = 255;
                vertex->light_base = 0;
            }
        }
    }

    {
        room->mesh.face4s.count = File_ReadCountS16(file);
        const size_t pos = File_Pos(file);
        File_Skip(file, 10 * room->mesh.face4s.count);
        room->mesh.face3s.count = File_ReadCountS16(file);
        File_Seek(file, pos, FILE_SEEK_SET);

        room->mesh.all_faces.count = room->mesh.face4s.count
            + inj_data.num_quads + room->mesh.face3s.count
            + inj_data.num_triangles;
        FACE *face_ptr = GameBuf_Alloc(
            sizeof(FACE) * room->mesh.all_faces.count, GBUF_ROOM_MESH);

        room->mesh.all_faces.data = face_ptr;
        room->mesh.face4s.data = face_ptr;
        for (int32_t i = 0; i < room->mesh.face4s.count; i++) {
            MUST(M_ReadFace(face_ptr++, 4, room->mesh.num_vertices, file));
        }
        for (int32_t i = 0; i < inj_data.num_quads; i++) {
            face_ptr->vertex_count = 4;
            face_ptr++;
        }

        File_Skip(file, 2);

        room->mesh.face3s.data = face_ptr;
        for (int32_t i = 0; i < room->mesh.face3s.count; i++) {
            MUST(M_ReadFace(face_ptr++, 3, room->mesh.num_vertices, file));
        }
        for (int32_t i = 0; i < inj_data.num_triangles; i++) {
            face_ptr->vertex_count = 3;
            face_ptr++;
        }
    }

    if (loader->layout != LEVEL_FORMAT_LAYOUT_TR4) {
        room->mesh.sprites.count = File_ReadCountS16(file);
        const int32_t alloc_count =
            room->mesh.sprites.count + inj_data.num_static_2ds;
        room->mesh.sprites.data =
            GameBuf_Alloc(sizeof(ROOM_SPRITE) * alloc_count, GBUF_ROOM_MESH);
        for (int32_t i = 0; i < room->mesh.sprites.count; i++) {
            ROOM_SPRITE *const sprite = &room->mesh.sprites.data[i];
            sprite->vertex = File_ReadU16(file);
            FAIL_IF(
                sprite->vertex >= room->mesh.num_vertices,
                "sprite names vertex %d of the %d the room holds",
                sprite->vertex, room->mesh.num_vertices);
            sprite->texture = File_ReadU16(file);
        }
    } else {
        room->mesh.sprites.count = File_ReadCountS16(file);
        room->mesh.sprites.data = nullptr;
    }

    const size_t total_read = (File_Pos(file) - start_pos) / sizeof(int16_t);
    FAIL_IF(
        total_read != mesh_length,
        "room mesh reads %zu words of the %u it declares", total_read,
        mesh_length);
    return OK;
}

static XYZ_16 M_ComputePortalNormal(PORTAL *const p)
{
    // This fixes a bug in TombEditor where certain portals would get emitted
    // with wrong normals. TE is guaranteed to emit normals with a good sign in
    // the Y component, but for sloped ceiling portals, their X and Z
    // compontents have the wrong sign.
    //
    // To fix this, we compute the normal the regular way. We don't know which
    // way the portal faces, but since the Y component is guaranteed to be
    // good, we can orient our vector using this information, which should fix
    // the X/Z components.

    ASSERT(p != nullptr);

    // Geometric normal (ab x ac)
    const XYZ_32 a = { p->vertex[0].x, p->vertex[0].y, p->vertex[0].z };
    const XYZ_32 b = { p->vertex[1].x, p->vertex[1].y, p->vertex[1].z };
    const XYZ_32 c = { p->vertex[2].x, p->vertex[2].y, p->vertex[2].z };
    const XYZ_32 ab = { b.x - a.x, b.y - a.y, b.z - a.z };
    const XYZ_32 ac = { c.x - a.x, c.y - a.y, c.z - a.z };
    XYZ_32 n = {
        (ab.y * ac.z) - (ab.z * ac.y),
        (ab.z * ac.x) - (ab.x * ac.z),
        (ab.x * ac.y) - (ab.y * ac.x),
    };

    // Degenerate guard
    if (n.x == 0 && n.y == 0 && n.z == 0) {
        return (XYZ_16) { .x = 0, .y = 1, .z = 0 };
    }

    // Integer normalization
    const int32_t gx = ABS(n.x);
    const int32_t gy = ABS(n.y);
    const int32_t gz = ABS(n.z);
    int32_t g = gx;
    if (gy != 0) {
        g = Math_GCD(g, gy);
    }
    if (gz != 0) {
        g = Math_GCD(g, gz);
    }
    if (g == 0) {
        g = 1;
    }
    n.x /= g;
    n.y /= g;
    n.z /= g;

    // NOTE: we only care about horizontal portals.
    if (p->normal.y == 0) {
        return p->normal;
    }
    if (p->normal.y != n.y) {
        n.x *= -1;
        n.y *= -1;
        n.z *= -1;
    }
    return (XYZ_16) { n.x, n.y, n.z };
}

RESULT Level_Section_ReadRooms(LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    RESULT result = OK;
    BENCHMARK benchmark = Benchmark_Start();
    const LEVEL_FORMAT_LOADER *const loader = ctx->loader;

    const int32_t num_rooms = File_ReadCountS16(file);
    LOG_INFO("rooms: %d", num_rooms);
    if (num_rooms > MAX_ROOMS) {
        result =
            FAIL("too many rooms: %d, at most %d fit", num_rooms, MAX_ROOMS);
        goto finish;
    }

    Room_InitialiseRooms(num_rooms);
    for (int32_t i = 0; i < num_rooms; i++) {
        ROOM *const room = Room_Get(i);

        room->pos.x = File_ReadS32(file);
        room->pos.y = 0;
        room->pos.z = File_ReadS32(file);

        room->min_floor = File_ReadS32(file);
        room->max_ceiling = File_ReadS32(file);

        const INJECTION_ROOM_META inj_data = Inject_GetRoomMeta(i);
        result = M_ReadRoomMesh(loader, i, file, inj_data);
        if (!IS_OK(result)) {
            result = Result_Prefix(result, "room %d", i);
            goto finish;
        }

        const int16_t num_portals = File_ReadCountS16(file);
        if (num_portals <= 0) {
            room->portals = nullptr;
        } else {
            room->portals = GameBuf_Alloc(
                sizeof(PORTAL) * num_portals + sizeof(PORTALS),
                GBUF_ROOM_PORTALS);
            room->portals->count = 0;
            for (int32_t j = 0; j < num_portals; j++) {
                PORTAL *const portal =
                    &room->portals->portal[room->portals->count];
                portal->room_num = File_ReadS16(file);
                M_ReadVertex(&portal->normal, file);
                for (int32_t k = 0; k < 4; k++) {
                    M_ReadVertex(&portal->vertex[k], file);
                }

                if (portal->room_num >= 0 && portal->room_num < num_rooms) {
                    room->portals->count++;
                } else {
                    LOG_WARNING(
                        "Ignoring invalid portal from room %d to %d", i,
                        portal->room_num);
                    *portal = (PORTAL) {};
                }
            }
        }

        room->size.z = File_ReadS16(file);
        room->size.x = File_ReadS16(file);

        const int32_t sector_count = room->size.x * room->size.z;
        room->sectors = GameBuf_Alloc(
            sizeof(SECTOR) * (sector_count + inj_data.num_sectors),
            GBUF_ROOM_SECTORS);
        for (int32_t j = 0; j < sector_count; j++) {
            SECTOR *const sector = &room->sectors[j];
            sector->idx = File_ReadU16(file);
            if (loader->game_version >= 3) {
                uint16_t misc_info = File_ReadU16(file);
                sector->fx = (uint8_t)(misc_info & 0x0F);
                sector->box = (int16_t)((misc_info & 0x7FF0) >> 4);
                sector->stopper = (bool)((misc_info & 0x8000) >> 15);
                if (sector->box == M_NO_BOX_TR3_LEGACY) {
                    sector->box = NO_BOX;
                }
            } else {
                sector->fx = 0;
                sector->box = File_ReadS16(file);
                sector->stopper = false;
            }
            sector->portal_room.pit = File_ReadU8(file);
            sector->floor.height = File_ReadS8(file) * STEP_L;
            sector->portal_room.sky = File_ReadU8(file);
            sector->ceiling.height = File_ReadS8(file) * STEP_L;
            if (sector->portal_room.pit == M_NO_ROOM_LEGACY) {
                sector->portal_room.pit = NO_ROOM;
            }
            if (sector->portal_room.sky == M_NO_ROOM_LEGACY) {
                sector->portal_room.sky = NO_ROOM;
            }
            if (sector->ceiling.height == M_NO_HEIGHT_LEGACY
                && sector->floor.height == M_NO_HEIGHT_LEGACY) {
                sector->ceiling.height = NO_HEIGHT;
            }
            if (sector->floor.height == M_NO_HEIGHT_LEGACY) {
                sector->floor.height = NO_HEIGHT;
            }
        }

        if (loader->game_version == 4) {
            const uint32_t room_color = File_ReadU32(file);
            const uint8_t room_r = (room_color >> 16) & 0xFF;
            const uint8_t room_g = (room_color >> 8) & 0xFF;
            const uint8_t room_b = room_color & 0xFF;
            room->ambient = (int16_t)((room_r + room_g + room_b) / 3);
            room->ambient_rgb = (RGB_888) { room_r, room_g, room_b };
            room->light_mode = RLM_NORMAL;
        } else {
            room->ambient = File_ReadS16(file);
            const uint8_t gray = (uint8_t)(room->ambient >> 5); // 0x1FFF scale
            room->ambient_rgb = (RGB_888) { gray, gray, gray };
            if (loader->game_version == 1) {
                room->light_mode = RLM_NORMAL;
            } else if (loader->game_version == 2) {
                File_Skip(file, sizeof(int16_t)); // Unused second ambient
                room->light_mode = File_ReadS16(file);
            } else {
                room->light_mode = File_ReadS16(file);
            }
        }

        room->num_lights = File_ReadCountS16(file);
        room->lights = room->num_lights == 0
            ? nullptr
            : GameBuf_Alloc(sizeof(LIGHT) * room->num_lights, GBUF_ROOM_LIGHTS);
        for (int32_t j = 0; j < room->num_lights; j++) {
            LIGHT *const light = &room->lights[j];
            if (loader->game_version == 4) {
                M_ReadRoomLightTR4(light, file);
            } else if (loader->game_version == 3) {
                M_InitialiseLegacyLight(light);
                // TR3 room lights use the LIGHT_INFO struct layout:
                // pos (s32*3) + rgb (u8*3) + type (u8) + union (8 bytes).
                M_ReadPosition(&light->pos, file);
                light->color.r = File_ReadU8(file);
                light->color.g = File_ReadU8(file);
                light->color.b = File_ReadU8(file);
                const uint8_t light_type = File_ReadU8(file);
                light->type =
                    light_type != 0 ? LIGHT_TYPE_SUN : LIGHT_TYPE_POINT;
                if (light_type != 0) {
                    light->u.legacy.dir.x = File_ReadS16(file);
                    light->u.legacy.dir.y = File_ReadS16(file);
                    light->u.legacy.dir.z = File_ReadS16(file);
                    File_Skip(file, sizeof(int16_t)); // pad
                    light->u.legacy.shade.value_1 = 0;
                    light->u.legacy.shade.value_2 = 0;
                    light->u.legacy.falloff.value_1 = 0;
                    light->u.legacy.falloff.value_2 = 0;
                } else {
                    int32_t intensity = File_ReadS32(file);
                    const int32_t falloff = File_ReadS32(file);
                    CLAMP(intensity, INT16_MIN, INT16_MAX);
                    light->u.legacy.shade.value_1 = (int16_t)intensity;
                    light->u.legacy.shade.value_2 = (int16_t)intensity;
                    light->u.legacy.falloff.value_1 = falloff;
                    light->u.legacy.falloff.value_2 = falloff;
                    light->u.legacy.dir = (XYZ_16) { 0, 0, 0 };
                }
            } else {
                M_InitialiseLegacyLight(light);
                M_ReadPosition(&light->pos, file);
                M_ReadShade(loader, &light->u.legacy.shade, file);
                light->u.legacy.falloff.value_1 = File_ReadS32(file);
                if (loader->game_version >= 2) {
                    light->u.legacy.falloff.value_2 = File_ReadS32(file);
                } else {
                    light->u.legacy.falloff.value_2 =
                        light->u.legacy.falloff.value_1;
                }
            }
        }

        room->num_static_meshes = File_ReadCountS16(file);
        const int32_t static_count =
            room->num_static_meshes + inj_data.num_static_3ds;
        room->static_meshes = static_count == 0
            ? nullptr
            : GameBuf_Alloc(
                  sizeof(STATIC_MESH) * static_count, GBUF_ROOM_STATIC_MESHES);
        for (int32_t j = 0; j < room->num_static_meshes; j++) {
            STATIC_MESH *const mesh = &room->static_meshes[j];
            M_ReadPosition(&mesh->pos, file);
            mesh->rot.y = File_ReadS16(file);
            if (loader->game_version == 4) {
                mesh->shade.value_1 = File_ReadU16(file);
                mesh->shade.value_2 = mesh->shade.value_1;
                File_Skip(file, sizeof(uint16_t)); // unused
                mesh->static_num = File_ReadU16(file);
            } else {
                M_ReadShade(loader, &mesh->shade, file);
                mesh->static_num = File_ReadS16(file);
            }
            mesh->draw_num = -1;
        }

        room->flipped_room = File_ReadS16(file);

        const uint16_t flags = File_ReadU16(file);
        // clang-format off
        room->flags.underwater    = (flags & 0x01) != 0;
        room->flags.outside       = (flags & 0x08) != 0;
        room->flags.dynamic_lit   = (flags & 0x10) != 0;
        room->flags.wind          = (flags & 0x20) != 0;
        room->flags.inside        = (flags & 0x40) != 0;
        room->flags.swamp         = (flags & 0x80) != 0 && g_TRVersion < 4;
        room->flags.no_lens_flare = (flags & 0x80) != 0 && g_TRVersion >= 4;
        // clang-format on

        room->item_num = NO_ITEM;
        room->effect_num = NO_EFFECT;

        if (loader->game_version == 4) {
            room->water_scheme = File_ReadU8(file);
            room->reverb_info = File_ReadU8(file);
            room->alternate_group = File_ReadU8(file);
        } else if (loader->game_version == 3) {
            room->water_scheme = File_ReadU8(file);
            room->reverb_info = File_ReadU8(file);
            room->alternate_group = 0;
            File_Skip(file, 1);
        } else {
            room->alternate_group = 0;
        }
    }

    for (int32_t i = 0; i < num_rooms; i++) {
        ROOM *const room = Room_Get(i);
        if (room->portals == nullptr) {
            continue;
        }
        for (int32_t j = 0; j < room->portals->count; j++) {
            PORTAL *const portal = &room->portals->portal[j];
            const XYZ_16 new_normal = M_ComputePortalNormal(portal);
            if (new_normal.x != portal->normal.x
                || new_normal.y != portal->normal.y
                || new_normal.z != portal->normal.z) {
                LOG_WARNING("Fixed room %d, portal normal %d", i, j);
                portal->normal = new_normal;
            }
        }
    }

    Room_InitialiseFlipStatus();

    const int32_t floor_data_size = File_ReadS32(file);
    int16_t *floor_data = Memory_Alloc(sizeof(int16_t) * floor_data_size);
    File_ReadData(file, floor_data, sizeof(int16_t) * floor_data_size);

    Room_ParseFloorData(floor_data, floor_data_size);
    Memory_FreePointer(&floor_data);

    Room_BuildOutsideTable();

finish:
    Benchmark_End(&benchmark, nullptr);
    return result;
}
