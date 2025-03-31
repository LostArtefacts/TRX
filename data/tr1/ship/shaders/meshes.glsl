#ifdef VERTEX

uniform int uTime;
uniform samplerBuffer uUVW; // texture u, v, layer
uniform vec2 uViewportSize;
uniform mat4 uMatProjection;
uniform mat4 uMatModelView;
uniform bool uTrapezoidFilterEnabled;
uniform bool uWibbleEffect;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in int inUVWIdx;
layout(location = 2) in vec2 inTrapezoidRatios;
layout(location = 3) in int inFlags;
layout(location = 4) in float inShade;

out vec4 gWorldPos;
flat out int gFlags;
flat out int gTexLayer;
out vec2 gTexUV;
out vec2 gTrapezoidRatios;
out float gShade;

void main(void) {
    gWorldPos = uMatModelView * vec4(inPosition.xyz, 1.0);
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
}

#elif defined(FRAGMENT)

uniform int uTime;
uniform sampler2DArray uTexAtlas;
uniform bool uSmoothingEnabled;
uniform bool uAlphaDiscardEnabled;
uniform bool uTrapezoidFilterEnabled;
uniform float uAlphaThreshold;
uniform float uBrightnessMultiplier;
uniform vec3 uGlobalTint;
uniform vec2 uFog; // x = fog start, y = fog end
uniform bool uWaterEffect;

in vec4 gWorldPos;
flat in int gFlags;
flat in int gTexLayer;
in vec2 gTexUV;
in vec2 gTrapezoidRatios;
in float gShade;
out vec4 outColor;

void main(void) {
    vec4 texColor = vec4(1);
    vec3 texCoords = vec3(gTexUV.x, gTexUV.y, gTexLayer);
    if (uTrapezoidFilterEnabled) {
        texCoords.xy /= gTrapezoidRatios.xy;
    }

    if (texCoords.z >= 0) {
        if (uAlphaDiscardEnabled && uSmoothingEnabled && discardTranslucent(uTexAtlas, texCoords)) {
            discard;
        }

        texColor = texture(uTexAtlas, texCoords);
        if (uAlphaThreshold >= 0.0 && texColor.a <= uAlphaThreshold) {
            discard;
        }
    }

    float shade = gShade;
    shade = shadeFog(shade, gWorldPos.z, uFog);

    texColor.rgb = applyShade(texColor.rgb, shade);
    texColor.rgb *= uBrightnessMultiplier;
    texColor.rgb *= uGlobalTint;
    outColor = vec4(texColor.rgb, 1.0);
}
#endif
