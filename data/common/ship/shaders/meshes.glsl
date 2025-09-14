uniform int uBillboardLockMode;
uniform int uLightingContrast;

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

    } else {
        return (modelView * vec4(pos, 1.0)) + vec4(displacement.xy, 0, 0);
    }
}

#ifdef VERTEX

uniform mat4 uMatProjection;
uniform mat4 uMatView;
uniform mat4 uMatModelView;
uniform bool uTrapezoidFilterEnabled;
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

    if ((inFlags & VERT_ABS_SPRITE) != 0u) {
        eyePos = offsetBillboard(
            inPosition.xyz, inNormal.xy, uMatView, uMatModelView, uMatProjection, BILLBOARD_LOCK_NONE);
    } else if ((inFlags & VERT_BILLBOARD) != 0u) {
        eyePos = offsetBillboard(
            inPosition.xyz, inNormal.xy, uMatView, uMatModelView, uMatProjection, uBillboardLockMode);
    } else {
        eyePos = uMatModelView * vec4(inPosition.xyz, 1.0);
    }

    gEyePos = eyePos;
    gNormal = inNormal;
    gl_Position = uMatProjection * eyePos;
    gl_Position.z += inPosition.w;

    // apply water wibble effect only to non-sprite vertices
    if ((uWibbleEffect
        && (inFlags & VERT_NO_CAUSTICS) == 0u
        && (inFlags & VERT_BILLBOARD) == 0u)
    ) {
        gl_Position.xyz = waterWibble(gl_Position, uViewportSize, uTimeInGame);
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

uniform sampler2DArray uTexAtlas;
uniform sampler2D uTexEnvMap;
uniform bool uSmoothingEnabled;
uniform bool uTrapezoidFilterEnabled;
uniform int uLightingMode;
uniform bool uReflectionsEnabled;
uniform vec3 uGlobalTint;
uniform vec2 uFogDistance; // x = fog start, y = fog end
uniform vec4 uFogColor;

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
        if (uTrapezoidFilterEnabled) {
            texCoords.xy /= gTrapezoidRatios;
        }
        texCoords.xy = clampTexAtlas(texCoords.xy, gAtlasSize);

        texColor *= texture(uTexAtlas, texCoords);
        if (texColor.a <= 0.0) {
            discard;
        }
    } else {
        texColor.rgb *= texColor.a;
    }

    if ((gFlags & VERT_REFLECTIVE) != 0u && uReflectionsEnabled) {
        texColor *= texture(uTexEnvMap, (normalize(gNormal) * 0.5 + 0.5).xy) * 2;
    }

    if ((gFlags & VERT_NO_LIGHTING) == 0u && uLightingMode != LIGHTING_MODE_OFF) {
        texColor.rgb = applyShade(texColor.rgb, gShade, uLightingContrast);
    }

    texColor.rgb *= uGlobalTint;

    if ((gFlags & VERT_NO_LIGHTING) == 0u && uLightingMode == LIGHTING_MODE_FULL) {
        texColor = applyFog(texColor, gEyePos.z, uFogDistance, uFogColor);
    }

    texColor.rgb *= uBrightnessMultiplier;

    outColor = texColor;
}
#endif
