#include "Patch.hpp"

#include <glrage_util/Logger.hpp>
#include <glrage_util/StringUtils.hpp>
#include <glrage_util/ErrorUtils.hpp>

namespace glrage {

GameID Patch::gameID()
{
    return GameID::Unknown;
}

void Patch::setContext(ModuleContext& ctx)
{
    m_ctx = ctx;
}

bool Patch::patch(
    uint32_t addr, const Chunk& expected, const Chunk& replacement)
{
    bool result = false;
    bool restoreProtect = false;

    const std::vector<uint8_t> expectedData = expected.data();
    const std::vector<uint8_t> replacementData = replacement.data();

    const size_t size = expectedData.size();
    std::vector<uint8_t> actualData(size);

    HANDLE proc = GetCurrentProcess();
    LPVOID lpaddr = NULL;
    DWORD numRead = 0;
    DWORD numWritten = 0;
    DWORD oldProtect = 0;

    // vectors must match in size
    if (size != replacementData.size()) {
        goto end;
    }

    // apply read/write flags to the memory page
    lpaddr = reinterpret_cast<LPVOID>(addr);
    if (!VirtualProtect(lpaddr, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        goto end;
    }

    restoreProtect = true;

    // read current memory to a temporary vector
    if (!ReadProcessMemory(proc, lpaddr, &actualData[0], size, &numRead) ||
        numRead != size) {
        goto end;
    }

    // compare actual data with expected data
    if (actualData != expectedData) {
        goto end;
    }

    // write patched data to memory
    if (!WriteProcessMemory(
            proc, lpaddr, &replacementData[0], size, &numWritten) ||
        numWritten != size) {
        goto end;
    }

    result = true;

end:
    // restore original page flags
    if (restoreProtect) {
        VirtualProtect(lpaddr, size, oldProtect, nullptr);
    }

    if (!result) {
#ifdef _DEBUG
        ErrorUtils::warning("Patch failed! See debug output for details.");
#endif
        LOG_INFO("Patch at 0x%x with %d bytes failed!", addr, size);
        LOG_INFO("Expected: " + StringUtils::bytesToHex(expectedData));
        LOG_INFO("Actual:   " + StringUtils::bytesToHex(actualData));
        LOG_INFO("Patch:    " + StringUtils::bytesToHex(replacementData));
    }

    return result;
}

bool Patch::patch(uint32_t addr, const Chunk& replacement)
{
    bool result = false;
    bool restoreProtect = false;

    const std::vector<uint8_t> replacementData = replacement.data();
    const size_t size = replacementData.size();

    HANDLE proc = GetCurrentProcess();
    DWORD numWritten = 0;
    DWORD oldProtect = 0;

    // apply read/write flags to the memory page
    LPVOID lpaddr = reinterpret_cast<LPVOID>(addr);
    if (!VirtualProtect(lpaddr, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        goto end;
    }

    restoreProtect = true;

    // write patched data to memory
    if (!WriteProcessMemory(
            proc, lpaddr, &replacementData[0], size, &numWritten) ||
        numWritten != size) {
        goto end;
    }

    result = true;

end:
    // restore original page flags
    if (restoreProtect) {
        VirtualProtect(lpaddr, size, oldProtect, nullptr);
    }

    if (!result) {
#ifdef _DEBUG
        ErrorUtils::warning("Patch failed! See debug output for details.");
#endif
        LOG_INFO("Patch at 0x%x with %d bytes failed!", addr, size);
        LOG_INFO("Patch:    " + StringUtils::bytesToHex(replacementData));
    }

    return result;
}

void Patch::patchAddr(
    int32_t addr, const Chunk& expected, void* func, uint8_t op)
{
    int32_t addrFunc = reinterpret_cast<int32_t>(func);
    int32_t addrCallNew = addrFunc - addr - 5;

    auto c = Chunk();
    c = c << op;
    c = c << addrCallNew;
    patch(addr, expected, c);
}

bool Patch::patchNop(uint32_t addr, const Chunk& expected)
{
    auto replacement = std::vector<uint8_t>(expected.data().size());
    std::fill(replacement.begin(), replacement.end(), 0x90);
    return patch(addr, expected, replacement);
}

} // namespace glrage
