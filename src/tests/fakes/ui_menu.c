// Records the pickups announced in the corner of the screen as show_pickup
// calls.

#include <harness/fake_calls.h>

#include <trx/game/ui/menu.h>

void UI_ShowPickup(const OBJECT_ID object_id)
{
    FAKE_RECORD("show_pickup", FV(object_id));
}
