// Runs the shipped level view module. The assertions live in
// modules/ui/screens/level_view.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "strings", nullptr };
    return ScreenModule_RunWith("modules/ui/screens/level_view", deps);
}
