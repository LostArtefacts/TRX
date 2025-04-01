#define NEUTRAL_SHADE 0x1000

#ifdef VERTEX

uniform samplerBuffer uUVW; // texture u, v, layer
uniform vec2 uViewportSize;
uniform mat4 uMatProjection;
uniform mat4 uMatModelView;
uniform float uWibbleOffset;
uniform vec2 uFog; // x = start, y = end

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inDisplacement;
layout(location = 2) in int inTextureIdx;
layout(location = 3) in float inShade;

out vec2 gTexUV;
flat out int gTexLayer;
out float gShade;

void main(void) {
    vec4 centerEyeSpace = uMatModelView * vec4(inPosition, 1.0);
    centerEyeSpace.xy += inDisplacement;
    gl_Position = uMatProjection * centerEyeSpace;

    if (uWibbleOffset >= 0.0) {
        gl_Position.xyz =
            waterWibble(gl_Position, uViewportSize, uWibbleOffset);
    }

    vec3 uvw = texelFetch(uUVW, int(inTextureIdx)).xyz;
    gTexUV = uvw.xy;
    gTexLayer = int(uvw.z);
    gShade = inShade;
}

#elif defined(FRAGMENT)

uniform sampler2DArray uTexture;
uniform bool uSmoothingEnabled;
uniform float uBrightnessMultiplier;
uniform vec3 uGlobalTint;

in vec2 gTexUV;
flat in int gTexLayer;
in float gShade;
out vec4 outColor;

void main(void) {
    vec3 uvw = vec3(gTexUV.x, gTexUV.y, gTexLayer);
    vec4 texColor = texture(uTexture, uvw);
    if (uSmoothingEnabled && discardTranslucent(uTexture, uvw)) {
        discard;
    }

    if (texColor.a <= 0.0) {
        discard;
    }

    texColor.rgb *= 2.0 - (gShade / NEUTRAL_SHADE);
    texColor.rgb *= uBrightnessMultiplier;
    texColor.rgb *= uGlobalTint;
    outColor = vec4(texColor.rgb, 1.0);
}
#endif
