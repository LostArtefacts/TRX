#pragma once

#include "ati3dcif/ATI3DCIF.h"
#include "gfx/gl/buffer.h"
#include "gfx/gl/vertex_array.h"

#include <vector>

namespace glrage {
namespace cif {

static const GLenum GLCIF_PRIM_MODES[] = {
    GL_LINES, // C3D_EPRIM_LINE
    GL_TRIANGLES, // C3D_EPRIM_TRI
};

class VertexStream {
public:
    VertexStream();
    ~VertexStream();
    bool addPrimStrip(C3D_VSTRIP vertStrip, int numVert);
    bool addPrimList(C3D_VLIST vertList, int numVert);
    void renderPending();
    void primType(C3D_EPRIM primType);
    void bind();
    void renderPrims(std::vector<C3D_VTCF> prims);

private:
    C3D_EPRIM m_primType = C3D_EPRIM_TRI;
    size_t m_vertexBufferSize = 0;
    GFX_GL_Buffer m_vertexBuffer;
    GFX_GL_VertexArray m_vtcFormat;
    std::vector<C3D_VTCF> m_vtcBuffer;
};

}
}
