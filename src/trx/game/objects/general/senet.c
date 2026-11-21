#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/fx/shockwave.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_SIDE_PIECES       3
#define M_PIECES            (M_SIDE_PIECES * 2)
#define M_SQUARES           17
#define M_EXIT              16
#define M_SHARED_START      5
#define M_OFF_BOARD         (-1)
#define M_STICKS            4
#define M_SPIN_TIME         120
#define M_SPIN_SOUND_TIME   100
#define M_SPIN_SETTLE_TIME  40
#define M_ALL_LIGHT         6
#define M_PIECE_SPEED       128
#define M_MOVE_FADE         48
#define M_FADE_END          128
#define M_SHOCKWAVE_RISE    384
#define M_SHOCKWAVE_LIFE    24
#define M_SIDE_LARA         1
#define M_SIDE_ENEMY        2
#define M_LARA_COLOR        ((RGB_888) { .r = 0xE0, .g = 0x60, .b = 0x60 })
#define M_ENEMY_COLOR       ((RGB_888) { .r = 0x20, .g = 0x80, .b = 0xFF })
// clang-format on

typedef struct {
    int16_t item_nums[M_PIECES];
    int8_t piece_squares[M_PIECES];
    int8_t board[M_SQUARES];
    int8_t last_throw;
    int8_t piece_moving;
    bool pieces_locked;
    XZ_32 target;
} M_GAME;

typedef struct {
    int16_t fade;
} M_PIECE_PRIV;

typedef struct {
    int16_t fade;
    int16_t stick_rots[M_STICKS];
    int16_t spin_timer;
    int16_t stick_bits;
} M_SPINNER_PRIV;

static const OBJECT_BOUNDS m_SpinnerBounds = {
    .shift = {
        .min = { .x = -STEP_L, .y = -200, .z = -STEP_L, },
        .max = { .x = +STEP_L, .y = +200, .z = +STEP_L, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = 0, },
    },
};

static const XYZ_32 m_SpinnerPosition = { .x = 0, .y = 0, .z = -100 };

static M_GAME m_Game = {};

static void M_ResetGame(void)
{
    m_Game = (M_GAME) {
        .item_nums = { NO_ITEM, NO_ITEM, NO_ITEM, NO_ITEM, NO_ITEM, NO_ITEM },
        .piece_moving = -1,
    };
    m_Game.board[0] = M_SIDE_LARA | M_SIDE_ENEMY;
}

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static int32_t M_GetSide(const int32_t piece)
{
    return piece >= M_SIDE_PIECES ? M_SIDE_ENEMY : M_SIDE_LARA;
}

static void M_TriggerInRoom(const int16_t room_num, const OBJECT_ID obj_id)
{
    int16_t item_num = Room_Get(room_num)->item_num;
    while (item_num != NO_ITEM) {
        ITEM *const item = Item_Get(item_num);
        const int16_t next_num = item->next_item;
        if (item->object_id == obj_id) {
            item->trigger.mask = TRIGGER_MASK_ALL;
            Item_Activate(item_num, false);
        }
        item_num = next_num;
    }
}

static bool M_CheckWinner(const int32_t side)
{
    const int32_t first = side == M_SIDE_LARA ? 0 : M_SIDE_PIECES;
    for (int32_t i = first; i < first + M_SIDE_PIECES; i++) {
        if (m_Game.piece_squares[i] != M_OFF_BOARD) {
            return false;
        }
    }

    // These rooms belong to Tomb of Semerkhet, the only level with the game.
    if (side == M_SIDE_LARA) {
        M_TriggerInRoom(0, O_RAISING_BLOCK_2);
        M_TriggerInRoom(19, O_RAISING_BLOCK_2);
    } else {
        M_TriggerInRoom(20, O_TRAPDOOR_1);
        M_TriggerInRoom(21, O_TRAPDOOR_1);
        M_TriggerInRoom(22, O_TRAPDOOR_1);
        M_TriggerInRoom(81, O_TRAPDOOR_1);
    }
    return true;
}

static void M_MakeMove(const int32_t piece, const int32_t distance)
{
    const int32_t side = M_GetSide(piece);
    const int32_t square = m_Game.piece_squares[piece] + distance;
    if (m_Game.piece_squares[piece] == M_OFF_BOARD || distance == 0
        || square > M_EXIT || (m_Game.board[square] & side) != 0) {
        return;
    }

    const int32_t old_square = m_Game.piece_squares[piece];
    m_Game.board[old_square] &= ~side;
    if (old_square == 0) {
        const int32_t first = side == M_SIDE_LARA ? 0 : M_SIDE_PIECES;
        for (int32_t i = first; i < first + M_SIDE_PIECES; i++) {
            if (i != piece && m_Game.piece_squares[i] == 0) {
                m_Game.board[0] |= side;
            }
        }
    }

    m_Game.piece_moving = piece;
    m_Game.piece_squares[piece] = square;
    if (square >= M_SHARED_START) {
        m_Game.board[square] = 0;
        const int32_t first = side == M_SIDE_LARA ? M_SIDE_PIECES : 0;
        for (int32_t i = first; i < first + M_SIDE_PIECES; i++) {
            if (m_Game.piece_squares[i] == square) {
                m_Game.piece_squares[i] = 0;
                m_Game.board[0] |= (M_SIDE_LARA | M_SIDE_ENEMY) - side;
            }
        }
    }

    if (square < M_EXIT) {
        m_Game.board[square] |= side;
    } else {
        m_Game.piece_squares[piece] = M_OFF_BOARD;
    }

    if ((square & 3) == 0) {
        m_Game.last_throw = 0;
    } else {
        m_Game.last_throw = m_Game.last_throw == M_ALL_LIGHT ? 0 : -1;
    }
}

static void M_Shockwave(
    const ITEM *const item, const RGB_888 color, const int32_t speed)
{
    const bool inward = speed < 0;
    for (int32_t i = 0; i < 4; i++) {
        FX_Shockwave_Create(&(FX_SHOCKWAVE_SETUP) {
            .pos = {
                .x = item->pos.x,
                .y = item->pos.y - M_SHOCKWAVE_RISE,
                .z = item->pos.z,
            },
            .inner_radius = inward ? 640 : 32,
            .outer_radius = inward ? 512 : 160,
            .speed = speed,
            .x_rot = i * DEG_45,
            .color = color,
            .life = M_SHOCKWAVE_LIFE,
        });
    }
}

static void M_ThrowSticks(ITEM *const item)
{
    M_SPINNER_PRIV *const p = item->priv;
    m_Game.last_throw = 0;
    p->stick_bits = 0;
    for (int32_t i = 0; i < M_STICKS; i++) {
        const int32_t light = Random_GetControl() & 1;
        m_Game.last_throw += light;
        if (light != 0) {
            p->stick_bits |= 1 << i;
        }
    }
    if (m_Game.last_throw == 0) {
        m_Game.last_throw = M_ALL_LIGHT;
    }
    p->spin_timer = M_SPIN_TIME;
    m_Game.pieces_locked = true;
}

static void M_SetPieceRoom(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    int16_t room_num = item->room_num;
    Room_GetSector(
        (XYZ_32) { item->pos.x, item->pos.y - 32, item->pos.z }, &room_num);
    if (room_num != item->room_num) {
        Item_UpdateRoom(item_num, room_num);
    }
}

static int16_t *M_GetFade(const ITEM *const item)
{
    if (item->object_id == O_SENET_SPINNER) {
        M_SPINNER_PRIV *const p = item->priv;
        return &p->fade;
    }
    M_PIECE_PRIV *const p = item->priv;
    return &p->fade;
}

static void M_FadeOutGame(void)
{
    for (int16_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->object_id != O_SENET_PIECE_1
            && item->object_id != O_SENET_PIECE_2
            && item->object_id != O_SENET_PIECE_3
            && item->object_id != O_SENET_ENEMY_PIECE
            && item->object_id != O_SENET_SPINNER) {
            continue;
        }
        if (item->is_destroyed) {
            continue;
        }
        item->trigger.mask = TRIGGER_MASK_ALL;
        item->is_collidable = false;
        *M_GetFade(item) = 1;
        Item_AddSimulated(i);
    }
}

static void M_SendHome(const int32_t piece, const int32_t side)
{
    const int16_t item_num = m_Game.item_nums[piece];
    ITEM *const item = Item_Get(item_num);
    const RGB_888 color = side == M_SIDE_LARA ? M_ENEMY_COLOR : M_LARA_COLOR;
    M_Shockwave(item, color, -64);
    item->pos.x = m_Game.target.x - (side << 12) + 7 * WALL_L;
    item->pos.z = m_Game.target.z + (piece % M_SIDE_PIECES) * WALL_L;
    M_SetPieceRoom(item_num);
    M_Shockwave(item, color, -64);
}

static void M_MovePiece(ITEM *const spinner)
{
    const int32_t piece = m_Game.piece_moving;
    const int32_t side = M_GetSide(piece);
    const int16_t item_num = m_Game.item_nums[piece];
    ITEM *const item = Item_Get(item_num);
    M_PIECE_PRIV *const p = item->priv;
    p->fade = M_MOVE_FADE;

    int32_t square = m_Game.piece_squares[piece];
    XZ_32 dest;
    if (square == M_OFF_BOARD || square >= M_SHARED_START) {
        if (square == M_OFF_BOARD) {
            square = M_EXIT;
        }
        dest.x = m_Game.target.x + WALL_L;
        dest.z = m_Game.target.z + (square - M_SHARED_START) * WALL_L;
    } else {
        dest.x = m_Game.target.x + (side << 11) - 2 * WALL_L;
        dest.z = m_Game.target.z + (4 - square) * WALL_L;
    }

    if (ABS(dest.x - item->pos.x) < M_PIECE_SPEED) {
        item->pos.x = dest.x;
    } else {
        item->pos.x += dest.x > item->pos.x ? M_PIECE_SPEED : -M_PIECE_SPEED;
    }
    if (ABS(dest.z - item->pos.z) < M_PIECE_SPEED) {
        item->pos.z = dest.z;
    } else {
        item->pos.z += dest.z > item->pos.z ? M_PIECE_SPEED : -M_PIECE_SPEED;
    }
    M_SetPieceRoom(item_num);

    if (item->pos.x != dest.x || item->pos.z != dest.z) {
        return;
    }

    p->fade = 0;
    if (square == M_EXIT) {
        const RGB_888 color =
            side == M_SIDE_LARA ? M_LARA_COLOR : M_ENEMY_COLOR;
        M_Shockwave(item, color, -32);
        M_Shockwave(item, color, 48);
        Item_Destroy(item_num);
        if (M_CheckWinner(side)) {
            M_FadeOutGame();
            m_Game.piece_moving = -1;
            return;
        }
    } else {
        for (int32_t i = 0; i < M_PIECES; i++) {
            const ITEM *const other = Item_Get(m_Game.item_nums[i]);
            if (i != piece && other->pos.x == dest.x
                && other->pos.z == dest.z) {
                M_SendHome(i, side);
            }
        }
    }

    if (m_Game.last_throw == 0) {
        Item_RemoveSimulated(Item_GetIndex(spinner));
    }
    m_Game.piece_moving = -1;
}

static void M_Spin(ITEM *const item)
{
    M_SPINNER_PRIV *const p = item->priv;
    if (p->spin_timer == M_SPIN_SOUND_TIME) {
        Sound_Effect(SFX_SPINNING_PUZZLE, &item->pos, SPM_NORMAL);
    }

    for (int32_t i = 0; i < M_STICKS; i++) {
        if (p->spin_timer >= M_SPIN_SOUND_TIME - 2 * i) {
            continue;
        }
        p->stick_rots[i] -= p->spin_timer << 7;
        if (p->spin_timer >= M_SPIN_SETTLE_TIME - 2 * i) {
            continue;
        }
        const bool is_light = (p->stick_bits & (1 << i)) != 0;
        if (is_light && p->stick_rots[i] > -DEG_90 / 4
            && p->stick_rots[i] < DEG_90 / 4) {
            p->stick_rots[i] = 0;
        } else if (
            !is_light
            && (p->stick_rots[i] > DEG_180 - DEG_90 / 4
                || p->stick_rots[i] < -DEG_180 + DEG_90 / 4)) {
            p->stick_rots[i] = -DEG_180;
        }
    }

    p->spin_timer--;
    if (p->spin_timer != 0) {
        return;
    }

    m_Game.pieces_locked = false;
    p->stick_bits = -1;
    if (m_Game.piece_moving == -1 && m_Game.last_throw == 0) {
        Item_RemoveSimulated(Item_GetIndex(item));
    }
}

static void M_EnemyTurn(ITEM *const item)
{
    M_ThrowSticks(item);
    for (int32_t i = M_SIDE_PIECES; i < M_PIECES; i++) {
        M_MakeMove(i, m_Game.last_throw);
        if (m_Game.last_throw == -1 || m_Game.last_throw == 0) {
            break;
        }
    }
    if (m_Game.last_throw == 0 || m_Game.last_throw == M_ALL_LIGHT) {
        m_Game.last_throw = -1;
    } else {
        m_Game.last_throw = 0;
    }
}

static void M_LaraTurn(ITEM *const item)
{
    M_ThrowSticks(item);
    bool can_move = false;
    for (int32_t i = 0; i < M_SIDE_PIECES; i++) {
        const int32_t square = m_Game.piece_squares[i] + m_Game.last_throw;
        if (m_Game.piece_squares[i] != M_OFF_BOARD && square <= M_EXIT
            && (m_Game.board[square] & M_SIDE_LARA) == 0) {
            can_move = true;
        }
    }
    if (!can_move) {
        m_Game.last_throw = m_Game.last_throw == M_ALL_LIGHT ? 0 : -1;
    }
}

static void M_InitialisePiece(const int16_t item_num)
{
    if (m_Game.item_nums[0] != NO_ITEM) {
        return;
    }

    for (int16_t i = 0; i < Item_GetLevelCount(); i++) {
        const ITEM *const item = Item_Get(i);
        if (item->object_id == O_SENET_PIECE_1) {
            m_Game.item_nums[0] = i;
            m_Game.target.x = item->pos.x + WALL_L;
            m_Game.target.z = item->pos.z;
        } else if (item->object_id == O_SENET_PIECE_2) {
            m_Game.item_nums[1] = i;
        } else if (item->object_id == O_SENET_PIECE_3) {
            m_Game.item_nums[2] = i;
        } else if (item->object_id == O_SENET_ENEMY_PIECE) {
            const int32_t slot = M_SIDE_PIECES + M_GetOCB(item);
            if (slot >= M_SIDE_PIECES && slot < M_PIECES) {
                m_Game.item_nums[slot] = i;
            }
        }
    }
}

static void M_ControlFade(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    int16_t *const fade = M_GetFade(item);
    (*fade)++;
    if (*fade >= M_FADE_END) {
        Item_Destroy(item_num);
    }
}

static void M_ControlPiece(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_PIECE_PRIV *const p = item->priv;
    if (p->fade != 0 && m_Game.piece_moving == -1) {
        M_ControlFade(item_num);
        return;
    }

    if (m_Game.last_throw > 0 && !m_Game.pieces_locked) {
        M_MakeMove(item->object_id - O_SENET_PIECE_1, m_Game.last_throw);
    }
    Item_RemoveSimulated(item_num);
}

static bool M_Draw(const ITEM *const item)
{
    const int16_t fade = *M_GetFade(item);
    if (fade == 0) {
        return Item_DrawAnimating(item);
    }

    RGBA_F tint = Output_GetTint();
    tint.a *= MAX(0, 256 - 2 * fade) / 255.0f;
    Output_PushTintOverride(tint);
    const bool result = Item_DrawAnimating(item);
    Output_PopTintOverride();
    return result;
}

static RESULT M_LoadPiecePriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PIECE_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "fade", &p->fade));
    return OK;
}

static void M_SavePiecePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PIECE_PRIV *const p = item->priv;
    JSONW_WRITE(io, "fade", p->fade);
}

static void M_InitialiseSpinner(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_SPINNER_PRIV *const p = item->priv;
    p->stick_bits = -1;
    item->extra_rotations = p->stick_rots;
}

static void M_CollideSpinner(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (!Lara_Interact_CanControl(LARA_INTERACT_SWITCH, item_num)) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    const ITEM *const item = Item_Get(item_num);
    lara_item->rot.y += DEG_180;
    const LARA_REACH reach =
        Lara_Interact_Reach(item, &m_SpinnerBounds, &m_SpinnerPosition);
    lara_item->rot.y += DEG_180;
    if (reach != LARA_REACH_ARRIVED) {
        return;
    }

    Item_SwitchToAnim(lara_item, LA(LA_ROTATE_SENET), 0);
    lara_item->current_anim_state = LS(LS_CONTROLLED);
    Lara_Interact_FinishControl(LARA_INTERACT_SWITCH);
    Item_AddSimulated(item_num);
}

static void M_ControlSpinner(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_SPINNER_PRIV *const p = item->priv;
    if (p->fade != 0) {
        M_ControlFade(item_num);
    } else if (p->stick_bits != -1) {
        M_Spin(item);
    } else if (m_Game.piece_moving != -1) {
        M_MovePiece(item);
    } else if (m_Game.last_throw == -1) {
        M_EnemyTurn(item);
    } else if (m_Game.last_throw == 0) {
        M_LaraTurn(item);
    }
}

static RESULT M_ReadS8Array(
    JSON_READ_IO *const io, const char *const key, int8_t *const values,
    const int32_t count)
{
    MUST(JSON_PUSH(io, key));
    const int32_t len = MIN(count, JSON_ARRAY_LEN(io));
    for (int32_t i = 0; i < len; i++) {
        MUST(JSON_READ_A(io, i, &values[i]));
    }
    return JSON_POP(io);
}

static RESULT M_ReadS16Array(
    JSON_READ_IO *const io, const char *const key, int16_t *const values,
    const int32_t count)
{
    MUST(JSON_PUSH(io, key));
    const int32_t len = MIN(count, JSON_ARRAY_LEN(io));
    for (int32_t i = 0; i < len; i++) {
        MUST(JSON_READ_A(io, i, &values[i]));
    }
    return JSON_POP(io);
}

static void M_WriteS8Array(
    JSON_WRITE_IO *const io, const char *const key, const int8_t *const values,
    const int32_t count)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < count; i++) {
        JSONW_PUSH_VALUE(io, values[i]);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, key);
}

static void M_WriteS16Array(
    JSON_WRITE_IO *const io, const char *const key, const int16_t *const values,
    const int32_t count)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < count; i++) {
        JSONW_PUSH_VALUE(io, values[i]);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, key);
}

static RESULT M_LoadSpinnerPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_SPINNER_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "fade", &p->fade));
    MUST(JSON_READ_OPT(io, "spin_timer", &p->spin_timer));
    MUST(JSON_READ_OPT(io, "stick_bits", &p->stick_bits));
    MUST(M_ReadS16Array(io, "stick_rots", p->stick_rots, M_STICKS));

    MUST(JSON_PUSH(io, "game"));
    MUST(M_ReadS8Array(io, "piece_squares", m_Game.piece_squares, M_PIECES));
    MUST(M_ReadS8Array(io, "board", m_Game.board, M_SQUARES));
    MUST(JSON_READ_OPT(io, "last_throw", &m_Game.last_throw));
    MUST(JSON_READ_OPT(io, "piece_moving", &m_Game.piece_moving));
    MUST(JSON_READ_OPT(io, "pieces_locked", &m_Game.pieces_locked));
    MUST(JSON_READ_OPT(io, "target", &m_Game.target));
    MUST(JSON_POP(io));
    return OK;
}

static void M_SaveSpinnerPriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_SPINNER_PRIV *const p = item->priv;
    JSONW_WRITE(io, "fade", p->fade);
    JSONW_WRITE(io, "spin_timer", p->spin_timer);
    JSONW_WRITE(io, "stick_bits", p->stick_bits);
    M_WriteS16Array(io, "stick_rots", p->stick_rots, M_STICKS);

    JSONW_PUSH_OBJECT(io);
    M_WriteS8Array(io, "piece_squares", m_Game.piece_squares, M_PIECES);
    M_WriteS8Array(io, "board", m_Game.board, M_SQUARES);
    JSONW_WRITE(io, "last_throw", m_Game.last_throw);
    JSONW_WRITE(io, "piece_moving", m_Game.piece_moving);
    JSONW_WRITE(io, "pieces_locked", m_Game.pieces_locked);
    JSONW_WRITE(io, "target", m_Game.target);
    JSONW_POP_AND_SET(io, "game");
}

static void M_SetupPiece(OBJECT *const obj)
{
    M_ResetGame();
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_InitialisePiece;
    obj->control_func = M_ControlPiece;
    obj->collision_func = Object_Collision;
    obj->draw_func = M_Draw;

    obj->priv_size = sizeof(M_PIECE_PRIV);
    obj->priv_load_func = M_LoadPiecePriv;
    obj->priv_save_func = M_SavePiecePriv;
    obj->save_flags = true;
    obj->save_hitpoints = true;
    obj->save_position = true;
}

static void M_SetupEnemyPiece(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_ControlFade;
    obj->collision_func = Object_Collision;
    obj->draw_func = M_Draw;

    obj->priv_size = sizeof(M_PIECE_PRIV);
    obj->priv_load_func = M_LoadPiecePriv;
    obj->priv_save_func = M_SavePiecePriv;
    obj->save_flags = true;
    obj->save_position = true;
}

static void M_SetupSpinner(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_InitialiseSpinner;
    obj->control_func = M_ControlSpinner;
    obj->collision_func = M_CollideSpinner;
    obj->draw_func = M_Draw;
    for (int32_t i = 0; i < M_STICKS; i++) {
        Object_GetBone(obj, i)->rot.z = true;
    }

    obj->priv_size = sizeof(M_SPINNER_PRIV);
    obj->priv_load_func = M_LoadSpinnerPriv;
    obj->priv_save_func = M_SaveSpinnerPriv;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_SENET_PIECE_1, M_SetupPiece)
REGISTER_OBJECT(O_SENET_PIECE_2, M_SetupPiece)
REGISTER_OBJECT(O_SENET_PIECE_3, M_SetupPiece)
REGISTER_OBJECT(O_SENET_ENEMY_PIECE, M_SetupEnemyPiece)
REGISTER_OBJECT(O_SENET_SPINNER, M_SetupSpinner)
