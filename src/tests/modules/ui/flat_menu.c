// Runs the shipped flat menu. The assertions live in modules/ui/flat_menu.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "items",   "lara",    "objects",
                                        "strings", "weapons", nullptr };
    return ScreenModule_RunWith("modules/ui/flat_menu", deps);
}
