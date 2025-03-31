#ifdef VERTEX

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inTexCoords;
layout(location = 2) in float inTexZ;
layout(location = 3) in vec4 inColor;

uniform mat4 matProjection;

out vec4 vertColor;
out vec4 vertTexCoords;
out float vertTexZ;

void main(void) {
    gl_Position = matProjection * vec4(inPosition, 1);
    vertColor = inColor / 255.0;
    vertTexCoords = inTexCoords;
    vertTexCoords.xy *= vertTexCoords.zw;
    vertTexCoords *= inTexZ;
    vertTexZ = inTexZ;
}

#elif defined(FRAGMENT)

uniform sampler2D tex0;
uniform bool texturingEnabled;
uniform bool smoothingEnabled;
uniform bool alphaPointDiscard;
uniform float alphaThreshold;
uniform float brightnessMultiplier;

in vec4 vertColor;
in vec4 vertTexCoords;
in float vertTexZ;
out vec4 outColor;

void main(void) {
    outColor = vertColor;

    vec2 texCoords = vertTexCoords.xy;
    texCoords.xy /= vertTexCoords.zw;

    if (texturingEnabled) {
        if (alphaPointDiscard && smoothingEnabled && discardTranslucent(tex0, texCoords)) {
            discard;
        }

        vec4 texColor = texture(tex0, texCoords.xy);
        if (alphaThreshold >= 0.0 && texColor.a <= alphaThreshold) {
            discard;
        }

        outColor = vec4(outColor.rgb * texColor.rgb * brightnessMultiplier, texColor.a);
    }
}
#endif
