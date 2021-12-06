#include "gfx/3d/vertex_stream.h"

#include "gfx/gl/utils.h"
#include "log.h"
#include "memory.h"

static const GLenum GLCIF_PRIM_MODES[] = {
    GL_LINES, // C3D_EPRIM_LINE
    GL_TRIANGLES, // C3D_EPRIM_TRI
};

static void GFX_3D_VertexStream_PushVertex(
    GFX_3D_VertexStream *vertex_stream, C3D_VTCF *vertex);

static void GFX_3D_VertexStream_PushVertex(
    GFX_3D_VertexStream *vertex_stream, C3D_VTCF *vertex)
{
    if (vertex_stream->pending_vertices.count + 1
        >= vertex_stream->pending_vertices.capacity) {
        vertex_stream->pending_vertices.capacity += 1000;
        vertex_stream->pending_vertices.data = Memory_Realloc(
            vertex_stream->pending_vertices.data,
            vertex_stream->pending_vertices.capacity * sizeof(C3D_VTCF));
    }

    vertex_stream->pending_vertices
        .data[vertex_stream->pending_vertices.count++] = *vertex;
}

void GFX_3D_VertexStream_Init(GFX_3D_VertexStream *vertex_stream)
{
    vertex_stream->prim_type = C3D_EPRIM_TRI;
    vertex_stream->buffer_size = 0;
    vertex_stream->pending_vertices.data = NULL;
    vertex_stream->pending_vertices.count = 0;
    vertex_stream->pending_vertices.capacity = 0;

    GFX_GL_Buffer_Init(&vertex_stream->buffer, GL_ARRAY_BUFFER);
    GFX_GL_Buffer_Bind(&vertex_stream->buffer);

    GFX_GL_VertexArray_Init(&vertex_stream->vtc_format);
    GFX_GL_VertexArray_Bind(&vertex_stream->vtc_format);
    GFX_GL_VertexArray_Attribute(
        &vertex_stream->vtc_format, 0, 3, GL_FLOAT, GL_FALSE, 40, 0);
    GFX_GL_VertexArray_Attribute(
        &vertex_stream->vtc_format, 1, 3, GL_FLOAT, GL_FALSE, 40, 12);
    GFX_GL_VertexArray_Attribute(
        &vertex_stream->vtc_format, 2, 4, GL_FLOAT, GL_FALSE, 40, 24);

    GFX_GL_CheckError();
}

void GFX_3D_VertexStream_Close(GFX_3D_VertexStream *vertex_stream)
{
    GFX_GL_VertexArray_Close(&vertex_stream->vtc_format);
    GFX_GL_Buffer_Close(&vertex_stream->buffer);

    if (vertex_stream->pending_vertices.data) {
        Memory_Free(vertex_stream->pending_vertices.data);
        vertex_stream->pending_vertices.data = NULL;
    }
}

void GFX_3D_VertexStream_Bind(GFX_3D_VertexStream *vertex_stream)
{
    GFX_GL_Buffer_Bind(&vertex_stream->buffer);
}

void GFX_3D_VertexStream_SetPrimType(
    GFX_3D_VertexStream *vertex_stream, C3D_EPRIM prim_type)
{
    vertex_stream->prim_type = prim_type;
}

bool GFX_3D_VertexStream_PushPrimStrip(
    GFX_3D_VertexStream *vertex_stream, C3D_VSTRIP vert_strip, int count)
{
    // NOTE: strips are converted to lists, since they can't be properly
    // batched otherwise
    C3D_VTCF *vertices = (C3D_VTCF *)vert_strip;

    if (vertex_stream->prim_type != C3D_EPRIM_TRI) {
        LOG_ERROR("Unsupported prim type: %d", vertex_stream->prim_type);
        return false;
    }

    if (count <= 2) {
        for (int i = 0; i < count; i++) {
            GFX_3D_VertexStream_PushVertex(vertex_stream, &vertices[i]);
        }
    } else {
        for (int i = 2; i < count; i++) {
            GFX_3D_VertexStream_PushVertex(vertex_stream, &vertices[i - 2]);
            GFX_3D_VertexStream_PushVertex(vertex_stream, &vertices[i - 1]);
            GFX_3D_VertexStream_PushVertex(vertex_stream, &vertices[i]);
        }
    }

    return true;
}

bool GFX_3D_VertexStream_PushPrimList(
    GFX_3D_VertexStream *vertex_stream, C3D_VLIST vert_list, int count)
{
    // copy vertices to vertex vector buffer, then to the vertex buffer
    // (OpenGL can't handle arrays of pointers)
    C3D_VTCF **vertices = (C3D_VTCF **)vert_list;
    for (int i = 0; i < count; i++) {
        GFX_3D_VertexStream_PushVertex(vertex_stream, vertices[i]);
    }
    return true;
}

void GFX_3D_VertexStream_RenderPending(GFX_3D_VertexStream *vertex_stream)
{
    // only render if there's something to render
    if (!vertex_stream->pending_vertices.count) {
        return;
    }

    GFX_GL_VertexArray_Bind(&vertex_stream->vtc_format);

    // resize GPU buffer if required
    size_t buffer_size =
        sizeof(C3D_VTCF) * vertex_stream->pending_vertices.count;
    if (buffer_size > vertex_stream->buffer_size) {
        LOG_INFO(
            "Vertex buffer resize: %d -> %d", vertex_stream->buffer_size,
            buffer_size);
        GFX_GL_Buffer_Data(
            &vertex_stream->buffer, buffer_size, NULL, GL_STREAM_DRAW);
        vertex_stream->buffer_size = buffer_size;
    }

    GFX_GL_Buffer_SubData(
        &vertex_stream->buffer, 0, buffer_size,
        vertex_stream->pending_vertices.data);

    glDrawArrays(
        GLCIF_PRIM_MODES[vertex_stream->prim_type], 0,
        vertex_stream->pending_vertices.count);

    GFX_GL_CheckError();

    vertex_stream->pending_vertices.count = 0;
}
