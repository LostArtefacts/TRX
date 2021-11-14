#include "GLRage.hpp"

#include <glrage_util/Logger.hpp>

using namespace glrage;

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    LOG_TRACE("%p,%d", hInst, dwReason);
    return TRUE;
}
