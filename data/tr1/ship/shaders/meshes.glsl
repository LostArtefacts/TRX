#define NEUTRAL_SHADE 0x1000

#define VERT_NO_CAUSTICS 0x01
#define VERT_FLAT_SHADED 0x02
#define VERT_REFLECTIVE  0x04
#define VERT_NO_LIGHTING 0x08

#ifdef VERTEX

uniform int uTime;
uniform samplerBuffer uUVW; // texture u, v, layer
uniform vec2 uViewportSize;
uniform mat4 uMatProjection;
uniform mat4 uMatModelView;
uniform bool uTrapezoidFilterEnabled;
uniform bool uWibbleEffect;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in int inUVWIdx;
layout(location = 3) in vec2 inTrapezoidRatios;
layout(location = 4) in int inFlags;
layout(location = 5) in vec4 inColor;
layout(location = 6) in float inShade;

out vec4 gWorldPos;
out vec3 gNormal;
flat out int gFlags;
flat out int gTexLayer;
out vec2 gTexUV;
out vec2 gTrapezoidRatios;
out float gShade;
out vec4 gColor;

void main(void) {
    gWorldPos = uMatModelView * vec4(inPosition.xyz, 1.0);
    gNormal = inNormal;
    gl_Position = uMatProjection * gWorldPos;

    if (uWibbleEffect && (inFlags & VERT_NO_CAUSTICS) == 0) {
        gl_Position.xyz =
            waterWibble(gl_Position, uViewportSize, uTime);
    }

    vec3 uvw = texelFetch(uUVW, int(inUVWIdx)).xyz;
    gFlags = inFlags;
    gTexLayer = int(uvw.z);
    gTexUV = uvw.xy;
    gTrapezoidRatios = inTrapezoidRatios;
    if (uTrapezoidFilterEnabled) {
        gTexUV *= inTrapezoidRatios;
    }
    gShade = inShade;
    gColor = inColor;
}

#elif defined(FRAGMENT)

uniform int uTime;
uniform sampler2DArray uTexAtlas;
uniform sampler2D uTexEnvMap;
uniform bool uSmoothingEnabled;
uniform bool uAlphaDiscardEnabled;
uniform bool uTrapezoidFilterEnabled;
uniform bool uReflectionsEnabled;
uniform float uAlphaThreshold;
uniform float uBrightnessMultiplier;
uniform vec3 uGlobalTint;
uniform vec2 uFog; // x = fog start, y = fog end
uniform bool uWaterEffect;

in vec4 gWorldPos;
in vec3 gNormal;
flat in int gFlags;
flat in int gTexLayer;
in vec2 gTexUV;
in float gShade;
in vec4 gColor;
in vec2 gTrapezoidRatios;
out vec4 outColor;

void main(void) {
    vec4 texColor = gColor;
    vec3 texCoords = vec3(gTexUV.x, gTexUV.y, gTexLayer);
    if (uTrapezoidFilterEnabled) {
        texCoords.xy /= gTrapezoidRatios.xy;
    }

    if ((gFlags & VERT_FLAT_SHADED) == 0 && texCoords.z >= 0) {
        if (uAlphaDiscardEnabled && uSmoothingEnabled && discardTranslucent(uTexAtlas, texCoords)) {
            discard;
        }

        texColor = texture(uTexAtlas, texCoords);
        if (uAlphaThreshold >= 0.0 && texColor.a <= uAlphaThreshold) {
            discard;
        }
    }
    if ((gFlags & VERT_REFLECTIVE) != 0 && uReflectionsEnabled) {
        texColor *= texture(uTexEnvMap, (normalize(gNormal) * 0.5 + 0.5).xy) * 2;
    }

    if ((gFlags & VERT_NO_LIGHTING) == 0) {
        float shade = gShade;
        shade = shadeFog(shade, gWorldPos.z, uFog);
        texColor.rgb = applyShade(texColor.rgb, shade);
    }

    texColor.rgb *= uBrightnessMultiplier;
    texColor.rgb *= uGlobalTint;
    outColor = vec4(texColor.rgb, 1.0);
}
#endif
