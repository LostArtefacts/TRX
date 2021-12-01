#include "ati3dcif/Renderer.hpp"

#include "ati3dcif/Error.hpp"
#include "glrage_gl/Utils.hpp"

namespace glrage {
namespace cif {

Renderer::Renderer()
{
    // The vertex stream passes primitives to the delayer to test if they
    // have translucency and in that case delay them. When the delayer needs
    // to display the delayed primitives it calls back to the vertex stream
    // to do so.
    m_vertexStream.setDelayer(
        [this](C3D_VTCF *verts) { return m_transDelay.delayTriangle(verts); });

    // bind sampler
    m_sampler.bind(0);

    // improve texture filtering quality
    // TODO: make me configurable
    float filterAniso = 16.0f;
    if (filterAniso > 0) {
        m_sampler.parameterf(GL_TEXTURE_MAX_ANISOTROPY_EXT, filterAniso);
    }

    // compile and link shaders and configure program
    std::string basePath = m_context.getBasePath();
    m_program.attach(gl::Shader(GL_VERTEX_SHADER)
                         .fromFile(basePath + "\\shaders\\ati3dcif.vsh"));
    m_program.attach(gl::Shader(GL_FRAGMENT_SHADER)
                         .fromFile(basePath + "\\shaders\\ati3dcif.fsh"));
    m_program.link();
    m_program.fragmentData("fragColor");
    m_program.bind();

    // negate Z axis so the model is rendered behind the viewport, which is
    // better than having a negative z_near in the ortho matrix, which seems
    // to mess up depth testing
    GLfloat model_view[4][4] = {
        { +1.0f, +0.0f, +0.0, +0.0f },
        { +0.0f, +1.0f, +0.0, +0.0f },
        { +0.0f, +0.0f, -1.0, -0.0f },
        { +0.0f, +0.0f, +0.0, +1.0f },
    };
    m_program.uniformMatrix4fv("matModelView", 1, GL_FALSE, &model_view[0][0]);

    // TODO: make me configurable
    m_wireframe = false;

    gl::Utils::checkError(__FUNCTION__);
}

void Renderer::renderBegin()
{
    glEnable(GL_BLEND);

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // bind objects
    m_program.bind();
    m_vertexStream.bind();
    m_sampler.bind(0);

    // restore texture binding
    tmapRestore();

    // CIF always uses an orthographic view, the application deals with the
    // perspective when required
    const auto left = 0.0f;
    const auto top = 0.0f;
    const auto right = static_cast<float>(m_context.getDisplayWidth());
    const auto bottom = static_cast<float>(m_context.getDisplayHeight());
    const auto z_near = -1e6;
    const auto z_far = 1e6;
    GLfloat projection[4][4] = {
        { 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
        { 0.0f, 0.0f, -2.0f / (z_far - z_near), 0.0f },
        { -(right + left) / (right - left), -(top + bottom) / (top - bottom),
          -(z_far + z_near) / (z_far - z_near), 1.0f }
    };

    m_program.uniformMatrix4fv("matProjection", 1, GL_FALSE, &projection[0][0]);

    gl::Utils::checkError(__FUNCTION__);
}

void Renderer::renderEnd()
{
    // make sure everything has been rendered
    m_vertexStream.renderPending();
    // including the delayed translucent primitives
    m_program.uniform1i("keyOnAlpha", true);
    m_transDelay.render([this](std::vector<C3D_VTCF> verts) {
        m_vertexStream.renderPrims(verts);
    });

    // restore polygon mode
    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    gl::Utils::checkError(__FUNCTION__);
}

void Renderer::textureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap)
{
    auto texture = std::make_shared<Texture>();
    texture->bind();
    texture->load(ptmapToReg, m_palettes[ptmapToReg->htxpalTexPalette]);

    // use id as texture handle
    *phtmap = reinterpret_cast<C3D_HTX>(texture->id());

    // store in texture map
    m_textures[*phtmap] = texture;

    // restore previously bound texture
    tmapRestore();

    gl::Utils::checkError(__FUNCTION__);
}

void Renderer::textureUnreg(C3D_HTX htxToUnreg)
{
    auto it = m_textures.find(htxToUnreg);
    if (it == m_textures.end()) {
        throw Error("Invalid texture handle", C3D_EC_BADPARAM);
    }

    // unbind texture if currently bound
    if (htxToUnreg == m_tmapSelect) {
        tmapSelect(0);
    }

    std::shared_ptr<Texture> texture = it->second;
    m_textures.erase(htxToUnreg);
}

void Renderer::texturePaletteCreate(
    C3D_ECI_TMAP_TYPE epalette, void *pPalette, C3D_PHTXPAL phtpalCreated)
{
    if (epalette != C3D_ECI_TMAP_8BIT) {
        throw Error(
            "Unsupported palette type: " + std::to_string(epalette),
            C3D_EC_NOTIMPYET);
    }

    // copy palette entries to vector
    auto palettePtr = static_cast<C3D_PPALETTENTRY>(pPalette);
    std::vector<C3D_PALETTENTRY> palette(palettePtr, palettePtr + 256);

    // create new palette handle
    auto handle = reinterpret_cast<C3D_HTXPAL>(m_paletteID++);

    // store palette
    m_palettes[handle] = palette;

    *phtpalCreated = handle;
}

void Renderer::texturePaletteDestroy(C3D_HTXPAL htxpalToDestroy)
{
    m_palettes.erase(htxpalToDestroy);
}

void Renderer::renderPrimStrip(C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert)
{
    m_context.setRendered();
    m_vertexStream.addPrimStrip(vStrip, u32NumVert);
}

void Renderer::renderPrimList(C3D_VLIST vList, C3D_UINT32 u32NumVert)
{
    m_context.setRendered();
    m_vertexStream.addPrimList(vList, u32NumVert);
}

void Renderer::setState(C3D_ERSID eRStateID, C3D_PRSDATA pRStateData)
{
    switch (eRStateID) {
    case C3D_ERS_VERTEX_TYPE:
        vertexType(*reinterpret_cast<C3D_EVERTEX *>(pRStateData));
        break;
    case C3D_ERS_PRIM_TYPE:
        primType(*reinterpret_cast<C3D_EPRIM *>(pRStateData));
        break;
    case C3D_ERS_SOLID_CLR:
        solidColor(*reinterpret_cast<C3D_COLOR *>(pRStateData));
        break;
    case C3D_ERS_SHADE_MODE:
        shadeMode(*reinterpret_cast<C3D_ESHADE *>(pRStateData));
        break;
    case C3D_ERS_TMAP_EN:
        tmapEnable(*reinterpret_cast<C3D_BOOL *>(pRStateData));
        break;
    case C3D_ERS_TMAP_SELECT:
        tmapSelect(*reinterpret_cast<C3D_HTX *>(pRStateData));
        break;
    case C3D_ERS_TMAP_LIGHT:
        tmapLight(*reinterpret_cast<C3D_ETLIGHT *>(pRStateData));
        break;
    case C3D_ERS_TMAP_FILTER:
        tmapFilter(*reinterpret_cast<C3D_ETEXFILTER *>(pRStateData));
        break;
    case C3D_ERS_TMAP_TEXOP:
        tmapTexOp(*reinterpret_cast<C3D_ETEXOP *>(pRStateData));
        break;
    case C3D_ERS_ALPHA_SRC:
        alphaSrc(*reinterpret_cast<C3D_EASRC *>(pRStateData));
        break;
    case C3D_ERS_ALPHA_DST:
        alphaDst(*reinterpret_cast<C3D_EADST *>(pRStateData));
        break;
    case C3D_ERS_Z_CMP_FNC:
        zCmpFunc(*reinterpret_cast<C3D_EZCMP *>(pRStateData));
        break;
    case C3D_ERS_Z_MODE:
        zMode(*reinterpret_cast<C3D_EZMODE *>(pRStateData));
        break;
    default:
        throw Error(
            "Unsupported state: " + std::to_string(eRStateID),
            C3D_EC_NOTIMPYET);
    }
}

void Renderer::vertexType(C3D_EVERTEX value)
{
    m_vertexStream.renderPending();
    m_vertexStream.vertexType(value);
}

void Renderer::primType(C3D_EPRIM value)
{
    m_vertexStream.renderPending();
    m_vertexStream.primType(value);
}

void Renderer::solidColor(C3D_COLOR value)
{
    m_vertexStream.renderPending();
    C3D_COLOR color = value;
    m_program.uniform4f(
        "solidColor", color.r / 255.0f, color.g / 255.0f, color.b / 255.0f,
        color.a / 255.0f);
}

void Renderer::shadeMode(C3D_ESHADE value)
{
    m_vertexStream.renderPending();
    m_program.uniform1i("shadeMode", value);
}

void Renderer::tmapEnable(C3D_BOOL value)
{
    m_vertexStream.renderPending();
    C3D_BOOL enable = value;
    m_program.uniform1i("tmapEn", enable);
    m_transDelay.setTexturingEnabled(enable != C3D_FALSE);
    if (enable) {
        glEnable(GL_TEXTURE_2D);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
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
    if (it == m_textures.end()) {
        throw Error("Invalid texture handle", C3D_EC_BADPARAM);
    }

    // get texture object and bind it
    auto texture = it->second;
    texture->bind();
    // Tell the transparent primitive delayer what texture is currently in
    // use
    m_transDelay.setTexture(texture);

    // send chroma key color to shader
    auto ck = texture->chromaKey();
    m_program.uniform3f(
        "chromaKey", ck.r / 255.0f, ck.g / 255.0f, ck.b / 255.0f);
    m_program.uniform1i("keyOnAlpha", texture->keyOnAlpha());
}

void Renderer::tmapRestore()
{
    tmapSelectImpl(m_tmapSelect);
}

void Renderer::tmapLight(C3D_ETLIGHT value)
{
    m_vertexStream.renderPending();
    m_program.uniform1i("tmapLight", value);
}

void Renderer::tmapFilter(C3D_ETEXFILTER value)
{
    m_vertexStream.renderPending();
    auto filter = value;
    m_sampler.parameteri(
        GL_TEXTURE_MAG_FILTER, GLCIF_TEXTURE_MAG_FILTER[filter]);
    m_sampler.parameteri(
        GL_TEXTURE_MIN_FILTER, GLCIF_TEXTURE_MIN_FILTER[filter]);
}

void Renderer::tmapTexOp(C3D_ETEXOP value)
{
    m_vertexStream.renderPending();
    m_program.uniform1i("texOp", value);
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

void Renderer::zCmpFunc(C3D_EZCMP value)
{
    m_vertexStream.renderPending();
    C3D_EZCMP func = value;
    if (func < C3D_EZCMP_MAX) {
        glDepthFunc(GLCIF_DEPTH_FUNC[func]);
    }
}

void Renderer::zMode(C3D_EZMODE value)
{
    m_vertexStream.renderPending();
    auto mode = value;
    glDepthMask(GLCIF_DEPTH_MASK[mode]);

    if (mode > C3D_EZMODE_TESTON) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

}
}
