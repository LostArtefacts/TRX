#define WALL_L 1024
#define SHADE_HIGH 0x800
#define SHADE_NEUTRAL 0x1000
#define SHADE_MAX 0x1FFF

#define VERT_NO_CAUSTICS      0x01u
#define VERT_FLAT_SHADED      0x02u
#define VERT_REFLECTIVE       0x04u
#define VERT_NO_LIGHTING      0x08u
#define VERT_BILLBOARD        0x10u
#define VERT_ABS_SPRITE       0x20u
#define VERT_NO_ALPHA_DISCARD 0x40u

#define LIGHTING_MODE_OFF         0
#define LIGHTING_MODE_ONLY_SHADES 1
#define LIGHTING_MODE_FULL        2

#define LIGHTING_CONTRAST_LOW    0
#define LIGHTING_CONTRAST_MEDIUM 1
#define LIGHTING_CONTRAST_HIGH   2

#define BILLBOARD_LOCK_NONE        0
#define BILLBOARD_LOCK_ROLL        1
#define BILLBOARD_LOCK_ROLL_PITCH  2
#define BILLBOARD_LOCK_PERSPECTIVE 3

#define PROJECTION_MODE_PERSPECTIVE 0
#define PROJECTION_MODE_ORTHOGRAPHIC 1

#define WIBBLE_SIZE 32
#define MAX_WIBBLE 2
#define PI 3.1415926538

layout(std140, binding = 0) uniform Globals {
    mat4 uMatProjOrtho;
    vec4 uFogColor;
    vec2 uFogDistance; // x = fog start, y = fog end
    vec2 uViewportSize;
    float uTime;
    float uTimeInGame;
    float uBrightnessMultiplier;
    int uBillboardLockMode;
    int uLightingContrast;
    int uTrapezoidFilterEnabled; // bool
    int uReflectionsEnabled; // bool
};

layout(std140, binding = 1) uniform Matrices {
    mat4 uMatProjPersp;
    mat4 uMatView;
};

uniform int uProjectionMode;

vec2 clampTexAtlas(vec2 uv, vec4 atlasSize)
{
    float epsilon = 0.5 / 256.0;
    return clamp(uv, atlasSize.xy + epsilon, atlasSize.zw - epsilon);
}

vec3 waterWibble(vec4 position)
{
    // get screen coordinates
    vec3 ndc = position.xyz / position.w; //perspective divide/normalize
    vec2 viewportCoord = ndc.xy * 0.5 + 0.5; //ndc is -1 to 1 in GL. scale for 0 to 1
    vec2 viewportPixelCoord = viewportCoord * uViewportSize;

    viewportPixelCoord.x += sin((uTimeInGame + viewportPixelCoord.y) * 2.0 * PI / WIBBLE_SIZE) * MAX_WIBBLE;
    viewportPixelCoord.y += sin((uTimeInGame + viewportPixelCoord.x) * 2.0 * PI / WIBBLE_SIZE) * MAX_WIBBLE;

    // reverse transform
    viewportCoord = viewportPixelCoord / uViewportSize;
    ndc.xy = (viewportCoord - 0.5) * 2.0;
    return ndc * position.w;
}

vec4 applyFog(vec4 color, float depth)
{
    float fogBegin = uFogDistance.x;
    float fogEnd = uFogDistance.y;
    if (depth < fogBegin) {
        return color;
    } else if (depth >= fogEnd) {
        return uFogColor;
    } else {
        return mix(color, uFogColor, (depth - fogBegin) / (fogEnd - fogBegin));
    }
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
