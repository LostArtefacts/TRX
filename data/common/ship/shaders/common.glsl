#define WALL_L 1024
#define SHADE_HIGH 0x800
#define SHADE_NEUTRAL 0x1000
#define SHADE_MAX 0x1FFF

#define VERT_NO_CAUSTICS 0x01u
#define VERT_FLAT_SHADED 0x02u
#define VERT_REFLECTIVE  0x04u
#define VERT_NO_LIGHTING 0x08u
#define VERT_BILLBOARD   0x10u
#define VERT_ABS_SPRITE  0x20u

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

#define WIBBLE_SIZE 32
#define MAX_WIBBLE 2
#define PI 3.1415926538

uniform int uTime;
uniform float uBrightnessMultiplier;

vec2 clampTexAtlas(vec2 uv, vec4 atlasSize)
{
    float epsilon = 0.5 / 256.0;
    return clamp(uv, atlasSize.xy + epsilon, atlasSize.zw - epsilon);
}

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

vec3 applyShade(vec3 color, float shade, int lightingContrast)
{
    if (lightingContrast == LIGHTING_CONTRAST_MEDIUM) {
        shade = max(shade, SHADE_HIGH);
    }
    if (lightingContrast == LIGHTING_CONTRAST_LOW) {
        shade = max(shade, SHADE_NEUTRAL);
    }

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

vec4 offsetBillboard(vec3 pos, vec2 displacement, mat4 view, mat4 modelView, mat4 projection, int lockMode)
{
    if (lockMode == BILLBOARD_LOCK_ROLL) {
        vec3 camForward = vec3(modelView[0][2], modelView[1][2], modelView[2][2]);
        vec3 up = vec3(0.0, 1.0, 0.0);
        vec3 right = normalize(cross(up, camForward));
        up = normalize(cross(camForward, right));
        pos.xyz += displacement.x * right + displacement.y * up;
        return modelView * vec4(pos, 1.0);

    } else if (lockMode == BILLBOARD_LOCK_ROLL_PITCH) {
        vec3 up = vec3(0.0, 1.0, 0.0);
        vec3 camForward = normalize(vec3(modelView[0][2], modelView[1][2], modelView[2][2]));
        vec3 fHoriz = camForward - up * dot(camForward, up);
        vec3 right = (length(fHoriz) < 1e-5)
            ? normalize(vec3(modelView[0][0], modelView[1][0], modelView[2][0]))
            : normalize(cross(up, fHoriz));
        pos.xyz += displacement.x * right + displacement.y * up;
        return modelView * vec4(pos, 1.0);

    } else if (lockMode == BILLBOARD_LOCK_PERSPECTIVE) {
        vec3 up = vec3(0.0, 1.0, 0.0);
        // compute camera yaw-forward (lock pitch and roll)
        vec3 camForward = normalize(vec3(view[0][2], view[1][2], view[2][2]));
        vec3 fHoriz = camForward - up * dot(camForward, up);
        vec3 forwardYaw = (length(fHoriz) < 1e-5)
            ? normalize(vec3(view[0][0], view[1][0], view[2][0]))
            : normalize(fHoriz);
        vec3 camRight = normalize(cross(forwardYaw, up));
        // gentle yaw based on screen X towards center
        vec4 clipPos = projection * modelView * vec4(pos, 1.0);
        float ndcX = clipPos.x / clipPos.w;
        float invLen = inversesqrt(1.0 + ndcX * ndcX);
        float cosAng = invLen;
        float sinAng = ndcX * invLen;
        vec3 forwardDir = cosAng * forwardYaw - sinAng * camRight;
        vec3 rightDir = normalize(cross(up, forwardDir));
        pos.xyz += displacement.x * rightDir + displacement.y * up;
        return modelView * vec4(pos, 1.0);

    } else if (lockMode == BILLBOARD_LOCK_NONE) {
        return (modelView * vec4(pos, 1.0)) + vec4(displacement.xy, 0, 0);
    }
}
