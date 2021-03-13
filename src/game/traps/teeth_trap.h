#ifndef T1M_GAME_TRAPS_TEETH_TRAP_H
#define T1M_GAME_TRAPS_TEETH_TRAP_H

#include "game/types.h"

#define TEETH_TRAP_DAMAGE 400

typedef enum {
    TT_NICE = 0,
    TT_NASTY = 1,
} TEETH_TRAP_STATE;

extern BITE_INFO Teeth1A;
extern BITE_INFO Teeth1B;
extern BITE_INFO Teeth2A;
extern BITE_INFO Teeth2B;
extern BITE_INFO Teeth3A;
extern BITE_INFO Teeth3B;

void SetupTeethTrap(OBJECT_INFO *obj);
void TeethTrapControl(int16_t item_num);

#endif
