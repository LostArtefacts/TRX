#include "game/effects/bubble.h"
#include "game/effects/chain_block.h"
#include "game/effects/dino_stomp.h"
#include "game/effects/earthquake.h"
#include "game/effects/explosion.h"
#include "game/effects/finish_level.h"
#include "game/effects/flicker.h"
#include "game/effects/flipmap.h"
#include "game/effects/flood.h"
#include "game/effects/lara_effects.h"
#include "game/effects/powerup.h"
#include "game/effects/raising_block.h"
#include "game/effects/sand.h"
#include "game/effects/stairs2slope.h"
#include "game/effects/turn_180.h"
#include "game/vars.h"

void (*EffectRoutines[])(ITEM_INFO *item) = {
    Turn180,    DinoStomp, LaraNormal,    LaraBubbles,  FinishLevel,
    EarthQuake, Flood,     RaisingBlock,  Stairs2Slope, DropSand,
    PowerUp,    Explosion, LaraHandsFree, FxFlipMap,    LaraDrawRightGun,
    ChainBlock, Flicker,
};

SAVEGAME_INFO SaveGame;
int32_t DemoLevel;

int16_t StoredLaraHealth = 0;

int16_t BarOffsetY[6];

GameFlow GF;
