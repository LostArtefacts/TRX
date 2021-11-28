#include "glrage/GLRageInterop.hpp"

#include "glrage/GLRage.hpp"

using namespace glrage;

void GLRAPI GLRage_Attach(HWND hwnd)
{
    auto& context = GLRage::getContext();
    context.attach(hwnd);
}

void GLRAPI GLRage_Detach()
{
    auto& context = GLRage::getContext();
    context.detach();
}

void GLRAPI GLRage_SetFullscreen(bool fullscreen)
{
    auto& context = GLRage::getContext();
    context.setFullscreen(fullscreen);
}

void GLRAPI GLRage_SetWindowSize(int width, int height)
{
    auto& context = GLRage::getContext();
    context.setWindowSize(width, height);
}
