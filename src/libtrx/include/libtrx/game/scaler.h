#pragma once

#include <stdint.h>

typedef enum {
    SCALER_TARGET_GENERIC,
    SCALER_TARGET_BAR,
    SCALER_TARGET_TEXT,
    SCALER_TARGET_ASSAULT_DIGITS,
} SCALER_TARGET;

double Scaler_GetScale(const SCALER_TARGET target);
float Scaler_Calc(float unit, SCALER_TARGET target);
float Scaler_CalcInverse(float unit, SCALER_TARGET target);
