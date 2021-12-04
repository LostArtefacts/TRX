#include "glrage/Interop.hpp"

#include "glrage/Context.hpp"

using namespace glrage;

static Context &context = Context::instance();

void GLRage_Attach(HWND hwnd)
{
    context.attach(hwnd);
}

void GLRage_Detach()
{
    context.detach();
}

void GLRage_SetFullscreen(bool fullscreen)
{
    context.setFullscreen(fullscreen);
}

void GLRage_SetWindowSize(int width, int height)
{
    context.setWindowSize(width, height);
}

bool GLRage_MakeScreenshot(const char *path)
{
    context.scheduleScreenshot(std::string(path));
    return true;
}
