#include "3dsystem/phd_math.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/sphere.h"
#include "game/traps.h"
#include "game/vars.h"
#include "specific/init.h"
#include "util.h"

#define ROLLINGBALL_DAMAGE_AIR 100
#define SPIKE_DAMAGE 15
#define PENDULUM_DAMAGE 100
#define TEETH_TRAP_DAMAGE 400
#define FALLING_CEILING_DAMAGE 300
#define DAMOCLES_SWORD_ACTIVATE_DIST ((WALL_L * 3) / 2)
#define DAMOCLES_SWORD_DAMAGE 100
#define FLAME_ONFIRE_DAMAGE 5
#define FLAME_TOONEAR_DAMAGE 3
#define LAVA_GLOB_DAMAGE 10
#define LAVA_WEDGE_SPEED 25

typedef enum {
    TT_NICE = 0,
    TT_NASTY = 1,
} TEETH_TRAP_STATE;

typedef enum {
    DE_IDLE = 0,
    DE_FIRE = 1,
} DART_EMITTER_STATE;

static BITE_INFO Teeth1A = { -23, 0, -1718, 0 };
static BITE_INFO Teeth1B = { 71, 0, -1718, 1 };
static BITE_INFO Teeth2A = { -23, 10, -1718, 0 };
static BITE_INFO Teeth2B = { 71, 10, -1718, 1 };
static BITE_INFO Teeth3A = { -23, -10, -1718, 0 };
static BITE_INFO Teeth3B = { 71, -10, -1718, 1 };

void InitialiseRollingBall(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    GAME_VECTOR *old = game_malloc(sizeof(GAME_VECTOR), GBUF_ROLLINGBALL_STUFF);
    item->data = old;
    old->x = item->pos.x;
    old->y = item->pos.y;
    old->z = item->pos.z;
    old->room_number = item->room_number;
}

void RollingBallControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->status == IS_ACTIVE) {
        if (item->pos.y < item->floor) {
            if (!item->gravity_status) {
                item->gravity_status = 1;
                item->fall_speed = -10;
            }
        } else if (item->current_anim_state == TRAP_SET) {
            item->goal_anim_state = TRAP_ACTIVATE;
        }

        int32_t oldx = item->pos.x;
        int32_t oldz = item->pos.z;
        AnimateItem(item);

        int16_t room_num = item->room_number;
        FLOOR_INFO *floor =
            GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        if (item->room_number != room_num) {
            ItemNewRoom(item_num, room_num);
        }

        item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

        TestTriggers(TriggerIndex, 1);

        if (item->pos.y >= item->floor - STEP_L) {
            item->gravity_status = 0;
            item->fall_speed = 0;
            item->pos.y = item->floor;
        }

        int32_t x = item->pos.x
            + (((WALL_L / 2) * phd_sin(item->pos.y_rot)) >> W2V_SHIFT);
        int32_t z = item->pos.z
            + (((WALL_L / 2) * phd_cos(item->pos.y_rot)) >> W2V_SHIFT);
        floor = GetFloor(x, item->pos.y, z, &room_num);
        if (GetHeight(floor, x, item->pos.y, z) < item->pos.y) {
            item->status = IS_DEACTIVATED;
            item->pos.x = oldx;
            item->pos.y = item->floor;
            item->pos.z = oldz;
            item->speed = 0;
            item->fall_speed = 0;
            item->touch_bits = 0;
        }
    } else if (item->status == IS_DEACTIVATED && !TriggerActive(item)) {
        item->status = IS_NOT_ACTIVE;
        GAME_VECTOR *old = item->data;
        item->pos.x = old->x;
        item->pos.y = old->y;
        item->pos.z = old->z;
        if (item->room_number != old->room_number) {
            RemoveDrawnItem(item_num);
            ROOM_INFO *r = &RoomInfo[old->room_number];
            item->next_item = r->item_number;
            r->item_number = item_num;
            item->room_number = old->room_number;
        }
        item->current_anim_state = TRAP_SET;
        item->goal_anim_state = TRAP_SET;
        item->anim_number = Objects[item->object_number].anim_index;
        item->frame_number = Anims[item->anim_number].frame_base;
        item->current_anim_state = Anims[item->anim_number].current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        item->required_anim_state = TRAP_SET;
        RemoveActiveItem(item_num);
    }
}

void RollingBallCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];

    if (item->status != IS_ACTIVE) {
        if (item->status != IS_INVISIBLE) {
            ObjectCollision(item_num, lara_item, coll);
        }
        return;
    }

    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (!TestCollision(item, lara_item)) {
        return;
    }

    int32_t x, y, z, d;
    if (lara_item->gravity_status) {
        if (coll->enable_baddie_push) {
            ItemPushLara(item, lara_item, coll, coll->enable_spaz, 1);
        }
        lara_item->hit_points -= ROLLINGBALL_DAMAGE_AIR;
        x = lara_item->pos.x - item->pos.x;
        z = lara_item->pos.z - item->pos.z;
        y = (lara_item->pos.y - 350) - (item->pos.y - WALL_L / 2);
        d = phd_sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z));
        if (d < WALL_L / 2) {
            d = WALL_L / 2;
        }
        x = item->pos.x + (x << WALL_SHIFT) / 2 / d;
        z = item->pos.z + (z << WALL_SHIFT) / 2 / d;
        y = item->pos.y - WALL_L / 2 + (y << WALL_SHIFT) / 2 / d;
        DoBloodSplat(x, y, z, item->speed, item->pos.y_rot, item->room_number);
    } else {
        lara_item->hit_status = 1;
        if (lara_item->hit_points > 0) {
            lara_item->hit_points = -1;
            if (lara_item->room_number != item->room_number) {
                ItemNewRoom(Lara.item_number, item->room_number);
            }

            lara_item->pos.x_rot = 0;
            lara_item->pos.z_rot = 0;
            lara_item->pos.y_rot = item->pos.y_rot;

            lara_item->current_anim_state = AS_SPECIAL;
            lara_item->goal_anim_state = AS_SPECIAL;
            lara_item->anim_number = AA_RBALL_DEATH;
            lara_item->frame_number = AF_RBALL_DEATH;

            Camera.flags = FOLLOW_CENTRE;
            Camera.target_angle = 170 * PHD_DEGREE;
            Camera.target_elevation = -25 * PHD_DEGREE;
            for (int i = 0; i < 15; i++) {
                x = lara_item->pos.x + (GetRandomControl() - 0x4000) / 256;
                z = lara_item->pos.z + (GetRandomControl() - 0x4000) / 256;
                y = lara_item->pos.y - GetRandomControl() / 64;
                d = item->pos.y_rot + (GetRandomControl() - 0x4000) / 8;
                DoBloodSplat(x, y, z, item->speed * 2, d, item->room_number);
            }
        }
    }
}

void SpikeCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];
    if (lara_item->hit_points < 0) {
        return;
    }

    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (!TestCollision(item, lara_item)) {
        return;
    }

    int32_t num = GetRandomControl() / 24576;
    if (lara_item->gravity_status) {
        if (lara_item->fall_speed > 0) {
            lara_item->hit_points = -1;
            num = 20;
        }
    } else if (lara_item->speed < 30) {
        return;
    }

    lara_item->hit_points -= SPIKE_DAMAGE;
    while (num > 0) {
        int32_t x = lara_item->pos.x + (GetRandomControl() - 0x4000) / 256;
        int32_t z = lara_item->pos.z + (GetRandomControl() - 0x4000) / 256;
        int32_t y = lara_item->pos.y - GetRandomControl() / 64;
        DoBloodSplat(x, y, z, 20, GetRandomControl(), item->room_number);
        num--;
    }

    if (lara_item->hit_points <= 0) {
        lara_item->current_anim_state = AS_DEATH;
        lara_item->goal_anim_state = AS_DEATH;
        lara_item->anim_number = AA_SPIKE_DEATH;
        lara_item->frame_number = AF_SPIKE_DEATH;
        lara_item->pos.y = item->pos.y;
        lara_item->gravity_status = 0;
    }
}

void TrapDoorControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (TriggerActive(item)) {
        if (item->current_anim_state == DOOR_CLOSED) {
            item->goal_anim_state = DOOR_OPEN;
        }
    } else if (item->current_anim_state == DOOR_OPEN) {
        item->goal_anim_state = DOOR_CLOSED;
    }
    AnimateItem(item);
}

void TrapDoorFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (!OnTrapDoor(item, x, z)) {
        return;
    }
    if (y <= item->pos.y && item->current_anim_state == DOOR_CLOSED
        && item->pos.y < *height) {
        *height = item->pos.y;
    }
}

void TrapDoorCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (!OnTrapDoor(item, x, z)) {
        return;
    }
    if (y > item->pos.y && item->current_anim_state == DOOR_CLOSED
        && item->pos.y > *height) {
        *height = (int16_t)item->pos.y + STEP_L;
    }
}

int32_t OnTrapDoor(ITEM_INFO *item, int32_t x, int32_t z)
{
    x >>= WALL_SHIFT;
    z >>= WALL_SHIFT;
    int32_t tx = item->pos.x >> WALL_SHIFT;
    int32_t tz = item->pos.z >> WALL_SHIFT;
    if (item->pos.y_rot == 0 && x == tx && (z == tz || z == tz + 1)) {
        return 1;
    } else if (
        item->pos.y_rot == -PHD_180 && x == tx && (z == tz || z == tz - 1)) {
        return 1;
    } else if (
        item->pos.y_rot == PHD_90 && z == tz && (x == tx || x == tx + 1)) {
        return 1;
    } else if (
        item->pos.y_rot == -PHD_90 && z == tz && (x == tx || x == tx - 1)) {
        return 1;
    }
    return 0;
}

// original name: Pendulum
void PendulumControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if (TriggerActive(item)) {
        if (item->current_anim_state == TRAP_SET) {
            item->goal_anim_state = TRAP_WORKING;
        }
    } else {
        if (item->current_anim_state == TRAP_WORKING) {
            item->goal_anim_state = TRAP_SET;
        }
    }

    if (item->current_anim_state == TRAP_WORKING && item->touch_bits) {
        LaraItem->hit_points -= PENDULUM_DAMAGE;
        LaraItem->hit_status = 1;
        int32_t x = LaraItem->pos.x + (GetRandomControl() - 0x4000) / 256;
        int32_t z = LaraItem->pos.z + (GetRandomControl() - 0x4000) / 256;
        int32_t y = LaraItem->pos.y - GetRandomControl() / 44;
        int32_t d = LaraItem->pos.y_rot + (GetRandomControl() - 0x4000) / 8;
        DoBloodSplat(x, y, z, LaraItem->speed, d, LaraItem->room_number);
    }

    FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &item->room_number);
    item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

    AnimateItem(item);
}

// original name: FallingBlock
void FallingBlockControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    switch (item->current_anim_state) {
    case TRAP_SET:
        if (LaraItem->pos.y == item->pos.y - STEP_L * 2) {
            item->goal_anim_state = TRAP_ACTIVATE;
        } else {
            item->status = IS_NOT_ACTIVE;
            RemoveActiveItem(item_num);
            return;
        }
        break;

    case TRAP_ACTIVATE:
        item->goal_anim_state = TRAP_WORKING;
        break;

    case TRAP_WORKING:
        if (item->goal_anim_state != TRAP_FINISHED) {
            item->gravity_status = 1;
        }
        break;
    }

    AnimateItem(item);
    if (item->status == IS_DEACTIVATED) {
        RemoveActiveItem(item_num);
        return;
    }

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_number != room_num) {
        ItemNewRoom(item_num, room_num);
    }

    item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

    if (item->current_anim_state == TRAP_WORKING
        && item->pos.y >= item->floor) {
        item->goal_anim_state = TRAP_FINISHED;
        item->pos.y = item->floor;
        item->fall_speed = 0;
        item->gravity_status = 0;
    }
}

void FallingBlockFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (y <= item->pos.y - STEP_L * 2
        && (item->current_anim_state == TRAP_SET
            || item->current_anim_state == TRAP_ACTIVATE)) {
        *height = item->pos.y - STEP_L * 2;
    }
}

void FallingBlockCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (y > item->pos.y - STEP_L * 2
        && (item->current_anim_state == TRAP_SET
            || item->current_anim_state == TRAP_ACTIVATE)) {
        *height = item->pos.y - STEP_L;
    }
}

// original name: TeethTrap
void TeethTrapControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (TriggerActive(item)) {
        item->goal_anim_state = TT_NASTY;
        if (item->touch_bits && item->current_anim_state == TT_NASTY) {
            LaraItem->hit_points -= TEETH_TRAP_DAMAGE;
            LaraItem->hit_status = 1;
            BaddieBiteEffect(item, &Teeth1A);
            BaddieBiteEffect(item, &Teeth1B);
            BaddieBiteEffect(item, &Teeth2A);
            BaddieBiteEffect(item, &Teeth2B);
            BaddieBiteEffect(item, &Teeth3A);
            BaddieBiteEffect(item, &Teeth3B);
        }
    } else {
        item->goal_anim_state = TT_NICE;
    }
    AnimateItem(item);
}

// original name: FallingCeiling
void FallingCeilingControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->current_anim_state == TRAP_SET) {
        item->goal_anim_state = TRAP_ACTIVATE;
        item->gravity_status = 1;
    } else if (item->current_anim_state == TRAP_ACTIVATE && item->touch_bits) {
        LaraItem->hit_points -= FALLING_CEILING_DAMAGE;
        LaraItem->hit_status = 1;
    }
    AnimateItem(item);
    if (item->status == IS_DEACTIVATED) {
        RemoveActiveItem(item_num);
    } else if (
        item->current_anim_state == TRAP_ACTIVATE
        && item->pos.y >= item->floor) {
        item->goal_anim_state = TRAP_WORKING;
        item->pos.y = item->floor;
        item->fall_speed = 0;
        item->gravity_status = 0;
    }
}

void InitialiseDamoclesSword(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    item->pos.y_rot = GetRandomControl();
    item->required_anim_state = (GetRandomControl() - 0x4000) / 16;
    item->fall_speed = 50;
}

void DamoclesSwordControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->gravity_status) {
        item->pos.y_rot += item->required_anim_state;
        item->fall_speed += item->fall_speed < FASTFALL_SPEED ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
        item->pos.x += item->current_anim_state;
        item->pos.z += item->goal_anim_state;

        if (item->pos.y > item->floor) {
            SoundEffect(103, &item->pos, 0);
            item->pos.y = item->floor + 10;
            item->gravity_status = 0;
            item->status = IS_DEACTIVATED;
            RemoveActiveItem(item_num);
        }
    } else if (item->pos.y != item->floor) {
        item->pos.y_rot += item->required_anim_state;
        int32_t x = LaraItem->pos.x - item->pos.x;
        int32_t y = LaraItem->pos.y - item->pos.y;
        int32_t z = LaraItem->pos.z - item->pos.z;
        if (ABS(x) <= DAMOCLES_SWORD_ACTIVATE_DIST
            && ABS(z) <= DAMOCLES_SWORD_ACTIVATE_DIST && y > 0
            && y < WALL_L * 3) {
            item->current_anim_state = x / 32;
            item->goal_anim_state = z / 32;
            item->gravity_status = 1;
        }
    }
}

void DamoclesSwordCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];
    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push) {
        ItemPushLara(item, lara_item, coll, 0, 1);
    }
    if (item->gravity_status) {
        lara_item->hit_points -= DAMOCLES_SWORD_DAMAGE;
        int32_t x = lara_item->pos.x + (GetRandomControl() - 0x4000) / 256;
        int32_t z = lara_item->pos.z + (GetRandomControl() - 0x4000) / 256;
        int32_t y = lara_item->pos.y - GetRandomControl() / 44;
        int32_t d = lara_item->pos.y_rot + (GetRandomControl() - 0x4000) / 8;
        DoBloodSplat(x, y, z, lara_item->speed, d, lara_item->room_number);
    }
}

void DartEmitterControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if (TriggerActive(item)) {
        if (item->current_anim_state == DE_IDLE) {
            item->goal_anim_state = DE_FIRE;
        }
    } else {
        if (item->current_anim_state == DE_FIRE) {
            item->goal_anim_state = DE_IDLE;
        }
    }

    if (item->current_anim_state == DE_FIRE
        && item->frame_number == Anims[item->anim_number].frame_base) {
        int16_t dart_item_num = CreateItem();
        if (dart_item_num != NO_ITEM) {
            ITEM_INFO *dart = &Items[dart_item_num];
            dart->object_number = O_DARTS;
            dart->room_number = item->room_number;
            dart->shade = -1;
            dart->pos.y_rot = item->pos.y_rot;
            dart->pos.y = item->pos.y - WALL_L / 2;

            int32_t x = 0;
            int32_t z = 0;
            switch (dart->pos.y_rot) {
            case 0:
                z = -WALL_L / 2 + 100;
                break;
            case PHD_90:
                x = -WALL_L / 2 + 100;
                break;
            case -PHD_180:
                z = WALL_L / 2 - 100;
                break;
            case -PHD_90:
                x = WALL_L / 2 - 100;
                break;
            }

            dart->pos.x = item->pos.x + x;
            dart->pos.z = item->pos.z + z;
            InitialiseItem(dart_item_num);
            AddActiveItem(dart_item_num);
            dart->status = IS_ACTIVE;

            int16_t fx_num = CreateEffect(dart->room_number);
            if (fx_num != NO_ITEM) {
                FX_INFO *fx = &Effects[fx_num];
                fx->pos = dart->pos;
                fx->speed = 0;
                fx->frame_number = 0;
                fx->counter = 0;
                fx->object_number = O_DART_EFFECT;
                SoundEffect(151, &fx->pos, 0);
            }
        }
    }
    AnimateItem(item);
}

void DartsControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->touch_bits) {
        LaraItem->hit_points -= 50;
        LaraItem->hit_status = 1;
        DoBloodSplat(
            item->pos.x, item->pos.y, item->pos.z, LaraItem->speed,
            LaraItem->pos.y_rot, LaraItem->room_number);
    }
    AnimateItem(item);

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_number != room_num) {
        ItemNewRoom(item_num, room_num);
    }

    item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
    if (item->pos.y >= item->floor) {
        KillItem(item_num);
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &Effects[fx_num];
            fx->pos = item->pos;
            fx->speed = 0;
            fx->counter = 6;
            fx->frame_number = -3 * GetRandomControl() / 0x8000;
            fx->object_number = O_RICOCHET1;
        }
    }
}

void DartEffectControl(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];
    fx->counter++;
    if (fx->counter >= 3) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
            KillEffect(fx_num);
        }
    }
}

void FlameEmitterControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (TriggerActive(item)) {
        if (!item->data) {
            int16_t fx_num = CreateEffect(item->room_number);
            if (fx_num != NO_ITEM) {
                FX_INFO *fx = &Effects[fx_num];
                fx->pos.x = item->pos.x;
                fx->pos.y = item->pos.y;
                fx->pos.z = item->pos.z;
                fx->frame_number = 0;
                fx->object_number = O_FLAME;
                fx->counter = 0;
            }
            item->data = (void *)(fx_num + 1);
        }
    } else if (item->data) {
        StopSoundEffect(150, NULL);
        KillEffect((int16_t)(size_t)item->data - 1);
        item->data = NULL;
    }
}

void FlameControl(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];

    fx->frame_number--;
    if (fx->frame_number <= Objects[O_FLAME].nmeshes) {
        fx->frame_number = 0;
    }

    if (fx->counter < 0) {
#ifdef T1M_FEAT_CHEATS
        if (Lara.water_status == LWS_CHEAT) {
            fx->counter = 0;
            StopSoundEffect(150, NULL);
            KillEffect(fx_num);
        }
#endif

        fx->pos.x = 0;
        fx->pos.y = 0;
        if (fx->counter == -1) {
            fx->pos.z = -100;
        } else {
            fx->pos.z = 0;
        }

        GetJointAbsPosition(LaraItem, (PHD_VECTOR *)&fx->pos, -1 - fx->counter);

#ifdef T1M_FEAT_OG_FIXES
        int32_t y = GetWaterHeight(
            LaraItem->pos.x, LaraItem->pos.y, LaraItem->pos.z,
            LaraItem->room_number);
#else
        int32_t y =
            GetWaterHeight(fx->pos.x, fx->pos.y, fx->pos.z, fx->room_number);
#endif

        if (y != NO_HEIGHT && fx->pos.y > y) {
            fx->counter = 0;
            StopSoundEffect(150, NULL);
            KillEffect(fx_num);
        } else {
            SoundEffect(150, &fx->pos, 0);
            LaraItem->hit_points -= FLAME_ONFIRE_DAMAGE;
            LaraItem->hit_status = 1;
        }
        return;
    }

    SoundEffect(150, &fx->pos, 0);
    if (fx->counter) {
        fx->counter--;
    } else if (ItemNearLara(&fx->pos, 600)) {
#ifdef T1M_FEAT_CHEATS
        if (Lara.water_status == LWS_CHEAT) {
            return;
        }
#endif

        int32_t x = LaraItem->pos.x - fx->pos.x;
        int32_t z = LaraItem->pos.z - fx->pos.z;
        int32_t distance = SQUARE(x) + SQUARE(z);

        LaraItem->hit_points -= FLAME_TOONEAR_DAMAGE;
        LaraItem->hit_status = 1;

        if (distance < SQUARE(300)) {
            fx->counter = 100;

            fx_num = CreateEffect(LaraItem->room_number);
            if (fx_num != NO_ITEM) {
                fx = &Effects[fx_num];
                fx->frame_number = 0;
                fx->object_number = O_FLAME;
                fx->counter = -1;
            }
        }
    }
}

void LavaBurn(ITEM_INFO *item)
{
#ifdef T1M_FEAT_CHEATS
    if (Lara.water_status == LWS_CHEAT) {
        return;
    }
#endif

    if (item->hit_points < 0) {
        return;
    }

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor = GetFloor(item->pos.x, 32000, item->pos.z, &room_num);

    if (item->floor != GetHeight(floor, item->pos.x, 32000, item->pos.z)) {
        return;
    }

    item->hit_points = -1;
    item->hit_status = 1;
    for (int i = 0; i < 10; i++) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &Effects[fx_num];
            fx->object_number = O_FLAME;
            fx->frame_number =
                (Objects[O_FLAME].nmeshes * GetRandomControl()) / 0x7FFF;
            fx->counter = -1 - GetRandomControl() * 24 / 0x7FFF;
        }
    }
}

// original name: LavaSpray
void LavaEmitterControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    int16_t fx_num = CreateEffect(item->room_number);
    if (fx_num != NO_ITEM) {
        FX_INFO *fx = &Effects[fx_num];
        fx->pos.x = item->pos.x;
        fx->pos.y = item->pos.y;
        fx->pos.z = item->pos.z;
        fx->pos.y_rot = (GetRandomControl() - 0x4000) * 2;
        fx->speed = GetRandomControl() >> 10;
        fx->fall_speed = -GetRandomControl() / 200;
        fx->frame_number = -4 * GetRandomControl() / 0x7FFF;
        fx->object_number = O_LAVA;
        SoundEffect(149, &item->pos, 0);
    }
}

// original name: ControlLavaBlob
void LavaControl(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];
    fx->pos.z += (fx->speed * phd_cos(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->pos.x += (fx->speed * phd_sin(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->fall_speed += GRAVITY;
    fx->pos.y += fx->fall_speed;

    int16_t room_num = fx->room_number;
    FLOOR_INFO *floor = GetFloor(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);
    if (fx->pos.y >= GetHeight(floor, fx->pos.x, fx->pos.y, fx->pos.z)
        || fx->pos.y < GetCeiling(floor, fx->pos.x, fx->pos.y, fx->pos.z)) {
        KillEffect(fx_num);
    } else if (ItemNearLara(&fx->pos, 200)) {
        LaraItem->hit_points -= LAVA_GLOB_DAMAGE;
        LaraItem->hit_status = 1;
        KillEffect(fx_num);
    } else if (room_num != fx->room_number) {
        EffectNewRoom(fx_num, room_num);
    }
}

// original name: LavaWedge
void LavaWedgeControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    int16_t room_num = item->room_number;
    GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_number) {
        ItemNewRoom(item_num, room_num);
    }

    if (item->status != IS_DEACTIVATED) {
        int32_t x = item->pos.x;
        int32_t z = item->pos.z;

        switch (item->pos.y_rot) {
        case 0:
            item->pos.z += LAVA_WEDGE_SPEED;
            z += 2 * WALL_L;
            break;
        case -PHD_180:
            item->pos.z -= LAVA_WEDGE_SPEED;
            z -= 2 * WALL_L;
            break;
        case PHD_90:
            item->pos.x += LAVA_WEDGE_SPEED;
            x += 2 * WALL_L;
            break;
        default:
            item->pos.x -= LAVA_WEDGE_SPEED;
            x -= 2 * WALL_L;
            break;
        }

        FLOOR_INFO *floor = GetFloor(x, item->pos.y, z, &room_num);
        if (GetHeight(floor, x, item->pos.y, z) != item->pos.y) {
            item->status = IS_DEACTIVATED;
        }
    }

#ifdef T1M_FEAT_CHEATS
    if (Lara.water_status == LWS_CHEAT) {
        item->touch_bits = 0;
    }
#endif

    if (item->touch_bits) {
        if (LaraItem->hit_points > 0) {
            LavaBurn(LaraItem);
        }

        Camera.item = item;
        Camera.flags = CHASE_OBJECT;
        Camera.type = CAM_FIXED;
        Camera.target_angle = -PHD_180;
        Camera.target_distance = WALL_L * 3;
    }
}

void T1MInjectGameTraps()
{
    INJECT(0x0043A010, InitialiseRollingBall);
    INJECT(0x0043A050, RollingBallControl);
    INJECT(0x0043A2B0, RollingBallCollision);
    INJECT(0x0043A520, SpikeCollision);
    INJECT(0x0043A670, TrapDoorControl);
    INJECT(0x0043A6D0, TrapDoorFloor);
    INJECT(0x0043A720, TrapDoorCeiling);
    INJECT(0x0043A770, OnTrapDoor);
    INJECT(0x0043A820, PendulumControl);
    INJECT(0x0043A970, FallingBlockControl);
    INJECT(0x0043AA70, FallingBlockFloor);
    INJECT(0x0043AAB0, FallingBlockCeiling);
    INJECT(0x0043AAF0, TeethTrapControl);
    INJECT(0x0043ABC0, FallingCeilingControl);
    INJECT(0x0043AC60, InitialiseDamoclesSword);
    INJECT(0x0043ACA0, DamoclesSwordControl);
    INJECT(0x0043ADD0, DamoclesSwordCollision);
    INJECT(0x0043AEC0, DartEmitterControl);
    INJECT(0x0043B060, DartsControl);
    INJECT(0x0043B1A0, DartEffectControl);
    INJECT(0x0043B1F0, FlameEmitterControl);
    INJECT(0x0043B2A0, FlameControl);
    INJECT(0x0043B430, LavaBurn);
    INJECT(0x0043B520, LavaEmitterControl);
    INJECT(0x0043B5F0, LavaControl);
    INJECT(0x0043B710, LavaWedgeControl);
}
