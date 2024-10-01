#ifdef VERTEX
// Vertex shader

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inTexCoords;
layout(location = 2) in vec4 inColor;

uniform mat4 matProjection;
uniform mat4 matModelView;

#ifdef OGL33C
    out vec4 vertColor;
    out vec3 vertTexCoords;
#else
    varying vec4 vertColor;
    varying vec3 vertTexCoords;
#endif

void main(void) {
    gl_Position = matProjection * matModelView * vec4(inPosition, 1);
    vertColor = inColor / 255.0;
    vertTexCoords = inTexCoords;
}

#else
// Fragment shader

uniform sampler2D tex0;
uniform bool texturingEnabled;
uniform bool smoothingEnabled;

#ifdef OGL33C
    #define OUTCOLOR outColor
    #define TEXTURESIZE textureSize
    #define TEXTURE texture
    #define TEXELFETCH texelFetch

    in vec4 vertColor;
    in vec3 vertTexCoords;
    out vec4 OUTCOLOR;
#else
    #define OUTCOLOR gl_FragColor
    #define TEXTURESIZE textureSize2D
    #define TEXELFETCH texelFetch2D
    #define TEXTURE texture2D

    varying vec4 vertColor;
    varying vec3 vertTexCoords;
#endif

void main(void) {
    OUTCOLOR = vertColor;

    if (texturingEnabled) {
#if defined(GL_EXT_gpu_shader4) || defined(OGL33C)
        if (smoothingEnabled) {
            // do not use smoothing for chroma key
            ivec2 size = TEXTURESIZE(tex0, 0);
            int tx = int((vertTexCoords.x / vertTexCoords.z) * size.x) % size.x;
            int ty = int((vertTexCoords.y / vertTexCoords.z) * size.y) % size.y;
            vec4 texel = TEXELFETCH(tex0, ivec2(tx, ty), 0);
            if (texel.a == 0.0) {
                discard;
            }
        }
#endif

        vec4 texColor = TEXTURE(tex0, vertTexCoords.xy / vertTexCoords.z);
        if (texColor.a == 0.0) {
            discard;
        }

        OUTCOLOR = vec4(OUTCOLOR.rgb * texColor.rgb, texColor.a);
    }
}
#endif // VERTEX
