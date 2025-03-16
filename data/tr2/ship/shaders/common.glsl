bool discardTranslucent(sampler2D tex, vec2 uv)
{
    // do not use smoothing for chroma key
    ivec2 size = textureSize(tex, 0);
    ivec2 texCoordsNN = ivec2(uv.xy * size.xy) % size.xy;
    vec4 texel = texelFetch(tex, texCoordsNN, 0);
    return texel.a == 0.0;
}
