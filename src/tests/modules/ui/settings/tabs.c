// Runs the shipped settings tabs. The assertions live in
// modules/ui/settings/tabs.lua.

#include <harness/screen_module.h>

int main(void)
{
    return ScreenModule_Run("modules/ui/settings/tabs");
}
