#define NEW_ZBUFFER 0
#define NEUTRAL_SHADE 0x1000

#ifdef VERTEX

uniform samplerBuffer uFrame; // texture u, v, layer
uniform vec2 uViewportCenter;
uniform vec2 uViewportSize;
uniform mat4 uMatProjection;
uniform mat4 uMatProjectionOG;
uniform mat4 uMatModelView;
uniform float uWibbleOffset;
uniform vec2 uFog; // x = start, y = end

uniform float uPhdPersp;
uniform float uPhdResZ;
uniform float uPhdResZBuf;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inDisplacement;
layout(location = 2) in int inTextureIdx;
layout(location = 3) in float inShade;

out vec3 gUV; // x = u, y = v, z = layer
out float gShade;

void main(void) {
    #if NEW_ZBUFFER
    vec4 centerEyeSpace = uMatModelView * vec4(inPosition, 1.0);
    centerEyeSpace.xy += inDisplacement;
    gl_Position = uMatProjection * centerEyeSpace;

    if (uWibbleOffset >= 0.0) {
        gl_Position.xyz =
            waterWibble(gl_Position, uViewportSize, uWibbleOffset);
    }
    #else
    vec3 localPos = inPosition;
    vec3 worldPos = (uMatModelView * vec4(localPos, 1)).xyz;

    if ((uMatProjection * vec4(worldPos, 1)).z <= 0) {
        // Terrible hack:
        // Push the vertex out of view (e.g. offscreen or behind clip).
        // Works for entire sprites, because worldPos is the position of the
        // sprite origin, the same for all vertices.
        gl_Position = vec4(2.0, 2.0, 1.0, 1.0);
        return;
    }

    vec3 screenCenterPos = vec3(
        uViewportCenter + worldPos.xy * uPhdPersp / worldPos.z,
        uPhdResZBuf - uPhdResZ / worldPos.z);
    vec3 screenCornerPos = screenCenterPos;
    screenCornerPos.xy += inDisplacement * uPhdPersp / worldPos.z;
    gl_Position = (uMatProjectionOG * vec4(screenCornerPos, 1));
    #endif

    gUV = texelFetch(uFrame, int(inTextureIdx)).xyz;
    gShade = inShade;
}

#elif defined(FRAGMENT)

uniform sampler2DArray uTexture;
uniform bool uSmoothingEnabled;
uniform float uBrightnessMultiplier;

in vec3 gUV;
in float gShade;
out vec4 outColor;

void main(void) {
    vec4 texColor = texture(uTexture, gUV);
    if (uSmoothingEnabled && discardTranslucent(uTexture, gUV)) {
        discard;
    }

    if (texColor.a <= 0.0) {
        discard;
    }

    texColor.rgb *= 2.0 - (gShade / NEUTRAL_SHADE);
    texColor.rgb *= uBrightnessMultiplier;
    outColor = vec4(texColor.rgb, 1.0);
}
#endif
