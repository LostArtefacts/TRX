#include <trx/game/fx/fx.h>

#include <trx/game/fx/explosion_ring.h>
#include <trx/game/fx/footprint.h>
#include <trx/game/fx/gun_flash.h>
#include <trx/game/fx/laser.h>
#include <trx/game/fx/wake.h>
#include <trx/game/fx/water.h>
#include <trx/game/fx/weather.h>

void FX_Control(void)
{
    FX_ExplosionRing_Control();
    FX_Wake_Control();
    FX_Water_Control();
    FX_Weather_Control();
    FX_Footprint_Control();
    FX_GunFlash_Control();
    FX_Laser_Control();
}

void FX_Draw(void)
{
    FX_ExplosionRing_Draw();
    FX_Water_Draw();
    FX_Weather_Draw();
    FX_GunFlash_Draw();
    FX_Laser_Draw();
    FX_Footprint_Draw();
}
