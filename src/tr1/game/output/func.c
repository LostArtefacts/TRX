#include "game/output.h"
#include "game/output/meshes/common.h"
#include "game/output/sprites.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/gfx/gl/utils.h>

#include <math.h>

static struct {
    GLint bound_program;
    GLint bound_vao;
    GLint bound_vbo;
    GLint bound_active_texture;
    GLint bound_texture;
    GLint bound_polygon_mode[2];
} m_CachedState;

void Output_ObserveFOVChange(void)
{
    Output_Meshes_UploadProjectionMatrix();
    Output_Sprites_UploadProjectionMatrix();
}

bool Output_MakeScreenshot(const char *const path)
{
    GFX_Context_ScheduleScreenshot(path);
    return true;
}

int32_t Output_GetObjectBounds(const BOUNDS_16 *const bounds)
{
    // TODO: remove
    return 1;
}

int32_t Output_CalcFogShade(const int32_t depth)
{
    // TODO: done in the shader
    return 0;
}

int32_t Output_GetRoomLightShade(const ROOM_LIGHT_MODE mode)
{
    // TODO: remove
    ASSERT_FAIL();
    return 0;
}

void Output_LightRoomVertices(const ROOM *const room)
{
    // TODO: remove
    ASSERT_FAIL();
}

void Output_GetProjectionMatrix(GLfloat output[][4])
{
    const float left = 0.0f;
    const float top = 0.0f;
    const float right = Viewport_GetWidth();
    const float bottom = Viewport_GetHeight();
    const float near = Output_GetNearZ() / (float)(1 << W2V_SHIFT);
    const float far = Output_GetFarZ() / (float)(1 << W2V_SHIFT);
    const float aspect = (float)right / (float)bottom;
    const float fov = Viewport_GetFOV() * M_PI / (float)DEG_180;
    const float f = 1.0f / tan(fov / 2.0f);

    output[0][0] = f / aspect;
    output[0][1] = 0.0f;
    output[0][2] = 0.0f;
    output[0][3] = 0.0f;

    output[1][0] = 0.0f;
    output[1][1] = -f;
    output[1][2] = 0.0f;
    output[1][3] = 0.0f;

    output[2][0] = 0.0f;
    output[2][1] = 0.0f;
    output[2][2] = g_FltResZBuf;
    output[2][3] = -g_FltResZ / (float)(1 << W2V_SHIFT);

    output[3][0] = 0.0f;
    output[3][1] = 0.0f;
    output[3][2] = 1.0f;
    output[3][3] = 0.0f;
}

void Output_EnableScissor(
    const float x, const float y, const float w, const float h)
{
    // Causes the rendering pipeline to discard every pixel outside of the
    // specified window. The window is in game framebuffer viewport's
    // coordinates; to make it work properly, we need to translate it to the
    // SDL window coordinates first.

    const int32_t border = 2; // to deal with precision issues

    struct {
        GLint x, y, w, h;
    } game_viewport = {
        .x = Viewport_GetMinX(),
        .y = Viewport_GetMinY(),
        .w = Viewport_GetWidth(),
        .h = Viewport_GetHeight(),
    }, gl_viewport, scissor;
    glGetIntegerv(GL_VIEWPORT, &gl_viewport.x);

    const float scale_x = gl_viewport.w / (float)game_viewport.w;
    const float scale_y = gl_viewport.h / (float)game_viewport.h;
    scissor.x = gl_viewport.x + (x * scale_x) - border;
    scissor.y = gl_viewport.y + (game_viewport.h - y) * scale_y - border;
    scissor.w = w * scale_x + border * 2;
    scissor.h = h * scale_y + border * 2;

    glEnable(GL_SCISSOR_TEST);
    glScissor(scissor.x, scissor.y, scissor.w, scissor.h);
}

void Output_DisableScissor(void)
{
    glDisable(GL_SCISSOR_TEST);
}

void Output_RememberState(void)
{
    glGetIntegerv(GL_CURRENT_PROGRAM, &m_CachedState.bound_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_CachedState.bound_vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &m_CachedState.bound_vbo);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &m_CachedState.bound_active_texture);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_CachedState.bound_texture);
    glGetIntegerv(GL_POLYGON_MODE, &m_CachedState.bound_polygon_mode[0]);
    GFX_GL_CheckError();
}

void Output_RestoreState(void)
{
    glUseProgram(m_CachedState.bound_program);
    glBindVertexArray(m_CachedState.bound_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_CachedState.bound_vbo);
    glActiveTexture(m_CachedState.bound_active_texture);
    glBindTexture(GL_TEXTURE_2D, m_CachedState.bound_texture);
    glPolygonMode(GL_FRONT_AND_BACK, m_CachedState.bound_polygon_mode[0]);
    GFX_GL_CheckError();
}
