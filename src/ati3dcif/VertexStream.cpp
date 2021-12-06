#include "ati3dcif/VertexStream.hpp"

#include "gfx/gl/utils.h"
#include "log.h"

namespace glrage {
namespace cif {

VertexStream::VertexStream()
{
    GFX_GL_Buffer_Init(&m_vertexBuffer, GL_ARRAY_BUFFER);
    GFX_GL_Buffer_Bind(&m_vertexBuffer);

    GFX_GL_VertexArray_Init(&m_vtcFormat);
    GFX_GL_VertexArray_Bind(&m_vtcFormat);
    GFX_GL_VertexArray_Attribute(&m_vtcFormat, 0, 3, GL_FLOAT, GL_FALSE, 40, 0);
    GFX_GL_VertexArray_Attribute(
        &m_vtcFormat, 1, 3, GL_FLOAT, GL_FALSE, 40, 12);
    GFX_GL_VertexArray_Attribute(
        &m_vtcFormat, 2, 4, GL_FLOAT, GL_FALSE, 40, 24);

    GFX_GL_CheckError();
}

VertexStream::~VertexStream()
{
    GFX_GL_VertexArray_Close(&m_vtcFormat);
    GFX_GL_Buffer_Close(&m_vertexBuffer);
}

bool VertexStream::addPrimStrip(C3D_VSTRIP vertStrip, int numVert)
{
    // note: strips are converted to lists, since they can't be properly
    // batched otherwise
    auto vStripVtcf = reinterpret_cast<C3D_VTCF *>(vertStrip);

    if (m_primType != C3D_EPRIM_TRI) {
        LOG_ERROR("Unsupported prim type: %d", m_primType);
        return false;
    }

    if (numVert <= 2) {
        for (int i = 0; i < numVert; i++) {
            m_vtcBuffer.push_back(vStripVtcf[i]);
        }
    } else {
        for (int i = 2; i < numVert; i++) {
            m_vtcBuffer.push_back(vStripVtcf[i - 2]);
            m_vtcBuffer.push_back(vStripVtcf[i - 1]);
            m_vtcBuffer.push_back(vStripVtcf[i]);
        }
    }

    return true;
}

bool VertexStream::addPrimList(C3D_VLIST vertList, int numVert)
{
    // copy vertices to vertex vector buffer, then to the vertex buffer
    // (OpenGL can't handle arrays of pointers)
    auto vListVtcf = reinterpret_cast<C3D_VTCF **>(vertList);
    for (int i = 0; i < numVert; i++) {
        m_vtcBuffer.push_back(*vListVtcf[i]);
    }
    return true;
}

void VertexStream::renderPrims(std::vector<C3D_VTCF> prims)
{
    GFX_GL_VertexArray_Bind(&m_vtcFormat);

    // resize GPU buffer if required
    size_t vertexBufferSize = sizeof(C3D_VTCF) * prims.size();
    if (vertexBufferSize > m_vertexBufferSize) {
        LOG_INFO(
            "Vertex buffer resize: %d -> %d", m_vertexBufferSize,
            vertexBufferSize);
        GFX_GL_Buffer_Data(
            &m_vertexBuffer, vertexBufferSize, nullptr, GL_STREAM_DRAW);
        m_vertexBufferSize = vertexBufferSize;
    }

    GFX_GL_Buffer_SubData(&m_vertexBuffer, 0, vertexBufferSize, &prims[0]);

    glDrawArrays(GLCIF_PRIM_MODES[m_primType], 0, prims.size());

    GFX_GL_CheckError();
}

void VertexStream::renderPending()
{
    // only render if there's something to render
    if (m_vtcBuffer.empty()) {
        return;
    }

    renderPrims(m_vtcBuffer);

    // mark buffer as empty
    m_vtcBuffer.clear();
}

void VertexStream::primType(C3D_EPRIM primType)
{
    m_primType = primType;
}

void VertexStream::bind()
{
    GFX_GL_Buffer_Bind(&m_vertexBuffer);
}

}
}
