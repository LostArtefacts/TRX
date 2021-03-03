#include "3dsystem/3d_gen.h"
#include "game/collide.h"
#include "game/const.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lightning.h"
#include "game/moveblock.h"
#include "game/sphere.h"
#include "game/vars.h"
#include "specific/init.h"
#include "specific/output.h"
#include "util.h"

#define LIGHTNING_DAMAGE 400
#define LIGHTNING_STEPS 8
#define LIGHTNING_RND ((64 << W2V_SHIFT) / 0x8000) // = 32
#define LIGHTNING_SHOOTS 2

typedef struct {
    int32_t onstate;
    int32_t count;
    int32_t zapped;
    int32_t notarget;
    PHD_VECTOR target;
    PHD_VECTOR main[LIGHTNING_STEPS], wibble[LIGHTNING_STEPS];
    int start[LIGHTNING_SHOOTS];
    PHD_VECTOR end[LIGHTNING_SHOOTS];
    PHD_VECTOR shoot[LIGHTNING_SHOOTS][LIGHTNING_STEPS];
} LIGHTNING;

typedef enum {
    THS_SET = 0,
    THS_TEASE = 1,
    THS_ACTIVE = 2,
    THS_DONE = 3,
} THOR_HAMMER_STATES;

void DrawLightning(ITEM_INFO* item)
{
    int16_t* frmptr[2];
    int32_t rate;
    GetFrames(item, frmptr, &rate);

    phd_PushMatrix();
    phd_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int32_t clip = S_GetObjectBounds(frmptr[0]);
    if (!clip) {
        phd_PopMatrix();
        return;
    }

    CalculateObjectLighting(item, frmptr[0]);

    phd_TranslateRel(
        frmptr[0][FRAME_POS_X], frmptr[0][FRAME_POS_Y], frmptr[0][FRAME_POS_Z]);

    int32_t x1 = PhdMatrixPtr->_03;
    int32_t y1 = PhdMatrixPtr->_13;
    int32_t z1 = PhdMatrixPtr->_23;

    phd_PutPolygons(Meshes[Objects[O_LIGHTNING_EMITTER].mesh_index], clip);

    phd_PopMatrix();

    LIGHTNING* l = item->data;
    if (!l->onstate) {
        return;
    }

    phd_PushMatrix();

    phd_TranslateAbs(l->target.x, l->target.y, l->target.z);
    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int32_t x2 = PhdMatrixPtr->_03;
    int32_t y2 = PhdMatrixPtr->_13;
    int32_t z2 = PhdMatrixPtr->_23;

    int32_t dx = (x2 - x1) / LIGHTNING_STEPS;
    int32_t dy = (y2 - y1) / LIGHTNING_STEPS;
    int32_t dz = (z2 - z1) / LIGHTNING_STEPS;

    for (int i = 0; i < LIGHTNING_STEPS; i++) {
        PHD_VECTOR* pos = &l->wibble[i];
        pos->x += (GetRandomDraw() - 0x4000) * LIGHTNING_RND;
        pos->y += (GetRandomDraw() - 0x4000) * LIGHTNING_RND;
        pos->z += (GetRandomDraw() - 0x4000) * LIGHTNING_RND;
        if (i == LIGHTNING_STEPS - 1) {
            pos->y = 0;
        }

        x2 = x1 + dx + pos->x;
        y2 = y1 + dy + pos->y;
        z2 = z1 + dz + pos->z;

        if (i > 0) {
            S_DrawLightningSegment(
                x1, y1 + l->wibble[i - 1].y, z1, x2, y2, z2, PhdWinWidth / 6);
        } else {
            S_DrawLightningSegment(x1, y1, z1, x2, y2, z2, PhdWinWidth / 6);
        }

        x1 = x2;
        y1 += dy;
        z1 = z2;

        l->main[i].x = x2;
        l->main[i].y = y2;
        l->main[i].z = z2;
    }

    for (int i = 0; i < LIGHTNING_SHOOTS; i++) {
        int j = l->start[i];
        x1 = l->main[j].x;
        y1 = l->main[j].y;
        z1 = l->main[j].z;

        phd_PopMatrix();
        phd_PushMatrix();

        phd_TranslateAbs(l->end[i].x, l->end[i].y, l->end[i].z);
        phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

        x2 = PhdMatrixPtr->_03;
        y2 = PhdMatrixPtr->_13;
        z2 = PhdMatrixPtr->_23;

        int32_t steps = LIGHTNING_STEPS - j;
        dx = (x2 - x1) / steps;
        dy = (y2 - y1) / steps;
        dz = (z2 - z1) / steps;

        for (j = 0; j < steps; j++) {
            PHD_VECTOR* pos = l->shoot[j];
            pos->x += (GetRandomDraw() - 0x4000) * LIGHTNING_RND;
            pos->y += (GetRandomDraw() - 0x4000) * LIGHTNING_RND;
            pos->z += (GetRandomDraw() - 0x4000) * LIGHTNING_RND;
            if (j == steps - 1) {
                pos->y = 0;
            }

            x2 = x1 + dx + pos->x;
            y2 = y1 + dy + pos->y;
            z2 = z1 + dz + pos->z;

            if (j > 0) {
                S_DrawLightningSegment(
                    x1, y1 + l->shoot[i][j - 1].y, z1, x2, y2, z2,
                    PhdWinWidth / 16);
            } else {
                S_DrawLightningSegment(
                    x1, y1, z1, x2, y2, z2, PhdWinWidth / 16);
            }

            x1 = x2;
            y1 += dy;
            z1 = z2;
        }
    }

    phd_PopMatrix();
}

void InitialiseLightning(int16_t item_num)
{
    LIGHTNING* l = game_malloc(sizeof(LIGHTNING), 0);
    Items[item_num].data = l;

    if (Objects[Items[item_num].object_number].nmeshes > 1) {
        Items[item_num].mesh_bits = 1;
        l->notarget = 0;
    } else {
        l->notarget = 1;
    }

    l->onstate = 0;
    l->count = 1;
    l->zapped = 0;
}

void LightningControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    LIGHTNING* l = item->data;

    if (!TriggerActive(item)) {
        l->count = 1;
        l->onstate = 0;
        l->zapped = 0;

        if (FlipStatus) {
            FlipMap();
        }

        RemoveActiveItem(item_num);
        item->status = IS_NOT_ACTIVE;
        return;
    }

    l->count--;
    if (l->count > 0) {
        return;
    }

    if (l->onstate) {
        l->onstate = 0;
        l->count = 35 + (GetRandomControl() * 45) / 0x8000;
        l->zapped = 0;
        if (FlipStatus) {
            FlipMap();
        }
    } else {
        l->onstate = 1;
        l->count = 20;

        for (int i = 0; i < LIGHTNING_STEPS; i++) {
            l->wibble[i].x = 0;
            l->wibble[i].y = 0;
            l->wibble[i].z = 0;
        }

        int32_t radius = l->notarget ? WALL_L : WALL_L * 5 / 2;
        if (ItemNearLara(&item->pos, radius)) {
            l->target.x = LaraItem->pos.x;
            l->target.y = LaraItem->pos.y;
            l->target.z = LaraItem->pos.z;

            LaraItem->hit_points -= LIGHTNING_DAMAGE;
            LaraItem->hit_status = 1;

            l->zapped = 1;
        } else if (l->notarget) {
            FLOOR_INFO* floor = GetFloor(
                item->pos.x, item->pos.y, item->pos.z, &item->room_number);
            int32_t h = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
            l->target.x = item->pos.x;
            l->target.y = h;
            l->target.z = item->pos.z;
            l->zapped = 0;
        } else {
            l->target.x = 0;
            l->target.y = 0;
            l->target.z = 0;
            GetJointAbsPosition(
                item, &l->target, 1 + (GetRandomControl() * 5) / 0x7FFF);
            l->zapped = 0;
        }

        for (int i = 0; i < LIGHTNING_SHOOTS; i++) {
            l->start[i] = GetRandomControl() * (LIGHTNING_STEPS - 1) / 0x7FFF;
            l->end[i].x = l->target.x + (GetRandomControl() * WALL_L) / 0x7FFF;
            l->end[i].y = l->target.y;
            l->end[i].z = l->target.z + (GetRandomControl() * WALL_L) / 0x7FFF;

            for (int j = 0; j < LIGHTNING_STEPS; j++) {
                l->shoot[i][j].x = 0;
                l->shoot[i][j].y = 0;
                l->shoot[i][j].z = 0;
            }
        }

        if (!FlipStatus) {
            FlipMap();
        }
    }

    SoundEffect(98, &item->pos, 0);
}

void LightningCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll)
{
    LIGHTNING* l = Items[item_num].data;
    if (!l->zapped) {
        return;
    }

    Lara.hit_direction = 1 + (GetRandomControl() * 4) / 0x7FFF;
    Lara.hit_frame++;
    if (Lara.hit_frame > 34) {
        Lara.hit_frame = 34;
    }
}

void InitialiseThorsHandle(int16_t item_num)
{
    ITEM_INFO* hand_item = &Items[item_num];
    int16_t head_item_num = CreateItem();
    ITEM_INFO* head_item = &Items[head_item_num];
    head_item->object_number = O_THORS_HEAD;
    head_item->room_number = hand_item->room_number;
    head_item->pos = hand_item->pos;
    head_item->shade = hand_item->shade;
    InitialiseItem(head_item_num);
    hand_item->data = head_item;
    LevelItemCount++;
}

void ThorsHandleControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    switch (item->current_anim_state) {
    case THS_SET:
        if (TriggerActive(item)) {
            item->goal_anim_state = THS_TEASE;
        } else {
            RemoveActiveItem(item_num);
            item->status = IS_NOT_ACTIVE;
        }
        break;

    case THS_TEASE:
        if (TriggerActive(item)) {
            item->goal_anim_state = THS_ACTIVE;
        } else {
            item->goal_anim_state = THS_SET;
        }
        break;

    case THS_ACTIVE: {
        int32_t frm = item->frame_number - Anims[item->anim_number].frame_base;
        if (frm > 30) {
            int32_t x = item->pos.x;
            int32_t z = item->pos.z;

            switch (item->pos.y_rot) {
            case 0:
                z += WALL_L * 3;
                break;
            case 0x4000:
                x += WALL_L * 3;
                break;
            case -0x4000:
                x -= WALL_L * 3;
                break;
            case -0x8000:
                z -= WALL_L * 3;
                break;
            }

            if (LaraItem->hit_points >= 0 && LaraItem->pos.x > x - 520
                && LaraItem->pos.x < x + 520 && LaraItem->pos.z > z - 520
                && LaraItem->pos.z < z + 520) {
                LaraItem->hit_points = -1;
                LaraItem->pos.y = item->pos.y;
                LaraItem->gravity_status = 0;
                LaraItem->current_anim_state = AS_SPECIAL;
                LaraItem->goal_anim_state = AS_SPECIAL;
                LaraItem->anim_number = AA_RBALL_DEATH;
                LaraItem->frame_number = AF_RBALL_DEATH;
            }
        }
        break;
    }

    case THS_DONE: {
        int32_t x = item->pos.x;
        int32_t z = item->pos.z;
        int32_t old_x = x;
        int32_t old_z = z;

        int16_t room_num = item->room_number;
        FLOOR_INFO* floor = GetFloor(x, item->pos.y, z, &room_num);
        GetHeight(floor, x, item->pos.y, z);
        TestTriggers(TriggerIndex, 1);

        switch (item->pos.y_rot) {
        case 0:
            z += WALL_L * 3;
            break;
        case 0x4000:
            x += WALL_L * 3;
            break;
        case -0x4000:
            x -= WALL_L * 3;
            break;
        case -0x8000:
            z -= WALL_L * 3;
            break;
        }

        item->pos.x = x;
        item->pos.z = z;
        if (LaraItem->hit_points >= 0) {
            AlterFloorHeight(item, -2 * WALL_L);
        }
        item->pos.x = old_x;
        item->pos.z = old_z;

        RemoveActiveItem(item_num);
        item->status = IS_DEACTIVATED;
        break;
    }
    }
    AnimateItem(item);

    ITEM_INFO* head_item = item->data;
    int32_t anim = item->anim_number - Objects[O_THORS_HANDLE].anim_index;
    int32_t frm = item->frame_number - Anims[item->anim_number].frame_base;
    head_item->anim_number = Objects[O_THORS_HEAD].anim_index + anim;
    head_item->frame_number = Anims[head_item->anim_number].frame_base + frm;
    head_item->current_anim_state = item->current_anim_state;
}

void ThorsHandleCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll)
{
    ITEM_INFO* item = &Items[item_num];
    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }

    if (coll->enable_baddie_push) {
        ItemPushLara(item, lara_item, coll, 0, 1);
    }
}

void T1MInjectGameLightning()
{
    INJECT(0x00429620, DrawLightning);
    INJECT(0x00429B00, InitialiseLightning);
    INJECT(0x00429B80, LightningControl);
    INJECT(0x00429E30, LightningCollision);

    INJECT(0x00429EA0, InitialiseThorsHandle);
    INJECT(0x00429F30, ThorsHandleControl);
    INJECT(0x0042A1F0, ThorsHandleCollision);
}
