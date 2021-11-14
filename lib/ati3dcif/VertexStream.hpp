#pragma once

#include <ati3dcif/ATI3DCIF.h>

#include "TransDelay.hpp"

#include <glrage_gl/VertexArray.hpp>
#include <glrage_gl/Buffer.hpp>

#include <vector>

namespace glrage {
namespace cif {

static const GLenum GLCIF_PRIM_MODES[] = {
    GL_LINES,     // C3D_EPRIM_LINE
    GL_TRIANGLES, // C3D_EPRIM_TRI
    GL_TRIANGLES, // C3D_EPRIM_QUAD
    GL_TRIANGLES, // C3D_EPRIM_RECT
    GL_POINTS     // C3D_EPRIM_POINT
};

class VertexStream
{
public:
    VertexStream();
    void addPrimStrip(C3D_VSTRIP vertStrip, C3D_UINT32 numVert);
    void addPrimList(C3D_VLIST vertList, C3D_UINT32 numVert);
    void renderPending();
    C3D_EVERTEX vertexType();
    void vertexType(C3D_EVERTEX vertexType);
    C3D_EPRIM primType();
    void primType(C3D_EPRIM primType);
    void bind();
    void renderPrims(std::vector<C3D_VTCF> prims);
    void setDelayer(std::function<BOOL(C3D_VTCF *)> delayer);

private:
    C3D_EVERTEX m_vertexType;
    C3D_EPRIM m_primType;
    size_t m_vertexBufferSize = 0;
    gl::Buffer m_vertexBuffer;
    gl::VertexArray m_vtcFormat;
    std::vector<C3D_VTCF> m_vtcBuffer;
    std::function<BOOL(C3D_VTCF *)> m_delayer;
};

} // namespace cif
} // namespace glrage
