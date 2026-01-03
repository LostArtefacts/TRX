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

vec3 gammaCurve(vec3 rgb, float gamma_exp)
{
    vec3 in8 = floor(clamp(rgb, 0.0, 1.0) * 255.0 + 0.5);
    vec3 out8 = floor(pow(in8 / 256.0, vec3(gamma_exp)) * 256.0);
    return clamp(out8 / 255.0, 0.0, 1.0);
}

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

    // The vertex diffuse is lit first and then modulated by the texture (or by
    // the flat polygon's palette color). Keep the lighting component separate
    // from the base color so gamma is applied in the right place.
    LightingResult lr =
        light(inShade, gFlags, inNormal.xyz, worldPos, inNormal.w, effectPhase);
    gShade = lr.shade;

    float gamma_exp = 1.0 / ((uGamma / 10.0) * 4.0);

#if TR_VERSION >= 3
    // Apply to the lit vertex diffuse (8-bit) before any modulation.
    lr.color.rgb = gammaCurve(lr.color.rgb, gamma_exp);

    // Room vertices in TR3 already have their lighting baked into the
    // per-vertex color; apply gamma to that base color too so rooms react
    // to the gamma option.
    vec4 baseColor = inColor;
    if ((gFlags & VERT_FLAT_SHADED) == 0u) {
        if (uLightingEnabled == 0) {
            baseColor.rgb = vec3(1);
        }
        baseColor.rgb = gammaCurve(baseColor.rgb, gamma_exp);
    }

#else
    float shade_mul = 1.0;
    if ((gFlags & VERT_NO_LIGHTING) == 0u) {
        shade_mul = (2.0 - (max(gShade, uMinShade) / SHADE_NEUTRAL));
    }

    // `shade_mul` is roughly in [0..2]. Remap to [0..1], apply the gamma
    // curve, and restore the range. Use sqrt() to limit the effect scope,
    // since we're applying it to the shade (TR1-2) rather than RGB (TR3).
    lr.color.rgb = gammaCurve(vec3(shade_mul * 0.5), sqrt(gamma_exp)) * 2.0;

    vec4 baseColor = inColor;
    if ((gFlags & VERT_FLAT_SHADED) == 0u) {
        baseColor.rgb = gammaCurve(baseColor.rgb, gamma_exp);
    }
#endif

    gColor = vec4(baseColor.rgb * lr.color.rgb, baseColor.a);
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
    if (texColor.a <= 0.0
        || (uDiscardAlpha && texColor.a < 0.99
            && (gFlags & VERT_NO_ALPHA_DISCARD) == 0u)) {
        discard;
    }

    // Reflections
    if ((gFlags & VERT_REFLECTIVE) != 0u && uReflectionsEnabled != 0) {
        texColor *= texture(uTexEnvMap, (normalize(gNormal) * 0.5 + 0.5).xy) * 2;
    }

    // Fog
    if ((gFlags & VERT_NO_LIGHTING) == 0u && uLightingEnabled != 0) {
        texColor = applyFog(texColor, gEyePos.z);
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
