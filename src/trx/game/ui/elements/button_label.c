#include <trx/game/ui/elements/button_label.h>

#include <trx/game/ui/elements/label.h>
#include <trx/memory.h>
#include <trx/strings.h>

void UI_ButtonLabel(INPUT_ROLE input_role, const char *const label)
{
    UI_LabelFmt(
        "\\{button empty} %s: %s", Input_GetRoleName(input_role), label);
}
