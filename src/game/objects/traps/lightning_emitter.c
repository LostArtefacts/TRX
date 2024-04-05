#include "game/objects/traps/lightning_emitter.h"

#include "game/collide.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/output.h"
#include "game/phase/phase.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/matrix.h"

#include <stdbool.h>

#define LIGHTNING_DAMAGE 400
#define LIGHTNING_STEPS 8
#define LIGHTNING_RND ((64 << W2V_SHIFT) / 0x8000) // = 32
#define LIGHTNING_SHOOTS 2

typedef struct LIGHTNING {
    bool active;
    int32_t count;
    bool zapped;
    bool no_target;
    XYZ_32 target;
    XYZ_32 main[LIGHTNING_STEPS];
    XYZ_32 wibble[LIGHTNING_STEPS];
    int32_t start[LIGHTNING_SHOOTS];
    XYZ_32 end[LIGHTNING_SHOOTS];
    XYZ_32 shoot[LIGHTNING_SHOOTS][LIGHTNING_STEPS];
} LIGHTNING;

void LightningEmitter_Setup(OBJECT_INFO *obj)
{
    obj->initialise = LightningEmitter_Initialise;
    obj->control = LightningEmitter_Control;
    obj->draw_routine = LightningEmitter_Draw;
    obj->collision = LightningEmitter_Collision;
    obj->save_flags = 1;
}

void LightningEmitter_Initialise(int16_t item_num)
{
    LIGHTNING *l = GameBuf_Alloc(sizeof(LIGHTNING), GBUF_TRAP_DATA);
    g_Items[item_num].data = l;

    if (g_Objects[g_Items[item_num].object_number].nmeshes > 1) {
        g_Items[item_num].mesh_bits = 1;
        l->no_target = false;
    } else {
        l->no_target = true;
    }

    l->active = false;
    l->count = 1;
    l->zapped = false;
}

void LightningEmitter_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    LIGHTNING *l = item->data;

    if (!Item_IsTriggerActive(item)) {
        l->count = 1;
        l->active = false;
        l->zapped = false;

        if (g_FlipStatus) {
            Room_FlipMap();
        }

        Item_RemoveActive(item_num);
        item->status = IS_NOT_ACTIVE;
        return;
    }

    l->count--;
    if (l->count > 0) {
        return;
    }

    if (l->active) {
        l->active = false;
        l->count = 35 + (Random_GetControl() * 45) / 0x8000;
        l->zapped = false;
        if (g_FlipStatus) {
            Room_FlipMap();
        }
    } else {
        l->active = true;
        l->count = 20;

        for (int i = 0; i < LIGHTNING_STEPS; i++) {
            l->wibble[i].x = 0;
            l->wibble[i].y = 0;
            l->wibble[i].z = 0;
        }

        int32_t radius = l->no_target ? WALL_L : WALL_L * 5 / 2;
        if (Lara_IsNearItem(&item->pos, radius)) {
            l->target.x = g_LaraItem->pos.x;
            l->target.y = g_LaraItem->pos.y;
            l->target.z = g_LaraItem->pos.z;

            Lara_TakeDamage(LIGHTNING_DAMAGE, true);

            l->zapped = true;
        } else if (l->no_target) {
            FLOOR_INFO *floor = Room_GetFloor(
                item->pos.x, item->pos.y, item->pos.z, &item->room_number);
            int32_t h =
                Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
            l->target.x = item->pos.x;
            l->target.y = h;
            l->target.z = item->pos.z;
            l->zapped = false;
        } else {
            l->target.x = 0;
            l->target.y = 0;
            l->target.z = 0;
            Collide_GetJointAbsPosition(
                item, &l->target, 1 + (Random_GetControl() * 5) / 0x7FFF);
            l->zapped = false;
        }

        for (int i = 0; i < LIGHTNING_SHOOTS; i++) {
            l->start[i] = Random_GetControl() * (LIGHTNING_STEPS - 1) / 0x7FFF;
            l->end[i].x = l->target.x + (Random_GetControl() * WALL_L) / 0x7FFF;
            l->end[i].y = l->target.y;
            l->end[i].z = l->target.z + (Random_GetControl() * WALL_L) / 0x7FFF;

            for (int j = 0; j < LIGHTNING_STEPS; j++) {
                l->shoot[i][j].x = 0;
                l->shoot[i][j].y = 0;
                l->shoot[i][j].z = 0;
            }
        }

        if (!g_FlipStatus) {
            Room_FlipMap();
        }
    }

    Sound_Effect(SFX_THUNDER, &item->pos, SPM_NORMAL);
}

void LightningEmitter_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    LIGHTNING *l = g_Items[item_num].data;
    if (!l->zapped) {
        return;
    }

    g_Lara.hit_direction = 1 + (Random_GetControl() * 4) / (PHD_180 - 1);
    g_Lara.hit_frame++;
    if (g_Lara.hit_frame > 34) {
        g_Lara.hit_frame = 34;
    }
}

void LightningEmitter_Draw(ITEM_INFO *item)
{
    int16_t *frmptr[2];
    int32_t rate;
    Item_GetFrames(item, frmptr, &rate);

    Matrix_Push();
    Matrix_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    Matrix_RotYXZ(item->rot.y, item->rot.x, item->rot.z);

    int32_t clip = Output_GetObjectBounds(frmptr[0]);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Output_CalculateObjectLighting(item, frmptr[0]);

    Matrix_TranslateRel(
        frmptr[0][FRAME_POS_X], frmptr[0][FRAME_POS_Y], frmptr[0][FRAME_POS_Z]);

    int32_t x1 = g_MatrixPtr->_03;
    int32_t y1 = g_MatrixPtr->_13;
    int32_t z1 = g_MatrixPtr->_23;

    Output_DrawPolygons(
        g_Meshes[g_Objects[O_LIGHTNING_EMITTER].mesh_index], clip);

    Matrix_Pop();

    LIGHTNING *l = item->data;
    if (!l->active) {
        return;
    }

    Matrix_Push();

    Matrix_TranslateAbs(l->target.x, l->target.y, l->target.z);
    Matrix_RotYXZ(item->rot.y, item->rot.x, item->rot.z);

    int32_t x2 = g_MatrixPtr->_03;
    int32_t y2 = g_MatrixPtr->_13;
    int32_t z2 = g_MatrixPtr->_23;

    int32_t dx = (x2 - x1) / LIGHTNING_STEPS;
    int32_t dy = (y2 - y1) / LIGHTNING_STEPS;
    int32_t dz = (z2 - z1) / LIGHTNING_STEPS;

    for (int i = 0; i < LIGHTNING_STEPS; i++) {
        XYZ_32 *pos = &l->wibble[i];
        if (Phase_Get() == PHASE_GAME) {
            pos->x += (Random_GetDraw() - PHD_90) * LIGHTNING_RND;
            pos->y += (Random_GetDraw() - PHD_90) * LIGHTNING_RND;
            pos->z += (Random_GetDraw() - PHD_90) * LIGHTNING_RND;
        }
        if (i == LIGHTNING_STEPS - 1) {
            pos->y = 0;
        }

        x2 = x1 + dx + pos->x;
        y2 = y1 + dy + pos->y;
        z2 = z1 + dz + pos->z;

        if (i > 0) {
            Output_DrawLightningSegment(
                x1, y1 + l->wibble[i - 1].y, z1, x2, y2, z2,
                Viewport_GetWidth() / 6);
        } else {
            Output_DrawLightningSegment(
                x1, y1, z1, x2, y2, z2, Viewport_GetWidth() / 6);
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

        Matrix_Pop();
        Matrix_Push();

        Matrix_TranslateAbs(l->end[i].x, l->end[i].y, l->end[i].z);
        Matrix_RotYXZ(item->rot.y, item->rot.x, item->rot.z);

        x2 = g_MatrixPtr->_03;
        y2 = g_MatrixPtr->_13;
        z2 = g_MatrixPtr->_23;

        int32_t steps = LIGHTNING_STEPS - j;
        dx = (x2 - x1) / steps;
        dy = (y2 - y1) / steps;
        dz = (z2 - z1) / steps;

        for (int k = 0; k < steps; k++) {
            XYZ_32 *pos = &l->shoot[i][k];
            if (Phase_Get() == PHASE_GAME) {
                pos->x += (Random_GetDraw() - PHD_90) * LIGHTNING_RND;
                pos->y += (Random_GetDraw() - PHD_90) * LIGHTNING_RND;
                pos->z += (Random_GetDraw() - PHD_90) * LIGHTNING_RND;
            }
            if (k == steps - 1) {
                pos->y = 0;
            }

            x2 = x1 + dx + pos->x;
            y2 = y1 + dy + pos->y;
            z2 = z1 + dz + pos->z;

            if (k > 0) {
                Output_DrawLightningSegment(
                    x1, y1 + l->shoot[i][k - 1].y, z1, x2, y2, z2,
                    Viewport_GetWidth() / 16);
            } else {
                Output_DrawLightningSegment(
                    x1, y1, z1, x2, y2, z2, Viewport_GetWidth() / 16);
            }

            x1 = x2;
            y1 += dy;
            z1 = z2;
        }
    }

    Matrix_Pop();
}
