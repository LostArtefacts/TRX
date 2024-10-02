#include "game/effects.h"

#include "game/matrix.h"
#include "game/output.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

static void M_RemoveActive(const int16_t fx_num);
static void M_RemoveDrawn(const int16_t fx_num);

static void M_RemoveActive(const int16_t fx_num)
{
    FX *const fx = &g_Effects[fx_num];
    int16_t link_num = g_NextEffectActive;
    if (link_num == fx_num) {
        g_NextEffectActive = fx->next_active;
        return;
    }

    while (link_num != NO_ITEM) {
        if (g_Effects[link_num].next_active == fx_num) {
            g_Effects[link_num].next_active = fx->next_active;
            return;
        }
        link_num = g_Effects[link_num].next_active;
    }
}

static void M_RemoveDrawn(const int16_t fx_num)
{
    FX *const fx = &g_Effects[fx_num];
    int16_t link_num = g_Rooms[fx->room_num].fx_num;
    if (link_num == fx_num) {
        g_Rooms[fx->room_num].fx_num = fx->next_free;
        return;
    }

    while (link_num != NO_ITEM) {
        if (g_Effects[link_num].next_free == fx_num) {
            g_Effects[link_num].next_free = fx->next_free;
            return;
        }
        link_num = g_Effects[link_num].next_free;
    }
}

void __cdecl Effect_InitialiseArray(void)
{
    g_NextEffectFree = 0;
    g_NextEffectActive = NO_ITEM;

    for (int32_t i = 0; i < MAX_EFFECTS - 1; i++) {
        FX *const fx = &g_Effects[i];
        fx->next_free = i + 1;
    }
    g_Effects[MAX_EFFECTS - 1].next_free = NO_ITEM;
}

int16_t __cdecl Effect_Create(const int16_t room_num)
{
    int16_t fx_num = g_NextEffectFree;
    if (fx_num == NO_ITEM) {
        return NO_ITEM;
    }

    FX *const fx = &g_Effects[fx_num];
    g_NextEffectFree = fx->next_free;

    ROOM *const room = &g_Rooms[room_num];
    fx->room_num = room_num;
    fx->next_free = room->fx_num;
    room->fx_num = fx_num;

    fx->next_active = g_NextEffectActive;
    g_NextEffectActive = fx_num;

    fx->shade = 0x1000;

    return fx_num;
}

void __cdecl Effect_Kill(const int16_t fx_num)
{
    FX *const fx = &g_Effects[fx_num];
    M_RemoveActive(fx_num);
    M_RemoveDrawn(fx_num);

    fx->next_free = g_NextEffectFree;
    g_NextEffectFree = fx_num;
}

void __cdecl Effect_NewRoom(const int16_t fx_num, const int16_t room_num)
{
    FX *const fx = &g_Effects[fx_num];
    ROOM *room = &g_Rooms[fx->room_num];

    int16_t link_num = room->fx_num;
    if (link_num == fx_num) {
        room->fx_num = fx->next_free;
    } else {
        while (link_num != NO_ITEM) {
            if (g_Effects[link_num].next_free == fx_num) {
                g_Effects[link_num].next_free = fx->next_free;
                break;
            }
            link_num = g_Effects[link_num].next_free;
        }
    }

    fx->room_num = room_num;
    room = &g_Rooms[room_num];
    fx->next_free = room->fx_num;
    room->fx_num = fx_num;
}

void __cdecl Effect_Draw(const int16_t fx_num)
{
    const FX *const fx = &g_Effects[fx_num];
    const OBJECT *const object = &g_Objects[fx->object_id];
    if (!object->loaded) {
        return;
    }

    if (fx->object_id == O_GLOW) {
        Output_DrawSprite(
            (fx->rot.y << 16) | (unsigned __int16)fx->rot.x, fx->pos.x,
            fx->pos.y, fx->pos.z, g_Objects[O_GLOW].mesh_idx, fx->shade,
            fx->frame_num);
        return;
    }

    if (object->mesh_count < 0) {
        Output_DrawSprite(
            SPRITE_ABS | (object->semi_transparent ? SPRITE_SEMITRANS : 0)
                | SPRITE_SHADE,
            fx->pos.x, fx->pos.y, fx->pos.z, object->mesh_idx - fx->frame_num,
            fx->shade, 0);
        return;
    }

    Matrix_Push();
    Matrix_TranslateAbs(fx->pos.x, fx->pos.y, fx->pos.z);
    if (g_MatrixPtr->_23 > g_PhdNearZ && g_MatrixPtr->_23 < g_PhdFarZ) {
        Matrix_RotYXZ(fx->rot.y, fx->rot.x, fx->rot.z);
        if (object->mesh_count) {
            S_CalculateStaticLight(fx->shade);
            Output_InsertPolygons(g_Meshes[object->mesh_idx], -1);
        } else {
            S_CalculateStaticLight(fx->shade);
            Output_InsertPolygons(g_Meshes[fx->frame_num], -1);
        }
    }
    Matrix_Pop();
}
