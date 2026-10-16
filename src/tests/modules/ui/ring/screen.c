// Runs the shipped inventory ring. The assertions live in
// modules/ui/ring/screen.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "strings", "weapons",
                                        nullptr };
    return ScreenModule_RunWith("modules/ui/ring/screen", deps);
}
