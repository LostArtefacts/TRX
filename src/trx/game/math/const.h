#pragma once

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// clang-format off
#define DEG_360 0x10000
#define DEG_180 ((DEG_360) / 2)
#define DEG_90  ((DEG_360) / 4)
#define DEG_45  ((DEG_360) / 8)
#define DEG_1   ((DEG_360) / 360) // = 182
#define DEG_135 ((DEG_45)  * 3)
// clang-format on
