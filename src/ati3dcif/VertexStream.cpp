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

bool VertexStream::addPrimStrip(C3D_VSTRIP vertStrip, C3D_UINT32 numVert)
{
    bool result = false;

    // note: strips are converted to lists, since they can't be properly
    // batched otherwise
    switch (m_vertexType) {
    case C3D_EV_VTCF: {
        auto vStripVtcf = reinterpret_cast<C3D_VTCF *>(vertStrip);

        if (m_primType != C3D_EPRIM_TRI) {
            LOG_ERROR("Unsupported prim type: %d", m_primType);
            return false;
        }

        if (numVert <= 2) {
            for (C3D_UINT32 i = 0; i < numVert; i++) {
                m_vtcBuffer.push_back(vStripVtcf[i]);
            }
        } else {
            for (C3D_UINT32 i = 2; i < numVert; i++) {
                m_vtcBuffer.push_back(vStripVtcf[i - 2]);
                m_vtcBuffer.push_back(vStripVtcf[i - 1]);
                m_vtcBuffer.push_back(vStripVtcf[i]);
            }
        }

        result = true;
        break;
    }

    default:
        LOG_ERROR("Unsupported vertex type: %d", m_vertexType);
    }

    return result;
}

bool VertexStream::addPrimList(C3D_VLIST vertList, C3D_UINT32 numVert)
{
    bool result = false;

    switch (m_vertexType) {
    case C3D_EV_VTCF: {
        // copy vertices to vertex vector buffer, then to the vertex buffer
        // (OpenGL can't handle arrays of pointers)
        auto vListVtcf = reinterpret_cast<C3D_VTCF **>(vertList);

        if (m_primType == C3D_EPRIM_QUAD) {
            // triangulate quads
            for (C3D_UINT32 i = 0; i < numVert; i += 4) {
                m_vtcBuffer.push_back(*vListVtcf[i + 0]);
                m_vtcBuffer.push_back(*vListVtcf[i + 1]);
                m_vtcBuffer.push_back(*vListVtcf[i + 3]);

                m_vtcBuffer.push_back(*vListVtcf[i + 1]);
                m_vtcBuffer.push_back(*vListVtcf[i + 2]);
                m_vtcBuffer.push_back(*vListVtcf[i + 3]);
            }
        } else {
            // direct copy
            for (C3D_UINT32 i = 0; i < numVert; i++) {
                m_vtcBuffer.push_back(*vListVtcf[i]);
            }
        }

        result = true;
        break;
    }

    default:
        LOG_ERROR("Unsupported vertex type: %d", m_vertexType);
    }

    return result;
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

void VertexStream::vertexType(C3D_EVERTEX vertexType)
{
    m_vertexType = vertexType;
}

void VertexStream::bind()
{
    GFX_GL_Buffer_Bind(&m_vertexBuffer);
}

}
}
