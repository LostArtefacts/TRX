#ifndef T1M_3DSYSTEM_PHD_MATH_H
#define T1M_3DSYSTEM_PHD_MATH_H

#include <stdint.h>

// clang-format off
#define phd_sqrt                ((int32_t      (*)(int32_t angle))0x0042A900)
// clang-format on

int32_t phd_cos(int32_t angle);
int32_t phd_sin(int32_t angle);
int32_t phd_atan(int32_t x, int32_t y);

void T1MInject3DSystemPHDMath();

#endif
