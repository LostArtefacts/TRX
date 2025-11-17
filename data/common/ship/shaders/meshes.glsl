#include "common.glsl"

vec4 offsetBillboard(vec3 pos, vec2 disp, mat4 view, mat4 model, mat4 proj, int mode)
{
    const vec3 worldUp = vec3(0,1,0);

    vec3 right, up;
    if (mode == BILLBOARD_LOCK_NONE) {
        right = normalize(vec3(view[0][0], view[1][0], view[2][0]));
        up    = normalize(vec3(view[0][1], view[1][1], view[2][1]));
    } else {
        // Base forward for all locked modes
        vec3 forward = -normalize(vec3(view[0][2], view[1][2], view[2][2]));

        if (mode != BILLBOARD_LOCK_ROLL) {
            // Kill pitch if requested by any cylindrical/perspective mode
            forward = normalize(vec3(forward.x, 0.0, forward.z));
        }

        if (mode == BILLBOARD_LOCK_PERSPECTIVE) {
            vec4 clip     = proj * view * model * vec4(pos,1);
            float ndcX    = clip.x / clip.w;
            vec3 yawRight = normalize(cross(forward, worldUp));
            float inv     = inversesqrt(1.0 + ndcX * ndcX);
            forward       = normalize(inv * forward - (ndcX * inv) * yawRight);
        }

        right = normalize(cross(forward, worldUp));
        up    = normalize(cross(right, forward));
    }

    vec4 wp = model * vec4(pos,1);
    wp.xyz += disp.x * right + disp.y * up;
    return view * wp;
}


#ifdef VERTEX

uniform mat4 uMatModel;
uniform bool uWibbleEffect;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
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

void main(void) {
    vec4 eyePos;

    mat4 matProjection = uProjectionMode == PROJECTION_MODE_ORTHOGRAPHIC ? uMatProjOrtho : uMatProjPersp;

    if ((inFlags & VERT_ABS_SPRITE) != 0u) {
        eyePos = offsetBillboard(
            inPosition.xyz, inNormal.xy, uMatView, uMatModel, matProjection, BILLBOARD_LOCK_NONE);
    } else if ((inFlags & VERT_BILLBOARD) != 0u) {
        eyePos = offsetBillboard(
            inPosition.xyz, inNormal.xy, uMatView, uMatModel, matProjection, uBillboardLockMode);
    } else {
        eyePos = uMatView * uMatModel * vec4(inPosition.xyz, 1.0);
    }

    gEyePos = eyePos;
    gNormal = inNormal;
    gl_Position = matProjection * eyePos;
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
    gShade = inShade;
    gColor = inColor;
}

#elif defined(FRAGMENT)

uniform sampler2DArray uTexAtlas;
uniform sampler2D uTexEnvMap;
uniform int uLightingMode;
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

    if ((gFlags & VERT_NO_LIGHTING) == 0u && uLightingMode != LIGHTING_MODE_OFF) {
        texColor.rgb = applyShade(texColor.rgb, gShade);
    }

    texColor.rgb *= uGlobalTint;

    if ((gFlags & VERT_NO_LIGHTING) == 0u && uLightingMode == LIGHTING_MODE_FULL) {
        texColor = applyFog(texColor, gEyePos.z);
    }

    texColor.rgb *= uBrightnessMultiplier;

    outColor = texColor;
}
#endif
