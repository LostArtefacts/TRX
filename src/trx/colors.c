#include <trx/colors.h>

#include <trx/utils.h>

#include <math.h>

RGBA_8888 Color_RGB888ToRGBA8888_Impl(const RGB_888 color)
{
    return Color_RGB888ToRGBA8888Ex_Impl(color, 255);
}

RGBA_8888 Color_RGB888ToRGBA8888Ex_Impl(
    const RGB_888 color, const uint8_t alpha)
{
    return (RGBA_8888) { color.r, color.g, color.b, alpha };
}

RGBA_F Color_RGBFToRGBAF_Impl(const RGB_F color)
{
    return Color_RGBFToRGBAFEx_Impl(color, 1.0f);
}

RGBA_F Color_RGBFToRGBAFEx_Impl(const RGB_F color, const float alpha)
{
    return (RGBA_F) { color.r, color.g, color.b, alpha };
}

RGB_888 Color_HSLToRGB(const float h, const float s, const float l)
{
    float hue = h < 0.0f ? 0.0f : fmodf(h, 360.0f);
    float sat = s;
    float light = l;
    CLAMP(hue, 0.0f, 360.0f);
    CLAMP(sat, 0.0f, 1.0f);
    CLAMP(light, 0.0f, 1.0f);

    const float c = (1.0f - fabsf(2.0f * light - 1.0f)) * sat;
    const float x = c * (1.0f - fabsf(fmodf(hue / 60.0f, 2.0f) - 1.0f));
    const float m = light - c / 2.0f;

    float rp = 0.0f;
    float gp = 0.0f;
    float bp = 0.0f;

    if (hue < 60.0f) {
        rp = c;
        gp = x;
    } else if (hue < 120.0f) {
        rp = x;
        gp = c;
    } else if (hue < 180.0f) {
        gp = c;
        bp = x;
    } else if (hue < 240.0f) {
        gp = x;
        bp = c;
    } else if (hue < 300.0f) {
        rp = x;
        bp = c;
    } else {
        rp = c;
        bp = x;
    }

    return (RGB_888) {
        .r = (uint8_t)roundf((rp + m) * 255.0f),
        .g = (uint8_t)roundf((gp + m) * 255.0f),
        .b = (uint8_t)roundf((bp + m) * 255.0f),
    };
}

void Color_RGBToHSL(
    const RGB_888 color, float *const out_h, float *const out_s,
    float *const out_l)
{
    const float r = color.r / 255.0f;
    const float g = color.g / 255.0f;
    const float b = color.b / 255.0f;
    const float max_c = MAX(r, MAX(g, b));
    const float min_c = MIN(r, MIN(g, b));
    const float delta = max_c - min_c;
    float light = (max_c + min_c) / 2.0f;

    float hue = 0.0f;
    float sat = 0.0f;
    if (delta > 0.0f) {
        if (max_c == r) {
            hue = 60.0f * fmodf((g - b) / delta, 6.0f);
        } else if (max_c == g) {
            hue = 60.0f * (((b - r) / delta) + 2.0f);
        } else {
            hue = 60.0f * (((r - g) / delta) + 4.0f);
        }
        if (hue < 0.0f) {
            hue += 360.0f;
        }
        sat = delta / (1.0f - fabsf(2.0f * light - 1.0f));
    }

    CLAMP(hue, 0.0f, 360.0f);
    CLAMP(sat, 0.0f, 1.0f);
    CLAMP(light, 0.0f, 1.0f);
    *out_h = hue;
    *out_s = sat;
    *out_l = light;
}
