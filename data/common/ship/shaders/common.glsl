#define PI 3.1415926538

#define WALL_L      1024
#define WIBBLE_SIZE 32
#define MAX_WIBBLE  2

#define SHADE_HIGH    0x800
#define SHADE_NEUTRAL 0x1000
#define SHADE_MAX     0x1FFF

#define VERT_NO_CAUSTICS      0x01u
#define VERT_FLAT_SHADED      0x02u
#define VERT_REFLECTIVE       0x04u
#define VERT_NO_LIGHTING      0x08u
#define VERT_BILLBOARD        0x10u
#define VERT_ABS_SPRITE       0x20u
#define VERT_NO_ALPHA_DISCARD 0x40u

#define LIGHTING_CONTRAST_LOW    0
#define LIGHTING_CONTRAST_MEDIUM 1
#define LIGHTING_CONTRAST_HIGH   2

layout(std140) uniform Globals {
    vec4 uFogColor;
    vec2 uFogDistance; // x = fog start, y = fog end
    vec2 uViewportSize;
    float uTime;
    float uTimeInGame;
    float uBrightnessMultiplier;
    int uBillboardLockMode;
    int uLightingContrast;
    int uLightingEnabled; // bool
    int uTrapezoidFilterEnabled; // bool
    int uReflectionsEnabled; // bool
};

layout(std140) uniform Matrices {
    mat4 uMatProj;
    mat4 uMatView;
};

vec2 clampTexAtlas(vec2 uv, vec4 atlasSize)
{
    float epsilon = 0.5 / 256.0;
    return clamp(uv, atlasSize.xy + epsilon, atlasSize.zw - epsilon);
}

vec3 applyShade(vec3 color, float shade)
{
    if (uLightingContrast == LIGHTING_CONTRAST_MEDIUM) {
        shade = max(shade, SHADE_HIGH);
    }
    if (uLightingContrast == LIGHTING_CONTRAST_LOW) {
        shade = max(shade, SHADE_NEUTRAL);
    }

    return color * (2.0 - (shade / SHADE_NEUTRAL));
}
