// Runs the shipped settings editor. The assertions live in
// modules/ui/settings/editor.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", "strings", "weapons",
                                        nullptr };
    return ScreenModule_RunWith("modules/ui/settings/editor", deps);
}
