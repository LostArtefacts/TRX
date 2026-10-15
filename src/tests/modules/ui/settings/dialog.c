// Runs the shipped settings dialog. The assertions live in
// modules/ui/settings/dialog.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = {
        "objects", "stats", "strings", "weapons", nullptr,
    };
    return ScreenModule_RunWith("modules/ui/settings/dialog", deps);
}
