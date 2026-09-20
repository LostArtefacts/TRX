#pragma once

#include <stdint.h>

#define TRX_GL_GPU_TIMER_SLOTS 12

void TRX_GL_GpuTimer_Init(bool is_enabled);
void TRX_GL_GpuTimer_Shutdown(void);

void TRX_GL_GpuTimer_Begin(int32_t slot);
void TRX_GL_GpuTimer_End(void);

void TRX_GL_GpuTimer_Collect(void);

double TRX_GL_GpuTimer_GetMs(int32_t slot);
