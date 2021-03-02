#include "3dsystem/3d_gen.h"
#include "game/const.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/lightning.h"
#include "game/vars.h"
#include "specific/output.h"
#include "util.h"

#define LIGHTNING_DAMAGE 400
#define LIGHTNING_STEPS 8
#define LIGHTNING_RND ((64 << W2V_SHIFT) / 32768) // = 32
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

void T1MInjectGameLightning()
{
    INJECT(0x00429620, DrawLightning);
}
