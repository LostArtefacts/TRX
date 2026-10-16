#pragma once

#include <trx/core/result.h>

#include <stdint.h>

// How far a dialog stays clear of the screen edges, in canvas units.
#define UI_SCREEN_MARGIN 5.0f

// Dimensions in virtual pixels of the screen area
// (640x480 for any 4:3 resolution on 1.00 text scaling)
int32_t UI_GetCanvasWidth(void);
int32_t UI_GetCanvasHeight(void);

// The canvas between the cinematic bars. It follows the bars as they move.
float UI_GetWorkingCanvasTop(void);
float UI_GetWorkingCanvasBottom(void);

// The width a dialog may occupy: the canvas less the screen margin at either
// edge. What sizes itself to fit the screen fits to this.
float UI_GetSafeCanvasWidth(void);

// Returns the vertical dialog bounds from the middle region box.
float UI_GetSafeCanvasTop(void);
float UI_GetSafeCanvasBottom(void);

// Returns the height available to dialogs.
float UI_GetSafeCanvasHeight(void);

// The same bounds for a dialog of the given width, centered: a region at the
// top or the bottom of the screen narrows them only if the dialog would reach
// it. A negative width is the widest a dialog can be.
float UI_GetSafeCanvasTopFor(float width);
float UI_GetSafeCanvasBottomFor(float width);
float UI_GetSafeCanvasHeightFor(float width);

float UI_ScaleX(float x);
float UI_ScaleY(float y);

// Begins a scene, and ends it by laying out the regions and drawing the
// layers that scripts paint.
void UI_BeginScene(void);
void UI_EndScene(void);

// The layers of the interface, bottom to top. A draw belongs to the layer
// whose scope it is scheduled in. UI_Draw draws the layers in this order, so
// the order in which draws were scheduled decides only their order within a
// layer.
typedef enum {
    // Pictures, snapshots and blacks that the screen stands on.
    UI_LAYER_BACKDROP,
    // Script layers and widgets under the touch controls.
    UI_LAYER_UNDER,
    // The touch controls.
    UI_LAYER_INTERFACE,
    // Script widgets over the touch controls.
    UI_LAYER_OVER,
    // Fades over the whole interface.
    UI_LAYER_FADE,
    // The console, over every other layer but the game-exit fade.
    UI_LAYER_CONSOLE,
    // The fade to black as the game exits, over everything.
    UI_LAYER_EXIT,
    UI_LAYER_NUMBER_OF,
} UI_LAYER;

// Opens and closes the scope of a layer. Scopes do not nest, and a scope
// closes the layer it opened.
void UI_BeginLayer(UI_LAYER layer);
void UI_EndLayer(UI_LAYER layer);

// Returns the layer whose scope is open. Scheduling a draw outside any scope
// is fatal.
UI_LAYER UI_GetLayer(void);

// Runs once for each layer that scripts paint (under, over and console),
// while reservation boxes are known.
void UI_SetPaintHook(void (*hook)(UI_LAYER layer));

void UI_ToggleState(const bool *config_setting);

void UI_HandleKeyDown(uint32_t key);
void UI_HandleKeyUp(uint32_t key);
void UI_HandleTextEdit(const char *text);

// Inserts the current clipboard contents (if any) into the currently
// focused text field, as if it had been typed.
void UI_HandlePaste(void);

// Puts text in the system clipboard and reports failure if the platform refuses
// it.
RESULT UI_SetClipboardText(const char *text);
