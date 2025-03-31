#define NEUTRAL_SHADE 0x1000
#define NO_VERT_MOVE 0x2000

#ifdef VERTEX

uniform samplerBuffer uUVW; // texture u, v, layer
uniform vec2 uViewportSize;
uniform mat4 uMatProjection;
uniform mat4 uMatModelView;
uniform float uWibbleOffset;
uniform bool uTrapezoidFilterEnabled;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in int inUVWIdx;
layout(location = 2) in vec2 inTrapezoidRatios;
layout(location = 3) in int inFlags;
layout(location = 4) in float inShade;

out vec2 gTexUV;
flat out int gTexLayer;
out vec2 gTrapezoidRatios;
out float gShade;

void main(void) {
    gl_Position = uMatProjection * uMatModelView * vec4(inPosition, 1.0);

    if (uWibbleOffset >= 0.0 && (inFlags & NO_VERT_MOVE) == 0) {
        gl_Position.xyz =
            waterWibble(gl_Position, uViewportSize, uWibbleOffset);
    }

    vec3 uvw = texelFetch(uUVW, int(inUVWIdx)).xyz;
    gTexUV = uvw.xy;
    gTexLayer = int(uvw.z);
    gTrapezoidRatios = inTrapezoidRatios;
    if (uTrapezoidFilterEnabled) {
        gTexUV *= inTrapezoidRatios;
    }
    gShade = inShade;
}

#elif defined(FRAGMENT)

uniform sampler2DArray uTexture;
uniform bool uSmoothingEnabled;
uniform bool uTrapezoidFilterEnabled;
uniform float uAlphaThreshold;
uniform float uBrightnessMultiplier;
uniform vec3 uGlobalTint;

in vec2 gTexUV;
flat in int gTexLayer;
in float gShade;
in vec2 gTrapezoidRatios;
out vec4 outColor;

void main(void) {
    vec4 texColor = vec4(1);
    vec3 texCoords = vec3(gTexUV.x, gTexUV.y, gTexLayer);
    if (texCoords.z >= 0) {
        if (uTrapezoidFilterEnabled) {
            texCoords.xy /= gTrapezoidRatios.xy;
        }

        if (uSmoothingEnabled && discardTranslucent(uTexture, texCoords)) {
            discard;
        }

        texColor = texture(uTexture, texCoords);
        if (uAlphaThreshold >= 0.0 && texColor.a <= uAlphaThreshold) {
            discard;
        }
    }

    texColor.rgb *= 2.0 - (gShade / NEUTRAL_SHADE);
    texColor.rgb *= uBrightnessMultiplier;
    texColor.rgb *= uGlobalTint;
    outColor = vec4(texColor.rgb, 1.0);
}
#endif
