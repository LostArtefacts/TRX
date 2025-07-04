#pragma once

bool Interpolation_IsEnabled(void);
void Interpolation_Disable(void);
void Interpolation_Enable(void);

double Interpolation_GetWorldRate(void);
double Interpolation_GetCameraRate(void);
double Interpolation_GetRate(void);
void Interpolation_SetRate(double rate);

void Interpolation_Interpolate(void);
void Interpolation_Remember(void);

// Instantly discard interpolation data
void Interpolation_CommitLara(void);
void Interpolation_CommitBraid(void);
