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
layout(location = 7) in float inShade1;
layout(location = 8) in float inShade2;

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
    vec2 screenPos = ndc.xy * 0.5 + 0.5;
    vec2 pixelPos = screenPos * uViewportSize;

    pixelPos.x += sin((uTimeInGame + pixelPos.y) * 2.0 * PI / WIBBLE_SIZE) * MAX_WIBBLE;
    pixelPos.y += sin((uTimeInGame + pixelPos.x) * 2.0 * PI / WIBBLE_SIZE) * MAX_WIBBLE;

    // reverse transform
    screenPos = pixelPos / uViewportSize;
    ndc.xy = (screenPos - 0.5) * 2.0;
    return ndc * position.w;
}

void main(void) {
    vec4 worldPos = uMatModel * vec4(inPosition.xyz, 1.0);

    vec4 eyePos;
    if ((inFlags & VERT_ABS_SPRITE) != 0u) {
        eyePos = offsetBillboard(
            inPosition.xyz, inNormal.xy, uMatView, uMatModel, uMatProj, BILLBOARD_LOCK_NONE);
    } else if ((inFlags & VERT_BILLBOARD) != 0u) {
        eyePos = offsetBillboard(
            inPosition.xyz, inNormal.xy, uMatView, uMatModel, uMatProj, uBillboardLockMode);
    } else {
        eyePos = uMatView * worldPos;
    }

    gEyePos = eyePos;
    gNormal = inNormal.xyz;
    gl_Position = uMatProj * eyePos;
    gl_Position.z += inPosition.w;

    // apply water wibble effect only to non-sprite vertices
    if ((uWibbleEffect
        && (inFlags & VERT_NO_CAUSTICS) == 0u
        && (inFlags & VERT_BILLBOARD) == 0u)
    ) {
        gl_Position.xyz = waterWibble(gl_Position);
    }

    gFlags = inFlags;
    gAtlasSize = inTextureSize;
    gTexUV = inUVW.xy;
    gTexLayer = int(inUVW.z);
    gTrapezoidRatios = inTrapezoidRatios;
    if (uTrapezoidFilterEnabled != 0) {
        gTexUV *= inTrapezoidRatios;
    }
    gShade = inShade1;
    gColor = inColor;

    if (uLightingEnabled == 0) {
        gShade = SHADE_NEUTRAL;
    } else if ((gFlags & VERT_NO_LIGHTING) == 0u) {
        float newShade = light(
            inShade2,
            gFlags,
            inNormal.xyz,
            worldPos,
            inNormal.w);

        if (uUseNewLighting == 0) {
            // OG lighting, untextured
            gFlags |= VERT_FLAT_SHADED;
        } else if (uUseNewLighting == 1) {
            // New lighting, untextured
            gFlags |= VERT_FLAT_SHADED;
            gShade = newShade;
        } else if (uUseNewLighting == 2) {
            // OG lighting, textured
        } else if (uUseNewLighting == 3) {
            // New lighting, textured
            gShade = newShade;
        }
    }
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

void main(void) {
    vec4 texColor = gColor;

    if ((gFlags & VERT_FLAT_SHADED) == 0u && gTexLayer >= 0) {
        vec3 texCoords = vec3(gTexUV.x, gTexUV.y, gTexLayer);
        if (uTrapezoidFilterEnabled != 0) {
            texCoords.xy /= gTrapezoidRatios;
        }
        texCoords.xy = clampTexAtlas(texCoords.xy, gAtlasSize);

        texColor *= texture(uTexAtlas, texCoords);
        if (texColor.a <= 0.0) {
            discard;
        }
        if (uDiscardAlpha && texColor.a < 0.99 && (gFlags & VERT_NO_ALPHA_DISCARD) == 0u) {
            discard;
        }
    } else {
        if (uDiscardAlpha && texColor.a < 0.99 && (gFlags & VERT_NO_ALPHA_DISCARD) == 0u) {
            discard;
        }
        texColor.rgb *= texColor.a;
    }

    if ((gFlags & VERT_REFLECTIVE) != 0u && uReflectionsEnabled != 0) {
        texColor *= texture(uTexEnvMap, (normalize(gNormal) * 0.5 + 0.5).xy) * 2;
    }

    if ((gFlags & VERT_NO_LIGHTING) == 0u) {
        texColor.rgb = applyShade(texColor.rgb, gShade);
    }

    texColor.rgb *= uGlobalTint;

    if ((gFlags & VERT_NO_LIGHTING) == 0u && uLightingEnabled != 0) {
        texColor = applyFog(texColor, gEyePos.z);
    }

    texColor.rgb *= uBrightnessMultiplier;

    outColor = texColor;
}

#endif
