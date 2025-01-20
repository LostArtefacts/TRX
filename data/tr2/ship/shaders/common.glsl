#if defined(OGL33C) || defined(OGL43C)
    #define OUTCOLOR outColor
    #define TEXTURESIZE2D textureSize
    #define TEXELFETCH2D texelFetch
    #define TEXTURE1D texture
    #define TEXTURE2D texture
    #define IN in
    #define OUT out
    out vec4 OUTCOLOR;
#else
    #define OUTCOLOR gl_FragColor
    #define TEXTURESIZE2D textureSize2D
    #define TEXELFETCH2D texelFetch2D
    #define TEXTURE1D texture1D
    #define TEXTURE2D texture2D
    #define IN varying
    #define OUT varying
#endif

#define PI 3.1415926538

bool discardTranslucent(sampler2D tex, vec2 uv)
{
#if defined(GL_EXT_gpu_shader4) || defined(OGL33C) || defined(OGL43C)
    // do not use smoothing for chroma key
    ivec2 size = TEXTURESIZE2D(tex, 0);
    ivec2 texCoordsNN = ivec2(uv.xy * size.xy) % size.xy;
    vec4 texel = TEXELFETCH2D(tex, texCoordsNN, 0);
    return texel.a == 0.0;
#else
    return false;
#endif
}
