#pragma once

#include <trx/game/output/uniforms.h>
#include <trx/game/rooms.h>
#include <trx/game/types.h>

#define OUTPUT_LIGHT_CYCLE 32
#define OUTPUT_DYNAMIC_FALLOFF_SHIFT 8

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

const LIGHTING_MODEL *Output_Lights_GetModel(void);

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
void Output_Lights_ResetUploadCache(void);
