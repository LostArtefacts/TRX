// Runs the shipped settings preset_tab. The assertions live in
// modules/ui/settings/preset_tab.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "json", "path", "strings", nullptr };
    return ScreenModule_RunWith("modules/ui/settings/preset_tab", deps);
}
