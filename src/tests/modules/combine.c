// Runs the shipped combine module. The assertions live in modules/combine.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "strings", nullptr };
    return ScreenModule_RunWith("modules/combine", deps);
}
