#include "glrage/GLRageInterop.hpp"

#include "glrage/GLRage.hpp"

using namespace glrage;

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
