#ifdef VERTEX
// Vertex shader

uniform vec2 uViewportSize;
uniform mat4 uMatProjection;
uniform mat4 uMatModelView;
uniform float uWibbleOffset;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inDisplacement;
layout(location = 2) in int inTextureIdx;
layout(location = 3) in float inShade;

uniform samplerBuffer uFrame;
OUT vec3 gUV; // x = u, y = v, z = layer
OUT float gShade;

void main(void) {
    vec4 centerEyeSpace = uMatModelView * vec4(inPosition, 1.0);
    centerEyeSpace.xy += inDisplacement;
    gl_Position = uMatProjection * centerEyeSpace;

    if (uWibbleOffset >= 0.0) {
        gl_Position.xyz = waterWibble(gl_Position, uViewportSize, uWibbleOffset);
    }

    gUV = TEXELFETCH1D(uFrame, int(inTextureIdx)).xyz;
    gShade = inShade;
}

#else
// Fragment shader

#define NEUTRAL_SHADE 0x1000

uniform sampler2D uTexture;
uniform bool uSmoothingEnabled;
uniform float uBrightnessMultiplier;

IN vec3 gUV;
IN float gShade;

void main(void) {
    vec4 texColor = TEXTURE2D(uTexture, gUV.xy);
    if (uSmoothingEnabled && discardTranslucent(uTexture, gUV.xy)) {
        discard;
    }

    if (texColor.a <= 0.0) {
        discard;
    }
    texColor.rgb *= 2.0 - (gShade / NEUTRAL_SHADE);
    texColor.rgb *= uBrightnessMultiplier;
    OUTCOLOR = vec4(texColor.rgb, 1.0);
}
#endif // VERTEX
