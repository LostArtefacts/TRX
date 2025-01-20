#define TEXTURE_3D 1

#ifdef VERTEX
// Vertex shader

uniform vec2 uViewportSize;
uniform mat4 uMatProjection;
uniform mat4 uMatModelView;
uniform float uWibbleOffset;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inDisplacement;
#if TEXTURE_3D
layout(location = 2) in float inTextureIdx;
#else
layout(location = 2) in vec2 inUV;
#endif
layout(location = 3) in float inShade;

#if TEXTURE_3D
uniform samplerBuffer uFrame;
#else
#endif
OUT vec2 gUV;
OUT float gShade;

void main(void) {
    vec4 centerEyeSpace = uMatModelView * vec4(inPosition, 1.0);
    centerEyeSpace.xy += inDisplacement;
    gl_Position = uMatProjection * centerEyeSpace;

    if (uWibbleOffset >= 0.0) {
        gl_Position.xyz = waterWibble(gl_Position, uViewportSize, uWibbleOffset);
    }

    #if TEXTURE_3D
    gUV = TEXELFETCH1D(uFrame, int(inTextureIdx)).xy;
    #else
    gUV = inUV;
    #endif
    gShade = inShade;
}

#else
// Fragment shader

#define NEUTRAL_SHADE 0x1000

uniform sampler2D uTexture;
uniform bool uSmoothingEnabled;
uniform float uBrightnessMultiplier;

IN vec2 gUV;
IN float gShade;

void main(void) {
    #if TEXTURE_3D
    vec4 texColor = TEXTURE2D(uTexture, gUV);
    #else
    if (uSmoothingEnabled && discardTranslucent(uTexture, gUV)) {
        discard;
    }

    vec4 texColor = TEXTURE2D(uTexture, gUV);
    #endif
    if (texColor.a <= 0.0) {
        discard;
    }
    texColor.rgb *= 2.0 - (gShade / NEUTRAL_SHADE);
    texColor.rgb *= uBrightnessMultiplier;
    OUTCOLOR = vec4(texColor.rgb, 1.0);
}
#endif // VERTEX
