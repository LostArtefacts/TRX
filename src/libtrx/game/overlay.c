#include "game/overlay.h"

#include "game/ui.h"

static UI_OVERLAY_STATE *m_UI = nullptr;

void Overlay_Init(void)
{
    if (m_UI == nullptr) {
        m_UI = UI_Overlay_Init();
    }
}

void Overlay_Shutdown(void)
{
    if (m_UI != nullptr) {
        UI_Overlay_Free(m_UI);
        m_UI = nullptr;
    }
}

void Overlay_Control(void)
{
    if (m_UI != nullptr) {
        UI_Overlay_Control(m_UI);
    }
}

void Overlay_Draw(void)
{
    if (m_UI != nullptr) {
        UI_Overlay(m_UI);
    }
}

void Overlay_ForceHealthBar(const bool show)
{
    UI_Overlay_ForceHealthBar(m_UI, show);
}

void Overlay_SetHealthBarTimer(const int16_t timer)
{
    UI_LaraHealthBar_SetTimer(timer);
}

void Overlay_ShowArrows(const UI_OVERLAY_ARROW arrow, const bool show)
{
    if (m_UI != nullptr) {
        UI_Overlay_ShowArrows(m_UI, arrow, show);
    }
}

void Overlay_ShowVersion(const bool show)
{
    if (m_UI != nullptr) {
        UI_Overlay_ShowVersion(m_UI, show);
    }
}

void Overlay_SetTopText(const char *const text, const bool flash)
{
    if (m_UI != nullptr) {
        UI_Overlay_SetTopText(m_UI, text, flash);
    }
}

void Overlay_SetBottomText(const char *const text, const bool flash)
{
    if (m_UI != nullptr) {
        UI_Overlay_SetBottomText(m_UI, text, flash);
    }
}
