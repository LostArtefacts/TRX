// Puts every draw in the interface layer, for tests that draw a leaf on its own
// without a scene to open the layer.

#include <trx/game/ui/common.h>

UI_LAYER UI_GetLayer(void)
{
    return UI_LAYER_INTERFACE;
}
