#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/misc.h"
#include "game/sphere.h"
#include "game/vars.h"
#include "specific/shed.h"
#include "config.h"
#include "util.h"

#define MAX_BOUNCE 100
#define WF_RANGE (WALL_L * 10) // = 10240
#define FLIPFLAG 0x40
#define UNFLIPFLAG 0x80

void (*effect_routines[])(ITEM_INFO* item) = {
    FxTurn180,    FxDinoStomp, FxLaraNormal,    FxLaraBubbles,  FxFinishLevel,
    FxEarthQuake, FxFlood,     FxRaisingBlock,  FxStairs2Slope, FxSand,
    FxPowerUp,    FxExplosion, FxLaraHandsFree, FxFlipMap,      FxDrawRightGun,
    FxChainBlock, FxFlicker,
};

int32_t ItemNearLara(PHD_3DPOS* pos, int32_t distance)
{
    int32_t x = pos->x - LaraItem->pos.x;
    int32_t y = pos->y - LaraItem->pos.y;
    int32_t z = pos->z - LaraItem->pos.z;

    if (x >= -distance && x <= distance && z >= -distance && z <= distance
        && y >= -WALL_L * 3 && y <= WALL_L * 3
        && SQUARE(x) + SQUARE(z) <= SQUARE(distance)) {
        int16_t* bounds = GetBoundsAccurate(LaraItem);
        if (y >= bounds[FRAME_BOUND_MIN_Y]
            && y <= bounds[FRAME_BOUND_MAX_Y] + 100) {
            return 1;
        }
    }

    return 0;
}

void SoundEffects()
{
    mn_reset_ambient_loudness();

    for (int i = 0; i < NumberSoundEffects; i++) {
        OBJECT_VECTOR* sound = &SoundEffectsTable[i];
        if (FlipStatus && (sound->flags & FLIPFLAG)) {
            SoundEffect(sound->data, (PHD_3DPOS*)sound, 0);
        } else if (!FlipStatus && (sound->flags & UNFLIPFLAG)) {
            SoundEffect(sound->data, (PHD_3DPOS*)&sound->x, 0);
        }
    }

    // NOTE: why are we firing this here?
    // Some of the FX routines rely on the item to be not null!
    if (FlipEffect != -1) {
        effect_routines[FlipEffect](NULL);
    }

    mn_update_sound_effects();
}

int16_t DoBloodSplat(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t direction,
    int16_t room_num)
{
    int16_t fx_num = CreateEffect(room_num);
    if (fx_num != NO_ITEM) {
        FX_INFO* fx = &Effects[fx_num];
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->pos.y_rot = direction;
        fx->object_number = O_BLOOD1;
        fx->frame_number = 0;
        fx->counter = 0;
        fx->speed = speed;
    }
    return fx_num;
}

void ControlBlood1(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->pos.x += (phd_sin(fx->pos.y_rot) * fx->speed) >> W2V_SHIFT;
    fx->pos.z += (phd_cos(fx->pos.y_rot) * fx->speed) >> W2V_SHIFT;
    fx->counter++;
    if (fx->counter == 4) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
            KillEffect(fx_num);
        }
    }
}

void ControlExplosion1(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->counter++;
    if (fx->counter == 2) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
            KillEffect(fx_num);
        }
    }
}

void Richochet(GAME_VECTOR* pos)
{
    int16_t fx_num = CreateEffect(pos->room_number);
    if (fx_num != NO_ITEM) {
        FX_INFO* fx = &Effects[fx_num];
        fx->pos.x = pos->x;
        fx->pos.y = pos->y;
        fx->pos.z = pos->z;
        fx->counter = 4;
        fx->object_number = O_RICOCHET1;
        fx->frame_number = -3 * GetRandomDraw() / 0x8000;
        SoundEffect(10, &fx->pos, 0);
    }
}

void ControlRicochet1(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->counter--;
    if (!fx->counter) {
        KillEffect(fx_num);
    }
}

void Twinkle(GAME_VECTOR* pos)
{
    int16_t fx_num = CreateEffect(pos->room_number);
    if (fx_num != NO_ITEM) {
        FX_INFO* fx = &Effects[fx_num];
        fx->pos.x = pos->x;
        fx->pos.y = pos->y;
        fx->pos.z = pos->z;
        fx->counter = 0;
        fx->object_number = O_TWINKLE;
        fx->frame_number = 0;
    }
}

void ControlTwinkle(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->counter++;
    if (fx->counter == 1) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
            KillEffect(fx_num);
        }
    }
}

void ItemSparkle(ITEM_INFO* item, int meshmask)
{
    SPHERE slist[34];
    GAME_VECTOR effect_pos;

    int32_t num = GetSpheres(item, slist, 1);
    effect_pos.room_number = item->room_number;
    for (int i = 0; i < num; i++) {
        if (meshmask & (1 << i)) {
            SPHERE* sptr = &slist[i];
            effect_pos.x =
                sptr->x + sptr->r * (GetRandomDraw() - 0x4000) / 0x4000;
            effect_pos.y =
                sptr->y + sptr->r * (GetRandomDraw() - 0x4000) / 0x4000;
            effect_pos.z =
                sptr->z + sptr->r * (GetRandomDraw() - 0x4000) / 0x4000;
            Twinkle(&effect_pos);
        }
    }
}

// original name: LaraBubbles
void FxLaraBubbles(ITEM_INFO* item)
{
#ifdef T1M_FEAT_CHEATS
    // NOTE: until we get Robolara, it makes sense for her to breathe underwater
    if (Lara.water_status == LWS_CHEAT
        && !(RoomInfo[LaraItem->room_number].flags & RF_UNDERWATER)) {
        return;
    }
#endif

    int32_t count = (GetRandomDraw() * 3) / 0x8000;
    if (!count) {
        return;
    }

    SoundEffect(37, &item->pos, SFX_UNDERWATER);

    PHD_VECTOR offset;
    offset.x = 0;
    offset.y = 0;
    offset.z = 50;
    GetJointAbsPosition(item, &offset, LM_HEAD);

    for (int i = 0; i < count; i++) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO* fx = &Effects[fx_num];
            fx->pos.x = offset.x;
            fx->pos.y = offset.y;
            fx->pos.z = offset.z;
            fx->object_number = O_BUBBLES1;
            fx->frame_number = -((GetRandomDraw() * 3) / 0x8000);
            fx->speed = 10 + ((GetRandomDraw() * 6) / 0x8000);
        }
    }
}

void ControlBubble1(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->pos.y_rot += 9 * PHD_DEGREE;
    fx->pos.x_rot += 13 * PHD_DEGREE;

    int32_t x = fx->pos.x + ((phd_sin(fx->pos.y_rot) * 11) >> W2V_SHIFT);
    int32_t y = fx->pos.y - fx->speed;
    int32_t z = fx->pos.z + ((phd_cos(fx->pos.x_rot) * 8) >> W2V_SHIFT);

    int16_t room_num = fx->room_number;
    FLOOR_INFO* floor = GetFloor(x, y, z, &room_num);
    if (!floor || !(RoomInfo[room_num].flags & RF_UNDERWATER)) {
        KillEffect(fx_num);
        return;
    }

    int32_t height = GetCeiling(floor, x, y, z);
    if (height == NO_HEIGHT || y <= height) {
        KillEffect(fx_num);
        return;
    }

    if (fx->room_number != room_num) {
        EffectNewRoom(fx_num, room_num);
    }
    fx->pos.x = x;
    fx->pos.y = y;
    fx->pos.z = z;
}

void Splash(ITEM_INFO* item)
{
    int16_t wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    int16_t room_num = item->room_number;
    GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);

    SoundEffect(33, &item->pos, 0);

    for (int i = 0; i < 10; i++) {
        int16_t fx_num = CreateEffect(room_num);
        if (fx_num != NO_ITEM) {
            FX_INFO* fx = &Effects[fx_num];
            fx->pos.x = item->pos.x;
            fx->pos.y = wh;
            fx->pos.z = item->pos.z;
            fx->pos.y_rot = 2 * GetRandomDraw() + 0x8000;
            fx->object_number = O_SPLASH1;
            fx->frame_number = 0;
            fx->speed = GetRandomDraw() / 256;
        }
    }
}

void ControlSplash1(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->frame_number--;
    if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
        KillEffect(fx_num);
        return;
    }

    fx->pos.z += (phd_cos(fx->pos.y_rot) * fx->speed) >> W2V_SHIFT;
    fx->pos.x += (phd_sin(fx->pos.y_rot) * fx->speed) >> W2V_SHIFT;
}

// original name: WaterFall
void ControlWaterFall(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return;
    }

    int32_t x = item->pos.x - LaraItem->pos.x;
    int32_t y = item->pos.y - LaraItem->pos.y;
    int32_t z = item->pos.z - LaraItem->pos.z;

    if (x >= -WF_RANGE && x <= WF_RANGE && z >= -WF_RANGE && z <= WF_RANGE
        && y >= -WF_RANGE && y <= WF_RANGE) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO* fx = &Effects[fx_num];
            fx->pos.x = item->pos.x
                + ((GetRandomDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
            fx->pos.z = item->pos.z
                + ((GetRandomDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
            fx->pos.y = item->pos.y;
            fx->speed = 0;
            fx->frame_number = 0;
            fx->object_number = O_SPLASH1;
        }
    }
}

// original name: finish_level_effect
void FxFinishLevel(ITEM_INFO* item)
{
    LevelComplete = 1;
}

// original name: turn180_effect
void FxTurn180(ITEM_INFO* item)
{
    item->pos.y_rot += 0x8000;
}

// original name: dino_stomp_effect
void FxDinoStomp(ITEM_INFO* item)
{
    int32_t dx = item->pos.x - Camera.pos.x;
    int32_t dy = item->pos.y - Camera.pos.y;
    int32_t dz = item->pos.z - Camera.pos.z;
    int32_t limit = 16 * WALL_L;
    if (ABS(dx) < limit && ABS(dy) < limit && ABS(dz) < limit) {
        int32_t dist = (SQUARE(dx) + SQUARE(dy) + SQUARE(dz)) / 256;
        Camera.bounce = ((SQUARE(WALL_L) - dist) * MAX_BOUNCE) / SQUARE(WALL_L);
    }
}

// original name: lara_normal_effect
void FxLaraNormal(ITEM_INFO* item)
{
    item->current_anim_state = AS_STOP;
    item->goal_anim_state = AS_STOP;
    item->anim_number = AA_STOP;
    item->frame_number = AF_STOP;
    Camera.type = CAM_CHASE;
    AlterFOV(GAME_FOV * PHD_DEGREE);
}

// original name: EarthQuakeFX
void FxEarthQuake(ITEM_INFO* item)
{
    if (FlipTimer == 0) {
        SoundEffect(99, NULL, 0);
        Camera.bounce = -250;
    } else if (FlipTimer == 3) {
        SoundEffect(147, NULL, 0);
    } else if (FlipTimer == 35) {
        SoundEffect(99, NULL, 0);
    } else if (FlipTimer == 20 || FlipTimer == 50 || FlipTimer == 70) {
        SoundEffect(70, NULL, 0);
    }

    FlipTimer++;
    if (FlipTimer == 105) {
        FlipEffect = -1;
    }
}

// original name: FloodFX
void FxFlood(ITEM_INFO* item)
{
    PHD_3DPOS pos;

    if (FlipTimer > 120) {
        FlipEffect = -1;
    } else {
        pos.x = LaraItem->pos.x;
        if (FlipTimer < 30) {
            pos.y = Camera.target.y + (30 - FlipTimer) * 100;
        } else {
            pos.y = Camera.target.y + (FlipTimer - 30) * 100;
        }
        pos.z = LaraItem->pos.z;
        SoundEffect(81, &pos, 0);
    }

    FlipTimer++;
}

// original name: RaisingBlockFX
void FxRaisingBlock(ITEM_INFO* item)
{
    SoundEffect(117, NULL, 0);
    FlipEffect = -1;
}

// original name: ChainBlockFX
void FxChainBlock(ITEM_INFO* item)
{
#ifdef T1M_FEAT_OG_FIXES
    if (T1MConfig.fix_tihocan_secret_sound) {
        SoundEffect(33, NULL, 0);
        FlipEffect = -1;
        return;
    }
#endif

    if (FlipTimer == 0) {
        SoundEffect(173, NULL, 0);
    }

    FlipTimer++;
    if (FlipTimer == 55) {
        SoundEffect(33, NULL, 0);
        FlipEffect = -1;
    }
}

// original name: Stairs2SlopeFX
void FxStairs2Slope(ITEM_INFO* item)
{
    if (FlipTimer == 5) {
        SoundEffect(119, NULL, 0);
        FlipEffect = -1;
    }
    FlipTimer++;
}

// original name: SandFX
void FxSand(ITEM_INFO* item)
{
    PHD_3DPOS pos;
    if (FlipTimer > 120) {
        FlipEffect = -1;
    } else {
        if (!FlipTimer) {
            SoundEffect(161, NULL, 0);
        }
        pos.x = Camera.target.x;
        pos.y = Camera.target.y + FlipTimer * 100;
        pos.z = Camera.target.z;
        SoundEffect(118, &pos, 0);
    }
    FlipTimer++;
}

// original name: PowerUpFX
void FxPowerUp(ITEM_INFO* item)
{
    PHD_3DPOS pos;
    if (FlipTimer > 120) {
        FlipEffect = -1;
    } else {
        pos.x = Camera.target.x;
        pos.y = Camera.target.y + FlipTimer * 100;
        pos.z = Camera.target.z;
        SoundEffect(155, &pos, 0);
    }
    FlipTimer++;
}

// original name: ExplosionFX
void FxExplosion(ITEM_INFO* item)
{
    SoundEffect(170, NULL, 0);
    Camera.bounce = -75;
    FlipEffect = -1;
}

// original name: FlickerFX
void FxFlicker(ITEM_INFO* item)
{
    if (FlipTimer > 125) {
        FlipMap();
        FlipEffect = -1;
    } else if (
        FlipTimer == 90 || FlipTimer == 92 || FlipTimer == 105
        || FlipTimer == 107) {
        FlipMap();
    }
    FlipTimer++;
}

// original name: lara_hands_free
void FxLaraHandsFree(ITEM_INFO* item)
{
    Lara.gun_status = LGS_ARMLESS;
}

// original name: flip_map_effect
void FxFlipMap(ITEM_INFO* item)
{
    FlipMap();
}

// original name: draw_right_gun
void FxDrawRightGun(ITEM_INFO* item)
{
    int16_t* tmp_mesh;
    OBJECT_INFO* obj = &Objects[item->object_number];
    tmp_mesh = Meshes[obj->mesh_index + LM_THIGH_R];
    Meshes[obj->mesh_index + LM_THIGH_R] =
        Meshes[Objects[O_PISTOLS].mesh_index + LM_THIGH_R];
    Meshes[Objects[O_PISTOLS].mesh_index + LM_THIGH_R] = tmp_mesh;
    tmp_mesh = Meshes[obj->mesh_index + LM_HAND_R];
    Meshes[obj->mesh_index + LM_HAND_R] =
        Meshes[Objects[O_PISTOLS].mesh_index + LM_HAND_R];
    Meshes[Objects[O_PISTOLS].mesh_index + LM_HAND_R] = tmp_mesh;
}

void T1MInjectGameEffects()
{
    INJECT(0x0041A210, ItemNearLara);
    INJECT(0x0041A2A0, SoundEffects);
    INJECT(0x0041A310, DoBloodSplat);
    INJECT(0x0041A370, ControlBlood1);
    INJECT(0x0041A400, ControlExplosion1);
    INJECT(0x0041A4D0, ControlRicochet1);
    INJECT(0x0041A500, ControlTwinkle);
    INJECT(0x0041A550, ItemSparkle);
    INJECT(0x0041A670, FxLaraBubbles);
    INJECT(0x0041A760, ControlBubble1);
    INJECT(0x0041A860, Splash);
    INJECT(0x0041A930, ControlSplash1);
    INJECT(0x0041A9B0, ControlWaterFall);
    INJECT(0x0041AAD0, FxFinishLevel);
    INJECT(0x0041AAE0, FxTurn180);
    INJECT(0x0041AAF0, FxDinoStomp);
    INJECT(0x0041AB90, FxLaraNormal);
    INJECT(0x0041ABD0, FxEarthQuake);
    INJECT(0x0041AC50, FxFlood);
    INJECT(0x0041ACE0, FxRaisingBlock);
    INJECT(0x0041AD00, FxChainBlock);
    INJECT(0x0041AD50, FxStairs2Slope);
    INJECT(0x0041AD80, FxSand);
    INJECT(0x0041AE00, FxPowerUp);
    INJECT(0x0041AE70, FxExplosion);
    INJECT(0x0041AEA0, FxFlicker);
    INJECT(0x0041AEF0, FxLaraHandsFree);
    INJECT(0x0041AF00, FxFlipMap);
    INJECT(0x0041AF10, FxDrawRightGun);
}
