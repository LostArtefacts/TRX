#ifdef VERTEX

uniform int uTime;
uniform vec2 uViewportSize;
uniform mat4 uMatProjection;
uniform mat4 uMatModelView;
uniform bool uTrapezoidFilterEnabled;
uniform bool uWibbleEffect;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inUVW;
layout(location = 3) in vec4 inTextureSize;
layout(location = 4) in vec2 inTrapezoidRatios;
layout(location = 5) in uint inFlags;
layout(location = 6) in vec4 inColor;
layout(location = 7) in float inShade;

out vec4 gWorldPos;
out vec3 gNormal;
flat out uint gFlags;
flat out int gTexLayer;
out vec2 gTexUV;
flat out vec4 gAtlasSize;
out vec2 gTrapezoidRatios;
out float gShade;
out vec4 gColor;

void main(void) {
    // billboard sprites if flagged, else standard vertex transform
    vec4 eyePos = uMatModelView * vec4(inPosition.xyz, 1.0);
    if ((inFlags & VERT_BILLBOARD) != 0u) {
        // inNormal.xy carries sprite displacement for billboarding
        eyePos.xy += inNormal.xy;
    }
    gWorldPos = eyePos;
    gNormal = inNormal;
    gl_Position = uMatProjection * eyePos;

    // apply water wibble effect only to non-sprite vertices
    if (uWibbleEffect && (inFlags & VERT_NO_CAUSTICS) == 0u && (inFlags & VERT_BILLBOARD) == 0u) {
        gl_Position.xyz =
            waterWibble(gl_Position, uViewportSize, uTime);
    }

    gFlags = inFlags;
    gAtlasSize = inTextureSize;
    gTexUV = inUVW.xy;
    gTexLayer = int(inUVW.z);
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
uniform int uLightingMode;
uniform bool uReflectionsEnabled;
uniform float uAlphaThreshold;
uniform float uBrightnessMultiplier;
uniform vec3 uGlobalTint;
uniform vec2 uFog; // x = fog start, y = fog end

in vec4 gWorldPos;
in vec3 gNormal;
flat in uint gFlags;
flat in int gTexLayer;
in vec2 gTexUV;
flat in vec4 gAtlasSize;
in float gShade;
in vec4 gColor;
in vec2 gTrapezoidRatios;
out vec4 outColor;

vec2 clampTexAtlas(vec2 uv, vec4 atlasSize)
{
    float epsilon = 0.5 / 256.0;
    return clamp(uv, atlasSize.xy + epsilon, atlasSize.zw - epsilon);
}

void main(void) {
    vec4 texColor = gColor;

    if ((gFlags & VERT_FLAT_SHADED) == 0u && gTexLayer >= 0) {
        vec3 texCoords = vec3(gTexUV.x, gTexUV.y, gTexLayer);
        if (uTrapezoidFilterEnabled) {
            texCoords.xy /= gTrapezoidRatios;
        }
        texCoords.xy = clampTexAtlas(texCoords.xy, gAtlasSize);

        if (uAlphaDiscardEnabled && uSmoothingEnabled && discardTranslucent(uTexAtlas, texCoords)) {
            discard;
        }

        texColor = texture(uTexAtlas, texCoords);
        if (uAlphaThreshold >= 0.0 && texColor.a <= uAlphaThreshold) {
            discard;
        }
        texColor.a *= gColor.a;
    }
    if ((gFlags & VERT_REFLECTIVE) != 0u && uReflectionsEnabled) {
        texColor *= texture(uTexEnvMap, (normalize(gNormal) * 0.5 + 0.5).xy) * 2;
    }

    if ((gFlags & VERT_NO_LIGHTING) == 0u) {
        float shade;
        if (uLightingMode == LIGHTING_MODE_ONLY_SHADES) {
            shade = gShade;
        } else if (uLightingMode == LIGHTING_MODE_FULL) {
            shade = gShade;
            shade = shadeFog(shade, gWorldPos.z, uFog);
        } else {
            shade = SHADE_NEUTRAL;
        }
        texColor.rgb = applyShade(texColor.rgb, shade);
    }

    texColor.rgb *= uGlobalTint;
    texColor.rgb *= uBrightnessMultiplier;

    outColor = texColor;
}
#endif
