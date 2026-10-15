// Runs the shipped color editor. The assertions live in
// modules/ui/settings/color_editor.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "strings", "weapons",
                                        nullptr };
    return ScreenModule_RunWith("modules/ui/settings/color_editor", deps);
}
