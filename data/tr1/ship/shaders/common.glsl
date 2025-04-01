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
    // do not use smoothing for chroma key
    ivec2 size = textureSize(tex, 0);
    ivec2 texCoordsNN = ivec2(uv.xy * size.xy) % size.xy;
    vec4 texel = texelFetch(tex, texCoordsNN, 0);
    return texel.a == 0.0;
}

bool discardTranslucent(sampler2DArray tex, vec3 uvw)
{
    // do not use smoothing for chroma key
    ivec2 size = textureSize(tex, 0).xy;
    ivec3 texCoordsNN = ivec3(ivec2(uvw.xy * size.xy) % size.xy, uvw.z);
    vec4 texel = texelFetch(tex, texCoordsNN, 0);
    return texel.a == 0.0;
}
