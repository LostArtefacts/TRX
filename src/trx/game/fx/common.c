#include <trx/game/fx/common.h>

#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
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

void FX_Save(JSON_WRITE_IO *const io)
{
    if (!g_Config.gameplay.enable_enhanced_saves) {
        return;
    }

    for (int32_t i = 0; i < m_ModuleCount; i++) {
        const FX_MODULE *const module = m_Modules[i];
        if (module->save_func == nullptr) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        module->save_func(io);
        JSONW_POP_AND_SET_NZ(io, module->save_key);
    }
}

RESULT FX_Load(JSON_READ_IO *const io)
{
    const bool enabled = g_Config.gameplay.enable_enhanced_saves;
    for (int32_t i = 0; i < m_ModuleCount; i++) {
        const FX_MODULE *const module = m_Modules[i];
        if (module->load_func == nullptr) {
            continue;
        }
        if (module->reset_func != nullptr) {
            module->reset_func();
        }
        if (!enabled || !JSON_ReadIO_HasKey(io, module->save_key)) {
            continue;
        }
        MUST(JSON_PUSH(io, module->save_key));
        MUST(module->load_func(io));
        MUST(JSON_POP(io));
    }
    return OK;
}
