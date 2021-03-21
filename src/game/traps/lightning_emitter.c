#include "game/traps/lightning_emitter.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/items.h"
#include "game/sound.h"
#include "game/sphere.h"
#include "global/vars.h"
#include "specific/init.h"
#include "specific/output.h"

void SetupLightningEmitter(OBJECT_INFO *obj)
{
    obj->initialise = InitialiseLightning;
    obj->control = LightningControl;
    obj->draw_routine = DrawLightning;
    obj->collision = LightningCollision;
    obj->save_flags = 1;
}

void InitialiseLightning(int16_t item_num)
{
    LIGHTNING *l = game_malloc(sizeof(LIGHTNING), 0);
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
    ITEM_INFO *item = &Items[item_num];
    LIGHTNING *l = item->data;

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
            FLOOR_INFO *floor = GetFloor(
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

    SoundEffect(SFX_THUNDER, &item->pos, SPM_NORMAL);
}

void LightningCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    LIGHTNING *l = Items[item_num].data;
    if (!l->zapped) {
        return;
    }

    Lara.hit_direction = 1 + (GetRandomControl() * 4) / (PHD_180 - 1);
    Lara.hit_frame++;
    if (Lara.hit_frame > 34) {
        Lara.hit_frame = 34;
    }
}

void DrawLightning(ITEM_INFO *item)
{
    int16_t *frmptr[2];
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

    LIGHTNING *l = item->data;
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
        PHD_VECTOR *pos = &l->wibble[i];
        pos->x += (GetRandomDraw() - PHD_90) * LIGHTNING_RND;
        pos->y += (GetRandomDraw() - PHD_90) * LIGHTNING_RND;
        pos->z += (GetRandomDraw() - PHD_90) * LIGHTNING_RND;
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
            PHD_VECTOR *pos = l->shoot[i];
            pos->x += (GetRandomDraw() - PHD_90) * LIGHTNING_RND;
            pos->y += (GetRandomDraw() - PHD_90) * LIGHTNING_RND;
            pos->z += (GetRandomDraw() - PHD_90) * LIGHTNING_RND;
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
