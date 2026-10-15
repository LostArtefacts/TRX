// Runs the shipped settings rows. The assertions live in
// modules/ui/settings/rows.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "strings", "weapons",
                                        nullptr };
    return ScreenModule_RunWith("modules/ui/settings/rows", deps);
}
