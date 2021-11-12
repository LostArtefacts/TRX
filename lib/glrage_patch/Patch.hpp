#pragma once

#include "Chunk.hpp"

#include <glrage/GameID.hpp>
#include <glrage_util/Config.hpp>

#include <Windows.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace glrage {

struct ModuleContext
{
    std::wstring path;
    std::string fileName;
    std::wstring fileNameW;
    VS_FIXEDFILEINFO fileInfo;
};

class Patch
{
public:
    virtual void apply() = 0;
    virtual GameID gameID();
    void setContext(ModuleContext& ctx);

protected:
    bool patch(uint32_t addr, const Chunk& expected,
        const Chunk& replacement);

    bool patch(uint32_t addr, const Chunk& replacement);

    void patchAddr(int32_t addr, const Chunk& expected, void* func, uint8_t op);

    bool patchNop(uint32_t addr, const Chunk& expected);

    ModuleContext m_ctx;
    Config& m_config{Config::instance()};
};

} // namespace glrage