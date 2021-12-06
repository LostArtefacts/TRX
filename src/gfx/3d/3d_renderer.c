#include "gfx/3d/3d_renderer.h"

#include "gfx/context.h"
#include "gfx/gl/utils.h"

#include <assert.h>

static const GLenum GLCIF_BLEND_FUNC[] = {
    GL_ZERO, // C3D_EASRC_ZERO / C3D_EADST_ZERO
    GL_ONE, // C3D_EASRC_ONE / C3D_EADST_ONE
    GL_DST_COLOR, // C3D_EASRC_DSTCLR / C3D_EADST_DSTCLR
    GL_ONE_MINUS_DST_COLOR, // C3D_EASRC_INVDSTCLR / C3D_EADST_INVDSTCLR
    GL_SRC_ALPHA, // C3D_EASRC_SRCALPHA / C3D_EADST_SRCALPHA
    GL_ONE_MINUS_SRC_ALPHA, // C3D_EASRC_INVSRCALPHA / C3D_EADST_INVSRCALPHA
    GL_DST_ALPHA, // C3D_EASRC_DSTALPHA / C3D_EADST_DSTALPHA
    GL_ONE_MINUS_DST_ALPHA // C3D_EASRC_INVDSTALPHA / C3D_EADST_INVDSTALPHA
};

static const GLenum GLCIF_TEXTURE_MIN_FILTER[] = {
    GL_NEAREST, // C3D_ETFILT_MINPNT_MAGPNT
    GL_LINEAR, // C3D_ETFILT_MINPNT_MAG2BY2
    GL_LINEAR, // C3D_ETFILT_MIN2BY2_MAG2BY2
    GL_LINEAR_MIPMAP_LINEAR, // C3D_ETFILT_MIPLIN_MAGPNT
    GL_LINEAR_MIPMAP_NEAREST, // C3D_ETFILT_MIPLIN_MAG2BY2
    GL_LINEAR_MIPMAP_LINEAR, // C3D_ETFILT_MIPTRI_MAG2BY2
    GL_NEAREST // C3D_ETFILT_MIN2BY2_MAGPNT
};

static const GLenum GLCIF_TEXTURE_MAG_FILTER[] = {
    GL_NEAREST, // C3D_ETFILT_MINPNT_MAGPNT (pick nearest texel (pnt)
                // min/mag)
    GL_NEAREST, // C3D_ETFILT_MINPNT_MAG2BY2 (pnt min/bi-linear mag)
    GL_LINEAR, // C3D_ETFILT_MIN2BY2_MAG2BY2 (2x2 blend min/bi-linear mag)
    GL_NEAREST, // C3D_ETFILT_MIPLIN_MAGPNT (1x1 blend min(between
                // maps)/pick nearest mag)
    GL_LINEAR, // C3D_ETFILT_MIPLIN_MAG2BY2 (1x1 blend min(between
               // maps)/bi-linear mag)
    GL_LINEAR, // C3D_ETFILT_MIPTRI_MAG2BY2 (Rage3: (2x2)x(2x2)(between
               // maps)/bi-linear mag)
    GL_LINEAR // C3D_ETFILT_MIN2BY2_MAGPNT (Rage3:2x2 blend min/pick nearest
              // mag)
};

static void GFX_3D_Renderer_TmapSelectImpl(
    GFX_3D_Renderer *renderer, int texture_num);

static void GFX_3D_Renderer_TmapSelectImpl(
    GFX_3D_Renderer *renderer, int texture_num)
{
    if (texture_num == GFX_NO_TEXTURE) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    assert(texture_num >= 0);
    assert(texture_num < GFX_MAX_TEXTURES);

    GFX_GL_Texture *texture = renderer->textures[texture_num];
    GFX_GL_Texture_Bind(texture);
}

void GFX_3D_Renderer_Init(GFX_3D_Renderer *renderer)
{
    // TODO: make me configurable
    renderer->wireframe = false;
    renderer->selected_texture_num = GFX_NO_TEXTURE;
    renderer->easrc = C3D_EASRC_ONE;
    renderer->eadst = C3D_EADST_ZERO;
    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        renderer->textures[i] = NULL;
    }

    GFX_GL_Sampler_Init(&renderer->sampler);
    GFX_GL_Sampler_Bind(&renderer->sampler, 0);
    GFX_GL_Sampler_Parameterf(
        &renderer->sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);

    GFX_GL_Program_Init(&renderer->program);
    GFX_GL_Program_AttachShader(
        &renderer->program, GL_VERTEX_SHADER, "shaders\\ati3dcif.vsh");
    GFX_GL_Program_AttachShader(
        &renderer->program, GL_FRAGMENT_SHADER, "shaders\\ati3dcif.fsh");
    GFX_GL_Program_Link(&renderer->program);

    renderer->loc_mat_projection =
        GFX_GL_Program_UniformLocation(&renderer->program, "matProjection");
    renderer->loc_mat_model_view =
        GFX_GL_Program_UniformLocation(&renderer->program, "matModelView");
    renderer->loc_tmap_en =
        GFX_GL_Program_UniformLocation(&renderer->program, "tmapEn");

    GFX_GL_Program_FragmentData(&renderer->program, "fragColor");
    GFX_GL_Program_Bind(&renderer->program);

    // negate Z axis so the model is rendered behind the viewport, which is
    // better than having a negative z_near in the ortho matrix, which seems
    // to mess up depth testing
    GLfloat model_view[4][4] = {
        { +1.0f, +0.0f, +0.0, +0.0f },
        { +0.0f, +1.0f, +0.0, +0.0f },
        { +0.0f, +0.0f, -1.0, -0.0f },
        { +0.0f, +0.0f, +0.0, +1.0f },
    };
    GFX_GL_Program_UniformMatrix4fv(
        &renderer->program, renderer->loc_mat_model_view, 1, GL_FALSE,
        &model_view[0][0]);

    GFX_3D_VertexStream_Init(&renderer->vertex_stream);

    GFX_GL_CheckError();
}

void GFX_3D_Renderer_Close(GFX_3D_Renderer *renderer)
{
    GFX_3D_VertexStream_Close(&renderer->vertex_stream);
    GFX_GL_Program_Close(&renderer->program);
    GFX_GL_Sampler_Close(&renderer->sampler);
}

void GFX_3D_Renderer_RenderBegin(GFX_3D_Renderer *renderer)
{
    glEnable(GL_BLEND);

    if (renderer->wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    GFX_GL_Program_Bind(&renderer->program);
    GFX_3D_VertexStream_Bind(&renderer->vertex_stream);
    GFX_GL_Sampler_Bind(&renderer->sampler, 0);

    // restore texture binding
    GFX_3D_Renderer_TmapRestore(renderer);

    // CIF always uses an orthographic view, the application deals with the
    // perspective when required
    const float left = 0.0f;
    const float top = 0.0f;
    const float right = GFX_Context_GetDisplayWidth();
    const float bottom = GFX_Context_GetDisplayHeight();
    const float z_near = -1e6;
    const float z_far = 1e6;
    GLfloat projection[4][4] = {
        { 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
        { 0.0f, 0.0f, -2.0f / (z_far - z_near), 0.0f },
        { -(right + left) / (right - left), -(top + bottom) / (top - bottom),
          -(z_far + z_near) / (z_far - z_near), 1.0f }
    };

    GFX_GL_Program_UniformMatrix4fv(
        &renderer->program, renderer->loc_mat_projection, 1, GL_FALSE,
        &projection[0][0]);

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    GFX_GL_CheckError();
}

void GFX_3D_Renderer_RenderEnd(GFX_3D_Renderer *renderer)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);

    if (renderer->wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    GFX_GL_CheckError();
}

int GFX_3D_Renderer_TextureReg(
    GFX_3D_Renderer *renderer, const void *data, int width, int height)
{
    GFX_GL_Texture *texture = GFX_GL_Texture_Create(GL_TEXTURE_2D);
    GFX_GL_Texture_Bind(texture);
    GFX_GL_Texture_Load(texture, data, width, height);

    int texture_num = GFX_NO_TEXTURE;
    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (!renderer->textures[i]) {
            renderer->textures[i] = texture;
            texture_num = i;
            break;
        }
    }

    GFX_3D_Renderer_TmapRestore(renderer);

    GFX_GL_CheckError();
    return texture_num;
}

bool GFX_3D_Renderer_TextureUnreg(GFX_3D_Renderer *renderer, int texture_num)
{
    assert(texture_num >= 0);
    assert(texture_num < GFX_MAX_TEXTURES);

    GFX_GL_Texture *texture = renderer->textures[texture_num];
    if (!texture) {
        LOG_ERROR("Invalid texture handle");
        return false;
    }

    // unbind texture if currently bound
    if (texture_num == renderer->selected_texture_num) {
        GFX_3D_Renderer_TmapSelectImpl(renderer, GFX_NO_TEXTURE);
        renderer->selected_texture_num = GFX_NO_TEXTURE;
    }

    GFX_GL_Texture_Free(texture);
    renderer->textures[texture_num] = NULL;
    return true;
}

void GFX_3D_Renderer_RenderPrimStrip(
    GFX_3D_Renderer *renderer, C3D_VSTRIP strip, int count)
{
    GFX_Context_SetRendered();
    GFX_3D_VertexStream_PushPrimStrip(&renderer->vertex_stream, strip, count);
}

void GFX_3D_Renderer_RenderPrimList(
    GFX_3D_Renderer *renderer, C3D_VLIST list, int count)
{
    GFX_Context_SetRendered();
    GFX_3D_VertexStream_PushPrimList(&renderer->vertex_stream, list, count);
}

bool GFX_3D_Renderer_SetState(
    GFX_3D_Renderer *renderer, C3D_ERSID eRStateID, C3D_PRSDATA pRStateData)
{
    switch (eRStateID) {
    case C3D_ERS_PRIM_TYPE:
        GFX_3D_Renderer_SetPrimType(renderer, *((C3D_EPRIM *)pRStateData));
        break;
    case C3D_ERS_TMAP_EN:
        GFX_3D_Renderer_TmapEnable(renderer, *((bool *)pRStateData));
        break;
    case C3D_ERS_TMAP_SELECT:
        GFX_3D_Renderer_TmapSelect(renderer, *((int *)pRStateData));
        break;
    case C3D_ERS_TMAP_FILTER:
        GFX_3D_Renderer_TmapFilter(renderer, *((C3D_ETEXFILTER *)pRStateData));
        break;
    case C3D_ERS_ALPHA_SRC:
        GFX_3D_Renderer_SetAlphaSrc(renderer, *((C3D_EASRC *)pRStateData));
        break;
    case C3D_ERS_ALPHA_DST:
        GFX_3D_Renderer_SetAlphaDst(renderer, *((C3D_EADST *)pRStateData));
        break;
    default:
        LOG_ERROR("Unsupported state: %d", eRStateID);
        return false;
    }
    return true;
}

void GFX_3D_Renderer_TmapEnable(GFX_3D_Renderer *renderer, bool is_enabled)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    GFX_GL_Program_Uniform1i(
        &renderer->program, renderer->loc_tmap_en, is_enabled);
}

void GFX_3D_Renderer_TmapSelect(GFX_3D_Renderer *renderer, int texture_num)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    renderer->selected_texture_num = texture_num;
    GFX_3D_Renderer_TmapSelectImpl(renderer, texture_num);
}

void GFX_3D_Renderer_TmapRestore(GFX_3D_Renderer *renderer)
{
    GFX_3D_Renderer_TmapSelectImpl(renderer, renderer->selected_texture_num);
}

void GFX_3D_Renderer_TmapFilter(
    GFX_3D_Renderer *renderer, C3D_ETEXFILTER filter)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_MAG_FILTER,
        GLCIF_TEXTURE_MAG_FILTER[filter]);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_MIN_FILTER,
        GLCIF_TEXTURE_MIN_FILTER[filter]);
}

void GFX_3D_Renderer_SetPrimType(GFX_3D_Renderer *renderer, C3D_EPRIM value)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    GFX_3D_VertexStream_SetPrimType(&renderer->vertex_stream, value);
}

void GFX_3D_Renderer_SetAlphaSrc(GFX_3D_Renderer *renderer, C3D_EASRC value)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    renderer->easrc = value;
    C3D_EASRC alphaSrc = value;
    C3D_EADST alphaDst = renderer->eadst;
    glBlendFunc(GLCIF_BLEND_FUNC[alphaSrc], GLCIF_BLEND_FUNC[alphaDst]);
}

void GFX_3D_Renderer_SetAlphaDst(GFX_3D_Renderer *renderer, C3D_EADST value)
{
    GFX_3D_VertexStream_RenderPending(&renderer->vertex_stream);
    renderer->eadst = value;
    C3D_EASRC alphaSrc = renderer->easrc;
    C3D_EADST alphaDst = value;
    glBlendFunc(GLCIF_BLEND_FUNC[alphaSrc], GLCIF_BLEND_FUNC[alphaDst]);
}
