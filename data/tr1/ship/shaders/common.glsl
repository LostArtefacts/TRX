#define WALL_L 1024
#define SHADE_NEUTRAL 0x1000
#define SHADE_MAX 0x1FFF

#define VERT_NO_CAUSTICS 0x01u
#define VERT_FLAT_SHADED 0x02u
#define VERT_REFLECTIVE  0x04u
#define VERT_NO_LIGHTING 0x08u
#define VERT_SPRITE      0x10u  // flag for billboarded sprites in mesh shader

#define WIBBLE_SIZE 32
#define MAX_WIBBLE 2
#define PI 3.1415926538

vec3 waterWibble(vec4 position, vec2 viewportSize, int time)
{
    // get screen coordinates
    vec3 ndc = position.xyz / position.w; //perspective divide/normalize
    vec2 viewportCoord = ndc.xy * 0.5 + 0.5; //ndc is -1 to 1 in GL. scale for 0 to 1
    vec2 viewportPixelCoord = viewportCoord * viewportSize;

    viewportPixelCoord.x += sin((float(time) + viewportPixelCoord.y) * 2.0 * PI / WIBBLE_SIZE) * MAX_WIBBLE;
    viewportPixelCoord.y += sin((float(time) + viewportPixelCoord.x) * 2.0 * PI / WIBBLE_SIZE) * MAX_WIBBLE;

    // reverse transform
    viewportCoord = viewportPixelCoord / viewportSize;
    ndc.xy = (viewportCoord - 0.5) * 2.0;
    return ndc * position.w;
}

float shadeFog(float shade, float depth, vec2 fog)
{
    float fogBegin = fog.x;
    float fogEnd = fog.y;
    if (depth < fogBegin) {
        return shade + 0.0;
    } else if (depth >= fogEnd) {
        return shade + float(SHADE_MAX);
    } else {
        return shade + (depth - fogBegin) * SHADE_MAX / (fogEnd - fogBegin);
    }
}

vec3 applyShade(vec3 color, float shade)
{
    return color * (2.0 - (shade / SHADE_NEUTRAL));
}

bool discardTranslucent(sampler2D tex, vec2 uv)
{
    // do not use smoothing for chroma key
    ivec2 size = textureSize(tex, 0);
    ivec2 texCoordsNN = ivec2(uv.xy * size.xy) % size.xy;
    vec4 texel = texelFetch(tex, texCoordsNN, 0);
    return texel.a == 0.0;
}

bool discardTranslucent(sampler2DArray tex, vec3 uvw)
{
    // do not use smoothing for chroma key
    ivec2 size = textureSize(tex, 0).xy;
    ivec3 texCoordsNN = ivec3(ivec2(uvw.xy * size.xy) % size.xy, uvw.z);
    vec4 texel = texelFetch(tex, texCoordsNN, 0);
    return texel.a == 0.0;
}
