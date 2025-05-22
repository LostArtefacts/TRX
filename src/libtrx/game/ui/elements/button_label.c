#include "game/ui/elements/button_label.h"

#include "game/ui/elements/label.h"
#include "memory.h"
#include "strings.h"

void UI_ButtonLabel(INPUT_ROLE input_role, const char *const label)
{
    UI_LabelFmt(
        "\\{button empty} %s: %s", Input_GetRoleName(input_role), label);
}
