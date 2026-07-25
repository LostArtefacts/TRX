#pragma once

// Lifecycle hooks an effect module takes part in. Every hook is optional.
typedef struct {
    void (*new_frame_func)(void);
    void (*control_func)(void);
    void (*draw_func)(void);
    void (*reset_func)(void);
} FX_MODULE;

void FX_RegisterModule(const FX_MODULE *module);

// One module per translation unit.
#define REGISTER_FX(module_)                                                   \
    __attribute__((constructor)) static void M_RegisterFX(void)                \
    {                                                                          \
        FX_RegisterModule(&(module_));                                         \
    }

void FX_Reset(void);
void FX_NewFrame(void);
void FX_Control(void);
void FX_Draw(void);
