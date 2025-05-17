#include "game/ui/elements/button_label.h"

#include "game/ui/elements/label.h"
#include "memory.h"
#include "strings.h"

void UI_ButtonLabel(INPUT_ROLE input_role, const char *const label)
{
    char *buf = String_Format(
        "\\{button empty} %s: %s", Input_GetRoleName(input_role), label);
    UI_Label(buf);
    Memory_FreePointer(&buf);
}
