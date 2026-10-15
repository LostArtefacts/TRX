// Runs the shipped text dialog widget. The assertions live in
// modules/ui/widgets/text_dialog.lua.

#include <harness/screen_module.h>

int main(void)
{
    static const char *const deps[] = { "strings", nullptr };
    return ScreenModule_RunWith("modules/ui/widgets/text_dialog", deps);
}
