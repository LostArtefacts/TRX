// Runs the shipped statistics module. The assertions live in modules/stats.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "stats", "assault", "strings",
                                        nullptr };
    return ScreenModule_RunWith("modules/stats", deps);
}
