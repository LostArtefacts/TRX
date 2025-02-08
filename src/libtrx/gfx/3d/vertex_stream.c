#include "gfx/3d/vertex_stream.h"

#include "gfx/gl/gl_core_3_3.h"
#include "gfx/gl/utils.h"
#include "log.h"
#include "memory.h"

#define M_PREALLOC_VERTEX_COUNT 8000

static const GLenum GL_PRIM_MODES[] = {
    GL_LINES, // GFX_3D_PRIM_LINE
    GL_TRIANGLES, // GFX_3D_PRIM_TRI
};

static void M_PushVertex(
    GFX_3D_VERTEX_STREAM *vertex_stream, const GFX_3D_VERTEX *vertex);

static void M_PushVertex(
    GFX_3D_VERTEX_STREAM *const vertex_stream,
    const GFX_3D_VERTEX *const vertex)
{
    if (vertex_stream->pending_vertices.count + 1
        >= vertex_stream->pending_vertices.capacity) {
        vertex_stream->pending_vertices.capacity += 1000;
        vertex_stream->pending_vertices.data = Memory_Realloc(
            vertex_stream->pending_vertices.data,
            vertex_stream->pending_vertices.capacity * sizeof(GFX_3D_VERTEX));
    }

    vertex_stream->pending_vertices
        .data[vertex_stream->pending_vertices.count++] = *vertex;
}

void GFX_3D_VertexStream_Init(GFX_3D_VERTEX_STREAM *const vertex_stream)
{
    vertex_stream->prim_type = GFX_3D_PRIM_TRI;
    vertex_stream->buffer_size =
        M_PREALLOC_VERTEX_COUNT * sizeof(GFX_3D_VERTEX);
    vertex_stream->rendered_count = 0;
    vertex_stream->pending_vertices.count = 0;
    vertex_stream->pending_vertices.capacity = M_PREALLOC_VERTEX_COUNT;
    vertex_stream->pending_vertices.data = Memory_Alloc(
        vertex_stream->pending_vertices.capacity * sizeof(GFX_3D_VERTEX));

    GFX_GL_Buffer_Init(&vertex_stream->buffer, GL_ARRAY_BUFFER);
    GFX_GL_Buffer_Bind(&vertex_stream->buffer);
    GFX_GL_Buffer_Data(
        &vertex_stream->buffer, vertex_stream->buffer_size, nullptr,
        GL_STREAM_DRAW);

    GFX_GL_VertexArray_Init(&vertex_stream->vtc_format);
    GFX_GL_VertexArray_Bind(&vertex_stream->vtc_format);
    GFX_GL_VertexArray_Attribute(
        &vertex_stream->vtc_format, 0, 3, GL_FLOAT, GL_FALSE,
        sizeof(GFX_3D_VERTEX), offsetof(GFX_3D_VERTEX, x));
    GFX_GL_VertexArray_Attribute(
        &vertex_stream->vtc_format, 1, 3, GL_FLOAT, GL_FALSE,
        sizeof(GFX_3D_VERTEX), offsetof(GFX_3D_VERTEX, s));
    GFX_GL_VertexArray_Attribute(
        &vertex_stream->vtc_format, 2, 4, GL_FLOAT, GL_FALSE,
        sizeof(GFX_3D_VERTEX), offsetof(GFX_3D_VERTEX, r));

    GFX_GL_CheckError();
}

void GFX_3D_VertexStream_Close(GFX_3D_VERTEX_STREAM *const vertex_stream)
{
    GFX_GL_VertexArray_Close(&vertex_stream->vtc_format);
    GFX_GL_Buffer_Close(&vertex_stream->buffer);
    Memory_FreePointer(&vertex_stream->pending_vertices.data);
}

void GFX_3D_VertexStream_Bind(GFX_3D_VERTEX_STREAM *const vertex_stream)
{
    GFX_GL_Buffer_Bind(&vertex_stream->buffer);
}

void GFX_3D_VertexStream_SetPrimType(
    GFX_3D_VERTEX_STREAM *const vertex_stream, const GFX_3D_PRIM_TYPE prim_type)
{
    vertex_stream->prim_type = prim_type;
}

bool GFX_3D_VertexStream_PushPrimStrip(
    GFX_3D_VERTEX_STREAM *const vertex_stream,
    const GFX_3D_VERTEX *const vertices, const int count)
{
    if (vertex_stream->prim_type != GFX_3D_PRIM_TRI) {
        LOG_ERROR("Unsupported prim type: %d", vertex_stream->prim_type);
        return false;
    }

    if (count <= 2) {
        for (int i = 0; i < count; i++) {
            M_PushVertex(vertex_stream, &vertices[i]);
        }
    } else {
        // convert strip to raw triangles
        for (int i = 2; i < count; i++) {
            M_PushVertex(vertex_stream, &vertices[i - 2]);
            M_PushVertex(vertex_stream, &vertices[i - 1]);
            M_PushVertex(vertex_stream, &vertices[i]);
        }
    }

    return true;
}

bool GFX_3D_VertexStream_PushPrimFan(
    GFX_3D_VERTEX_STREAM *const vertex_stream,
    const GFX_3D_VERTEX *const vertices, const int count)
{
    if (vertex_stream->prim_type != GFX_3D_PRIM_TRI) {
        LOG_ERROR("Unsupported prim type: %d", vertex_stream->prim_type);
        return false;
    }

    if (count <= 2) {
        for (int i = 0; i < count; i++) {
            M_PushVertex(vertex_stream, &vertices[i]);
        }
    } else {
        // convert fan to raw triangles
        for (int i = 2; i < count; i++) {
            M_PushVertex(vertex_stream, &vertices[0]);
            M_PushVertex(vertex_stream, &vertices[i - 1]);
            M_PushVertex(vertex_stream, &vertices[i]);
        }
    }

    return true;
}

bool GFX_3D_VertexStream_PushPrimList(
    GFX_3D_VERTEX_STREAM *const vertex_stream,
    const GFX_3D_VERTEX *const vertices, const int count)
{
    for (int i = 0; i < count; i++) {
        M_PushVertex(vertex_stream, &vertices[i]);
    }
    return true;
}

void GFX_3D_VertexStream_RenderPending(
    GFX_3D_VERTEX_STREAM *const vertex_stream)
{
    if (!vertex_stream->pending_vertices.count) {
        return;
    }

    GFX_GL_Buffer_Bind(&vertex_stream->buffer);
    GFX_GL_VertexArray_Bind(&vertex_stream->vtc_format);

    // resize GPU buffer if required
    size_t buffer_size =
        sizeof(GFX_3D_VERTEX) * vertex_stream->pending_vertices.count;
    if (buffer_size > vertex_stream->buffer_size) {
        LOG_INFO(
            "Vertex buffer resize: %d -> %d", vertex_stream->buffer_size,
            buffer_size);
        GFX_GL_Buffer_Data(
            &vertex_stream->buffer, buffer_size, nullptr, GL_STREAM_DRAW);
        vertex_stream->buffer_size = buffer_size;
    }

    GFX_GL_Buffer_SubData(
        &vertex_stream->buffer, 0, buffer_size,
        vertex_stream->pending_vertices.data);

    glDrawArrays(
        GL_PRIM_MODES[vertex_stream->prim_type], 0,
        vertex_stream->pending_vertices.count);
    GFX_GL_CheckError();

    vertex_stream->rendered_count += vertex_stream->pending_vertices.count;
    vertex_stream->pending_vertices.count = 0;
}
