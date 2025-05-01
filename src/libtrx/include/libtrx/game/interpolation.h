#pragma once

bool Interpolation_IsEnabled(void);
void Interpolation_Disable(void);
void Interpolation_Enable(void);

double Interpolation_GetWorldRate(void);
double Interpolation_GetCameraRate(void);
void Interpolation_SetRate(double rate);

void Interpolation_Interpolate(void);
void Interpolation_Remember(void);
