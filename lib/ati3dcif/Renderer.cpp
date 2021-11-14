#include "Renderer.hpp"
#include "Error.hpp"
#include "Utils.hpp"

#include <glrage_gl/Utils.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace glrage {
namespace cif {

using std::placeholders::_1;

Renderer::Renderer()
{
    // The vertex stream passes primitives to the delayer to test if they have translucency and
    // in that case delay them. When the delayer needs to display the delayed primitives it calls
    // back to the vertex stream to do so.
    m_vertexStream.setDelayer([this](C3D_VTCF *verts) {return m_transDelay.delayTriangle(verts);});
    // register state observers
    // clang-format off
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_VERTEX_TYPE);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_PRIM_TYPE);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_SOLID_CLR);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_SHADE_MODE);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_TMAP_EN);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_TMAP_SELECT);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_TMAP_LIGHT);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_TMAP_FILTER);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_TMAP_TEXOP);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_ALPHA_SRC);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_ALPHA_DST);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_Z_CMP_FNC);
    m_state.registerObserver(std::bind(&Renderer::switchState, this, _1), C3D_ERS_Z_MODE);

    m_state.registerObserver(std::bind(&Renderer::vertexType, this, _1), C3D_ERS_VERTEX_TYPE);
    m_state.registerObserver(std::bind(&Renderer::primType, this, _1), C3D_ERS_PRIM_TYPE);
    m_state.registerObserver(std::bind(&Renderer::solidColor, this, _1), C3D_ERS_SOLID_CLR);
    m_state.registerObserver(std::bind(&Renderer::shadeMode, this, _1), C3D_ERS_SHADE_MODE);
    m_state.registerObserver(std::bind(&Renderer::tmapEnable, this, _1), C3D_ERS_TMAP_EN);
    m_state.registerObserver(std::bind(&Renderer::tmapSelect, this, _1), C3D_ERS_TMAP_SELECT);
    m_state.registerObserver(std::bind(&Renderer::tmapLight, this, _1), C3D_ERS_TMAP_LIGHT);
    m_state.registerObserver(std::bind(&Renderer::tmapFilter, this, _1), C3D_ERS_TMAP_FILTER);
    m_state.registerObserver(std::bind(&Renderer::tmapTexOp, this, _1), C3D_ERS_TMAP_TEXOP);
    m_state.registerObserver(std::bind(&Renderer::alphaSrc, this, _1), C3D_ERS_ALPHA_SRC);
    m_state.registerObserver(std::bind(&Renderer::alphaDst, this, _1), C3D_ERS_ALPHA_DST);
    m_state.registerObserver(std::bind(&Renderer::zCmpFunc, this, _1), C3D_ERS_Z_CMP_FNC);
    m_state.registerObserver(std::bind(&Renderer::zMode, this, _1), C3D_ERS_Z_MODE);
    // clang-format on

    // bind sampler
    m_sampler.bind(0);

    // improve texture filtering quality
    float filterAniso = m_config.getFloat("ati3dcif.filter_anisotropy", 16.0f);
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
    // better
    // than having a negative zNear in the ortho matrix, which seems to mess up
    // depth testing
    auto modelView = glm::scale(glm::mat4(), glm::vec3(1, 1, -1));
    m_program.uniformMatrix4fv(
        "matModelView", 1, GL_FALSE, glm::value_ptr(modelView));

    // cache frequently used config values
    m_wireframe = m_config.getBool("ati3dcif.wireframe", false);

    // apply default state
    resetState();

    gl::Utils::checkError(__FUNCTION__);
}

void Renderer::renderBegin(C3D_HRC hRC)
{
    glEnable(GL_BLEND);

    // set wireframe mode if set
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
    auto width = static_cast<float>(m_context.getDisplayWidth());
    auto height = static_cast<float>(m_context.getDisplayHeight());
    auto projection = glm::ortho<float>(0, width, height, 0, -1e6, 1e6);
    m_program.uniformMatrix4fv(
        "matProjection", 1, GL_FALSE, glm::value_ptr(projection));

    gl::Utils::checkError(__FUNCTION__);
}

void Renderer::renderEnd()
{
    // make sure everything has been rendered
    m_vertexStream.renderPending();
    // including the delayed translucent primitives
    m_program.uniform1i("keyOnAlpha", true);
    m_transDelay.render([this](std::vector<C3D_VTCF> verts) {m_vertexStream.renderPrims(verts);});

    // restore polygon mode
    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    gl::Utils::checkError(__FUNCTION__);
}

void Renderer::textureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap)
{
    // LOG_TRACE("fmt=%d, xlg2=%d, ylg2=%d, mip=%d",
    //    ptmapToReg->eTexFormat, ptmapToReg->u32MaxMapXSizeLg2,
    //    ptmapToReg->u32MaxMapYSizeLg2, ptmapToReg->bMipMap);

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
    // LOG_TRACE("id=%d", id);

    auto it = m_textures.find(htxToUnreg);
    if (it == m_textures.end()) {
        throw Error("Invalid texture handle", C3D_EC_BADPARAM);
    }

    // unbind texture if currently bound
    if (htxToUnreg == m_state.get(C3D_ERS_TMAP_SELECT).htx) {
        m_state.set(C3D_ERS_TMAP_SELECT, StateVar::Value{0});
    }

    std::shared_ptr<Texture> texture = it->second;
    m_textures.erase(htxToUnreg);
}

void Renderer::texturePaletteCreate(
    C3D_ECI_TMAP_TYPE epalette, void* pPalette, C3D_PHTXPAL phtpalCreated)
{
    if (epalette != C3D_ECI_TMAP_8BIT) {
        throw Error("Unsupported palette type: " +
                        std::string(C3D_ECI_TMAP_TYPE_NAMES[epalette]),
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

void Renderer::texturePaletteAnimate(C3D_HTXPAL htxpalToAnimate,
    C3D_UINT32 u32StartIndex, C3D_UINT32 u32NumEntries,
    C3D_PPALETTENTRY pclrPalette)
{
    throw Error(std::string(__FUNCTION__) + ": Not implemented", C3D_EC_NOTIMPYET);
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
    m_state.set(eRStateID, pRStateData);
}

void Renderer::resetState()
{
    m_state.reset();
}

void Renderer::switchState(StateVar::Value& value)
{
    // render pending polygons from the previous state
    m_vertexStream.renderPending();
}

void Renderer::vertexType(StateVar::Value& value)
{
    m_vertexStream.vertexType(value.evertex);
}

void Renderer::primType(StateVar::Value& value)
{
    m_vertexStream.primType(value.eprim);
}

void Renderer::solidColor(StateVar::Value& value)
{
    C3D_COLOR color = value.color;
    m_program.uniform4f("solidColor", color.r / 255.0f, color.g / 255.0f,
        color.b / 255.0f, color.a / 255.0f);
}

void Renderer::shadeMode(StateVar::Value& value)
{
    m_program.uniform1i("shadeMode", value.eshade);
}

void Renderer::tmapEnable(StateVar::Value& value)
{
    C3D_BOOL enable = value.boolean;
    m_program.uniform1i("tmapEn", enable);
    m_transDelay.setTexturingEnabled(enable != C3D_FALSE);
    if (enable) {
        glEnable(GL_TEXTURE_2D);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
}

void Renderer::tmapSelect(StateVar::Value& value)
{
    tmapSelectImpl(value.htx);
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
    // Tell the transparent primitive delayer what texture is currently in use
    m_transDelay.setTexture(texture);

    // send chroma key color to shader
    auto ck = texture->chromaKey();
    m_program.uniform3f(
        "chromaKey", ck.r / 255.0f, ck.g / 255.0f, ck.b / 255.0f);
    m_program.uniform1i("keyOnAlpha", texture->keyOnAlpha());
}

void Renderer::tmapRestore() {
    tmapSelectImpl(m_state.get(C3D_ERS_TMAP_SELECT).htx);
}

void Renderer::tmapLight(StateVar::Value& value)
{
    m_program.uniform1i("tmapLight", value.etlight);
}

void Renderer::tmapFilter(StateVar::Value& value)
{
    auto filter = value.etexfilter;
    m_sampler.parameteri(
        GL_TEXTURE_MAG_FILTER, GLCIF_TEXTURE_MAG_FILTER[filter]);
    m_sampler.parameteri(
        GL_TEXTURE_MIN_FILTER, GLCIF_TEXTURE_MIN_FILTER[filter]);
}

void Renderer::tmapTexOp(StateVar::Value& value)
{
    m_program.uniform1i("texOp", value.etexop);
}

void Renderer::alphaSrc(StateVar::Value& value)
{
    C3D_EASRC alphaSrc = value.easrc;
    C3D_EADST alphaDst = m_state.get(C3D_ERS_ALPHA_DST).eadst;
    glBlendFunc(GLCIF_BLEND_FUNC[alphaSrc], GLCIF_BLEND_FUNC[alphaDst]);
}

void Renderer::alphaDst(StateVar::Value& value)
{
    C3D_EASRC alphaSrc =  m_state.get(C3D_ERS_ALPHA_SRC).easrc;
    C3D_EADST alphaDst = value.eadst;
    glBlendFunc(GLCIF_BLEND_FUNC[alphaSrc], GLCIF_BLEND_FUNC[alphaDst]);
}

void Renderer::zCmpFunc(StateVar::Value& value)
{
    C3D_EZCMP func = value.ezcmp;
    if (func < C3D_EZCMP_MAX) {
        glDepthFunc(GLCIF_DEPTH_FUNC[func]);
    }
}

void Renderer::zMode(StateVar::Value& value)
{
    auto mode = value.ezmode;
    glDepthMask(GLCIF_DEPTH_MASK[mode]);

    if (mode > C3D_EZMODE_TESTON) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

} // namespace cif
} // namespace glrage
