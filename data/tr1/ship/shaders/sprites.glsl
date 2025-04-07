#ifdef VERTEX

uniform samplerBuffer uUVW; // texture u, v, layer
uniform vec2 uViewportSize;
uniform mat4 uMatProjection;
uniform mat4 uMatModelView;
uniform bool uWibbleEffect;
uniform int uTime;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inDisplacement;
layout(location = 2) in int inUVWIdx;
layout(location = 3) in float inShade;

out vec4 gWorldPos;
out vec2 gTexUV;
flat out int gTexLayer;
out float gShade;

void main(void) {
    vec4 centerEyeSpace = uMatModelView * vec4(inPosition, 1.0);
    gWorldPos = centerEyeSpace;
    centerEyeSpace.xy += inDisplacement;
    gl_Position = uMatProjection * centerEyeSpace;

    if (uWibbleEffect) {
        gl_Position.xyz =
            waterWibble(gl_Position, uViewportSize, uTime);
    }

    vec3 uvw = texelFetch(uUVW, int(inUVWIdx)).xyz;
    gTexUV = uvw.xy;
    gTexLayer = int(uvw.z);
    gShade = inShade;
}

#elif defined(FRAGMENT)

uniform sampler2DArray uTexAtlas;
uniform sampler2D uTexEnvMap;
uniform bool uSmoothingEnabled;
uniform float uBrightnessMultiplier;
uniform vec3 uGlobalTint;
uniform vec2 uFog; // x = fog start, y = fog end

in vec4 gWorldPos;
in vec2 gTexUV;
flat in int gTexLayer;
in float gShade;
out vec4 outColor;

void main(void) {
    vec3 uvw = vec3(gTexUV.x, gTexUV.y, gTexLayer);
    vec4 texColor = texture(uTexAtlas, uvw);
    if (uSmoothingEnabled && discardTranslucent(uTexAtlas, uvw)) {
        discard;
    }

    if (texColor.a <= 0.0) {
        discard;
    }

    float shade = gShade;
    shade = shadeFog(shade, gWorldPos.z, uFog);
    texColor.rgb = applyShade(texColor.rgb, shade);
    texColor.rgb *= uGlobalTint;
    texColor.rgb *= uBrightnessMultiplier;
    outColor = vec4(texColor.rgb, 1.0);
}
#endif
