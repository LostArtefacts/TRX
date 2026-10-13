// Runs the shipped timer module. The assertions live in modules/timer.lua.

#include <harness/screen_module.h>

int main(void)
{
    return ScreenModule_Run("modules/timer");
}
