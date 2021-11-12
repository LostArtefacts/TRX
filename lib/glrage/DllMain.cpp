#include "GLRage.hpp"

#include <glrage_util/Logger.hpp>

using namespace glrage;

extern "C" { int _afxForceUSRDLL; }

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    LOG_TRACE("%p,%d", hInst, dwReason);

    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            GLRage::getPatcher().patch();
            break;
    }

    return TRUE;
}