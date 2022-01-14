#pragma once

#include "global/types.h"

#define TEETH_TRAP_DAMAGE 400

typedef enum {
    TT_NICE = 0,
    TT_NASTY = 1,
} TEETH_TRAP_STATE;

extern BITE_INFO g_Teeth1A;
extern BITE_INFO g_Teeth1B;
extern BITE_INFO g_Teeth2A;
extern BITE_INFO g_Teeth2B;
extern BITE_INFO g_Teeth3A;
extern BITE_INFO g_Teeth3B;

void SetupTeethTrap(OBJECT_INFO *obj);
void TeethTrapControl(int16_t item_num);
