// Runs the menu of the flat menu system once Lara has died. The assertions
// live in modules/ui/flat_menu/death.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "stats", "strings",
                                        "weapons", nullptr };
    return ScreenModule_RunWith("modules/ui/flat_menu/death", deps);
}
