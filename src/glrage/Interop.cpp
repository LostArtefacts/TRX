#include "glrage/Interop.hpp"

#include "glrage/GLRage.hpp"

using namespace glrage;

void GLRage_Attach(HWND hwnd)
{
    auto &context = GLRage::getContext();
    context.attach(hwnd);
}

void GLRage_Detach()
{
    auto &context = GLRage::getContext();
    context.detach();
}

void GLRage_SetFullscreen(bool fullscreen)
{
    auto &context = GLRage::getContext();
    context.setFullscreen(fullscreen);
}

void GLRage_SetWindowSize(int width, int height)
{
    auto &context = GLRage::getContext();
    context.setWindowSize(width, height);
}
