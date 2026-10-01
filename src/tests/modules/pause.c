// Runs the shipped pause module. The assertions live in modules/pause.lua.

#include <harness/screen_module.h>

int main(void)
{
    return ScreenModule_Run("modules/pause");
}
