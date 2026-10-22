// Runs the shipped controls model setting. The assertions live in
// modules/ui/controls_model.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", nullptr };
    return ScreenModule_RunWith("modules/ui/controls_model", deps);
}
