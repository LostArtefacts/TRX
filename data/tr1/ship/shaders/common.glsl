#if defined(OGL33C) || defined(OGL43C)
    #define OUTCOLOR outColor
    #define TEXTURESIZE2D textureSize
    #define TEXELFETCH2D texelFetch
    #define TEXELFETCH1D texelFetch
    #define TEXTURE1D texture
    #define TEXTURE2D texture
    #define IN in
    #define OUT out
    out vec4 OUTCOLOR;
#else
    #define OUTCOLOR gl_FragColor
    #define TEXTURESIZE2D textureSize2D
    #define TEXELFETCH2D texelFetch2D
    #define TEXELFETCH1D texelFetch1D
    #define TEXTURE1D texture1D
    #define TEXTURE2D texture2D
    #define IN varying
    #define OUT varying
#endif

#define PI 3.1415926538

vec3 waterWibble(vec4 position, vec2 viewportSize, float wibbleOffset)
{
    // get screen coordinates
    vec3 ndc = position.xyz / position.w; //perspective divide/normalize
    vec2 viewportCoord = ndc.xy * 0.5 + 0.5; //ndc is -1 to 1 in GL. scale for 0 to 1
    vec2 viewportPixelCoord = viewportCoord * viewportSize;

    float amplitude = 2.0;
    viewportPixelCoord.x += sin((wibbleOffset + viewportPixelCoord.y) * 2.0 * PI / 32) * amplitude;
    viewportPixelCoord.y += sin((wibbleOffset + viewportPixelCoord.x) * 2.0 * PI / 32) * amplitude;

    // reverse transform
    viewportCoord = viewportPixelCoord / viewportSize;
    ndc.xy = (viewportCoord - 0.5) * 2.0;
    return ndc * position.w;
}

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
