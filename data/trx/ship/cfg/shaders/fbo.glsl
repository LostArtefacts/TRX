#ifdef VERTEX

layout(location = 0) in vec2 inPosition;

out vec2 vertTexCoords;

void main(void) {
    vertTexCoords = inPosition;
    gl_Position = vec4(vertTexCoords * vec2(2.0, 2.0) + vec2(-1.0, -1.0), 0.0, 1.0);
}

#elif defined(FRAGMENT)

uniform sampler2D uTex0;
uniform int uDitherMode;
uniform int uSupersample;

in vec2 vertTexCoords;
out vec4 outColor;

const float BAYER[16] = float[16](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0);

// Levels of a 3-3-2 bit color, the arrangement 8-bit displays gave the
// channels.
const vec3 DITHER_STEPS_8_BIT = vec3(7.0, 7.0, 3.0);

#define DITHER_MODE_SOFTWARE_RENDERER 1
#define DITHER_MODE_PS1               2

// Indexing by source texel rather than by output pixel keeps the pattern tied
// to the rendered image, so upscaling magnifies it along with the geometry.
vec3 dither(vec3 rgb, vec2 texel, vec3 steps)
{
    ivec2 pos = ivec2(texel) & 3;
    float bias = (BAYER[pos.y * 4 + pos.x] + 0.5) / 16.0;
    return floor(rgb * steps + bias) / steps;
}

vec3 ditherPS1(vec3 rgb, vec2 texel)
{
    ivec2 pos = ivec2(texel) & 3;
    float offset = floor(BAYER[pos.y * 4 + pos.x] * 0.5) - 4.0;
    vec3 channel = clamp(rgb * 255.0 + offset, 0.0, 255.0);
    return floor(channel / 8.0) / 31.0;
}

// Average the block of source texels the output pixel covers. The block is
// aligned to the output grid rather than centered on the sample point, so the
// result is the exact box average the supersampled image was rendered for.
vec4 resolve(vec2 uv)
{
    ivec2 base = ivec2(uv * vec2(textureSize(uTex0, 0)));
    base -= base % uSupersample;
    vec4 sum = vec4(0.0);
    for (int y = 0; y < uSupersample; y++) {
        for (int x = 0; x < uSupersample; x++) {
            sum += texelFetch(uTex0, base + ivec2(x, y), 0);
        }
    }
    return sum / float(uSupersample * uSupersample);
}

void main(void) {
    outColor = uSupersample > 1 ? resolve(vertTexCoords)
                                : texture(uTex0, vertTexCoords);
    if (uDitherMode == DITHER_MODE_SOFTWARE_RENDERER) {
        outColor.rgb = dither(
            outColor.rgb, vertTexCoords * vec2(textureSize(uTex0, 0)),
            DITHER_STEPS_8_BIT);
    } else if (uDitherMode == DITHER_MODE_PS1) {
        outColor.rgb = ditherPS1(
            outColor.rgb, vertTexCoords * vec2(textureSize(uTex0, 0)));
    }
}

#endif
