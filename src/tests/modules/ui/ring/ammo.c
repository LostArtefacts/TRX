// Runs the shipped ammunition module. The assertions live in
// modules/ui/ring/ammo.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "strings", "weapons",
                                        nullptr };
    return ScreenModule_RunWith("modules/ui/ring/ammo", deps);
}
