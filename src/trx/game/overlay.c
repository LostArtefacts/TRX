#include <trx/game/overlay.h>

#include <trx/core/strings.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/objects.h>
#include <trx/game/objects/names.h>

// Last requested overlay state for the current frame.
typedef struct {
    OVERLAY_TEXT top_text;
    bool arrows[OVERLAY_ARROW_NUMBER_OF];
    bool show_version;
    bool force_health_bar;
} M_STATE;

static M_STATE m_State;

static const char *M_ResolveTextRaw(const OVERLAY_TEXT *const text)
{
    switch (text->kind) {
    case OVERLAY_TEXT_NONE:
        return nullptr;
    case OVERLAY_TEXT_LITERAL:
        return text->literal;
    case OVERLAY_TEXT_GS_KEY:
        return GameString_Get(text->gs_key);
    case OVERLAY_TEXT_OBJECT_NAME:
        return Object_GetName(text->object_id);
    }
    return nullptr;
}

// Resolves deferred overlay text in the current language.
static const char *M_ResolveText(const OVERLAY_TEXT *const text)
{
    const char *const raw = M_ResolveTextRaw(text);
    if (raw == nullptr || text->fmt_gs_key == nullptr) {
        return raw;
    }
    return String_FormatStatic(GameString_Get(text->fmt_gs_key), raw);
}

void Overlay_Reset(void)
{
    m_State = (M_STATE) {};
}

void Overlay_Control(void)
{
    Overlay_ForceHealthBar(false);
}

const char *Overlay_GetTopText(void)
{
    return M_ResolveText(&m_State.top_text);
}

bool Overlay_IsArrowShown(const OVERLAY_ARROW arrow)
{
    return m_State.arrows[arrow];
}

bool Overlay_IsVersionShown(void)
{
    return m_State.show_version;
}

void Overlay_ForceHealthBar(const bool show)
{
    m_State.force_health_bar = show;
}

bool Overlay_IsHealthBarForced(void)
{
    return m_State.force_health_bar;
}

void Overlay_ShowArrow(const OVERLAY_ARROW arrow, const bool show)
{
    m_State.arrows[arrow] = show;
}

void Overlay_ShowVersion(const bool show)
{
    m_State.show_version = show;
}

void Overlay_SetTopText(const OVERLAY_TEXT text)
{
    m_State.top_text = text;
}
