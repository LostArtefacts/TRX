#pragma once

#include "ati3dcif/Texture.hpp"
#include "ati3dcif/VertexStream.hpp"
#include "gfx/gl/program.h"
#include "gfx/gl/sampler.h"

#include <array>
#include <map>
#include <memory>

namespace glrage {
namespace cif {

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

class Renderer {
public:
    Renderer();
    ~Renderer();

    void renderBegin();
    void renderEnd();
    bool textureReg(C3D_PTMAP, C3D_PHTX);
    bool textureUnreg(C3D_HTX);
    void renderPrimStrip(C3D_VSTRIP, int);
    void renderPrimList(C3D_VLIST, int);
    bool setState(C3D_ERSID eRStateID, C3D_PRSDATA pRStateData);
    void resetState();

private:
    // state functions start
    void primType(C3D_EPRIM value);
    void tmapEnable(bool value);
    void tmapSelect(C3D_HTX value);
    void tmapFilter(C3D_ETEXFILTER value);
    void alphaSrc(C3D_EASRC value);
    void alphaDst(C3D_EADST value);
    // state functions end

    void tmapSelectImpl(C3D_HTX handle);
    void tmapRestore();

    bool m_wireframe = false;
    std::map<C3D_HTX, std::shared_ptr<Texture>> m_textures;
    GFX_GL_Program m_program;
    GFX_GL_Sampler m_sampler;
    VertexStream m_vertexStream;

    // state data
    C3D_HTX m_tmapSelect = NULL;
    C3D_EASRC m_easrc = C3D_EASRC_ONE;
    C3D_EADST m_eadst = C3D_EADST_ZERO;

    // shader variable locations
    GLint m_loc_matProjection;
    GLint m_loc_matModelView;
    GLint m_loc_tmapEn;
};

}
}
