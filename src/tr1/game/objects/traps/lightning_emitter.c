#include "game/game.h"
#include "game/lara.h"

#include <libtrx/game/collision.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/output.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/viewport.h>

#define LIGHTNING_DAMAGE 400
#define LIGHTNING_STEPS 8
#define LIGHTNING_RND 64
#define LIGHTNING_SHOOTS 2

typedef struct {
    bool active;
    int32_t count;
    bool zapped;
    bool no_target;
    XYZ_32 target;
    int32_t start[LIGHTNING_SHOOTS];
    XYZ_32 end[LIGHTNING_SHOOTS];
    XYZ_32 main[LIGHTNING_STEPS];
    XYZ_32 wibble[LIGHTNING_STEPS];
    XYZ_32 shoot[LIGHTNING_SHOOTS][LIGHTNING_STEPS];
} M_LIGHTNING;

static void M_Initialise(const int16_t item_num)
{
    M_LIGHTNING *l = GameBuf_Alloc(sizeof(M_LIGHTNING), GBUF_ITEM_DATA);
    ITEM *const item = Item_Get(item_num);
    item->data = l;

    if (Object_Get(item->object_id)->mesh_count > 1) {
        item->mesh_bits = 1;
        l->no_target = false;
    } else {
        l->no_target = true;
    }

    l->active = false;
    l->count = 1;
    l->zapped = false;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_LIGHTNING *l = item->data;

    if (!Item_IsTriggerActive(item)) {
        l->count = 1;
        l->active = false;
        l->zapped = false;

        if (Room_GetFlipStatus()) {
            Room_FlipMap();
        }

        Item_RemoveActive(item_num);
        item->status = IS_INACTIVE;
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
        if (Room_GetFlipStatus()) {
            Room_FlipMap();
        }
    } else {
        l->active = true;
        l->count = 20;

        for (int32_t i = 0; i < LIGHTNING_STEPS; i++) {
            l->wibble[i].x = 0;
            l->wibble[i].y = 0;
            l->wibble[i].z = 0;
        }

        const int32_t radius = l->no_target ? WALL_L : WALL_L * 5 / 2;
        if (Lara_IsNearItem(&item->pos, radius)) {
            const ITEM *const lara_item = Lara_GetItem();
            l->target.x = lara_item->pos.x;
            l->target.y = lara_item->pos.y;
            l->target.z = lara_item->pos.z;

            Lara_TakeDamage(LIGHTNING_DAMAGE, true);

            l->zapped = true;
        } else if (l->no_target) {
            const SECTOR *const sector = Room_GetSector(
                item->pos.x, item->pos.y, item->pos.z, &item->room_num);
            const int32_t h =
                Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
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

        for (int32_t i = 0; i < LIGHTNING_SHOOTS; i++) {
            l->start[i] = Random_GetControl() * (LIGHTNING_STEPS - 1) / 0x7FFF;
            l->end[i].x = l->target.x + (Random_GetControl() * WALL_L) / 0x7FFF;
            l->end[i].y = l->target.y;
            l->end[i].z = l->target.z + (Random_GetControl() * WALL_L) / 0x7FFF;

            for (int32_t j = 0; j < LIGHTNING_STEPS; j++) {
                l->shoot[i][j].x = 0;
                l->shoot[i][j].y = 0;
                l->shoot[i][j].z = 0;
            }
        }

        if (!Room_GetFlipStatus()) {
            Room_FlipMap();
        }
    }

    Sound_Effect(SFX_THUNDER, &item->pos, SPM_NORMAL);
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const M_LIGHTNING *const l = Item_Get(item_num)->data;
    if (!l->zapped) {
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->hit_direction = 1 + (Random_GetControl() * 4) / (DEG_180 - 1);
    lara->hit_frame++;
    CLAMPG(lara->hit_frame, 34);
}

static void M_DrawBolts(const ITEM *const item)
{
    const OBJECT *const obj = Object_Get(O_LIGHTNING_EMITTER);

    ANIM_FRAME *frmptr[2];
    int32_t rate;
    Item_GetFrames(item, frmptr, &rate);

    M_LIGHTNING *l = item->data;
    if (!l->active) {
        return;
    }

    int32_t x1 = item->interp.result.pos.x + frmptr[0]->offset.x;
    int32_t y1 = item->interp.result.pos.y + frmptr[0]->offset.y;
    int32_t z1 = item->interp.result.pos.z + frmptr[0]->offset.z;

    int32_t x2 = l->target.x;
    int32_t y2 = l->target.y;
    int32_t z2 = l->target.z;

    int32_t dx = (x2 - x1) / LIGHTNING_STEPS;
    int32_t dy = (y2 - y1) / LIGHTNING_STEPS;
    int32_t dz = (z2 - z1) / LIGHTNING_STEPS;

    for (int32_t i = 0; i < LIGHTNING_STEPS; i++) {
        XYZ_32 *pos = &l->wibble[i];
        if (Game_IsPlaying()) {
            pos->x += (Random_GetDraw() - 0x4000) * LIGHTNING_RND / 0x8000;
            pos->y += (Random_GetDraw() - 0x4000) * LIGHTNING_RND / 0x8000;
            pos->z += (Random_GetDraw() - 0x4000) * LIGHTNING_RND / 0x8000;
        }
        if (i == LIGHTNING_STEPS - 1) {
            pos->y = 0;
        }

        x2 = x1 + dx + pos->x;
        y2 = y1 + dy + pos->y;
        z2 = z1 + dz + pos->z;

        if (i > 0) {
            Output_DrawLightningSegment((LIGHTNING_SEGMENT) {
                .from = { x1, y1 + l->wibble[i - 1].y, z1 },
                .to = { x2, y2, z2 },
                .thickness = Viewport_GetWidth(VIEWPORT_GAME) / 6 });
        } else {
            Output_DrawLightningSegment((LIGHTNING_SEGMENT) {
                .from = { x1, y1, z1 },
                .to = { x2, y2, z2 },
                .thickness = Viewport_GetWidth(VIEWPORT_GAME) / 6 });
        }

        x1 = x2;
        y1 += dy;
        z1 = z2;

        l->main[i].x = x2;
        l->main[i].y = y2;
        l->main[i].z = z2;
    }

    for (int32_t i = 0; i < LIGHTNING_SHOOTS; i++) {
        int32_t j = l->start[i];
        x1 = l->main[j].x;
        y1 = l->main[j].y;
        z1 = l->main[j].z;

        x2 = l->end[i].x;
        y2 = l->end[i].y;
        z2 = l->end[i].z;

        int32_t steps = LIGHTNING_STEPS - j;
        dx = (x2 - x1) / steps;
        dy = (y2 - y1) / steps;
        dz = (z2 - z1) / steps;

        for (int32_t k = 0; k < steps; k++) {
            XYZ_32 *pos = &l->shoot[i][k];
            if (Game_IsPlaying()) {
                pos->x += (Random_GetDraw() - 0x4000) * LIGHTNING_RND / 0x8000;
                pos->y += (Random_GetDraw() - 0x4000) * LIGHTNING_RND / 0x8000;
                pos->z += (Random_GetDraw() - 0x4000) * LIGHTNING_RND / 0x8000;
            }
            if (k == steps - 1) {
                pos->y = 0;
            }

            x2 = x1 + dx + pos->x;
            y2 = y1 + dy + pos->y;
            z2 = z1 + dz + pos->z;

            if (k > 0) {
                Output_DrawLightningSegment((LIGHTNING_SEGMENT) {
                    .from = { x1, y1 + l->shoot[i][k - 1].y, z1 },
                    .to = { x2, y2, z2 },
                    .thickness = Viewport_GetWidth(VIEWPORT_GAME) / 16 });
            } else {
                Output_DrawLightningSegment((LIGHTNING_SEGMENT) {
                    .from = { x1, y1, z1 },
                    .to = { x2, y2, z2 },
                    .thickness = Viewport_GetWidth(VIEWPORT_GAME) / 16 });
            }

            x1 = x2;
            y1 += dy;
            z1 = z2;
        }
    }
}

static void M_Draw(const ITEM *const item)
{
    const OBJECT *const obj = Object_Get(O_LIGHTNING_EMITTER);
    ANIM_FRAME *frmptr[2];
    int32_t rate;
    Item_GetFrames(item, frmptr, &rate);

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);
    const CLIP clip = Output_CheckBoundsClip(&frmptr[0]->bounds);
    if (clip == CLIP_NOT_VISIBLE) {
        Matrix_Pop();
        return;
    }

    Output_CalculateObjectLighting(item, &frmptr[0]->bounds);

    Matrix_TranslateRel16(frmptr[0]->offset);
    Object_DrawMesh(obj->mesh_idx, clip, false);
    Matrix_Pop();

    M_DrawBolts(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = M_Draw;
    obj->collision_func = M_Collision;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_LIGHTNING_EMITTER, M_Setup)
