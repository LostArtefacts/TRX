#pragma once

#include <trx/game/matrix.h>
#include <trx/game/output/shaders/generic.h>
#include <trx/game/output/types.h>

// clang-format off
#define VERT_NO_WIBBLE         0b0000'0000'0001 // = 0x0001
#define VERT_FLAT_SHADED       0b0000'0000'0010 // = 0x0002
#define VERT_REFLECTIVE        0b0000'0000'0100 // = 0x0004
#define VERT_NO_LIGHTING       0b0000'0000'1000 // = 0x0008
#define VERT_BILLBOARD         0b0000'0001'0000 // = 0x0010
#define VERT_ABS_SPRITE        0b0000'0010'0000 // = 0x0020
#define VERT_NO_ALPHA_DISCARD  0b0000'0100'0000 // = 0x0040
#define VERT_USE_DYNAMIC_LIGHT 0b0000'1000'0000 // = 0x0080
#define VERT_USE_OBJECT_LIGHT  0b0001'0000'0000 // = 0x0100
#define VERT_USE_OWN_LIGHT     0b0010'0000'0000 // = 0x0200
#define VERT_MOVE              0b0100'0000'0000 // = 0x0400
#define VERT_GLOW              0b1000'0000'0000 // = 0x0800
#define VERT_OVERBRIGHT      0b1'0000'0000'0000 // = 0x1000
#define VERT_TEX_WRAP       0b10'0000'0000'0000 // = 0x2000
#define VERT_ADDITIVE      0b100'0000'0000'0000 // = 0x4000
#define VERT_NO_FOG       0b1000'0000'0000'0000 // = 0x8000
// clang-format on

// GL attribute mapping in the shader
typedef enum {
    // clang-format off
    OUTPUT_MESH_ATTR_POS             = 0,
    OUTPUT_MESH_ATTR_NORMAL          = 1,
    OUTPUT_MESH_ATTR_UVW             = 2,
    OUTPUT_MESH_ATTR_TEXTURE_SIZE    = 3,
    OUTPUT_MESH_ATTR_TRAPEZOID_RATIO = 4,
    OUTPUT_MESH_ATTR_FLAGS           = 5,
    OUTPUT_MESH_ATTR_COLOR           = 6,
    OUTPUT_MESH_ATTR_SHADE           = 7,
    OUTPUT_MESH_ATTR_REFLECTIVITY    = 8,
    OUTPUT_MESH_ATTR_UV_SCROLL       = 9,
    OUTPUT_MESH_ATTR_TINT_FACTOR     = 10,
    // clang-format on
} OUTPUT_MESH_ATTRIBUTE;

// The values of one instance, in the layout of the shader's Instances block.
// A batch holds a slot for each of its instances, and every other draw takes
// its instance from slot 0.
typedef struct {
    float model[4][4];
    float tint[4];
    float water[4];
    float water_params[4];
    float submerged[4];
    float ambient_span[4];
    float room[4];
    float light[7][4];
} OUTPUT_MESH_SLOT;

// One light of the list that the instances of a draw share, in the layout of
// the shader's InstanceLights block. Positions and directions are in world
// space.
typedef struct {
    float pos[4]; // w = type
    float color[4]; // w = range
    float dir[4]; // w = att1
    float cone[4]; // x = cosine of a spot's cone edge
} OUTPUT_MESH_LIGHT;

typedef struct {
    OUTPUT_MESH_LIGHT *data;
    int32_t count;
    int32_t capacity;
} OUTPUT_MESH_LIGHT_LIST;

// Where a batch takes its instances and its vertices from. The vertices are
// read from buffer textures over the mesh buffers, counted from the vertex
// base.
typedef struct {
    GLuint slots;
    size_t slot_offset;
    GLuint lights;
    size_t light_offset;
    GLuint geom_data;
    GLuint tex_data;
    GLuint shade_data;
    GLuint geom_colors;
    int32_t vertex_base;
} OUTPUT_MESH_BATCH;

typedef struct OUTPUT_MESH_SHADER OUTPUT_MESH_SHADER;

RESULT Output_MeshShader_Create(OUTPUT_MESH_SHADER **out_shader);
void Output_MeshShader_Free(OUTPUT_MESH_SHADER *shader);
void Output_MeshShader_Bind(OUTPUT_MESH_SHADER *shader);

// Binds the variant with no geometry stage, and returns to the variant the
// settings ask for once the pass is done. The geometry stage takes triangles,
// so a pass that draws lines fails on it.
void Output_MeshShader_SuspendSubdivision(
    OUTPUT_MESH_SHADER *shader, bool is_suspended);

// Puts up the whole instance and the lights it names for the draws that
// follow.
void Output_MeshShader_UploadSlot(
    OUTPUT_MESH_SHADER *shader, const OUTPUT_MESH_SLOT *slot,
    const OUTPUT_MESH_LIGHT *lights, int32_t light_count);
// Puts up the light of the instance alone.
void Output_MeshShader_UploadLight(
    OUTPUT_MESH_SHADER *shader, const float light[7][4]);

void Output_MeshShader_UploadModelMatrix(
    OUTPUT_MESH_SHADER *shader, const MATRIX *source);
void Output_MeshShader_UploadWaterEffect(
    OUTPUT_MESH_SHADER *shader, int32_t water_effect);
void Output_MeshShader_UploadWibbleEffect(
    OUTPUT_MESH_SHADER *shader, bool is_enabled);
void Output_MeshShader_UploadTint(OUTPUT_MESH_SHADER *shader, RGBA_F tint);

// Sets the world-space water surface height for subsequent instances;
// without a surface, the whole mesh is tinted.
void Output_MeshShader_UploadWaterLine(
    OUTPUT_MESH_SHADER *shader, bool is_enabled, float world_y);

// Sets how far the ambient light of the submerged part of subsequent
// instances shifts; without it, that part takes the tint instead.
void Output_MeshShader_UploadSubmergedAmbient(
    OUTPUT_MESH_SHADER *shader, bool is_enabled, RGB_F delta);

// Sets the ambient light at the far end of a mesh spanning two rooms; without
// it, the mesh uses one ambient light.
void Output_MeshShader_UploadAmbientSpan(
    OUTPUT_MESH_SHADER *shader, bool is_enabled, RGB_F from);
void Output_MeshShader_SetNoAlphaDiscard(
    OUTPUT_MESH_SHADER *shader, bool is_omitted);

// Reports how many slots a batch holds.
int32_t Output_MeshShader_GetMaxSlots(const OUTPUT_MESH_SHADER *shader);

// Reports how many lights the list that a draw shares among its instances
// holds, or 0 where the lighting model lights without one.
int32_t Output_MeshShader_GetMaxLights(const OUTPUT_MESH_SHADER *shader);

// Draws from a batch until Output_MeshShader_UnbindBatch: gl_VertexID names
// a vertex and a slot, as meshes.glsl describes.
void Output_MeshShader_BindBatch(
    OUTPUT_MESH_SHADER *shader, const OUTPUT_MESH_BATCH *batch);
// Returns to drawing the instance in slot 0 from the vertex attributes.
void Output_MeshShader_UnbindBatch(OUTPUT_MESH_SHADER *shader);

// Writes the water effect parameters that the shader takes for an effect.
void Output_MeshShader_GetWaterEffectParams(int32_t water_effect, float out[3]);

void Output_MeshShader_UploadAlphaDiscard(
    OUTPUT_MESH_SHADER *shader, bool is_enabled);
