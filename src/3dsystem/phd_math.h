#ifndef T1M_3DSYSTEM_PHD_MATH_H
#define T1M_3DSYSTEM_PHD_MATH_H

#include <stdint.h>

int32_t phd_cos(int32_t angle);
int32_t phd_sin(int32_t angle);
int32_t phd_atan(int32_t x, int32_t y);
uint32_t phd_sqrt(uint32_t n);

double phd_cos_f(double angle);
double phd_sin_f(double angle);
double phd_sqrt_f(double n);
double phd_atan_f(double x, double y);

void T1MInject3DSystemPHDMath();

#endif
