#include "game/output/warmup.h"

#include "config.h"
#include "debug.h"
#include "game/clock.h"
#include "game/matrix.h"
#include "game/output/common.h"
#include "game/output/textures.h"
#include "game/output/utils.h"
#include "gfx/context.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    XYZW_F pos;
    XYZ_F normal;
    OUTPUT_USHORT flags;
    RGBA_8888 color;
    OUTPUT_UVW uvw;
    OUTPUT_TEXTURE_SIZE texture_size;
    float trapezoid_ratio[2];
    OUTPUT_SHORT shade;
} M_VERTEX;

void Output_WarmUp(void)
{
    CONFIG saved_config = g_Config;

    OUTPUT_SHADER *const shader = Output_GetMeshShader();
    ASSERT(shader != nullptr);

    GLuint vao;
    GLuint vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, normal));
    glVertexAttribIPointer(
        OUTPUT_MESH_ATTR_FLAGS, 1, OUTPUT_USHORT_GL, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, flags));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, color));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_UVW, 3, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, uvw));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_TEXTURE_SIZE, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, texture_size));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_TRAPEZOID_RATIO, 2, GL_FLOAT, GL_FALSE,
        sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, trapezoid_ratio));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_SHADE, 1, OUTPUT_SHORT_GL, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, shade));

    const BILLBOARD_LOCK_MODE old_lock_mode =
        g_Config.rendering.sprite_lock_mode;

    Output_Shader_Bind(shader);
    Output_Shader_UploadOrthoProjectionMatrix(shader);
    Output_Shader_UploadViewModelMatrix(shader, &g_IDMatrix);

    SDL_GL_SetSwapInterval(0);
    GFX_Context_Clear();
    GFX_Renderer_BindGeometryFbo();

    M_VERTEX verts[3];
    for (int32_t i = 0; i < 3; i++) {
        const uint16_t flags = VERT_FLAT_SHADED | VERT_BILLBOARD
            | (i % 2 == 0 ? VERT_ABS_SPRITE : 0);
        const float fx = i * 200 - 100;
        const float fy = i * 200 - 50;
        const float fz = i * 100;
        verts[i] = (M_VERTEX) {
            .pos = { fx, fy, fz, 1 },
            .normal = { fy * 0.01f, fz * 0.01f, fx * 0.01f },
            .uvw = { fy * 0.01f, fz * 0.01f, fx * 0.01f, },
            .trapezoid_ratio = { fx * 0.001f, fy * 0.001f },
            .shade = SHADE_NEUTRAL,
            .color = {i, i, i, 0},
            .flags = flags
        };
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

    for (BILLBOARD_LOCK_MODE mode = 0; mode < BILLBOARD_LOCK_NUMBER_OF;
         mode++) {
        g_Config.rendering.sprite_lock_mode = mode;
        Output_Shader_UploadCommonUniforms(shader);
        glDrawArrays(GL_TRIANGLES, 0, 100 * 3);
    }
    glFinish();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    GFX_Renderer_BindUiFbo();
    g_Config = saved_config;
}
