#include "3dsystem/3d_gen.h"
#include "game/const.h"
#include "game/data.h"
#include "game/draw.h"
#include "specific/output.h"
#include "util.h"

void __cdecl PrintRooms(int16_t room_number)
{
    ROOM_INFO* r = &RoomInfo[room_number];
    if (r->flags & UNDERWATER) {
        S_SetupBelowWater(CameraUnderwater);
    } else {
        S_SetupAboveWater(CameraUnderwater);
    }

    r->bound_active = 0;

    phd_PushMatrix();
    phd_TranslateAbs(r->x, r->y, r->z);

    PhdLeft = r->bound_left;
    PhdRight = r->bound_right;
    PhdTop = r->bound_top;
    PhdBottom = r->bound_bottom;

    S_InsertRoom(r->data);

    for (int i = r->item_number; i != NO_ITEM; i = Items[i].next_item) {
        ITEM_INFO* item = &Items[i];
        if (item->status != INVISIBLE) {
            Objects[item->object_number].draw_routine(item);
        }
    }

    for (int i = 0; i < r->num_meshes; i++) {
        MESH_INFO* mesh = &r->mesh[i];
        if (StaticObjects[mesh->static_number].flags & 2) {
            phd_PushMatrix();
            phd_TranslateAbs(mesh->x, mesh->y, mesh->z);
            phd_RotY(mesh->y_rot);
            int clip =
                S_GetObjectBounds(&StaticObjects[mesh->static_number].x_minp);
            if (clip) {
                S_CalculateStaticLight(mesh->shade);
                phd_PutPolygons(
                    Meshes[StaticObjects[mesh->static_number].mesh_number],
                    clip);
            }
            phd_PopMatrix();
        }
    }

    for (int i = r->fx_number; i != NO_ITEM; i = Effects[i].next_fx) {
        DrawEffect(i);
    }

    phd_PopMatrix();

    r->bound_left = PhdWinMaxX;
    r->bound_bottom = 0;
    r->bound_right = 0;
    r->bound_top = PhdWinMaxY;
}

void TR1MInjectGameDraw()
{
    INJECT(0x004171E0, PrintRooms);
}
