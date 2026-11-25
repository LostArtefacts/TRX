#include <harness/harness.h>

#include <trx/core/file.h>
#include <trx/core/memory.h>
#include <trx/game/game_buf.h>
#include <trx/game/inject.h>
#include <trx/game/objects/common.h>

#include <string.h>

#define M_NUM_VERTICES 6

static void (*m_Handler)(
    const INJECTION_CONTEXT *, const INJECTION *, int32_t) = nullptr;
static OBJECT m_Object = {};
static OBJECT_MESH m_Mesh = {};
static FACE m_Faces[3] = {};
static void *m_Allocs[8] = {};
static int32_t m_AllocCount = 0;

static void M_SetFace(
    FACE *const face, const int32_t vertex_count, const uint16_t texture_idx,
    const uint16_t v0, const uint16_t v1, const uint16_t v2, const uint16_t v3)
{
    *face = (FACE) {
        .vertex_count = vertex_count,
        .texture_idx = texture_idx,
        .vertices = { v0, v1, v2, v3 },
    };
}

// Builds a mesh of one textured quad, one textured triangle and one coloured
// triangle, laid out in one array as the level loader lays them out.
static void M_ResetMesh(void)
{
    for (int32_t i = 0; i < m_AllocCount; i++) {
        Memory_FreePointer(&m_Allocs[i]);
    }
    m_AllocCount = 0;

    M_SetFace(&m_Faces[0], 4, 10, 0, 1, 2, 3);
    M_SetFace(&m_Faces[1], 3, 11, 0, 1, 2, 0);
    M_SetFace(&m_Faces[2], 3, 12, 3, 4, 5, 0);

    m_Object = (OBJECT) { .loaded = true, .mesh_count = 1, .mesh_idx = 0 };
    m_Mesh = (OBJECT_MESH) { .num_vertices = M_NUM_VERTICES };
    m_Mesh.all_faces.data = &m_Faces[0];
    m_Mesh.all_faces.count = 3;
    m_Mesh.tex_faces.data = &m_Faces[0];
    m_Mesh.tex_faces.count = 2;
    m_Mesh.tex_face4s.data = &m_Faces[0];
    m_Mesh.tex_face4s.count = 1;
    m_Mesh.tex_face3s.data = &m_Faces[1];
    m_Mesh.tex_face3s.count = 1;
    m_Mesh.flat_faces.data = &m_Faces[2];
    m_Mesh.flat_faces.count = 1;
    m_Mesh.flat_face4s.data = &m_Faces[2];
    m_Mesh.flat_face4s.count = 0;
    m_Mesh.flat_face3s.data = &m_Faces[2];
    m_Mesh.flat_face3s.count = 1;
}

static void M_Put(char **const at, const uint32_t value, const int32_t size)
{
    for (int32_t i = 0; i < size; i++) {
        *(*at)++ = (value >> (i * 8)) & 0xFF;
    }
}

static void M_PutHeader(
    char **const at, const MESH_FACE_EDIT_TYPE edit_type,
    const FACE_TYPE face_type)
{
    M_Put(at, edit_type, 4);
    M_Put(at, OBJ_TYPE_OBJECT, 4);
    M_Put(at, 0, 4);
    M_Put(at, 0, 2);
    M_Put(at, face_type, 4);
}

static int32_t M_WriteRotation(
    char *const out, const FACE_TYPE face_type, const int16_t face_idx,
    const uint8_t num_rotations)
{
    char *at = out;
    M_PutHeader(&at, MFET_ROTATE_FACE, face_type);
    M_Put(&at, face_idx, 2);
    M_Put(&at, num_rotations, 1);
    return at - out;
}

static int32_t M_WriteAddition(
    char *const out, const FACE_TYPE face_type, const uint16_t texture,
    const uint16_t effects, const int32_t vertex_count,
    const uint16_t *const vertices)
{
    char *at = out;
    M_PutHeader(&at, MFET_ADD_FACE, face_type);
    M_Put(&at, texture, 2);
    M_Put(&at, effects, 2);
    for (int32_t i = 0; i < vertex_count; i++) {
        M_Put(&at, vertices[i], 2);
    }
    return at - out;
}

static void M_Run(
    const INJECTION_MODE mode, const char *const data, const int32_t size,
    const int32_t count)
{
    TRX_FILE *const file = File_OpenBuffer(data, size);
    const INJECTION injection = { .fp = file };
    const INJECTION_CONTEXT ctx = { .mode = mode };
    m_Handler(&ctx, &injection, count);
    CHECK_EQ_INT(File_BytesLeft(file), 0);
    File_Close(file);
}

static void M_CheckVertices(
    const FACE *const face, const uint16_t v0, const uint16_t v1,
    const uint16_t v2, const uint16_t v3)
{
    CHECK_EQ_INT(face->vertices[0], v0);
    CHECK_EQ_INT(face->vertices[1], v1);
    CHECK_EQ_INT(face->vertices[2], v2);
    if (face->vertex_count == 4) {
        CHECK_EQ_INT(face->vertices[3], v3);
    }
}

void Inject_RegisterEditor(
    const INJECTION_DATA_TYPE type,
    void (*const handle_func)(
        const INJECTION_CONTEXT *, const INJECTION *, int32_t))
{
    if (type == IDT_MESH_FACE_EDITS) {
        m_Handler = handle_func;
    }
}

INJECTION_OBJECT_INFO Inject_ReadObjectPtr(const INJECTION *const injection)
{
    return (INJECTION_OBJECT_INFO) {
        .type = File_ReadS32(injection->fp),
        .id = File_ReadS32(injection->fp),
    };
}

RESULT Inject_GetObject(
    const INJECTION_OBJECT_INFO obj_info, OBJECT **const out_obj)
{
    *out_obj = &m_Object;
    return OK;
}

uint16_t Inject_GetPaletteIndex(const uint16_t index)
{
    return index;
}

OBJECT_SLOT Object_IDToSlot(const OBJECT_ID id)
{
    return id;
}

int32_t Object_GetStaticObjects3DCount(void)
{
    return 0;
}

STATIC_OBJECT_3D *Object_Get3DStatic(const int32_t static_id)
{
    return nullptr;
}

OBJECT_MESH *Object_GetMesh(const int32_t index)
{
    return index == 0 ? &m_Mesh : nullptr;
}

void *GameBuf_Alloc(const size_t alloc_size, const GAME_BUFFER buffer)
{
    void *const result = Memory_Alloc(alloc_size);
    if (m_AllocCount < (int32_t)(sizeof(m_Allocs) / sizeof(m_Allocs[0]))) {
        m_Allocs[m_AllocCount++] = result;
    }
    return result;
}

TEST(handler_is_registered)
{
    CHECK_NOT_NULL(m_Handler);
}

TEST(rotations_shift_the_vertices_of_a_quad)
{
    M_ResetMesh();
    char data[64];
    const int32_t size = M_WriteRotation(data, FT_TEXTURED_QUAD, 0, 3);

    M_Run(INJECTION_MODE_FULL, data, size, 1);

    M_CheckVertices(&m_Mesh.tex_face4s.data[0], 3, 0, 1, 2);
}

TEST(rotations_shift_the_vertices_of_a_triangle)
{
    M_ResetMesh();
    char data[64];
    const int32_t size = M_WriteRotation(data, FT_COLOURED_TRIANGLE, 0, 1);

    M_Run(INJECTION_MODE_FULL, data, size, 1);

    M_CheckVertices(&m_Mesh.flat_face3s.data[0], 4, 5, 3, 0);
}

TEST(rotations_of_a_missing_face_change_nothing)
{
    M_ResetMesh();
    char data[64];
    const int32_t size = M_WriteRotation(data, FT_TEXTURED_TRIANGLE, 1, 1);

    M_Run(INJECTION_MODE_FULL, data, size, 1);

    M_CheckVertices(&m_Mesh.tex_face3s.data[0], 0, 1, 2, 0);
}

TEST(added_faces_join_their_group_and_keep_the_others)
{
    M_ResetMesh();
    char data[64];
    const uint16_t vertices[3] = { 5, 4, 3 };
    const int32_t size = M_WriteAddition(
        data, FT_TEXTURED_TRIANGLE, 0x8000 | 20, 123, 3, vertices);

    M_Run(INJECTION_MODE_FULL, data, size, 1);

    CHECK_EQ_INT(m_Mesh.all_faces.count, 4);
    CHECK_EQ_INT(m_Mesh.tex_faces.count, 3);
    CHECK_EQ_INT(m_Mesh.tex_face4s.count, 1);
    CHECK_EQ_INT(m_Mesh.tex_face3s.count, 2);
    CHECK_EQ_INT(m_Mesh.flat_faces.count, 1);
    CHECK_EQ_INT(m_Mesh.flat_face3s.count, 1);

    CHECK(m_Mesh.tex_faces.data == m_Mesh.all_faces.data);
    CHECK(m_Mesh.tex_face4s.data == m_Mesh.all_faces.data);
    CHECK(m_Mesh.tex_face3s.data == m_Mesh.all_faces.data + 1);
    CHECK(m_Mesh.flat_faces.data == m_Mesh.all_faces.data + 3);
    CHECK(m_Mesh.flat_face3s.data == m_Mesh.all_faces.data + 3);

    CHECK_EQ_INT(m_Mesh.tex_face4s.data[0].texture_idx, 10);
    CHECK_EQ_INT(m_Mesh.tex_face3s.data[0].texture_idx, 11);
    CHECK_EQ_INT(m_Mesh.flat_face3s.data[0].texture_idx, 12);

    const FACE *const face = &m_Mesh.tex_face3s.data[1];
    CHECK_EQ_INT(face->vertex_count, 3);
    CHECK_EQ_INT(face->texture_idx, 20);
    CHECK(face->double_sided);
    CHECK_EQ_INT(face->effects, 123);
    M_CheckVertices(face, 5, 4, 3, 0);
}

TEST(added_quads_take_four_vertices)
{
    M_ResetMesh();
    char data[64];
    const uint16_t vertices[4] = { 3, 2, 1, 0 };
    const int32_t size =
        M_WriteAddition(data, FT_TEXTURED_QUAD, 21, 0, 4, vertices);

    M_Run(INJECTION_MODE_FULL, data, size, 1);

    CHECK_EQ_INT(m_Mesh.tex_face4s.count, 2);
    CHECK_EQ_INT(m_Mesh.tex_face3s.count, 1);
    CHECK(m_Mesh.tex_face3s.data == m_Mesh.all_faces.data + 2);

    const FACE *const face = &m_Mesh.tex_face4s.data[1];
    CHECK_EQ_INT(face->vertex_count, 4);
    CHECK_EQ_INT(face->texture_idx, 21);
    CHECK(!face->double_sided);
    M_CheckVertices(face, 3, 2, 1, 0);
}

TEST(added_faces_beyond_the_mesh_vertices_are_dropped)
{
    M_ResetMesh();
    char data[64];
    const uint16_t vertices[3] = { 0, 1, M_NUM_VERTICES };
    const int32_t size =
        M_WriteAddition(data, FT_TEXTURED_TRIANGLE, 20, 0, 3, vertices);

    M_Run(INJECTION_MODE_FULL, data, size, 1);

    CHECK_EQ_INT(m_Mesh.all_faces.count, 3);
    CHECK_EQ_INT(m_Mesh.tex_face3s.count, 1);
}

TEST(edits_are_read_past_when_gathering_stats)
{
    M_ResetMesh();
    char data[128];
    const uint16_t vertices[3] = { 5, 4, 3 };
    int32_t size = M_WriteRotation(data, FT_TEXTURED_QUAD, 0, 1);
    size +=
        M_WriteAddition(data + size, FT_TEXTURED_TRIANGLE, 20, 0, 3, vertices);

    M_Run(INJECTION_MODE_STATS, data, size, 2);

    CHECK_EQ_INT(m_Mesh.all_faces.count, 3);
    M_CheckVertices(&m_Mesh.tex_face4s.data[0], 0, 1, 2, 3);
}
