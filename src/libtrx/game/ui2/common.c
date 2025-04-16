#include "game/ui2/common.h"

#include "config.h"
#include "debug.h"
#include "game/console/common.h"
#include "game/game_string.h"
#include "game/ui2/elements/anchor.h"
#include "game/ui2/events.h"
#include "memory.h"

#include <SDL2/SDL.h>
#include <string.h>

static struct {
    MEMORY_ARENA_ALLOCATOR alloc;

    int32_t node_count;
    int32_t node_capacity;
    UI2_NODE *nodes;

    UI2_NODE *root; // The top-level container
    UI2_NODE *current; // The current container into which we attach nodes
} m_Priv = {
    .alloc = {
        .default_chunk_size = 1024 * 4,
    },
};

static UI2_INPUT M_TranslateInput(uint32_t system_keycode);
static void M_MeasureNode(UI2_NODE *node);
static void M_LayoutNode(UI2_NODE *node, float x, float y, float w, float h);
static void M_DrawNode(const UI2_NODE *node);

static UI2_INPUT M_TranslateInput(const uint32_t system_keycode)
{
    // clang-format off
    switch (system_keycode) {
    case SDLK_UP:        return UI2_KEY_UP;
    case SDLK_DOWN:      return UI2_KEY_DOWN;
    case SDLK_LEFT:      return UI2_KEY_LEFT;
    case SDLK_RIGHT:     return UI2_KEY_RIGHT;
    case SDLK_HOME:      return UI2_KEY_HOME;
    case SDLK_END:       return UI2_KEY_END;
    case SDLK_BACKSPACE: return UI2_KEY_BACK;
    case SDLK_RETURN:    return UI2_KEY_RETURN;
    case SDLK_ESCAPE:    return UI2_KEY_ESCAPE;
    }
    // clang-format on
    return -1;
}

// Depth-first measure pass
static void M_MeasureNode(UI2_NODE *const node)
{
    if (node == nullptr || node->ops == nullptr
        || node->ops->measure == nullptr) {
        return;
    }

    // Recurse to children
    UI2_NODE *child = node->first_child;
    while (child != nullptr) {
        M_MeasureNode(child);
        child = child->next_sibling;
    }

    node->ops->measure(node);
}

// Depth-first layout pass
static void M_LayoutNode(
    UI2_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    if (node == nullptr || node->ops == nullptr
        || node->ops->layout == nullptr) {
        return;
    }

    node->ops->layout(node, x, y, w, h);
    // Recursing to children is a responsibility of the layout function.
}

// Depth-first draw pass
static void M_DrawNode(const UI2_NODE *const node)
{
    if (node == nullptr || node->ops == nullptr || node->ops->draw == nullptr) {
        return;
    }

    node->ops->draw(node);
    // Recursing to children is a responsibility of the draw function.
}

// Allocate a new node
UI2_NODE *UI2_AllocNode(
    const UI2_WIDGET_OPS *const ops, const size_t additional_size)
{
    const size_t size = sizeof(UI2_NODE) + additional_size;
    UI2_NODE *const node = Memory_ArenaAlloc(&m_Priv.alloc, size);
    memset(node, 0, size);
    node->ops = ops;
    node->data = (char *)node + sizeof(UI2_NODE);
    m_Priv.node_count++;
    return node;
}

// Attach child to parent's child list
void UI2_AddChild(UI2_NODE *const child)
{
    // Special case - the root widget
    if (m_Priv.root == nullptr) {
        m_Priv.root = child;
        return;
    }

    UI2_NODE *const parent = m_Priv.current;
    if (parent == nullptr || child == nullptr) {
        return;
    }
    child->parent = parent;
    if (parent->first_child == nullptr) {
        parent->first_child = child;
    } else {
        parent->last_child->next_sibling = child;
    }
    parent->last_child = child;
}

void UI2_PushCurrent(UI2_NODE *const child)
{
    m_Priv.current = child;
}

void UI2_PopCurrent(void)
{
    ASSERT(m_Priv.current != nullptr);
    m_Priv.current = m_Priv.current->parent;
    if (m_Priv.current == nullptr) {
        m_Priv.root = nullptr;
    }
}

// Scene management
void UI2_BeginScene(void)
{
    Memory_ArenaReset(&m_Priv.alloc);
    m_Priv.node_count = 0;

    // Make a root node.
    UI2_BeginAnchor(0.5f, 0.5f);
}

void UI2_EndScene(void)
{
    M_MeasureNode(m_Priv.root);
    M_LayoutNode(
        m_Priv.root, 0, 0, UI2_GetCanvasWidth(), UI2_GetCanvasHeight());
    M_DrawNode(m_Priv.root);
    UI2_EndAnchor();
    ASSERT(m_Priv.root == nullptr);
}

void UI2_Init(void)
{
    UI2_InitEvents();
}

void UI2_Shutdown(void)
{
    Memory_ArenaFree(&m_Priv.alloc);
    UI2_ShutdownEvents();
}

void UI2_ToggleState(bool *const config_setting)
{
    *config_setting ^= true;
    Config_Write();
    Console_Log(*config_setting ? GS(OSD_UI_ON) : GS(OSD_UI_OFF));
}

void UI2_HandleKeyDown(const uint32_t key)
{
    UI2_FireEvent((EVENT) {
        .name = "key_down",
        .sender = nullptr,
        .data = (void *)M_TranslateInput(key),
    });
}

void UI2_HandleKeyUp(const uint32_t key)
{
    UI2_FireEvent((EVENT) {
        .name = "key_up",
        .sender = nullptr,
        .data = (void *)M_TranslateInput(key),
    });
}

void UI2_HandleTextEdit(const char *const text)
{
    UI2_FireEvent((EVENT) {
        .name = "text_edit", .sender = nullptr, .data = (void *)text });
}
