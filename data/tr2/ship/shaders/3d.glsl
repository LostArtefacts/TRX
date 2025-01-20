#ifdef VERTEX
// Vertex shader

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inTexCoords;
layout(location = 2) in float inTexZ;
layout(location = 3) in vec4 inColor;

uniform mat4 matProjection;
uniform mat4 matModelView;

OUT vec4 vertColor;
OUT vec4 vertTexCoords;
OUT float vertTexZ;

void main(void) {
    gl_Position = matProjection * matModelView * vec4(inPosition, 1);
    vertColor = inColor / 255.0;
    vertTexCoords = inTexCoords;
    vertTexCoords.xy *= vertTexCoords.zw;
    vertTexCoords *= inTexZ;
    vertTexZ = inTexZ;
}

#else
// Fragment shader

uniform sampler2D tex0;
uniform bool texturingEnabled;
uniform bool smoothingEnabled;
uniform bool alphaPointDiscard;
uniform float alphaThreshold;
uniform float brightnessMultiplier;

IN vec4 vertColor;
IN vec4 vertTexCoords;
IN float vertTexZ;

void main(void) {
    OUTCOLOR = vertColor;

    vec2 texCoords = vertTexCoords.xy;
    texCoords.xy /= vertTexCoords.zw;

    if (texturingEnabled) {
        if (alphaPointDiscard && smoothingEnabled && discardTranslucent(tex0, texCoords)) {
            discard;
        }

        vec4 texColor = TEXTURE2D(tex0, texCoords.xy);
        if (alphaThreshold >= 0.0 && texColor.a <= alphaThreshold) {
            discard;
        }

        OUTCOLOR = vec4(OUTCOLOR.rgb * texColor.rgb * brightnessMultiplier, texColor.a);
    }
}
#endif // VERTEX
