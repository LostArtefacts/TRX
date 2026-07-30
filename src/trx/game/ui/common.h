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

// Dimensions in virtual pixels of the screen area
// (640x480 for any 4:3 resolution on 1.00 text scaling)
int32_t UI_GetCanvasWidth(void);
int32_t UI_GetCanvasHeight(void);
float UI_ScaleX(float x);
float UI_ScaleY(float y);

// Public API for scene management
void UI_BeginScene(void);
void UI_EndScene(void);

// Helpers to add children, etc.
UI_NODE *UI_AllocNode(const UI_WIDGET_OPS *ops, size_t additional_size);
void UI_AddChild(UI_NODE *child);
void UI_PushCurrent(UI_NODE *child);
void UI_PopCurrent(void);
const UI_NODE *UI_GetCurrent(void);

// The tree the last UI_EndScene measured and laid out. Its nodes stay valid
// until the next UI_BeginScene resets the arena they live in.
const UI_NODE *UI_GetSceneRoot(void);

void UI_Init(void);
void UI_Shutdown(void);
void UI_ToggleState(bool *config_setting);

void UI_HandleKeyDown(uint32_t key);
void UI_HandleKeyUp(uint32_t key);
void UI_HandleTextEdit(const char *text);

// Inserts the current clipboard contents (if any) into the currently
// focused text field, as if it had been typed.
void UI_HandlePaste(void);
