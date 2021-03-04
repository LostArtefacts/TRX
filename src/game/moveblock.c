#include "game/moveblock.h"
#include "game/types.h"
#include "game/vars.h"
#include "util.h"

// original name: InitialiseMovingBlock
void InitialiseMovableBlock(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    if (item->status != IS_INVISIBLE) {
        AlterFloorHeight(item, -1024);
    }
}

void T1MInjectGameMoveBlock()
{
    INJECT(0x0042B430, InitialiseMovableBlock);
}
