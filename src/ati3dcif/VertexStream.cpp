#include "ati3dcif/VertexStream.hpp"

#include "ati3dcif/Error.hpp"
#include "glrage_gl/Utils.hpp"
#include "log.h"

namespace glrage {
namespace cif {

VertexStream::VertexStream()
    : m_vertexBuffer(GL_ARRAY_BUFFER)
{
    // bind vertex buffer
    m_vertexBuffer.bind();

    // define vertex formats
    m_vtcFormat.bind();
    m_vtcFormat.attribute(0, 3, GL_FLOAT, GL_FALSE, 40, 0);
    m_vtcFormat.attribute(1, 3, GL_FLOAT, GL_FALSE, 40, 12);
    m_vtcFormat.attribute(2, 4, GL_FLOAT, GL_FALSE, 40, 24);

    gl::Utils::checkError(__FUNCTION__);
}

void VertexStream::setDelayer(std::function<BOOL(C3D_VTCF *)> delayer)
{
    m_delayer = delayer;
}

void VertexStream::addPrimStrip(C3D_VSTRIP vertStrip, C3D_UINT32 numVert)
{
    // note: strips are converted to lists, since they can't be properly
    // batched otherwise
    switch (m_vertexType) {
    case C3D_EV_VTCF: {
        auto vStripVtcf = reinterpret_cast<C3D_VTCF *>(vertStrip);

        if (m_primType != C3D_EPRIM_TRI) {
            throw Error(
                "Unsupported prim type: " + std::to_string(m_primType),
                C3D_EC_NOTIMPYET);
        }

        if (numVert <= 2) {
            for (C3D_UINT32 i = 0; i < numVert; i++) {
                m_vtcBuffer.push_back(vStripVtcf[i]);
            }
        } else {
            for (C3D_UINT32 i = 2; i < numVert; i++) {
                if (!m_delayer(&vStripVtcf[i - 2])) {
                    m_vtcBuffer.push_back(vStripVtcf[i - 2]);
                    m_vtcBuffer.push_back(vStripVtcf[i - 1]);
                    m_vtcBuffer.push_back(vStripVtcf[i]);
                }
            }
        }

        break;
    }

    default:
        throw Error(
            "Unsupported vertex type: " + std::to_string(m_vertexType),
            C3D_EC_NOTIMPYET);
    }
}

void VertexStream::addPrimList(C3D_VLIST vertList, C3D_UINT32 numVert)
{
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
        break;
    }

    default:
        throw Error(
            "Unsupported vertex type: " + std::to_string(m_vertexType),
            C3D_EC_NOTIMPYET);
    }
}

void VertexStream::renderPrims(std::vector<C3D_VTCF> prims)
{
    // bind vertex format
    m_vtcFormat.bind();

    // resize GPU buffer if required
    size_t vertexBufferSize = sizeof(C3D_VTCF) * prims.size();
    if (vertexBufferSize > m_vertexBufferSize) {
        LOG_INFO(
            "Vertex buffer resize: %d -> %d", m_vertexBufferSize,
            vertexBufferSize);
        m_vertexBuffer.data(vertexBufferSize, nullptr, GL_STREAM_DRAW);
        m_vertexBufferSize = vertexBufferSize;
    }

    // upload vertices
    m_vertexBuffer.subData(0, vertexBufferSize, &prims[0]);

    // draw vertices
    glDrawArrays(GLCIF_PRIM_MODES[m_primType], 0, prims.size());

    // check for errors
    gl::Utils::checkError(__FUNCTION__);
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
    m_vertexBuffer.bind();
}

}
}
