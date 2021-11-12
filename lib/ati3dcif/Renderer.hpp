#pragma once

#include "State.hpp"
#include "Texture.hpp"
#include "VertexStream.hpp"
#include "TransDelay.hpp"

#include <glrage/GLRage.hpp>
#include <glrage_gl/Program.hpp>
#include <glrage_gl/Sampler.hpp>
#include <glrage_gl/Shader.hpp>
#include <glrage_util/Config.hpp>

#include <array>
#include <map>
#include <memory>

namespace glrage {
namespace cif {

// ATI3DCIF -> OpenGL mapping tables
static const GLenum GLCIF_DEPTH_MASK[] = {
    GL_FALSE, // C3D_EZMODE_OFF (ignore z)
    GL_FALSE, // C3D_EZMODE_TESTON (test z, but do not update the z buffer)
    GL_TRUE   // C3D_EZMODE_TESTON_WRITEZ (test z and update the z buffer)
};

static const GLenum GLCIF_DEPTH_FUNC[] = {
    GL_NEVER,    // C3D_EZCMP_NEVER
    GL_LESS,     // C3D_EZCMP_LESS
    GL_LEQUAL,   // C3D_EZCMP_LEQUAL
    GL_EQUAL,    // C3D_EZCMP_EQUAL
    GL_GEQUAL,   // C3D_EZCMP_GEQUAL
    GL_GREATER,  // C3D_EZCMP_GREATER
    GL_NOTEQUAL, // C3D_EZCMP_NOTEQUAL
    GL_ALWAYS    // C3D_EZCMP_ALWAYS
};

static const GLenum GLCIF_BLEND_FUNC[] = {
    GL_ZERO,                // C3D_EASRC_ZERO / C3D_EADST_ZERO
    GL_ONE,                 // C3D_EASRC_ONE / C3D_EADST_ONE
    GL_DST_COLOR,           // C3D_EASRC_DSTCLR / C3D_EADST_DSTCLR
    GL_ONE_MINUS_DST_COLOR, // C3D_EASRC_INVDSTCLR / C3D_EADST_INVDSTCLR
    GL_SRC_ALPHA,           // C3D_EASRC_SRCALPHA / C3D_EADST_SRCALPHA
    GL_ONE_MINUS_SRC_ALPHA, // C3D_EASRC_INVSRCALPHA / C3D_EADST_INVSRCALPHA
    GL_DST_ALPHA,           // C3D_EASRC_DSTALPHA / C3D_EADST_DSTALPHA
    GL_ONE_MINUS_DST_ALPHA  // C3D_EASRC_INVDSTALPHA / C3D_EADST_INVDSTALPHA
};

static const GLenum GLCIF_TEXTURE_MIN_FILTER[] = {
    GL_NEAREST,               // C3D_ETFILT_MINPNT_MAGPNT
    GL_LINEAR,                // C3D_ETFILT_MINPNT_MAG2BY2
    GL_LINEAR,                // C3D_ETFILT_MIN2BY2_MAG2BY2
    GL_LINEAR_MIPMAP_LINEAR,  // C3D_ETFILT_MIPLIN_MAGPNT
    GL_LINEAR_MIPMAP_NEAREST, // C3D_ETFILT_MIPLIN_MAG2BY2
    GL_LINEAR_MIPMAP_LINEAR,  // C3D_ETFILT_MIPTRI_MAG2BY2
    GL_NEAREST                // C3D_ETFILT_MIN2BY2_MAGPNT
};

static const GLenum GLCIF_TEXTURE_MAG_FILTER[] = {
    GL_NEAREST, // C3D_ETFILT_MINPNT_MAGPNT (pick nearest texel (pnt) min/mag)
    GL_NEAREST, // C3D_ETFILT_MINPNT_MAG2BY2 (pnt min/bi-linear mag)
    GL_LINEAR,  // C3D_ETFILT_MIN2BY2_MAG2BY2 (2x2 blend min/bi-linear mag)
    GL_NEAREST, // C3D_ETFILT_MIPLIN_MAGPNT (1x1 blend min(between maps)/pick
                // nearest mag)
    GL_LINEAR,  // C3D_ETFILT_MIPLIN_MAG2BY2 (1x1 blend min(between
                // maps)/bi-linear mag)
    GL_LINEAR,  // C3D_ETFILT_MIPTRI_MAG2BY2 (Rage3: (2x2)x(2x2)(between
                // maps)/bi-linear mag)
    GL_LINEAR   // C3D_ETFILT_MIN2BY2_MAGPNT (Rage3:2x2 blend min/pick nearest
                // mag)
};

class Renderer
{
public:
    Renderer();
    void renderBegin(C3D_HRC);
    void renderEnd();
    void textureReg(C3D_PTMAP, C3D_PHTX);
    void textureUnreg(C3D_HTX);
    void texturePaletteCreate(C3D_ECI_TMAP_TYPE, void*, C3D_PHTXPAL);
    void texturePaletteDestroy(C3D_HTXPAL);
    void texturePaletteAnimate(
        C3D_HTXPAL, C3D_UINT32, C3D_UINT32, C3D_PPALETTENTRY);
    void renderPrimStrip(C3D_VSTRIP, C3D_UINT32);
    void renderPrimList(C3D_VLIST, C3D_UINT32);
    void setState(C3D_ERSID eRStateID, C3D_PRSDATA pRStateData);
    void resetState();

private:
    // state functions start
    void switchState(StateVar::Value& value);
    void vertexType(StateVar::Value& value);
    void primType(StateVar::Value& value);
    void solidColor(StateVar::Value& value);
    void shadeMode(StateVar::Value& value);
    void tmapEnable(StateVar::Value& value);
    void tmapSelect(StateVar::Value& value);
    void tmapLight(StateVar::Value& value);
    void tmapFilter(StateVar::Value& value);
    void tmapTexOp(StateVar::Value& value);
    void alphaSrc(StateVar::Value& value);
    void alphaDst(StateVar::Value& value);
    void zCmpFunc(StateVar::Value& value);
    void zMode(StateVar::Value& value);
    // state functions end

    void tmapSelectImpl(C3D_HTX handle);
    void tmapRestore();

    Context& m_context{GLRage::getContext()};
    Config& m_config{GLRage::getConfig()};
    bool m_wireframe;
    std::map<C3D_HTX, std::shared_ptr<Texture>> m_textures;
    std::map<C3D_HTXPAL, std::vector<C3D_PALETTENTRY>> m_palettes;
    int32_t m_paletteID{0};
    gl::Program m_program;
    gl::Sampler m_sampler;
    VertexStream m_vertexStream;
    State m_state;
    TransDelay m_transDelay;
};

} // namespace cif
} // namespace glrage
