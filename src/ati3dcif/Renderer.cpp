#include "ati3dcif/Renderer.hpp"

#include "gfx/context.h"
#include "gfx/gl/utils.h"

#include <cassert>

namespace glrage {
namespace cif {

Renderer::Renderer()
{
    GFX_GL_Sampler_Init(&m_sampler);
    GFX_GL_Sampler_Bind(&m_sampler, 0);
    GFX_GL_Sampler_Parameterf(&m_sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);

    GFX_GL_Program_Init(&m_program);
    GFX_GL_Program_AttachShader(
        &m_program, GL_VERTEX_SHADER, "shaders\\ati3dcif.vsh");
    GFX_GL_Program_AttachShader(
        &m_program, GL_FRAGMENT_SHADER, "shaders\\ati3dcif.fsh");
    GFX_GL_Program_Link(&m_program);

    m_loc_matProjection =
        GFX_GL_Program_UniformLocation(&m_program, "matProjection");
    m_loc_matModelView =
        GFX_GL_Program_UniformLocation(&m_program, "matModelView");
    m_loc_tmapEn = GFX_GL_Program_UniformLocation(&m_program, "tmapEn");

    GFX_GL_Program_FragmentData(&m_program, "fragColor");
    GFX_GL_Program_Bind(&m_program);

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
        &m_program, m_loc_matModelView, 1, GL_FALSE, &model_view[0][0]);

    // TODO: make me configurable
    m_wireframe = false;

    GFX_GL_CheckError();
}

Renderer::~Renderer()
{
    GFX_GL_Program_Close(&m_program);
    GFX_GL_Sampler_Close(&m_sampler);
}

void Renderer::renderBegin()
{
    glEnable(GL_BLEND);

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    GFX_GL_Program_Bind(&m_program);
    m_vertexStream.bind();
    GFX_GL_Sampler_Bind(&m_sampler, 0);

    // restore texture binding
    tmapRestore();

    // CIF always uses an orthographic view, the application deals with the
    // perspective when required
    const auto left = 0.0f;
    const auto top = 0.0f;
    const auto right = static_cast<float>(GFX_Context_GetDisplayWidth());
    const auto bottom = static_cast<float>(GFX_Context_GetDisplayHeight());
    const auto z_near = -1e6;
    const auto z_far = 1e6;
    GLfloat projection[4][4] = {
        { 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
        { 0.0f, 0.0f, -2.0f / (z_far - z_near), 0.0f },
        { -(right + left) / (right - left), -(top + bottom) / (top - bottom),
          -(z_far + z_near) / (z_far - z_near), 1.0f }
    };

    GFX_GL_Program_UniformMatrix4fv(
        &m_program, m_loc_matProjection, 1, GL_FALSE, &projection[0][0]);

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    GFX_GL_CheckError();
}

void Renderer::renderEnd()
{
    m_vertexStream.renderPending();

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    GFX_GL_CheckError();
}

bool Renderer::textureReg(
    const void *data, int width, int height, C3D_PHTX phtmap)
{
    auto texture = std::make_shared<Texture>();
    texture->bind();
    if (!texture->load(data, width, height)) {
        return false;
    }

    // use id as texture handle
    *phtmap = reinterpret_cast<C3D_HTX>(texture->id());

    // store in texture map
    m_textures[*phtmap] = texture;

    // restore previously bound texture
    tmapRestore();

    GFX_GL_CheckError();
    return true;
}

bool Renderer::textureUnreg(C3D_HTX htxToUnreg)
{
    auto it = m_textures.find(htxToUnreg);
    if (it == m_textures.end()) {
        LOG_ERROR("Invalid texture handle");
        return false;
    }

    // unbind texture if currently bound
    if (htxToUnreg == m_tmapSelect) {
        tmapSelect(0);
    }

    std::shared_ptr<Texture> texture = it->second;
    m_textures.erase(htxToUnreg);
    return true;
}

void Renderer::renderPrimStrip(C3D_VSTRIP vStrip, int u32NumVert)
{
    GFX_Context_SetRendered();
    m_vertexStream.addPrimStrip(vStrip, u32NumVert);
}

void Renderer::renderPrimList(C3D_VLIST vList, int u32NumVert)
{
    GFX_Context_SetRendered();
    m_vertexStream.addPrimList(vList, u32NumVert);
}

bool Renderer::setState(C3D_ERSID eRStateID, C3D_PRSDATA pRStateData)
{
    switch (eRStateID) {
    case C3D_ERS_PRIM_TYPE:
        primType(*reinterpret_cast<C3D_EPRIM *>(pRStateData));
        break;
    case C3D_ERS_TMAP_EN:
        tmapEnable(*reinterpret_cast<bool *>(pRStateData));
        break;
    case C3D_ERS_TMAP_SELECT:
        tmapSelect(*reinterpret_cast<C3D_HTX *>(pRStateData));
        break;
    case C3D_ERS_TMAP_FILTER:
        tmapFilter(*reinterpret_cast<C3D_ETEXFILTER *>(pRStateData));
        break;
    case C3D_ERS_ALPHA_SRC:
        alphaSrc(*reinterpret_cast<C3D_EASRC *>(pRStateData));
        break;
    case C3D_ERS_ALPHA_DST:
        alphaDst(*reinterpret_cast<C3D_EADST *>(pRStateData));
        break;
    default:
        LOG_ERROR("Unsupported state: %d", eRStateID);
        return false;
    }
    return true;
}

void Renderer::primType(C3D_EPRIM value)
{
    m_vertexStream.renderPending();
    m_vertexStream.primType(value);
}

void Renderer::tmapEnable(bool value)
{
    m_vertexStream.renderPending();
    bool enable = value;
    GFX_GL_Program_Uniform1i(&m_program, m_loc_tmapEn, enable);
}

void Renderer::tmapSelect(C3D_HTX value)
{
    m_vertexStream.renderPending();
    m_tmapSelect = value;
    tmapSelectImpl(value);
}

void Renderer::tmapSelectImpl(C3D_HTX handle)
{
    // unselect texture if handle is zero
    if (handle == 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    // check if handle is correct
    auto it = m_textures.find(handle);
    assert(it != m_textures.end());

    // get texture object and bind it
    auto texture = it->second;
    texture->bind();
}

void Renderer::tmapRestore()
{
    tmapSelectImpl(m_tmapSelect);
}

void Renderer::tmapFilter(C3D_ETEXFILTER value)
{
    m_vertexStream.renderPending();
    auto filter = value;
    GFX_GL_Sampler_Parameteri(
        &m_sampler, GL_TEXTURE_MAG_FILTER, GLCIF_TEXTURE_MAG_FILTER[filter]);
    GFX_GL_Sampler_Parameteri(
        &m_sampler, GL_TEXTURE_MIN_FILTER, GLCIF_TEXTURE_MIN_FILTER[filter]);
}

void Renderer::alphaSrc(C3D_EASRC value)
{
    m_vertexStream.renderPending();
    m_easrc = value;
    C3D_EASRC alphaSrc = value;
    C3D_EADST alphaDst = m_eadst;
    glBlendFunc(GLCIF_BLEND_FUNC[alphaSrc], GLCIF_BLEND_FUNC[alphaDst]);
}

void Renderer::alphaDst(C3D_EADST value)
{
    m_vertexStream.renderPending();
    m_eadst = value;
    C3D_EASRC alphaSrc = m_easrc;
    C3D_EADST alphaDst = value;
    glBlendFunc(GLCIF_BLEND_FUNC[alphaSrc], GLCIF_BLEND_FUNC[alphaDst]);
}

}
}
