#ifdef VERTEX

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoords;

out vec2 vertTexCoords;
out vec2 vertCoords;

void main(void) {
    gl_Position = vec4(inPosition * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);
    vertCoords = inPosition;
    vertTexCoords = inTexCoords;
}

#elif defined(FRAGMENT)

#define EFFECT_NONE 0
#define EFFECT_VIGNETTE 1

uniform sampler2D texMain;
uniform sampler1D texPalette;
uniform sampler2D texAlpha;
uniform bool paletteEnabled;
uniform bool alphaEnabled;
uniform bool tintEnabled;
uniform vec3 tintColor;
uniform int effect;

in vec2 vertTexCoords;
in vec2 vertCoords;
out vec4 outColor;

void main(void) {
    vec2 uv = vertTexCoords;

    if (alphaEnabled) {
        float alpha = texture(texAlpha, uv).r;
        if (alpha < 0.5) {
            discard;
        }
    }

    if (paletteEnabled) {
        float paletteIndex = texture(texMain, uv).r;
        outColor = texture(texPalette, paletteIndex);
    } else {
        outColor = texture(texMain, uv);
    }

    if (tintEnabled) {
        outColor.rgb *= tintColor.rgb;
    }

    if (effect == EFFECT_VIGNETTE) {
        float x_dist = vertCoords.x - 0.5;
        float y_dist = vertCoords.y - 0.5;
        float light = 256 - sqrt(x_dist * x_dist + y_dist * y_dist ) * 300.0;
        light = clamp(light, 0, 255) / 255;
        outColor *= vec4(light, light, light, 1);
    }
}
#endif
