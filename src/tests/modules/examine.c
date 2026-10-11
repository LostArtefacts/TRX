// Runs the shipped examine module. The assertions live in modules/examine.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "strings", nullptr };
    return ScreenModule_RunWith("modules/examine", deps);
}
