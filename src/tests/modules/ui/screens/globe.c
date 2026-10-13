// Runs the shipped globe module. The assertions live in
// modules/ui/screens/globe.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "strings", nullptr };
    return ScreenModule_RunWith("modules/ui/screens/globe", deps);
}
