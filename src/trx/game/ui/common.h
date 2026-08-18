#pragma once

#include <stddef.h>
#include <stdint.h>

// Forward declaration of the node and its vtable.
struct UI_NODE;
typedef struct {
    void (*measure)(struct UI_NODE *node);
    void (*layout)(struct UI_NODE *node, float x, float y, float w, float h);
    void (*draw)(const struct UI_NODE *node);
} UI_WIDGET_OPS;

// Node structure that forms the UI tree
typedef struct UI_NODE {
    // Common operations on a widget
    UI_WIDGET_OPS ops;

    // Final layout rectangle
    float x;
    float y;
    float w;
    float h;

    // Needed size from measure pass
    float measure_w;
    float measure_h;

    // Link to parent and siblings to form a tree
    struct UI_NODE *parent;
    struct UI_NODE *first_child;
    struct UI_NODE *last_child;
    struct UI_NODE *next_sibling;

    // Widget-specific data
    void *data;
} UI_NODE;

// How far a dialog stays clear of the screen edges, in canvas units.
#define UI_SCREEN_MARGIN 5.0f

// Dimensions in virtual pixels of the screen area
// (640x480 for any 4:3 resolution on 1.00 text scaling)
int32_t UI_GetCanvasWidth(void);
int32_t UI_GetCanvasHeight(void);

// The width a dialog may occupy: the canvas less the screen margin at either
// edge. What sizes itself to fit the screen fits to this.
float UI_GetSafeCanvasWidth(void);

// What is drawn over the edges of the screen, and so says how far into it a
// dialog must not reach.
typedef enum {
    UI_SCREEN_INSET_OVERLAY,
    UI_SCREEN_INSET_INVENTORY_RING,
    UI_SCREEN_INSET_SOURCE_COUNT,
} UI_SCREEN_INSET_SOURCE;

// States what a source keeps clear at the top and bottom of the screen, in
// text units, for the scene being built. A source states this as it draws, so
// one that draws nothing gives its room back and the dialogs grow into it.
void UI_SetScreenInset(UI_SCREEN_INSET_SOURCE source, float top, float bottom);

// The deepest any source reaches into the screen from the top or the bottom,
// in text units, as of the scene last drawn.
float UI_GetScreenInsetTop(void);
float UI_GetScreenInsetBottom(void);

// Where the area a dialog may occupy begins and ends down the screen, in
// canvas units: the screen margin, or the inset a source keeps where that
// reaches further in.
float UI_GetSafeCanvasTop(void);
float UI_GetSafeCanvasBottom(void);

// The height a dialog may occupy: what lies between the two.
float UI_GetSafeCanvasHeight(void);

// The factor a dialog is drawn at so that it fits the safe canvas, to pass
// to UI_Scaler_PushTextScale. Both sizes are the whole dialog, frame and
// padding included, in text units before the player's text scale; -1 leaves
// an axis unconstrained. The factor never goes below two thirds: wording that
// does not fit there is a string to shorten. Every answer also feeds the
// smallest-fit record below.
float UI_GetFitScale(float content_width, float content_height);

// The smallest factor UI_GetFitScale has answered with since it was last
// forgotten, and whether that answer was the floor: the screen then has no
// room for the dialog at any wording.
void UI_ForgetSmallestFitScale(void);
float UI_GetSmallestFitScale(void);
bool UI_HasFitScaleFloored(void);
float UI_ScaleX(float x);
float UI_ScaleY(float y);

// Public API for scene management
void UI_BeginScene(void);
void UI_EndScene(void);

// Measures widgets built between these calls without drawing them.
// Measured nodes stay valid until UI_BeginScene resets the arena.
void UI_BeginMeasure(void);
void UI_EndMeasure(float *out_w, float *out_h);

// Helpers to add children, etc.
UI_NODE *UI_AllocNode(const UI_WIDGET_OPS *ops, size_t additional_size);
void UI_AddChild(UI_NODE *child);
void UI_PushCurrent(UI_NODE *child);

void UI_PopCurrent(void);
const UI_NODE *UI_GetCurrent(void);

// The tree the last UI_EndScene measured and laid out. Its nodes stay valid
// until the next UI_BeginScene resets the arena they live in.
const UI_NODE *UI_GetSceneRoot(void);

void UI_ToggleState(const bool *config_setting);

void UI_HandleKeyDown(uint32_t key);
void UI_HandleKeyUp(uint32_t key);
void UI_HandleTextEdit(const char *text);

// Inserts the current clipboard contents (if any) into the currently
// focused text field, as if it had been typed.
void UI_HandlePaste(void);
