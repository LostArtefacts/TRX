// Runs the shipped weapon dialog module. The assertions live in
// modules/ui/ring/weapon.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "strings", "weapons",
                                        nullptr };
    return ScreenModule_RunWith("modules/ui/ring/weapon", deps);
}
