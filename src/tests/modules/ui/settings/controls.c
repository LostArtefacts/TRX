// Runs the shipped controls dialog. The assertions live in
// modules/ui/settings/controls.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = {
        "json", "objects", "path", "stats", "strings", "weapons", nullptr,
    };
    return ScreenModule_RunWith("modules/ui/settings/controls", deps);
}
