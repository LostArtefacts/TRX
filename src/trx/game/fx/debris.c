// TR4 debris: the textured triangles a mesh breaks into when it shatters.
// Each piece keeps the three vertices of one source face, spins as it flies,
// and dies where it meets the floor or the ceiling.

#include <trx/game/fx/debris.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/fx/common.h>
#include <trx/game/interpolation.h>
#include <trx/game/matrix.h>
#include <trx/game/objects/common.h>
#include <trx/game/output.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/rules.h>

#define M_MAX_DEBRIS 256
#define M_MAX_VERTICES 256

typedef struct {
    bool on;
    uint16_t texture_idx;
    uint8_t uv_corners[3];
    XYZ_32 pos;
    XYZ_32 prev_pos;
    XYZ_16 offsets[3];
    int16_t dir;
    int16_t speed;
    int16_t y_vel;
    int16_t gravity;
    int16_t room_num;
    uint8_t x_rot;
    uint8_t y_rot;
    uint8_t prev_x_rot;
    uint8_t prev_y_rot;
    RGB_888 color;
    uint8_t intensity[3];
    RGB_888 ambient;
    uint16_t flags;
} M_DEBRIS;

typedef struct {
    const OBJECT_MESH *mesh;
    uint16_t vertices[3];
    RGB_888 ambient;
    uint16_t flags;
} M_MESH_DATA;

typedef struct {
    const OBJECT_MESH *mesh;
    XYZ_32 pos;
    int16_t room_num;
    int16_t yaw;
    int16_t shade;
    int16_t flags;
    int16_t face_count;
    int32_t xz_vel;
} M_SHATTER_SOURCE;

typedef struct {
    M_DEBRIS debris[M_MAX_DEBRIS];
    int32_t next_idx;
} M_PRIV;

static M_PRIV m_Priv = {};

static int32_t M_GetFreeDebris(void)
{
    int32_t free = m_Priv.next_idx;
    int32_t eldest_free = 0;
    int32_t eldest_age = -0x4000;
    const M_DEBRIS *debris = &m_Priv.debris[m_Priv.next_idx];

    for (int32_t i = 0; i < M_MAX_DEBRIS; i++) {
        if (!debris->on) {
            m_Priv.next_idx = (free + 1) & (M_MAX_DEBRIS - 1);
            return free;
        }

        if (debris->y_vel > eldest_age) {
            eldest_free = free;
            eldest_age = debris->y_vel;
        }

        if (free == (M_MAX_DEBRIS - 1)) {
            debris = &m_Priv.debris[0];
            free = 0;
        } else {
            free++;
            debris++;
        }
    }

    m_Priv.next_idx = (eldest_free + 1) & (M_MAX_DEBRIS - 1);
    return eldest_free;
}

static void M_Control(void)
{
    for (int32_t i = 0; i < M_MAX_DEBRIS; i++) {
        M_DEBRIS *const debris = &m_Priv.debris[i];
        if (!debris->on) {
            continue;
        }

        debris->prev_pos = debris->pos;
        debris->prev_x_rot = debris->x_rot;
        debris->prev_y_rot = debris->y_rot;

        debris->y_vel += debris->gravity;
        CLAMPG(debris->y_vel, 0x1000);

        debris->speed -= debris->speed >> 4;
        debris->pos = XYZ_32_OffsetYaw(debris->pos, debris->dir, debris->speed);
        debris->pos.y += debris->y_vel >> 4;

        const SECTOR *const sector =
            Room_GetSector(debris->pos, &debris->room_num);
        const int32_t height = Room_GetHeight(sector, debris->pos);
        const int32_t ceiling = Room_GetCeiling(sector, debris->pos);

        if (debris->pos.y >= height || debris->pos.y < ceiling) {
            debris->on = false;
        } else {
            debris->x_rot += debris->y_vel >> 6;
            if (debris->y_vel != 0) {
                debris->y_rot += debris->speed >> 5;
            }

            if (Room_Get(debris->room_num)->flags.underwater) {
                debris->y_vel -= debris->y_vel >> 2;
            }
        }
    }
}

static void M_Reset(void)
{
    m_Priv = (M_PRIV) {};
}

// Reports whether the mesh carries a shade per vertex.
static bool M_IsMeshPrelit(const OBJECT_MESH *const mesh)
{
    return mesh->num_lights <= 0;
}

// Reads how bright one vertex of the source face is. A mesh that carries no
// shade leaves the piece to the room ambient alone.
static uint8_t M_GetVertexIntensity(
    const OBJECT_MESH *const mesh, const uint16_t vertex_idx)
{
    if (!M_IsMeshPrelit(mesh) || vertex_idx >= -mesh->num_lights) {
        return 0;
    }

    const int32_t shade = mesh->lighting.lights[vertex_idx];
    const int32_t intensity =
        255 * (2 * SHADE_NEUTRAL - shade) / (2 * SHADE_NEUTRAL);
    return MAX(0, MIN(255, intensity));
}

static void M_TriggerDebris(
    const GAME_VECTOR *const pos, const uint16_t texture_idx,
    const uint8_t uv_corners[3], const XYZ_16 offsets[3], XYZ_32 velocities,
    int16_t rgb, const M_MESH_DATA *const mesh_data)
{
    // TR4 puffs smoke off every fourth piece. TRX has no smoke spark system
    // to puff with yet.
    if ((Random_GetControl() & 3) != 0) {
        rgb = ABS(rgb);
    }

    M_DEBRIS *const debris = &m_Priv.debris[M_GetFreeDebris()];
    *debris = (M_DEBRIS) {};
    debris->on = true;
    debris->pos = pos->pos;
    debris->prev_pos = pos->pos;

    if ((mesh_data->flags & 1) != 0) {
        debris->dir = Random_GetControl() << 1;
        debris->speed = (Random_GetControl() & 0xF) + 16;
    } else {
        debris->dir = Math_Atan(velocities.z, velocities.x);
        debris->speed = (ABS(velocities.x) + ABS(velocities.z)) >> 2;
    }

    if (velocities.y != 0) {
        debris->y_vel = -512 - (Random_GetControl() & 0x1FF);
        debris->gravity = (Random_GetControl() & 0x3F) + 64;

        if (velocities.y == -1) {
            debris->y_vel <<= 1;
        } else if (velocities.y == -2) {
            debris->y_vel >>= 1;
        }
    } else {
        debris->y_vel = 0;
        debris->gravity = (Random_GetControl() & 0x1F) + 32;
    }

    debris->room_num = pos->room_num;
    for (int32_t i = 0; i < 3; i++) {
        debris->offsets[i] = offsets[i];
        debris->uv_corners[i] = uv_corners[i];
        debris->intensity[i] =
            M_GetVertexIntensity(mesh_data->mesh, mesh_data->vertices[i]);
    }

    if ((mesh_data->flags & 1) != 0) {
        debris->y_rot = Random_GetControl() << 1;
        debris->x_rot = debris->y_rot;
    }
    debris->prev_x_rot = debris->x_rot;
    debris->prev_y_rot = debris->y_rot;

    debris->texture_idx = texture_idx;
    debris->color.r = (rgb & 0x1F) << 3;
    debris->color.g = ((rgb >> 5) & 0x1F) << 3;
    debris->color.b = ((rgb >> 10) & 0x1F) << 3;
    debris->ambient = mesh_data->ambient;
    debris->flags = mesh_data->flags;
}

// Turns one face of the shattered mesh into a piece of debris. A quad breaks
// along its first, second and fourth corners, which is the triangle TR4 takes
// from it.
static bool M_SpawnFace(
    const M_SHATTER_SOURCE *const source, const FACE *const face,
    const bool is_quad, const XYZ_16 *const rot_verts,
    const int32_t vertex_count, const XYZ_32 mesh_center,
    M_MESH_DATA *const mesh_data)
{
    const uint8_t uv_corners[3] = { 0, 1, is_quad ? 3 : 2 };
    mesh_data->flags = face->effects;
    XYZ_16 offsets[3] = {};
    for (int32_t i = 0; i < 3; i++) {
        const uint16_t vertex_idx = face->vertices[uv_corners[i]];
        if (vertex_idx >= vertex_count) {
            return false;
        }
        mesh_data->vertices[i] = vertex_idx;
        offsets[i] = rot_verts[vertex_idx];
    }

    GAME_VECTOR vec = {
        .x = (offsets[0].x + offsets[1].x + offsets[2].x) / 3,
        .y = (offsets[0].y + offsets[1].y + offsets[2].y) / 3,
        .z = (offsets[0].z + offsets[1].z + offsets[2].z) / 3,
        .room_num = source->room_num,
    };

    for (int32_t i = 0; i < 3; i++) {
        offsets[i].x -= vec.x;
        offsets[i].y -= vec.y;
        offsets[i].z -= vec.z;
    }

    XYZ_32 velocities = {};
    if (source->xz_vel <= 0) {
        velocities.x = vec.x - mesh_center.x;
        velocities.y = vec.y - mesh_center.y;
        velocities.z = vec.z - mesh_center.z;
    }
    if (source->xz_vel < 0) {
        velocities.y = source->xz_vel;
    }

    vec.x += source->pos.x;
    vec.y += source->pos.y;
    vec.z += source->pos.z;

    const int16_t color =
        (source->flags & 0x400) != 0 ? -source->shade : source->shade;
    M_TriggerDebris(
        &vec, face->texture_idx, uv_corners, offsets, velocities, color,
        mesh_data);
    return true;
}

static void M_Spawn(const M_SHATTER_SOURCE *const source)
{
    int32_t face_count = source->face_count;
    const bool random_face = face_count < 0;
    face_count = ABS(face_count);

    int32_t vertex_count = source->mesh->num_vertices;
    if (vertex_count == 0) {
        return;
    }
    CLAMPG(vertex_count, M_MAX_VERTICES);

    M_MESH_DATA mesh_data = {
        .mesh = source->mesh,
        .ambient = M_IsMeshPrelit(source->mesh)
            ? (RGB_888) {}
            : Room_Get(source->room_num)->ambient_rgb,
    };

    Matrix_PushUnit();
    Matrix_RotY(source->yaw);

    XYZ_16 rot_verts[M_MAX_VERTICES] = {};
    XYZ_32 total = {};
    for (int32_t i = 0; i < vertex_count; i++) {
        const XYZ_16 vec = source->mesh->vertices[i];
        const XYZ_32 rotated =
            Matrix_MulVec32_M(g_WMatrixPtr, (XYZ_32) { vec.x, vec.y, vec.z });
        rot_verts[i] = (XYZ_16) { rotated.x, rotated.y, rotated.z };
        total.x += rotated.x;
        total.y += rotated.y;
        total.z += rotated.z;
    }
    Matrix_Pop();

    const XYZ_32 mesh_center = {
        .x = total.x / vertex_count,
        .y = total.y / vertex_count,
        .z = total.z / vertex_count,
    };

    for (int32_t pass = 0; pass < 2 && face_count != 0; pass++) {
        const bool is_quad = pass == 1;
        const int32_t count = is_quad ? source->mesh->tex_face4s.count
                                      : source->mesh->tex_face3s.count;
        const FACE *const faces = is_quad ? source->mesh->tex_face4s.data
                                          : source->mesh->tex_face3s.data;
        for (int32_t i = 0; i < count && face_count != 0; i++) {
            if (random_face && (Random_GetControl() & 1) == 0) {
                continue;
            }
            if (M_SpawnFace(
                    source, &faces[i], is_quad, rot_verts, vertex_count,
                    mesh_center, &mesh_data)) {
                face_count--;
            }
        }
    }
}

static RGBA_8888 M_GetVertexColor(
    const M_DEBRIS *const debris, const int32_t idx)
{
    const int32_t intensity = debris->intensity[idx];
    return (RGBA_8888) {
        .r = MIN(255, (intensity * debris->color.r >> 8) + debris->ambient.r),
        .g = MIN(255, (intensity * debris->color.g >> 8) + debris->ambient.g),
        .b = MIN(255, (intensity * debris->color.b >> 8) + debris->ambient.b),
        .a = 255,
    };
}

static void M_DrawDebris(const M_DEBRIS *const debris, const double ratio)
{
    const bool do_interp =
        Interpolation_IsActive() && ratio > 0.0 && ratio < 1.0;
    const XYZ_32 pos = do_interp
        ? (XYZ_32) {
              .x = (int32_t)LERP(debris->prev_pos.x, debris->pos.x, ratio),
              .y = (int32_t)LERP(debris->prev_pos.y, debris->pos.y, ratio),
              .z = (int32_t)LERP(debris->prev_pos.z, debris->pos.z, ratio),
          }
        : debris->pos;
    const int16_t x_rot = do_interp
        ? Math_AngleMean(debris->prev_x_rot << 8, debris->x_rot << 8, ratio)
        : debris->x_rot << 8;
    const int16_t y_rot = do_interp
        ? Math_AngleMean(debris->prev_y_rot << 8, debris->y_rot << 8, ratio)
        : debris->y_rot << 8;

    Matrix_Push();
    Matrix_TranslateAbs32(pos);
    if (g_Rules.fx.rotate_debris) {
        Matrix_RotY(y_rot);
        Matrix_RotX(x_rot);
    }

    XYZ_32 world[3] = {};
    OUTPUT_UVW uvw[3] = {};
    OUTPUT_TEXTURE_SIZE texture_size[3] = {};
    RGBA_8888 color[3] = {};
    for (int32_t i = 0; i < 3; i++) {
        const XYZ_16 offset = debris->offsets[i];
        world[i] = Matrix_MulVec32_M(
            g_WMatrixPtr, (XYZ_32) { offset.x, offset.y, offset.z });
        const int32_t uvw_idx = Output_Textures_GetObjectUVWIndex(
            debris->texture_idx, debris->uv_corners[i]);
        uvw[i] = Output_Textures_GetUVW(uvw_idx);
        texture_size[i] = Output_Textures_GetAtlasSize(uvw_idx / 4);
        color[i] = M_GetVertexColor(debris, i);
    }
    Matrix_Pop();

    const DRAW_TYPE draw_type = (debris->flags & 1) != 0
        ? DRAW_BLEND_ADD
        : Output_GetObjectTexture(debris->texture_idx)->draw_type;
    OutputSource_PolyFX_StageTriExtUV(
        world, uvw, texture_size, nullptr, color,
        VERT_NO_LIGHTING | VERT_NO_WIBBLE, draw_type);
}

static void M_Draw(void)
{
    const double ratio = Interpolation_GetWorldRate();
    for (int32_t i = 0; i < M_MAX_DEBRIS; i++) {
        const M_DEBRIS *const debris = &m_Priv.debris[i];
        if (debris->on) {
            M_DrawDebris(debris, ratio);
        }
    }
}

static void M_Save(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < M_MAX_DEBRIS; i++) {
        const M_DEBRIS *const debris = &m_Priv.debris[i];
        if (!debris->on) {
            continue;
        }

        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "pos", debris->pos);
        JSONW_WRITE(io, "texture_idx", debris->texture_idx);
        JSONW_PUSH_ARRAY(io);
        for (int32_t j = 0; j < 3; j++) {
            JSONW_PUSH_VALUE(io, debris->offsets[j]);
            JSONW_POP_AND_APPEND(io);
        }
        JSONW_POP_AND_SET(io, "offsets");
        JSONW_PUSH_ARRAY(io);
        for (int32_t j = 0; j < 3; j++) {
            JSONW_PUSH_VALUE(io, debris->uv_corners[j]);
            JSONW_POP_AND_APPEND(io);
        }
        JSONW_POP_AND_SET(io, "uv_corners");
        JSONW_PUSH_ARRAY(io);
        for (int32_t j = 0; j < 3; j++) {
            JSONW_PUSH_VALUE(io, debris->intensity[j]);
            JSONW_POP_AND_APPEND(io);
        }
        JSONW_POP_AND_SET(io, "intensity");
        JSONW_WRITE(io, "dir", debris->dir);
        JSONW_WRITE(io, "speed", debris->speed);
        JSONW_WRITE(io, "y_vel", debris->y_vel);
        JSONW_WRITE(io, "gravity", debris->gravity);
        JSONW_WRITE(io, "room_num", debris->room_num);
        JSONW_WRITE(io, "x_rot", debris->x_rot);
        JSONW_WRITE(io, "y_rot", debris->y_rot);
        JSONW_WRITE(io, "color", debris->color);
        JSONW_WRITE(io, "ambient", debris->ambient);
        JSONW_WRITE(io, "flags", debris->flags);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET_NZ(io, "pieces");
}

static RESULT M_LoadDebris(JSON_READ_IO *const io, M_DEBRIS *const debris)
{
    MUST(JSON_READ(io, "pos", &debris->pos));
    MUST(JSON_READ(io, "texture_idx", &debris->texture_idx));

    MUST(JSON_PUSH(io, "offsets"));
    for (int32_t i = 0; i < 3; i++) {
        MUST(JSON_READ_A(io, i, &debris->offsets[i]));
    }
    MUST(JSON_POP(io));

    MUST(JSON_PUSH(io, "uv_corners"));
    for (int32_t i = 0; i < 3; i++) {
        MUST(JSON_READ_A(io, i, &debris->uv_corners[i]));
    }
    MUST(JSON_POP(io));

    MUST(JSON_PUSH(io, "intensity"));
    for (int32_t i = 0; i < 3; i++) {
        MUST(JSON_READ_A(io, i, &debris->intensity[i]));
    }
    MUST(JSON_POP(io));

    MUST(JSON_READ(io, "dir", &debris->dir));
    MUST(JSON_READ(io, "speed", &debris->speed));
    MUST(JSON_READ(io, "y_vel", &debris->y_vel));
    MUST(JSON_READ(io, "gravity", &debris->gravity));
    MUST(JSON_READ(io, "room_num", &debris->room_num));
    MUST(JSON_READ(io, "x_rot", &debris->x_rot));
    MUST(JSON_READ(io, "y_rot", &debris->y_rot));
    MUST(JSON_READ(io, "color", &debris->color));
    MUST(JSON_READ(io, "ambient", &debris->ambient));
    MUST(JSON_READ(io, "flags", &debris->flags));

    debris->on = true;
    debris->prev_pos = debris->pos;
    debris->prev_x_rot = debris->x_rot;
    debris->prev_y_rot = debris->y_rot;
    return OK;
}

static RESULT M_Load(JSON_READ_IO *const io)
{
    if (!JSON_ReadIO_HasKey(io, "pieces")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "pieces"));

    const int32_t count = JSON_ARRAY_LEN(io);
    for (int32_t i = 0; i < count; i++) {
        if (i >= M_MAX_DEBRIS) {
            LOG_WARNING(
                "Malformed save: too many debris pieces. Extra pieces will be "
                "ignored.");
            break;
        }

        MUST(JSON_PUSH_INDEX(io, i));
        MUST(M_LoadDebris(io, &m_Priv.debris[i]));
        MUST(JSON_POP(io));
        m_Priv.next_idx = (i + 1) & (M_MAX_DEBRIS - 1);
    }

    MUST(JSON_POP(io));
    return OK;
}

void FX_Debris_ShatterItem(
    const SHATTER_ITEM *const shatter_item, const int16_t face_count,
    const int16_t room_num, const int32_t xz_vel)
{
    const M_SHATTER_SOURCE source = {
        .mesh = shatter_item->mesh,
        .pos = shatter_item->pos,
        .yaw = shatter_item->yaw,
        .shade = 0,
        .flags = shatter_item->flags,
        .face_count = face_count,
        .room_num = room_num,
        .xz_vel = xz_vel,
    };
    M_Spawn(&source);
}

void FX_Debris_ShatterStatic(
    const STATIC_MESH *const static_mesh, const int16_t face_count,
    const int16_t room_num, const int32_t xz_vel)
{
    const STATIC_OBJECT_3D *const obj =
        Object_Get3DStatic(static_mesh->static_num);
    const M_SHATTER_SOURCE source = {
        .mesh = Object_GetMesh(obj->mesh_idx),
        .pos = static_mesh->pos,
        .yaw = static_mesh->rot.y,
        .shade = static_mesh->shade.value_1,
        .flags = 0,
        .face_count = face_count,
        .room_num = room_num,
        .xz_vel = xz_vel,
    };
    M_Spawn(&source);
}

static const FX_MODULE m_Module = {
    .control_func = M_Control,
    .reset_func = M_Reset,
    .draw_func = M_Draw,
    .save_key = "debris",
    .save_func = M_Save,
    .load_func = M_Load,
};

REGISTER_FX(m_Module)
