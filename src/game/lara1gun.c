#include "game/effects.h"
#include "game/data.h"
#include "game/lara.h"
#include "specific/game.h"
#include "specific/input.h"
#include "mod.h"

void __cdecl RifleHandler(int32_t weapon_type)
{
    WEAPON_INFO* winfo = &Weapons[LGT_SHOTGUN];

    if (Input & IN_ACTION) {
        LaraTargetInfo(winfo);
    } else {
        Lara.target = NULL;
    }
    if (!Lara.target) {
        LaraGetNewTarget(winfo);
    }

    AimWeapon(winfo, &Lara.left_arm);

    if (Lara.left_arm.lock) {
        Lara.torso_y_rot = Lara.left_arm.y_rot / 2;
        Lara.torso_x_rot = Lara.left_arm.x_rot / 2;
        Lara.head_x_rot = 0;
        Lara.head_y_rot = 0;
    }

    AnimateShotgun();
}

void __cdecl AnimateShotgun()
{
    int16_t ani = Lara.left_arm.frame_number;

    if (Lara.left_arm.lock) {
        if (ani >= AF_SG_AIM && ani < AF_SG_DRAW) {
            ani++;
            if (ani == AF_SG_DRAW) {
                ani = AF_SG_RECOIL;
            }
        } else if (ani == AF_SG_RECOIL) {
            if (Input & IN_ACTION) {
                FireShotgun();
                ani++;
            }
        } else if (ani > AF_SG_RECOIL && ani < AF_SG_UNDRAW) {
            ani++;
            if (ani == AF_SG_UNDRAW) {
                ani = AF_SG_RECOIL;
            } else if (ani == AF_SG_RECOIL + 10) {
                SoundEffect(9, &LaraItem->pos, 0);
            }
        } else if (ani >= AF_SG_UNAIM && ani < AF_SG_END) {
            ani++;
            if (ani == AF_SG_END) {
                ani = AF_SG_AIM;
            }
        }
    } else {
        if (ani == AF_SG_AIM && (Input & IN_ACTION)) {
            ani++;
        } else if (ani > AF_SG_AIM && ani < AF_SG_DRAW) {
            ani++;
            if (ani == AF_SG_DRAW) {
                if (Input & IN_ACTION) {
                    ani = AF_SG_RECOIL;
                } else {
                    ani = AF_SG_UNAIM;
                }
            }
        } else {
            if (ani == AF_SG_RECOIL) {
                if (Input & IN_ACTION) {
                    FireShotgun();
                    ani++;
                } else {
                    ani = AF_SG_UNAIM;
                }
            } else if (ani > AF_SG_RECOIL && ani < AF_SG_UNDRAW) {
                ani++;
                if (ani == AF_SG_RECOIL + 12 + 1) {
                    ani = AF_SG_AIM;
                } else if (ani == AF_SG_UNDRAW) {
                    ani = AF_SG_UNAIM;
                } else if (ani == AF_SG_RECOIL + 10) {
                    SoundEffect(9, &LaraItem->pos, 0);
                }
            } else if (ani >= AF_SG_UNAIM && ani < AF_SG_END) {
                ani++;
                if (ani == AF_SG_END) {
                    ani = AF_SG_AIM;
                }
            }
        }
    }

    Lara.right_arm.frame_number = ani;
    Lara.left_arm.frame_number = ani;
}

void __cdecl FireShotgun()
{
    int i, r, fired;
    PHD_ANGLE angles[2];
    PHD_ANGLE dangles[2];

    angles[0] = Lara.left_arm.y_rot + LaraItem->pos.y_rot;
    angles[1] = Lara.left_arm.x_rot;

    for (int i = 0; i < SHOTGUN_AMMO_CLIP; i++) {
        r = (int)((GetRandomControl() - 16384) * PELLET_SCATTER) / 65536;
        dangles[0] = angles[0] + r;
        r = (int)((GetRandomControl() - 16384) * PELLET_SCATTER) / 65536;
        dangles[1] = angles[1] + r;
        if (FireWeapon(LGT_SHOTGUN, Lara.target, LaraItem, dangles)) {
            fired = 1;
        }
    }
    if (fired) {
        if (Tomb1MConfig.enable_shotgun_flash) {
            Lara.right_arm.flash_gun = Weapons[LGT_SHOTGUN].flash_time;
        }
        SoundEffect(Weapons[LGT_SHOTGUN].sample_num, &LaraItem->pos, 0);
    }
}

void Tomb1MInjectGameLaraGun1()
{
    INJECT(0x004260F0, RifleHandler);
}
