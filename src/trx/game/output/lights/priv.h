#pragma once

#include <trx/game/output/lights.h>
#include <trx/game/output/uniforms.h>
#include <trx/game/rooms.h>
#include <trx/game/types.h>

#define OUTPUT_LIGHT_CYCLE 32
#define OUTPUT_DYNAMIC_FALLOFF_SHIFT 8

// A colored dynamic light's falloff counts in steps of this many world units
// (the OG's >> 7 in the TR3 CreateDynamicLight). How far the light reaches is
// then that many steps, give or take what each game makes of it.
#define OUTPUT_DYNAMIC_RADIUS_SHIFT 7
// The furthest one can be asked to reach. The falloff is stored shifted up by
// OUTPUT_DYNAMIC_FALLOFF_SHIFT, and this keeps the result in range; no game
// looks past eight sectors for a dynamic light in any case.
#define OUTPUT_DYNAMIC_FALLOFF_MAX 0x7FFF

// How a dynamic light's shade/falloff fields are encoded. Uploaded to the
// shader as the light's `kind`, where the TR1/2 family picks the lighting
// formula based on it (TR3+ treats every dynamic light as RGB).
typedef enum {
    // shade/falloff are log2 exponents (Output_AddDynamicLight)
    OUTPUT_DYNAMIC_LIGHT_LUM = 0,
    // colored; falloff is a radius << OUTPUT_DYNAMIC_FALLOFF_SHIFT
    OUTPUT_DYNAMIC_LIGHT_RGB = 1,
} OUTPUT_DYNAMIC_LIGHT_KIND;

typedef struct {
    LIGHT light;
    OUTPUT_DYNAMIC_LIGHT_KIND kind;
} OUTPUT_DYNAMIC_LIGHT;

// Per-lighting-family strategy. Three families exist: TR1/2 (scalar shade),
// TR3 (3 directional lights + ambient) and TR4 (full colored light lists).
// TR1-vs-TR2 deltas stay as small branches inside the TR1/2 implementation.
typedef struct {
    void (*init)(void);
    void (*shutdown)(void);
    void (*observe_level_load)(void);

    void (*calculate_light)(XYZ_32 pos, int16_t room_num);
    void (*calculate_object_lighting_at)(
        const ITEM *item, GAME_VECTOR sample_pos);
    void (*calculate_static_light)(int16_t adder);
    void (*calculate_static_light_rgb15)(int16_t rgb15); // nullable
    void (*calculate_static_light_rgb_f)(RGB_F rgb); // nullable
    void (*calculate_static_mesh_light)(
        XYZ_32 pos, SHADE shade, const ROOM *room);
    void (*add_dynamic_light)(XYZ_32 pos, int32_t intensity, int32_t falloff);
    void (*add_dynamic_light_rgb)(
        XYZ_32 pos, int32_t falloff, RGB_888 color); // nullable

    void (*begin_scene)(void); // nullable
    // Called at render time once the scene's view matrix is final;
    // TR4 stages and uploads the fog bulbs here.
    void (*prepare_scene)(void); // nullable
    void (*animate)(int32_t num_frames); // nullable
    // Lets the model refine the light snapshot per staged mesh instance
    // (TR4 resolves its light list relative to each mesh's world position).
    void (*fill_instance_light)(
        OUTPUT_LIGHT_INFO *info, const MATRIX *wmatrix); // nullable

    void (*upload_cpu_light)(
        const OUTPUT_UNIFORMS *uniforms, const OUTPUT_LIGHT_INFO *info);
    void (*upload_own_light)(
        const OUTPUT_UNIFORMS *uniforms, const OUTPUT_LIGHT_INFO *info);

    int32_t shader_variant;
} LIGHTING_MODEL;

extern const LIGHTING_MODEL g_LightingModelTR12;
extern const LIGHTING_MODEL g_LightingModelTR3;
extern const LIGHTING_MODEL g_LightingModelTR4;

const LIGHTING_MODEL *Output_Lights_GetModel(void);

// TR3's dynamic light encoding, reused by the TR4 model.
void Output_Lights_TR3_AddDynamicLight(
    XYZ_32 pos, int32_t intensity, int32_t falloff);

// Per-family LightSource UBO layouts (binding point 3). The GL buffer is
// shared and sized to the largest layout; each family uploads only its own.
#pragma pack(push, 4)
typedef struct {
    float adder;
    float divider;
    float _pad0[2];
    float vector_view[4];
} OUTPUT_UNIFORM_LS_TR12;

typedef struct {
    float ambient[4];
    float dir_view[3][4];
    float color[3][4];
} OUTPUT_UNIFORM_LS_TR3;

// 21 current + 21 previous room lights plus dynamics, truncated beyond.
#define OUTPUT_TR4_MAX_STAGED_LIGHTS 48

typedef struct {
    float ambient[4]; // 0..1 scale where 128/255 is the OG neutral
    int32_t num_lights;
    int32_t _pad[3];
    struct {
        float vec[4]; // view space, attenuation-scaled
        float color[4]; // rgb 0..1; w = radial attenuation
    } lights[OUTPUT_TR4_MAX_STAGED_LIGHTS];
} OUTPUT_UNIFORM_LS_TR4;

// Binding point 4; OUTPUT_MAX_FOG_BULBS bulbs at once.
typedef struct {
    int32_t count;
    int32_t _pad[3];
    struct {
        float pos[4]; // view space center; w = distance to camera
        float edge[4]; // view space sphere edge toward camera; w = sqrad
        float color[4]; // rgb 0..1; w = density (0..255)
        float params[4]; // x = 1 / sqrad, y = 1 for FX bulbs
    } bulbs[OUTPUT_MAX_FOG_BULBS];
} OUTPUT_UNIFORM_FOG_BULBS;
#pragma pack(pop)

typedef enum {
    LS_MODE_NONE = 0,
    LS_MODE_FULL = 1,
    LS_MODE_OWN = 2,
} OUTPUT_LS_MODE;

typedef struct {
    OUTPUT_LIGHT_INFO last_info;
    OUTPUT_LS_MODE mode;
    int32_t last_own_adder;
    RGB_F last_own_tr3_ambient;
} OUTPUT_LS_CACHE;

OUTPUT_LS_CACHE *Output_Lights_GetLSCache(void);
size_t Output_Lights_GetLSBufferSize(void);

// Shared light data accessors.
const LIGHT_LEGACY_DATA *Output_Lights_GetLegacyData(const LIGHT *light);
bool Output_Lights_IsSunLight(const LIGHT *light);
int32_t Output_Lights_GetLightIntensity(const LIGHT *light);
int32_t Output_Lights_GetLightOuterRadius(const LIGHT *light);
XYZ_32 Output_Lights_GetLightDirWorld(const LIGHT *light);

// Shared math helpers.
XYZ_32 Output_Lights_VectorViewFromWorld(XYZ_32 v_world);
int16_t Output_Lights_ShadeFromMul(float mul);
void Output_Lights_SetScalarStaticLight(int16_t adder);

// Model dispatch entry points for the render pipeline.
void Output_Lights_UploadCPULight(const OUTPUT_LIGHT_INFO *info);
void Output_Lights_UploadOwnLight(const OUTPUT_LIGHT_INFO *info);
void Output_Lights_FillInstanceLight(
    OUTPUT_LIGHT_INFO *info, const MATRIX *wmatrix);
void Output_Lights_BeginScene(void);
void Output_Lights_PrepareScene(void);
void Output_Lights_ResetUploadCache(void);
