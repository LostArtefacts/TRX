// Runs the shipped settings presets. The assertions live in
// modules/ui/settings/presets.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "json", "path", "strings", nullptr };
    return ScreenModule_RunWith("modules/ui/settings/presets", deps);
}
