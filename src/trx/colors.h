#pragma once

#include <stdint.h>

typedef struct {
    float r, g, b;
} RGB_F;

typedef struct {
    float r, g, b, a;
} RGBA_F;

typedef struct {
    uint8_t r, g, b;
} RGB_888;

typedef struct {
    uint8_t r, g, b, a;
} RGBA_8888;

#define COLOR_RGBA_8888_BLACK ((RGBA_8888) { 0x00, 0x00, 0x00, 0xFF })
#define COLOR_RGBA_8888_WHITE ((RGBA_8888) { 0xFF, 0xFF, 0xFF, 0xFF })
#define COLOR_RGB_888_BLACK ((RGB_888) { 0x00, 0x00, 0x00 })
#define COLOR_RGB_888_WHITE ((RGB_888) { 0xFF, 0xFF, 0xFF })

RGBA_8888 Color_RGB888ToRGBA8888_Impl(RGB_888 color);
RGBA_8888 Color_RGB888ToRGBA8888Ex_Impl(RGB_888 color, uint8_t alpha);
RGBA_F Color_RGBFToRGBAF_Impl(RGB_F color);
RGBA_F Color_RGBFToRGBAFEx_Impl(RGB_F color, float alpha);

#define Color_RGBToRGBA(color)                                                 \
    _Generic(                                                                  \
        (color),                                                               \
        RGB_888: Color_RGB888ToRGBA8888_Impl,                                  \
        const RGB_888: Color_RGB888ToRGBA8888_Impl,                            \
        RGB_F: Color_RGBFToRGBAF_Impl,                                         \
        const RGB_F: Color_RGBFToRGBAF_Impl)(color)

#define Color_RGBToRGBAEx(color, alpha)                                        \
    _Generic(                                                                  \
        (color),                                                               \
        RGB_888: Color_RGB888ToRGBA8888Ex_Impl,                                \
        const RGB_888: Color_RGB888ToRGBA8888Ex_Impl,                          \
        RGB_F: Color_RGBFToRGBAFEx_Impl,                                       \
        const RGB_F: Color_RGBFToRGBAFEx_Impl)(color, alpha)

RGB_888 Color_HSLToRGB(float h, float s, float l);
void Color_RGBToHSL(RGB_888 color, float *out_h, float *out_s, float *out_l);
