#include <trx/game/ui/common.h>

#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/console/common.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/elements/anchor.h>
#include <trx/game/ui/events.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/ui/text.h>
#include <trx/game/viewport.h>

#include <string.h>

static struct {
    MEMORY_ARENA_ALLOCATOR alloc;
    UI_NODE *root; // The top-level container
    UI_NODE *current; // The current container into which we attach nodes
    UI_NODE *scene_root; // The tree the last UI_EndScene laid out
    uint32_t generation; // scene counter
} m_Priv = {
    .alloc = {
        .default_chunk_size = 1024 * 4,
    },
};

static void (*m_PaintHook)(void);
static float m_SmallestFitScale = 1.0f;

#ifdef TESTING
static UI_MEASURE_NOTE m_Widest = {};
static char m_WidestText[256] = "";
#endif

extern void UI_ClearDraw(void);

static struct {
    UI_NODE *saved_root;
    UI_NODE *saved_current;
    bool active;
} m_Measure = {};

// Depth-first measure pass
static void M_MeasureNode(UI_NODE *const node)
{
    if (node == nullptr || node->ops.measure == nullptr) {
        return;
    }

    // Recurse to children
    UI_NODE *child = node->first_child;
    while (child != nullptr) {
        M_MeasureNode(child);
        child = child->next_sibling;
    }

    node->ops.measure(node);
}

// Depth-first layout pass
static void M_LayoutNode(
    UI_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    if (node == nullptr || node->ops.layout == nullptr) {
        return;
    }

    node->ops.layout(node, x, y, w, h);
    // Recursing to children is a responsibility of the layout function.
}

// Depth-first draw pass
static void M_DrawNode(const UI_NODE *const node)
{
    if (node == nullptr || node->ops.draw == nullptr) {
        return;
    }

    node->ops.draw(node);
    // Recursing to children is a responsibility of the draw function.
}

static float M_GetAxisFitScale(const float natural, const float available)
{
    if (natural <= 0.0f || natural <= available) {
        return 1.0f;
    }
    return available / natural;
}

static void M_Init(void)
{
    UI_InitEvents();
    UI_InitText();
    UI_InitDraw();
}

static void M_Shutdown(void)
{
    UI_ShutdownDraw();
    UI_ShutdownText();
    Memory_ArenaFree(&m_Priv.alloc);
    UI_ShutdownEvents();
}

// Allocate a new node
UI_NODE *UI_AllocNode(
    const UI_WIDGET_OPS *const ops, const size_t additional_size)
{
    const size_t size =
        Memory_Align(sizeof(UI_NODE)) + Memory_Align(additional_size);
    UI_NODE *const node = Memory_ArenaAlloc(&m_Priv.alloc, size);
    memset(node, 0, size);
    node->ops = *ops;
    node->data = (char *)node + Memory_Align(sizeof(UI_NODE));
    return node;
}

// Attach child to parent's child list
void UI_AddChild(UI_NODE *const child)
{
    // Special case - the root widget
    if (m_Priv.root == nullptr) {
        m_Priv.root = child;
        return;
    }

    UI_NODE *const parent = m_Priv.current;
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

void UI_PushCurrent(UI_NODE *const child)
{
    m_Priv.current = child;
}

void UI_SetNodeName(const char *const name)
{
    if (m_Priv.current != nullptr) {
        m_Priv.current->name = name;
    }
}

void UI_PopCurrent(void)
{
    ASSERT(m_Priv.current != nullptr);
    m_Priv.current = m_Priv.current->parent;
    if (m_Priv.current == nullptr) {
        m_Priv.root = nullptr;
    }
}

const UI_NODE *UI_GetCurrent(void)
{
    return m_Priv.current;
}

uint32_t UI_GetSceneGeneration(void)
{
    return m_Priv.generation;
}

UI_NODE *UI_GetBuildRoot(void)
{
    return m_Priv.root;
}

void UI_SetCurrent(UI_NODE *const node)
{
    m_Priv.current = node;
}

const UI_NODE *UI_GetSceneRoot(void)
{
    return m_Priv.scene_root;
}

void UI_BeginMeasure(void)
{
    ASSERT(!m_Measure.active);
    m_Measure.active = true;
    m_Measure.saved_root = m_Priv.root;
    m_Measure.saved_current = m_Priv.current;
    m_Priv.root = nullptr;
    m_Priv.current = nullptr;
    UI_BeginAnchor(0.5f, 0.5f);
}

void UI_EndMeasure(float *const out_w, float *const out_h)
{
    ASSERT(m_Measure.active);
    UI_NODE *const root = m_Priv.root;
    UI_EndAnchor();
    ASSERT(m_Priv.root == nullptr);
    M_MeasureNode(root);
    if (out_w != nullptr) {
        *out_w = root->measure_w;
    }
    if (out_h != nullptr) {
        *out_h = root->measure_h;
    }
    m_Priv.root = m_Measure.saved_root;
    m_Priv.current = m_Measure.saved_current;
    m_Measure.active = false;
}

// Scene management

void UI_BeginScene(void)
{
    m_Priv.generation++;
    UI_ClearDraw();
    Memory_ArenaReset(&m_Priv.alloc);
    UI_BeginAnchor(0.5f, 0.5f); // Make a root node.
}

void UI_SetPaintHook(void (*const hook)(void))
{
    m_PaintHook = hook;
}

void UI_EndScene(void)
{
    m_Priv.scene_root = m_Priv.root;
    M_MeasureNode(m_Priv.root);
    UI_Region_Layout();
    M_LayoutNode(m_Priv.root, 0, 0, UI_GetCanvasWidth(), UI_GetCanvasHeight());
    if (m_PaintHook != nullptr) {
        m_PaintHook();
    }
    M_DrawNode(m_Priv.root);
    UI_EndAnchor();
    ASSERT(m_Priv.root == nullptr);
}

void UI_ToggleState(const bool *const config_setting)
{
    CONFIG_OPTION *const option = Config_FindOptionByMirror(config_setting);
    const TRX_VALUE value = {
        .type = TVT_BOOL,
        .as_bool = !*config_setting,
    };
    Config_Option_Write(option, &value);
    Config_Update();
    Console_Info(
        *config_setting ? GS("general/osd/ui_on") : GS("general/osd/ui_off"));
}

int32_t UI_GetCanvasWidth(void)
{
    return UI_Scaler_CalcInverse(
        Viewport_GetWidth(VIEWPORT_UI), UI_SCALER_TARGET_GENERIC);
}

int32_t UI_GetCanvasHeight(void)
{
    return UI_Scaler_CalcInverse(
        Viewport_GetHeight(VIEWPORT_UI), UI_SCALER_TARGET_GENERIC);
}

float UI_GetSafeCanvasWidth(void)
{
    return MAX(0.0f, UI_GetCanvasWidth() - 2.0f * UI_SCREEN_MARGIN);
}

float UI_GetSafeCanvasTop(void)
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    UI_Region_GetCenterBox(&x, &y, &w, &h);
    return MAX(UI_SCREEN_MARGIN, y);
}

float UI_GetSafeCanvasBottom(void)
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    UI_Region_GetCenterBox(&x, &y, &w, &h);
    return MIN(UI_GetCanvasHeight() - UI_SCREEN_MARGIN, y + h);
}

float UI_GetSafeCanvasHeight(void)
{
    return MAX(0.0f, UI_GetSafeCanvasBottom() - UI_GetSafeCanvasTop());
}

void UI_ForgetSmallestFitScale(void)
{
    m_SmallestFitScale = 1.0f;
}

float UI_GetSmallestFitScale(void)
{
    return m_SmallestFitScale;
}

float UI_GetFitScale(const float content_width, const float content_height)
{
    const float text_scale = UI_Scaler_GetTextScale();
    if (text_scale <= 0.0f) {
        return 1.0f;
    }
    const float result = MIN(
        M_GetAxisFitScale(content_width, UI_GetSafeCanvasWidth() / text_scale),
        M_GetAxisFitScale(
            content_height, UI_GetSafeCanvasHeight() / text_scale));
    m_SmallestFitScale = MIN(m_SmallestFitScale, result);
    return result;
}

#ifdef TESTING
void UI_Measure_Note(
    const char *const part, const char *const text, const float width)
{
    if (width <= m_Widest.width) {
        return;
    }
    const char *const source = text != nullptr ? text : "?";
    strncpy(m_WidestText, source, sizeof(m_WidestText) - 1);
    m_WidestText[sizeof(m_WidestText) - 1] = '\0';
    m_Widest = (UI_MEASURE_NOTE) {
        .part = part,
        .text = m_WidestText,
        .width = width,
    };
}

void UI_Measure_Forget(void)
{
    m_Widest = (UI_MEASURE_NOTE) {};
    m_WidestText[0] = '\0';
}

UI_MEASURE_NOTE UI_Measure_GetWidest(void)
{
    return m_Widest;
}
#endif

float UI_ScaleX(const float x)
{
    return UI_Scaler_Calc(x * 0x10000, UI_SCALER_TARGET_GENERIC) / 0x10000.p0;
}

float UI_ScaleY(const float y)
{
    return UI_Scaler_Calc(y * 0x10000, UI_SCALER_TARGET_GENERIC) / 0x10000.p0;
}

REGISTER_SUBSYSTEM(.init = M_Init, .shutdown = M_Shutdown)
