#include <trx/game/ui/elements/scrollable_area.h>

#include <trx/game/ui/elements/anchor.h>
#include <trx/game/ui/elements/fixed.h>
#include <trx/game/ui/elements/hide.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/ui/elements/spacer.h>
#include <trx/game/ui/elements/stack.h>
#include <trx/version.h>

static void M_UpArrow(const UI_SCROLLABLE *const s)
{
    const bool show_arrow = s->first_item != 0;
    UI_BeginHide(!show_arrow);
    UI_Spacer(0.0f, g_TRVersion >= 2 ? 6.0f : 4.0f);
    UI_BeginAnchor(0.5f, 0.5f);
    UI_BeginFixed(0.5f, g_TRVersion >= 2 ? 0.7f : 0.9f);
    UI_LabelEx("\\{arrow up}", (UI_LABEL_SETTINGS) { .scale = 0.7 });
    UI_EndFixed();
    UI_EndAnchor();
    UI_EndHide();
}

static void M_DownArrow(const UI_SCROLLABLE *const s)
{
    const bool show_arrow = s->first_item + s->vis_items < s->max_items;
    UI_BeginHide(!show_arrow);
    UI_BeginAnchor(0.5f, 0.0f);
    UI_BeginFixed(0.5f, -0.3f);
    UI_LabelEx("\\{arrow down}", (UI_LABEL_SETTINGS) { .scale = 0.7 });
    UI_EndFixed();
    UI_EndAnchor();
    UI_Spacer(0.0f, g_TRVersion >= 2 ? 6.0f : 4.0f);
    UI_EndHide();
}

void UI_BeginScrollableArea(const UI_SCROLLABLE *const s, const bool force_show)
{
    if (s->vis_items > 0 || force_show) {
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_VERTICAL,
            .align = { .h = UI_STACK_H_ALIGN_SPAN },
        });
        M_UpArrow(s);
    }
}

void UI_EndScrollableArea(const UI_SCROLLABLE *const s, const bool force_show)
{
    if (s->vis_items > 0 || force_show) {
        M_DownArrow(s);
        UI_EndStack();
    }
}
