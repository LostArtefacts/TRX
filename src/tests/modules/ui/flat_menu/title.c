// Runs the title menu of the flat menu system. The assertions live in
// modules/ui/flat_menu/title.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "stats", "strings",
                                        "weapons", nullptr };
    return ScreenModule_RunWith("modules/ui/flat_menu/title", deps);
}
