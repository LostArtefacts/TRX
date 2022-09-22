#pragma once

#include "gfx/gl/buffer.h"
#include "gfx/gl/vertex_array.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    GFX_3D_PRIM_LINE = 0,
    GFX_3D_PRIM_TRI = 1,
} GFX_3D_PrimType;

typedef struct {
    float x, y, z;
    float s, t, w;
    float r, g, b, a;
} GFX_3D_Vertex;

typedef struct GFX_3D_VertexStream {
    GFX_3D_PrimType prim_type;
    size_t buffer_size;
    GFX_GL_Buffer buffer;
    GFX_GL_VertexArray vtc_format;
    struct {
        GFX_3D_Vertex *data;
        size_t count;
        size_t capacity;
    } pending_vertices;
} GFX_3D_VertexStream;

void GFX_3D_VertexStream_Init(GFX_3D_VertexStream *vertex_stream);
void GFX_3D_VertexStream_Close(GFX_3D_VertexStream *vertex_stream);

void GFX_3D_VertexStream_Bind(GFX_3D_VertexStream *vertex_stream);

void GFX_3D_VertexStream_SetPrimType(
    GFX_3D_VertexStream *vertex_stream, GFX_3D_PrimType prim_type);

bool GFX_3D_VertexStream_PushPrimStrip(
    GFX_3D_VertexStream *vertex_stream, GFX_3D_Vertex *vertices, int count);
bool GFX_3D_VertexStream_PushPrimFan(
    GFX_3D_VertexStream *vertex_stream, GFX_3D_Vertex *vertices, int count);
bool GFX_3D_VertexStream_PushPrimList(
    GFX_3D_VertexStream *vertex_stream, GFX_3D_Vertex *vertices, int count);

void GFX_3D_VertexStream_RenderPending(GFX_3D_VertexStream *vertex_stream);
