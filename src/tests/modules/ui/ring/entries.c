// Runs the entries of the shipped inventory ring. The assertions live in
// modules/ui/ring/entries.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "objects", nullptr };
    return ScreenModule_RunWith("modules/ui/ring/entries", deps);
}
