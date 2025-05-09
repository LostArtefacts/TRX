#include "game/output.h"
#include "game/output/meshes/common.h"
#include "game/output/meshes/dynamic.h"
#include "game/output/meshes/objects.h"
#include "game/output/meshes/rooms.h"
#include "game/output/sprites.h"
#include "game/output/utils.h"

#include <libtrx/config.h>
#include <libtrx/memory.h>

#include <math.h>

static struct {
    GLint bound_polygon_mode[2];
} m_CachedState;

static bool m_IsSkyboxEnabled = false;
static void M_DrawSphere(XYZ_32 pos, int32_t radius);

static void M_DrawSphere(const XYZ_32 pos, const int32_t radius)
{
    // More subdivisions means smoother spheres.
    const int32_t subdivisions = 12;
    const int32_t position_count = SQUARE(subdivisions + 1);
    XYZ_F positions[position_count];
    int32_t index = 0;

    for (int32_t i = 0; i <= subdivisions; i++) {
        const float theta = (M_PI * i) / subdivisions; // Latitude angle
        const float sin_theta = sinf(theta);
        const float cos_theta = cosf(theta);

        for (int32_t j = 0; j <= subdivisions; j++) {
            const float phi = (2 * M_PI * j) / subdivisions; // Longitude angle
            const float sin_phi = sinf(phi);
            const float cos_phi = cosf(phi);

            // Convert spherical coordinates to 3D points.
            positions[index] = (XYZ_F) {
                .x = pos.x + radius * cos_phi * sin_theta,
                .y = pos.y + radius * cos_theta,
                .z = pos.z + radius * sin_phi * sin_theta,
            };
            index++;
        }
    }

    const int32_t vertex_count =
        subdivisions * subdivisions * OUTPUT_QUAD_VERTICES;
    OUTPUT_MESH_VERTEX vertices[vertex_count];
    OUTPUT_MESH_VERTEX *out_vertex = vertices;
    for (int32_t i = 0; i < subdivisions; i++) {
        for (int32_t j = 0; j < subdivisions; j++) {
            const int32_t indices[4] = {
                i * (subdivisions + 1) + j,
                (i + 1) * (subdivisions + 1) + j,
                (i + 1) * (subdivisions + 1) + (j + 1),
                i * (subdivisions + 1) + (j + 1),
            };
            for (int32_t k = 0; k < OUTPUT_QUAD_VERTICES; k++) {
                out_vertex->pos = positions[indices[OUTPUT_QUAD_TO_FAN(k)]];
                out_vertex++;
            }
        }
    }

    const RGBA_8888 color_black = { 0, 0, 0, 128 };
    const RGBA_8888 color_white = { 255, 255, 255, 128 };
    const bool wireframe_state = GFX_Context_GetWireframeMode();
    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].flags =
            VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_CAUSTICS;
        vertices[i].color = wireframe_state ? color_black : color_white;
    }

    glGetIntegerv(GL_POLYGON_MODE, &m_CachedState.bound_polygon_mode[0]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Output_Shader_UploadMatrix(Output_Meshes_GetShader(), g_MatrixPtr);
    Output_Meshes_DrawTriangles(vertex_count, vertices);
    glBlendFunc(GL_ONE, GL_ZERO);
    glPolygonMode(GL_FRONT_AND_BACK, m_CachedState.bound_polygon_mode[0]);
}

void Output_SetSkyboxEnabled(const bool enabled)
{
    m_IsSkyboxEnabled = enabled;
}

bool Output_IsSkyboxEnabled(void)
{
    return m_IsSkyboxEnabled;
}

void Output_DrawSkybox(const OBJECT_MESH *const mesh)
{
    Output_RememberState();
    glDisable(GL_DEPTH_TEST);
    Output_Meshes_RenderObjectMesh(g_MatrixPtr, Output_GetTint(), mesh);
    glEnable(GL_DEPTH_TEST);
    Output_RestoreState();
}

void Output_DrawObjectMesh(const OBJECT_MESH *const mesh, const int32_t clip)
{
    Output_RememberState();
    Output_Meshes_RenderObjectMesh(g_MatrixPtr, Output_GetTint(), mesh);
    Output_RestoreState();

    if (g_Config.rendering.enable_debug_spheres) {
        M_DrawSphere(
            (XYZ_32) { mesh->center.x, mesh->center.y, mesh->center.z },
            mesh->radius);
    }
}

void Output_DrawObjectMesh_I(const OBJECT_MESH *const mesh, const int32_t clip)
{
    Matrix_Push();
    Matrix_Interpolate();
    Output_DrawObjectMesh(mesh, clip);
    Matrix_Pop();
}

void Output_DrawRoomMesh(ROOM *const room)
{
    Output_RememberState();
    Output_LightRoom(room);
    Output_EnableScissor(
        room->bound_left, room->bound_bottom,
        room->bound_right - room->bound_left,
        room->bound_bottom - room->bound_top);
    Output_Meshes_RenderRoomMesh(g_MatrixPtr, Output_GetTint(), room);
    Output_DisableScissor();
    Output_RestoreState();
}

void Output_DrawRoomPortals(const ROOM *const room)
{
    if (room->portals == nullptr) {
        return;
    }

    const int32_t vertex_count = room->portals->count * 8;
    OUTPUT_MESH_VERTEX vertices[vertex_count];
    OUTPUT_MESH_VERTEX *out_vertex = vertices;
    const RGBA_8888 color = { 0, 0, 255, 255 };
    for (int32_t i = 0; i < room->portals->count; i++) {
        const PORTAL *const portal = &room->portals->portal[i];
        const XYZ_F positions[4] = {
            { portal->vertex[0].x, portal->vertex[0].y, portal->vertex[0].z },
            { portal->vertex[1].x, portal->vertex[1].y, portal->vertex[1].z },
            { portal->vertex[2].x, portal->vertex[2].y, portal->vertex[2].z },
            { portal->vertex[3].x, portal->vertex[3].y, portal->vertex[3].z },
        };
        const int32_t indices[8] = { 0, 1, 1, 2, 2, 3, 3, 0 };
        for (int32_t j = 0; j < 8; j++) {
            out_vertex->pos = positions[indices[j]];
            out_vertex++;
        }
    }
    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].flags =
            VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_CAUSTICS;
        vertices[i].color = color;
    }

    glDisable(GL_DEPTH_TEST);
    glGetIntegerv(GL_POLYGON_MODE, &m_CachedState.bound_polygon_mode[0]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Output_Shader_UploadMatrix(Output_Meshes_GetShader(), g_MatrixPtr);
    Output_Meshes_DrawPrimitives(GL_LINES, vertex_count, vertices);
    glBlendFunc(GL_ONE, GL_ZERO);
    glPolygonMode(GL_FRONT_AND_BACK, m_CachedState.bound_polygon_mode[0]);
    glEnable(GL_DEPTH_TEST);
}

void Output_DrawRoomTriggers(const ROOM *const room)
{
    const RGBA_8888 color = { .r = 255, .g = 0, .b = 255, .a = 128 };
    const XZ_16 offsets[4] = { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };

    int32_t vertex_count = 0;
    for (int32_t z = 0; z < room->size.z; z++) {
        for (int32_t x = 0; x < room->size.x; x++) {
            const SECTOR *sector = Room_GetUnitSector(room, x, z);
            if (sector->trigger == nullptr) {
                continue;
            }
            vertex_count += OUTPUT_QUAD_VERTICES;
        }
    }

    OUTPUT_MESH_VERTEX *vertices =
        Memory_Alloc(vertex_count * sizeof(OUTPUT_MESH_VERTEX));
    OUTPUT_MESH_VERTEX *out_vertex = vertices;

    for (int32_t z = 0; z < room->size.z; z++) {
        for (int32_t x = 0; x < room->size.x; x++) {
            const SECTOR *sector = Room_GetUnitSector(room, x, z);
            if (sector->trigger == nullptr) {
                continue;
            }
            for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
                const int32_t j = OUTPUT_QUAD_TO_FAN(i);
                XYZ_16 vertex_pos = {
                    .x = (x + offsets[j].x) * WALL_L,
                    .z = (z + offsets[j].z) * WALL_L,
                };
                XYZ_32 world_pos = {
                    .x = room->pos.x + x * WALL_L + offsets[j].x * (WALL_L - 1),
                    .z = room->pos.z + z * WALL_L + offsets[j].z * (WALL_L - 1),
                    .y = room->pos.y,
                };

                int16_t room_num = room - Room_Get(0);
                sector = Room_GetSector(
                    world_pos.x, world_pos.y, world_pos.z, &room_num);
                vertex_pos.y =
                    Room_GetHeight(
                        sector, world_pos.x, world_pos.y, world_pos.z)
                    + (Output_GetWaterEffect() ? -16 : -2);

                out_vertex->pos =
                    (XYZ_F) { vertex_pos.x, vertex_pos.y, vertex_pos.z };
                out_vertex->flags = VERT_FLAT_SHADED | VERT_NO_LIGHTING;
                out_vertex->color = color;
                out_vertex++;
            }
        }
    }

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Output_Shader_UploadMatrix(Output_Meshes_GetShader(), g_MatrixPtr);
    Output_Meshes_DrawTriangles(vertex_count, vertices);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthMask(GL_TRUE);
    Memory_FreePointer(&vertices);
}

void Output_DrawShadow(
    const int16_t size, const BOUNDS_16 *const bounds, const ITEM *const item)
{
    if (!item->enable_shadow) {
        return;
    }

    const int32_t vertex_count = g_Config.visuals.enable_round_shadow ? 32 : 8;
    const int32_t x0 = bounds->min.x;
    const int32_t x1 = bounds->max.x;
    const int32_t z0 = bounds->min.z;
    const int32_t z1 = bounds->max.z;
    const int32_t x_mid = (x0 + x1) / 2;
    const int32_t z_mid = (z0 + z1) / 2;
    const int32_t x_add = (x1 - x0) * size / 1024;
    const int32_t z_add = (z1 - z0) * size / 1024;

    Matrix_Push();
    Matrix_TranslateAbs(
        item->interp.result.pos.x, item->floor, item->interp.result.pos.z);
    Matrix_RotY(item->rot.y);

    OUTPUT_MESH_VERTEX vertices[vertex_count * 3];
    for (int32_t i = 0; i < vertex_count; i++) {
        for (int32_t j = 0; j < 2; j++) {
            const int32_t angle = (DEG_180 + (i + j) * DEG_360) / vertex_count;
            vertices[i * 3 + j].pos.x =
                x_mid + ((x_add * 2) * Math_Sin(angle) >> W2V_SHIFT);
            vertices[i * 3 + j].pos.z =
                z_mid + ((z_add * 2) * Math_Cos(angle) >> W2V_SHIFT);
        }
        vertices[i * 3 + 2].pos.x = x_mid;
        vertices[i * 3 + 2].pos.z = z_mid;
        for (int32_t j = 0; j < 3; j++) {
            vertices[i * 3 + j].pos.y = -5;
            vertices[i * 3 + j].flags =
                VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_CAUSTICS;
            vertices[i * 3 + j].color = (RGBA_8888) { 0, 0, 0, 128 };
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Output_Shader_UploadMatrix(Output_Meshes_GetShader(), g_MatrixPtr);
    Output_Meshes_DrawTriangles(vertex_count * 3, vertices);
    glBlendFunc(GL_ONE, GL_ZERO);

    Matrix_Pop();
}

void Output_DrawSprite(
    const int32_t x, const int32_t y, const int32_t z, const int16_t sprite_idx,
    const int16_t shade, const RGB_F tint)
{
    Matrix_Push();
    Matrix_TranslateAbs(x, y, z);
    Output_Sprites_RenderSingleSprite(
        g_MatrixPtr, (XYZ_32) { 0, 0, 0 }, sprite_idx, shade, tint);
    Matrix_Pop();
}
