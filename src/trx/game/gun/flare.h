#pragma once

#include <stdint.h>

bool Gun_Flare_HasExpired(void);
bool Gun_Flare_IsMeshActive(void);
void Gun_Flare_DrawMeshes(void);

void Gun_Flare_Control(void);
void Gun_Flare_Draw(void);
void Gun_Flare_Undraw(void);
void Gun_Flare_Dispose(bool thrown);

// Puts a lit flare in Lara's hand, raised and burning from the given age, as
// she arrives in a level carrying one.
void Gun_Flare_Hold(int16_t age);

// Takes the flare out of Lara's hand and forgets how far it had burned,
// leaving nothing behind in the level.
void Gun_Flare_Clear(void);
