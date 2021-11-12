#pragma once

#include "Patch.hpp"
#include "RuntimePatcher.hpp"

#include <string>
#include <array>

namespace glrage {

class RuntimePatcher
{
public:
    static RuntimePatcher& instance();

    void patch();

private:
    RuntimePatcher(){};
    RuntimePatcher(RuntimePatcher const&) = delete;
    void operator=(RuntimePatcher const&) = delete;
    
    void getModulePath();
    void getModuleVersion();

    ModuleContext m_ctx;
};

} // namespace glrage
