#include "gfx/fade/fade_renderer.h"

#include "debug.h"
#include "gfx/context.h"
#include "gfx/gl/utils.h"
#include "memory.h"

#include <GL/glew.h>

struct GFX_FADE_RENDERER {
    GFX_GL_VERTEX_ARRAY surface_format;
    GFX_GL_BUFFER surface_buffer;
    GFX_GL_PROGRAM program;

    // shader variable locations
    GLint loc_opacity;
};

GFX_FADE_RENDERER *GFX_FadeRenderer_Create(void)
{
    GFX_FADE_RENDERER *const r = Memory_Alloc(sizeof(GFX_FADE_RENDERER));
    const GFX_CONFIG *const config = GFX_Context_GetConfig();

    GFX_GL_Buffer_Init(&r->surface_buffer, GL_ARRAY_BUFFER);
    GFX_GL_Buffer_Bind(&r->surface_buffer);
    GLfloat vertices[] = {
        0.0, 0.0, // t0
        1.0, 0.0, // t1
        0.0, 1.0, // t2
        0.0, 1.0, // t3
        1.0, 0.0, // t4
        1.0, 1.0, // t5
    };
    GFX_GL_Buffer_Data(
        &r->surface_buffer, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GFX_GL_VertexArray_Init(&r->surface_format);
    GFX_GL_VertexArray_Bind(&r->surface_format);
    GFX_GL_VertexArray_Attribute(
        &r->surface_format, 0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GFX_GL_Program_Init(&r->program);
    GFX_GL_Program_AttachShader(
        &r->program, GL_VERTEX_SHADER, "shaders/fade.glsl", config->backend);
    GFX_GL_Program_AttachShader(
        &r->program, GL_FRAGMENT_SHADER, "shaders/fade.glsl", config->backend);
    GFX_GL_Program_FragmentData(&r->program, "outColor");
    GFX_GL_Program_Link(&r->program);

    r->loc_opacity = GFX_GL_Program_UniformLocation(&r->program, "opacity");

    GFX_GL_Program_Bind(&r->program);
    GFX_GL_Program_Uniform1f(&r->program, r->loc_opacity, 0.0f);

    GFX_GL_CheckError();
    return r;
}

void GFX_FadeRenderer_Destroy(GFX_FADE_RENDERER *const r)
{
    ASSERT(r != nullptr);
    GFX_GL_VertexArray_Close(&r->surface_format);
    GFX_GL_Buffer_Close(&r->surface_buffer);
    GFX_GL_Program_Close(&r->program);
    Memory_Free(r);
}

void GFX_FadeRenderer_SetOpacity(
    GFX_FADE_RENDERER *const r, const float opacity)
{
    ASSERT(r != nullptr);
    GFX_GL_Program_Bind(&r->program);
    GFX_GL_Program_Uniform1f(&r->program, r->loc_opacity, opacity);
    GFX_GL_CheckError();
}

void GFX_FadeRenderer_Render(GFX_FADE_RENDERER *const r)
{
    ASSERT(r != nullptr);
    GFX_GL_Program_Bind(&r->program);
    GFX_GL_Buffer_Bind(&r->surface_buffer);
    GFX_GL_VertexArray_Bind(&r->surface_format);

    GLboolean blend = glIsEnabled(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (!blend) {
        glEnable(GL_BLEND);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GLboolean depth_test = glIsEnabled(GL_DEPTH_TEST);
    if (depth_test) {
        glDisable(GL_DEPTH_TEST);
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);
    GFX_GL_CheckError();

    if (!blend) {
        glDisable(GL_BLEND);
    }
    if (depth_test) {
        glEnable(GL_DEPTH_TEST);
    }
}
