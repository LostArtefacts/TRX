#ifdef VERTEX
// Vertex shader

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in float inTexZ;
layout(location = 3) in vec4 inColor;

uniform mat4 matProjection;
uniform mat4 matModelView;

#ifdef OGL33C
    out vec4 vertColor;
    out vec2 vertTexCoords;
    out float vertTexZ;
#else
    varying vec4 vertColor;
    varying vec2 vertTexCoords;
    varying float vertTexZ;
#endif

void main(void) {
    gl_Position = matProjection * matModelView * vec4(inPosition, 1);
    vertColor = inColor / 255.0;
    vertTexCoords = inTexCoords;
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

#ifdef OGL33C
    #define OUTCOLOR outColor
    #define TEXTURESIZE textureSize
    #define TEXTURE texture
    #define TEXELFETCH texelFetch

    in vec4 vertColor;
    in vec2 vertTexCoords;
    in float vertTexZ;
    out vec4 OUTCOLOR;
#else
    #define OUTCOLOR gl_FragColor
    #define TEXTURESIZE textureSize2D
    #define TEXELFETCH texelFetch2D
    #define TEXTURE texture2D

    varying vec4 vertColor;
    varying vec2 vertTexCoords;
    varying float vertTexZ;
#endif

void main(void) {
    OUTCOLOR = vertColor;

    vec2 texCoords = vertTexCoords.xy;
    texCoords.xy /= vertTexZ;

    if (texturingEnabled) {
#if defined(GL_EXT_gpu_shader4) || defined(OGL33C)
        if (alphaPointDiscard && smoothingEnabled) {
            // do not use smoothing for chroma key
            ivec2 size = TEXTURESIZE(tex0, 0);
            ivec2 texCoordsNN = ivec2(texCoords.xy * size.xy) % size.xy;
            vec4 texel = TEXELFETCH(tex0, texCoordsNN, 0);
            if (texel.a == 0.0) {
                discard;
            }
        }
#endif

        vec4 texColor = TEXTURE(tex0, texCoords.xy);
        if (alphaThreshold >= 0.0 && texColor.a <= alphaThreshold) {
            discard;
        }

        OUTCOLOR = vec4(OUTCOLOR.rgb * texColor.rgb * brightnessMultiplier, texColor.a);
    }
}
#endif // VERTEX
