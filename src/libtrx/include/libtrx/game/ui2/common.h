#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
    UI2_KEY_UP,
    UI2_KEY_DOWN,
    UI2_KEY_LEFT,
    UI2_KEY_RIGHT,
    UI2_KEY_HOME,
    UI2_KEY_END,
    UI2_KEY_BACK,
    UI2_KEY_RETURN,
    UI2_KEY_ESCAPE,
} UI2_INPUT;

// Forward declaration of the node and its vtable.
struct UI2_NODE;
typedef struct {
    void (*measure)(struct UI2_NODE *node);
    void (*layout)(struct UI2_NODE *node, float x, float y, float w, float h);
    void (*draw)(const struct UI2_NODE *node);
} UI2_WIDGET_OPS;

// Node structure that forms the UI tree
typedef struct UI2_NODE {
    // Common operations on a widget
    const UI2_WIDGET_OPS *ops;

    // Final layout rectangle
    float x;
    float y;
    float w;
    float h;

    // Needed size from measure pass
    float measure_w;
    float measure_h;

    // Link to parent and siblings to form a tree
    struct UI2_NODE *parent;
    struct UI2_NODE *first_child;
    struct UI2_NODE *last_child;
    struct UI2_NODE *next_sibling;

    // Widget-specific data
    void *data;
} UI2_NODE;

// Dimensions in virtual pixels of the screen area
// (640x480 for any 4:3 resolution on 1.00 text scaling)
extern int32_t UI2_GetCanvasWidth(void);
extern int32_t UI2_GetCanvasHeight(void);
extern float UI2_ScaleX(float x);
extern float UI2_ScaleY(float y);

// Public API for scene management
void UI2_BeginScene(void);
void UI2_EndScene(void);

// Helpers to add children, etc.
UI2_NODE *UI2_AllocNode(const UI2_WIDGET_OPS *ops, size_t additional_size);
void UI2_AddChild(UI2_NODE *child);
void UI2_PushCurrent(UI2_NODE *child);
void UI2_PopCurrent(void);

void UI2_Init(void);
void UI2_Shutdown(void);
void UI2_ToggleState(bool *config_setting);

void UI2_HandleKeyDown(uint32_t key);
void UI2_HandleKeyUp(uint32_t key);
void UI2_HandleTextEdit(const char *text);
