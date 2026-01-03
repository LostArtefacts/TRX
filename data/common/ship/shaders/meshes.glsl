#include "common.glsl"

#ifdef VERTEX

uniform mat4 uMatModel;
uniform bool uWibbleEffect;

#include "billboard.glsl"
#include "lights.glsl"

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec3 inUVW;
layout(location = 3) in vec4 inTextureSize;
layout(location = 4) in vec2 inTrapezoidRatios;
layout(location = 5) in uint inFlags;
layout(location = 6) in vec4 inColor;
layout(location = 7) in float inShade;

out vec4 gEyePos;
out vec3 gNormal;
flat out uint gFlags;
flat out int gTexLayer;
out vec2 gTexUV;
flat out vec4 gAtlasSize;
out vec2 gTrapezoidRatios;
out float gShade;
out vec4 gColor;

vec3 waterWibble(vec4 position)
{
    vec3 ndc = position.xyz / position.w;
    vec2 pixelPos = (ndc.xy * 0.5 + 0.5) * uViewportSize;
    vec2 phases = (uTimeInGame + pixelPos.yx) * (2.0 * PI / WIBBLE_SIZE);
    pixelPos += sin(phases) * MAX_WIBBLE;
    // reverse transform
    ndc.xy = (pixelPos / uViewportSize - 0.5) * 2.0;
    return ndc * position.w;
}

void main(void) {
    vec4 worldPos = uMatModel * vec4(inPosition.xyz, 1.0);

    float effectPhase = getEffectPhase(worldPos);

    if ((inFlags & VERT_MOVE) != 0u) {
        float waterMul = (uWaterEffect != 0) ? 1.0 : 0.0;
        worldPos.y += effectChoppy(effectPhase) * waterMul;
    }

    if ((inFlags & (VERT_ABS_SPRITE | VERT_BILLBOARD)) != 0u) {
        int lockMode = (inFlags & VERT_ABS_SPRITE) != 0u ? BILLBOARD_LOCK_NONE : uBillboardLockMode;
        gEyePos = offsetBillboard(inPosition.xyz, inNormal.xy, uMatView, uMatModel, uMatProj, lockMode);
    } else {
        gEyePos = uMatView * worldPos;
    }

    gNormal = inNormal.xyz;
    gl_Position = uMatProj * gEyePos;
    gl_Position.z += inPosition.w;

    // Apply water wibble effect only to non-sprite vertices
    if (uWibbleEffect && (inFlags & (VERT_NO_WIBBLE | VERT_BILLBOARD)) == 0u) {
        gl_Position.xyz = waterWibble(gl_Position);
    }

    gFlags = inFlags;
    gAtlasSize = inTextureSize;
    gTexLayer = (uTexturesEnabled != 0) && (gFlags & VERT_FLAT_SHADED) == 0u ? int(inUVW.z) : -1;
    gTrapezoidRatios = inTrapezoidRatios;
    gTexUV = inUVW.xy;
    if (uTrapezoidFilterEnabled != 0) {
        gTexUV *= inTrapezoidRatios;
    }
    LightingResult lr =
        light(inColor, inShade, gFlags, inNormal.xyz, worldPos, inNormal.w, effectPhase);
    gShade = lr.shade;
    gColor = lr.color;
}

#elif defined(FRAGMENT)

uniform sampler2DArray uTexAtlas;
uniform sampler2D uTexEnvMap;
uniform vec3 uGlobalTint;
uniform bool uDiscardAlpha;

in vec4 gEyePos;
in vec3 gNormal;
flat in uint gFlags;
flat in int gTexLayer;
in vec2 gTexUV;
flat in vec4 gAtlasSize;
in float gShade;
in vec4 gColor;
in vec2 gTrapezoidRatios;
out vec4 outColor;

vec4 applyFog(vec4 color, float depth)
{
    float fogFactor = clamp(
        (depth - uFogDistance.x) / (uFogDistance.y - uFogDistance.x), 0.0, 1.0);
    return mix(color, uFogColor, fogFactor);
}

void main(void) {
    vec4 texColor = gColor;

    // Texturing and base color
    if (gTexLayer >= 0) {
        vec3 texCoords = vec3(gTexUV.x, gTexUV.y, gTexLayer);
        if (uTrapezoidFilterEnabled != 0) {
            texCoords.xy /= gTrapezoidRatios;
        }
        texCoords.xy = clampTexAtlas(texCoords.xy, gAtlasSize);
        texColor *= texture(uTexAtlas, texCoords);
    } else {
        texColor.rgb *= texColor.a;
    }

    // Alpha discard - chroma keying || transparent pixels in the opaque pass
    if (texColor.a <= 0.0 || (uDiscardAlpha && texColor.a < 0.99 && (gFlags & VERT_NO_ALPHA_DISCARD) == 0u)) {
        discard;
    }

    // Reflections
    if ((gFlags & VERT_REFLECTIVE) != 0u && uReflectionsEnabled != 0) {
        texColor *= texture(uTexEnvMap, (normalize(gNormal) * 0.5 + 0.5).xy) * 2;
    }

    // Shading and fog
    if ((gFlags & VERT_NO_LIGHTING) == 0u) {
        texColor.rgb *= (2.0 - (max(gShade, uMinShade) / SHADE_NEUTRAL));
        if (uLightingEnabled != 0) {
            texColor = applyFog(texColor, gEyePos.z);
        }
    }

    texColor.rgb *= uGlobalTint * uBrightnessMultiplier;

    // Optional desaturation (0 = original, 1 = monochrome).
    if (uDesaturation != 0.0) {
        const vec3 luma = vec3(0.2126, 0.7152, 0.0722);
        float y = dot(texColor.rgb, luma) * 0.5;
        texColor.rgb = mix(texColor.rgb, vec3(y), clamp(uDesaturation, 0.0, 1.0));
    }

    outColor = texColor;
}

#endif
