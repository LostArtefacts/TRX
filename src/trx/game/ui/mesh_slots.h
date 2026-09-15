#pragma once

// Models the interface keeps on screen across ticks.
//
// A script writes the pose a model should hold at the end of the tick it is
// in. The engine keeps the pose of the tick before it and blends the two on
// every drawn frame, so a model moves smoothly while the script that moves it
// runs only once a tick.

#include <trx/core/handle.h>
#include <trx/game/objects/ids.h>

#include <stdint.h>

#define UI_MESH_SLOT_MAX 64

// The box in canvas units, so that a window resized between two ticks does not
// blend the pixels of the old size with those of the new.
typedef struct {
    float x;
    float y;
    float w;
    float h;
    int32_t rot_y;
} UI_MESH_POSE;

typedef struct {
    bool in_use;
    bool visible;
    // False until a second pose arrives, while there is nothing to blend from.
    bool has_prev;
    OBJECT_ID object_id;
    UI_MESH_POSE cur;
    UI_MESH_POSE prev;
} UI_MESH_SLOT;

// Takes a free slot and returns a handle to it. Releasing a slot retires its
// handle, so the next taker of the same slot is reached only by the handle it
// was given. Returns a handle that resolves to nothing where every slot is
// taken.
TRX_HANDLE UI_MeshSlot_Acquire(void);

// Gives the slot back and retires the handle. Does nothing for a handle that
// is spent already.
void UI_MeshSlot_Release(TRX_HANDLE handle);

// Returns the slot, or nullptr where the handle is spent.
UI_MESH_SLOT *UI_MeshSlot_Resolve(TRX_HANDLE handle);

// Records the pose the model reaches at the end of this tick, keeping the one
// it starts from, and shows the model.
void UI_MeshSlot_Move(
    TRX_HANDLE handle, OBJECT_ID object_id, UI_MESH_POSE pose);
void UI_MeshSlot_Hide(TRX_HANDLE handle);

// One shown slot, blended between its two poses. The box stays in canvas
// units; whoever draws it scales it.
typedef struct {
    OBJECT_ID object_id;
    UI_MESH_POSE pose;
} UI_MESH_DRAW;

// Reports whether any slot is shown, so that a frame with no model in one
// opens no batch.
bool UI_MeshSlots_AnyShown(void);

// Fills out with each shown slot, blended between its two poses by how far the
// frame sits between the ticks, and returns how many were written.
int32_t UI_MeshSlots_Collect(UI_MESH_DRAW *out, int32_t max);

// Frees every slot, which is what a level change does.
void UI_MeshSlots_Reset(void);
