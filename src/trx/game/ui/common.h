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

    // Names a node for diagnostics.
    const char *name;
} UI_NODE;

// Shared names for dialog nodes.
#define UI_NODE_NAME_REGIONS "regions"
#define UI_NODE_NAME_DIALOG_HEADER "dialog header"
#define UI_NODE_NAME_DIALOG_FOOTER "dialog footer"
#define UI_NODE_NAME_DIALOG_HINT_ROW "dialog hint row"
#define UI_NODE_NAME_DIALOG_NAV_BAR "dialog nav bar"
#define UI_NODE_NAME_DIALOG_ROW "dialog row"
#define UI_NODE_NAME_ROW_TITLE "title"
#define UI_NODE_NAME_ROW_VALUE "value"

// How far a dialog stays clear of the screen edges, in canvas units.
#define UI_SCREEN_MARGIN 5.0f

// Dimensions in virtual pixels of the screen area
// (640x480 for any 4:3 resolution on 1.00 text scaling)
int32_t UI_GetCanvasWidth(void);
int32_t UI_GetCanvasHeight(void);

// The width a dialog may occupy: the canvas less the screen margin at either
// edge. What sizes itself to fit the screen fits to this.
float UI_GetSafeCanvasWidth(void);

// Returns the vertical dialog bounds from the middle region box.
float UI_GetSafeCanvasTop(void);
float UI_GetSafeCanvasBottom(void);

// Returns the height available to dialogs.
float UI_GetSafeCanvasHeight(void);

// Returns the text-scale factor needed to fit a dialog into the safe canvas.
// Pass -1 for an unconstrained axis. Both sizes include frame and padding, in
// text units before the player's text scale.
float UI_GetFitScale(float content_width, float content_height);

// Tracks the smallest fit factor returned since the last reset.
void UI_ForgetSmallestFitScale(void);
float UI_GetSmallestFitScale(void);
float UI_ScaleX(float x);
float UI_ScaleY(float y);

#ifdef TESTING
// Records the widest dialog text measured by a test scene.
typedef struct {
    // Identifies the dialog part that owns the text.
    const char *part;
    const char *text;
    float width;
} UI_MEASURE_NOTE;

// Reports measured dialog text and keeps the widest entry.
void UI_Measure_Note(const char *part, const char *text, float width);
void UI_Measure_Forget(void);
UI_MEASURE_NOTE UI_Measure_GetWidest(void);
#else
    #define UI_Measure_Note(part, text, width) ((void)0)
#endif

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

// Names the current node.
void UI_SetNodeName(const char *name);
void UI_PopCurrent(void);
const UI_NODE *UI_GetCurrent(void);

// Returns the scene counter.
uint32_t UI_GetSceneGeneration(void);

// Returns the scene root used while building the current scene.
UI_NODE *UI_GetBuildRoot(void);

// Sets the current node without changing the tree.
void UI_SetCurrent(UI_NODE *node);

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
