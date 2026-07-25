#include <trx/game/fx/common.h>

#include <trx/debug.h>

#include <stdint.h>

#define M_MAX_MODULES 16

static const FX_MODULE *m_Modules[M_MAX_MODULES];
static int32_t m_ModuleCount = 0;

void FX_RegisterModule(const FX_MODULE *const module)
{
    ASSERT(m_ModuleCount < M_MAX_MODULES);
    m_Modules[m_ModuleCount++] = module;
}

void FX_NewFrame(void)
{
    for (int32_t i = 0; i < m_ModuleCount; i++) {
        if (m_Modules[i]->new_frame_func != nullptr) {
            m_Modules[i]->new_frame_func();
        }
    }
}

void FX_Control(void)
{
    for (int32_t i = 0; i < m_ModuleCount; i++) {
        if (m_Modules[i]->control_func != nullptr) {
            m_Modules[i]->control_func();
        }
    }
}

void FX_Draw(void)
{
    for (int32_t i = 0; i < m_ModuleCount; i++) {
        if (m_Modules[i]->draw_func != nullptr) {
            m_Modules[i]->draw_func();
        }
    }
}

void FX_Reset(void)
{
    for (int32_t i = 0; i < m_ModuleCount; i++) {
        if (m_Modules[i]->reset_func != nullptr) {
            m_Modules[i]->reset_func();
        }
    }
}
