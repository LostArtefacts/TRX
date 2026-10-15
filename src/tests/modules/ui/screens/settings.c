// Runs the shipped settings screens. The assertions live in
// modules/ui/screens/settings.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = {
        "json", "objects", "path", "stats", "strings", "weapons", nullptr,
    };
    return ScreenModule_RunWith("modules/ui/screens/settings", deps);
}
