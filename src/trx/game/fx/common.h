#pragma once

#include <trx/core/result.h>

typedef struct JSON_READ_IO JSON_READ_IO;
typedef struct JSON_WRITE_IO JSON_WRITE_IO;

// Defines the optional lifecycle hooks for an effect module. A persistent
// module sets save_key, save_func and load_func.
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

// Writes the state of each persistent module, or nothing while the effect
// saving setting is off.
void FX_Save(JSON_WRITE_IO *io);

// Resets each persistent module before applying what the save holds. While
// the effect saving setting is off, the reset stands and the save is ignored.
RESULT FX_Load(JSON_READ_IO *io);
