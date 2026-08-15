#pragma once

#include <trx/core/result.h>

typedef struct JSON_READ_IO JSON_READ_IO;
typedef struct JSON_WRITE_IO JSON_WRITE_IO;

// Lifecycle hooks an effect module takes part in. Every hook is optional.
// A module that persists across saves fills in save_key together with save
// and load; its state is written as an object under that key.
typedef struct {
    void (*new_frame_func)(void);
    void (*control_func)(void);
    void (*draw_func)(void);
    void (*reset_func)(void);
    const char *save_key;
    void (*save_func)(JSON_WRITE_IO *io);
    RESULT (*load_func)(JSON_READ_IO *io);
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

void FX_Save(JSON_WRITE_IO *io);
// Resets every persisting module before applying what the save holds.
RESULT FX_Load(JSON_READ_IO *io);
