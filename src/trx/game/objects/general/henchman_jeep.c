#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_ENEMY_EXIT_DIST (STEP_L * 10 - 4) // = 2556
#define M_ACCELERATION    (STEP_L / 8) // = 32
#define M_DECELERATION    (M_ACCELERATION / 10) // = 3
#define M_MAX_SPEED       (WALL_L * 16) // = 16384
#define M_MIN_EXIT_SPEED  WALL_L
#define M_INITIAL_SHIFT   (-80)
#define M_MAX_SHIFT       400
// clang-format on

typedef struct {
    int16_t enemy_item_num;
    int16_t velocity;
    int16_t x_shift;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ(io, "velocity", &p->velocity));
    MUST(JSON_READ(io, "x_shift", &p->x_shift));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "velocity", p->velocity);
    JSONW_WRITE(io, "x_shift", p->x_shift);
}

static int16_t M_FindEnemy(const ITEM *const item)
{
    for (int16_t i = 0; i < Item_GetLevelCount(); i++) {
        const ITEM *const enemy = Item_Get(i);
        if (Object_Get(enemy->object_id)->intelligent
            && enemy->pos.y == item->pos.y
            && ROUND_TO_SECTOR(enemy->pos.x) == ROUND_TO_SECTOR(item->pos.x)
            && ROUND_TO_SECTOR(enemy->pos.z) == ROUND_TO_SECTOR(item->pos.z)) {
            return i;
        }
    }

    return NO_ITEM;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->x_shift = M_INITIAL_SHIFT;
    p->enemy_item_num = M_FindEnemy(item);
    if (p->enemy_item_num != NO_ITEM) {
        Item_Get(p->enemy_item_num)->pos.y -= WALL_L;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (p->enemy_item_num == NO_ITEM) {
        return;
    }

    if (p->x_shift == M_INITIAL_SHIFT) {
        if (p->velocity < M_MAX_SPEED) {
            p->velocity += M_ACCELERATION;
        }
    } else if (p->velocity > M_MIN_EXIT_SPEED) {
        p->velocity -= M_MIN_EXIT_SPEED / 2;
    }

    const int32_t pitch = p->velocity << 9;
    Sound_Effect(SFX_JEEP_MOVE, &item->pos, pitch + (0x1000000 | SPM_PITCH));

    item->pos.x += p->x_shift;
    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    Item_UpdateRoom(item_num, room_num);

    const ITEM *const enemy = Item_Get(p->enemy_item_num);
    const int32_t dist = XYZ_32_GetDistance(item->pos, enemy->pos);
    if (enemy->hit_points <= 0 || dist >= M_ENEMY_EXIT_DIST) {
        p->x_shift += M_DECELERATION;
    }

    if (p->x_shift > M_MAX_SHIFT) {
        Item_Destroy(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = Object_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_position = true;
}

REGISTER_OBJECT(O_HENCHMAN_JEEP, M_Setup)
