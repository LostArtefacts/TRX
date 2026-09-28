#include <trx/game/lara/misc.h>

#include <trx/config.h>
#include <trx/game/effects.h>
#include <trx/game/gun/common.h>
#include <trx/game/lara.h>
#include <trx/game/lara/skin/common.h>
#include <trx/game/level/settings.h>
#include <trx/game/matrix.h>
#include <trx/game/objects/effects/flame.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/rooms/geometry.h>
#include <trx/game/sound.h>
#include <trx/version.h>

typedef struct {
    int32_t rot[3][3];
    int32_t pos[3];
    const XYZ_16 *frame_rots;
} M_PS1_MATRIX;

// Turns columns a and b of a 4.12 fixed-point matrix by an angle. Each
// product is rounded down before it is stored, as the PS1 geometry
// coprocessor does.
static void M_PS1_Rotate(
    M_PS1_MATRIX *const m, const int32_t a, const int32_t b,
    const int16_t angle)
{
    const int32_t sin = Math_Sin(angle) >> 2;
    const int32_t cos = Math_Cos(angle) >> 2;
    for (int32_t row = 0; row < 3; row++) {
        const int32_t col_a = m->rot[row][a];
        const int32_t col_b = m->rot[row][b];
        m->rot[row][a] = (col_a * cos + col_b * sin) >> 12;
        m->rot[row][b] = (col_b * cos - col_a * sin) >> 12;
    }
}

static void M_PS1_RotYXZ(M_PS1_MATRIX *const m, const XYZ_16 rot)
{
    M_PS1_Rotate(m, 2, 0, rot.y);
    M_PS1_Rotate(m, 1, 2, rot.x);
    M_PS1_Rotate(m, 0, 1, rot.z);
}

static void M_PS1_TranslateRel(M_PS1_MATRIX *const m, const XYZ_32 offset)
{
    for (int32_t row = 0; row < 3; row++) {
        const int64_t sum = (int64_t)m->rot[row][0] * offset.x
            + (int64_t)m->rot[row][1] * offset.y
            + (int64_t)m->rot[row][2] * offset.z;
        m->pos[row] += (int32_t)(sum >> 12);
    }
}

// Blends two positions the way the PS1 release blends Lara's arms between
// two animation frames: by halves and quarters only.
static int32_t M_PS1_Blend(
    const int32_t pos_1, const int32_t pos_2, const int32_t frac,
    const int32_t rate)
{
    if (rate == 2 || (frac == 2 && rate == 4)) {
        return (pos_1 + pos_2) >> 1;
    } else if (frac == 1) {
        return pos_1 + ((pos_2 - pos_1) >> 2);
    }
    return pos_2 - ((pos_2 - pos_1) >> 2);
}

// Picks the two key frames around the current frame the way the PS1 release
// does, without the smoothing between game ticks.
static int32_t M_PS1_GetFrames(
    const ITEM *const item, ANIM_FRAME *frames[2], int32_t *const rate)
{
    const ANIM *const anim = Item_GetAnim(item);
    *rate = anim->interpolation;
    const int32_t first = (item->frame_num - anim->frame_base) / *rate;
    const int32_t frac = (item->frame_num - anim->frame_base) % *rate;
    frames[0] = &anim->frame_ptr[first];
    frames[1] = &anim->frame_ptr[first + 1];
    if (frac == 0) {
        return 0;
    }

    // The original compares the end frame against a frame counted from the
    // start of the animation, so this trim rarely applies.
    const int32_t second = first * *rate + *rate;
    if (anim->frame_end < second) {
        *rate = anim->frame_end - first * *rate;
    }
    return frac;
}

// Builds Lara's torso for the two key frames around her current frame, and
// the heading alone that her arms start from.
static bool M_PS1_GetTorso(
    M_PS1_MATRIX *const base, M_PS1_MATRIX body[2], int32_t *const frac,
    int32_t *const rate)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();

    ANIM_FRAME *frames[2] = { nullptr, nullptr };
    *rate = 1;
    *frac = 0;
    if (lara->hit_direction < 0) {
        *frac = M_PS1_GetFrames(lara_item, frames, rate);
    } else {
        frames[0] = (ANIM_FRAME *)Lara_GetHitFrame(lara_item);
    }
    if (frames[0] == nullptr) {
        return false;
    }
    if (*frac == 0) {
        frames[1] = frames[0];
    }

    const ANIM_BONE *const bone = Lara_Skin_GetBoneBase();
    *base = (M_PS1_MATRIX) {
        .rot = { { 4096, 0, 0 }, { 0, 4096, 0 }, { 0, 0, 4096 } },
    };
    M_PS1_RotYXZ(base, lara_item->rot);

    for (int32_t i = 0; i < 2; i++) {
        const XYZ_16 *const rots = frames[i]->mesh_rots;
        body[i] = *base;
        body[i].frame_rots = rots;
        M_PS1_TranslateRel(&body[i], XYZ_32_From16(frames[i]->offset));
        M_PS1_RotYXZ(&body[i], rots[LM_HIPS]);
        M_PS1_TranslateRel(&body[i], bone[LM_TORSO - 1].pos);
        M_PS1_RotYXZ(&body[i], rots[LM_TORSO]);
        M_PS1_RotYXZ(&body[i], lara->torso_rot);
    }
    return true;
}

static void M_GetJointAbsPosition_I(
    XYZ_32 *const vec, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const ITEM *const item = Lara_GetItem();
    const OBJECT *obj = Object_Get(item->object_id);

    Matrix_PushUnit();
    Matrix_Rot16(item->rot);

    const ANIM_BONE *const bone = Object_GetBone(obj, 0);
    const XYZ_16 *mesh_rots_1 = frame1->mesh_rots;
    const XYZ_16 *mesh_rots_2 = frame2->mesh_rots;
    Matrix_InitInterpolate(frac, rate);

    Matrix_TranslateRel16_ID(frame1->offset, frame2->offset);
    Matrix_Rot16_ID(mesh_rots_1[LM_HIPS], mesh_rots_2[LM_HIPS]);

    Matrix_TranslateRel32_I(bone[LM_TORSO - 1].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_TORSO], mesh_rots_2[LM_TORSO]);
    Matrix_Rot16_I(lara_info->torso_rot);

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (lara_info->gun_status == LGS_READY
        || lara_info->gun_status == LGS_SPECIAL
        || lara_info->gun_status == LGS_DRAW
        || lara_info->gun_status == LGS_UNDRAW) {
        gun_type = lara_info->gun_type;
    }

    if (Gun_IsFlareType(lara_info->gun_type)) {
        Matrix_Interpolate();
        Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
        if (lara_info->flare.control) {
            const LARA_ARM *const arm = &lara_info->left_arm;
            const ANIM *const anim = Anim_GetAnim(arm->anim_num);
            mesh_rots_1 =
                arm->frame_base[arm->frame_num - anim->frame_base].mesh_rots;
        }
        Matrix_Rot16(mesh_rots_1[LM_UARM_L]);

        Matrix_TranslateRel32(bone[LM_LARM_L - 1].pos);
        Matrix_Rot16(mesh_rots_1[LM_LARM_L]);

        Matrix_TranslateRel32(bone[LM_HAND_L - 1].pos);
        Matrix_Rot16(mesh_rots_1[LM_HAND_L]);
    } else if (gun_type != LGT_UNARMED) {
        Matrix_Interpolate();
        Matrix_TranslateRel32(bone[LM_UARM_R - 1].pos);

        const LARA_ARM *const arm = &lara_info->right_arm;
        const ANIM *const anim = Anim_GetAnim(arm->anim_num);
        mesh_rots_1 = arm->frame_base[arm->frame_num].mesh_rots;
        Matrix_Rot16(mesh_rots_1[LM_UARM_R]);

        Matrix_TranslateRel32(bone[LM_LARM_R - 1].pos);
        Matrix_Rot16(mesh_rots_1[LM_LARM_R]);

        Matrix_TranslateRel32(bone[LM_HAND_R - 1].pos);
        Matrix_Rot16(mesh_rots_1[LM_HAND_R]);
    }

    Matrix_TranslateRel32(*vec);
    vec->x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    vec->y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    vec->z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    Matrix_Pop();
}

// TODO: joint is ignored - this only works for hands.
void Lara_GetJointAbsPosition(XYZ_32 *const vec, const LARA_MESH joint)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    ANIM_FRAME *frmptr[2] = { nullptr, nullptr };
    if (lara_info->hit_direction < 0) {
        int32_t rate;
        const int32_t frac = Item_GetFrames(lara_item, frmptr, &rate);
        if (frac != 0) {
            M_GetJointAbsPosition_I(vec, frmptr[0], frmptr[1], frac, rate);
            return;
        }
    }

    const ANIM_FRAME *const hit_frame = Lara_GetHitFrame(lara_item);
    const ANIM_FRAME *const frame_ptr =
        hit_frame == nullptr ? frmptr[0] : hit_frame;

    Matrix_PushUnit();
    Matrix_Rot16(lara_item->rot);

    const XYZ_16 *mesh_rots = frame_ptr->mesh_rots;
    const OBJECT *const obj = Object_Get(lara_item->object_id);
    const ANIM_BONE *bone = Object_GetBone(obj, 0);

    Matrix_TranslateRel16(frame_ptr->offset);
    Matrix_Rot16(mesh_rots[LM_HIPS]);

    Matrix_TranslateRel32(bone[LM_TORSO - 1].pos);
    Matrix_Rot16(mesh_rots[LM_TORSO]);
    Matrix_Rot16(lara_info->torso_rot);

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (lara_info->gun_status == LGS_READY
        || lara_info->gun_status == LGS_SPECIAL
        || lara_info->gun_status == LGS_DRAW
        || lara_info->gun_status == LGS_UNDRAW) {
        gun_type = lara_info->gun_type;
    }

    if (Gun_IsFlareType(lara_info->gun_type)) {
        Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
        if (lara_info->flare.control) {
            const LARA_ARM *const arm = &lara_info->left_arm;
            const ANIM *const anim = Anim_GetAnim(arm->anim_num);
            mesh_rots =
                arm->frame_base[arm->frame_num - anim->frame_base].mesh_rots;
        }
        Matrix_Rot16(mesh_rots[LM_UARM_L]);

        Matrix_TranslateRel32(bone[LM_LARM_L - 1].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_L]);

        Matrix_TranslateRel32(bone[LM_HAND_L - 1].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_L]);
    } else if (gun_type != LGT_UNARMED) {
        Matrix_TranslateRel32(bone[LM_UARM_R - 1].pos);

        const LARA_ARM *const arm = &lara_info->right_arm;
        const ANIM *const anim = Anim_GetAnim(arm->anim_num);
        mesh_rots = arm->frame_base[arm->frame_num].mesh_rots;
        Matrix_Rot16(mesh_rots[LM_UARM_R]);

        Matrix_TranslateRel32(bone[LM_LARM_R - 1].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_R]);

        Matrix_TranslateRel32(bone[LM_HAND_R - 1].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_R]);
    }

    Matrix_TranslateRel32(*vec);
    vec->x = lara_item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    vec->y = lara_item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    vec->z = lara_item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    Matrix_Pop();
}

bool Lara_GetHandPosFromAnim(const LARA_MESH hand, XYZ_32 *const vec)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (lara->gun_status == LGS_READY || lara->gun_status == LGS_SPECIAL
        || lara->gun_status == LGS_DRAW || lara->gun_status == LGS_UNDRAW) {
        gun_type = lara->gun_type;
    }
    if (!Gun_IsSinglePistolType(gun_type) && !Gun_IsDualPistolType(gun_type)) {
        return false;
    }

    M_PS1_MATRIX base;
    M_PS1_MATRIX body[2];
    int32_t frac;
    int32_t rate;
    if (!M_PS1_GetTorso(&base, body, &frac, &rate)) {
        return false;
    }

    const ANIM_BONE *const bone = Lara_Skin_GetBoneBase();
    const bool is_right = hand == LM_HAND_R;
    const LARA_ARM *const arm = is_right ? &lara->right_arm : &lara->left_arm;
    const LARA_MESH upper = is_right ? LM_UARM_R : LM_UARM_L;
    const LARA_MESH lower = is_right ? LM_LARM_R : LM_LARM_L;

    // The arm starts from the heading alone and from the shoulder between the
    // two frames.
    M_PS1_MATRIX m = base;
    for (int32_t i = 0; i < 2; i++) {
        M_PS1_TranslateRel(&body[i], bone[upper - 1].pos);
    }
    for (int32_t i = 0; i < 3; i++) {
        m.pos[i] = frac == 0
            ? body[0].pos[i]
            : M_PS1_Blend(body[0].pos[i], body[1].pos[i], frac, rate);
    }
    M_PS1_RotYXZ(
        &m, Gun_IsSinglePistolType(gun_type) ? lara->torso_rot : arm->rot);

    const ANIM *const anim = Anim_GetAnim(arm->anim_num);
    const XYZ_16 *const arm_rots =
        arm->frame_base[arm->frame_num - anim->frame_base].mesh_rots;
    M_PS1_RotYXZ(&m, arm_rots[upper]);
    M_PS1_TranslateRel(&m, bone[lower - 1].pos);
    M_PS1_RotYXZ(&m, arm_rots[lower]);
    M_PS1_TranslateRel(&m, bone[hand - 1].pos);
    M_PS1_RotYXZ(&m, arm_rots[hand]);
    M_PS1_TranslateRel(&m, *vec);

    vec->x = lara_item->pos.x + m.pos[0];
    vec->y = lara_item->pos.y + m.pos[1];
    vec->z = lara_item->pos.z + m.pos[2];
    return true;
}

void Lara_RefuseInteraction(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (!XYZ_32_AreEquivalent(
            lara_info->interact_target.initial_pos, lara_item->pos)) {
        lara_info->interact_target.initial_pos = lara_item->pos;
        Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_ALWAYS);
    }
}

void Lara_TakeHit(ITEM *const lara_item, const int32_t dx, const int32_t dz)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const int16_t hit_angle = lara_item->rot.y + DEG_180 - Math_Atan(dz, dx);
    lara_info->hit_direction = Math_GetDirection(hit_angle);
    if (lara_info->hit_frame == 0) {
        Sound_Effect(
            g_TRVersion == 1 ? SFX_LARA_BODYSL : SFX_LARA_INJURY,
            &lara_item->pos, SPM_NORMAL);
    }
    lara_info->hit_frame++;
    if (lara_info->interact_target.is_moving
        && lara_info->gun_status == LGS_HANDS_BUSY) {
        lara_info->gun_status = LGS_ARMLESS;
    }
    lara_info->interact_target.is_moving = false;
    lara_info->interact_target.item_num = NO_ITEM;
    CLAMPG(lara_info->hit_frame, 34);
}

void Lara_TouchDeathSector(const GF_DEATH_TILE death_tile)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_item->hit_points < 0 || lara_info->water_status == LWS_CHEAT) {
        return;
    }

    int16_t room_num = lara_item->room_num;
    const XYZ_32 pos = { lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z };
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    if (lara_item->floor != height) {
        return;
    }

    if (g_Config.debug.enable_invulnerability) {
        switch (death_tile) {
        case GF_DEATH_TILE_RAPIDS:
        case GF_DEATH_TILE_ELECTRIC:
            Lara_CatchFire();
            break;
        case GF_DEATH_TILE_LAVA:
            Lara_TouchLava();
            break;
        }
        return;
    }

    Lara_Kill();
    lara_item->hit_status = true;

    switch (death_tile) {
    case GF_DEATH_TILE_RAPIDS:
        Lara_RapidsDrown();
        break;
    case GF_DEATH_TILE_ELECTRIC:
        lara_info->electric = 1;
        break;
    case GF_DEATH_TILE_LAVA:
        Lara_TouchLava();
        break;
    }
}

void Lara_TouchLava(void)
{
    if (g_TRVersion == 3) {
        Lara_CatchFire();
        return;
    }

    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->burn) {
        return;
    }

    if (lara_info->water_status != LWS_ABOVE_WATER
        && (lara_info->water_status != LWS_WADE
            || !Room_Get(lara_item->room_num)->flags.swamp)) {
        return;
    }

    const OBJECT *const obj = Object_Get(O_FLAME);
    for (int32_t i = 0; i < 10; i++) {
        const int16_t effect_num = Effect_Create(O_FLAME, lara_item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->frame_num = obj->mesh_count * Random_GetControl() / 0x7FFF;
            effect->counter = -1 - 24 * Random_GetControl() / 0x7FFF;
        }
    }
    lara_info->burn = true;
}

void Lara_RapidsDrown(void)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    Lara_SwitchToExtraState(LS_EXTRA_RAPIDS_DROWN);
    Lara_Kill();
    lara_item->hit_status = true;
    lara_item->gravity = false;
    lara_item->fall_speed = 0;
    lara_item->speed = 0;

    lara_info->gun_type = LGT_UNARMED;
}

void Lara_StopSlidingSFX(void)
{
    if (g_Config.audio.fix_sliding_sfx) {
        Sound_StopEffect(SFX_LARA_SLIDING);
    }
}

int32_t Lara_FloorFront(
    const ITEM *const item, const int16_t ang, const int32_t dist)
{
    XYZ_32 pos = item->pos;
    pos.y -= LARA_HEIGHT;
    pos = XYZ_32_OffsetYaw(pos, ang, dist);
    if (Room_IsPathBlocked(
            item->pos, pos, item->room_num, LARA_HEIGHT, LARA_RADIUS)) {
        return NO_HEIGHT;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    int32_t height = Room_GetHeight(sector, pos);
    if (height != NO_HEIGHT) {
        height -= item->pos.y;
        if (height > 0
            && Room_GetPitSector(sector, pos.x, pos.z)->is_death_sector) {
            return STEP_L * 2;
        }
    }
    return height;
}

int32_t Lara_CeilingFront(
    const ITEM *const item, const int16_t ang, const int32_t dist,
    const int32_t item_height)
{
    XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, ang, dist);
    pos.y -= item_height;
    // Something reaching down to where Lara's head would be leaves her no more
    // room than a ceiling at her feet does.
    if (Room_IsPathBlocked(
            item->pos, pos, item->room_num, STEP_L, LARA_RADIUS)) {
        return item_height;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    int32_t height = Room_GetCeiling(sector, pos);
    if (height != NO_HEIGHT) {
        height += item_height - item->pos.y;
    }
    return height;
}

void Lara_UpdateRoomToHeight(const int32_t height)
{
    ITEM *const lara_item = Lara_GetItem();
    XYZ_32 pos = lara_item->pos;
    pos.y += height;

    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    lara_item->floor = Room_GetHeight(sector, pos);

    const int16_t item_num = Item_GetIndex(lara_item);
    Item_UpdateRoom(item_num, room_num);
}

int32_t Lara_GetWaterDepth(const XYZ_32 pos, int16_t room_num)
{
    const ROOM *room = Room_Get(room_num);
    const SECTOR *sector;

    while (true) {
        int32_t z_sector = (pos.z - room->pos.z) >> WALL_SHIFT;
        int32_t x_sector = (pos.x - room->pos.x) >> WALL_SHIFT;

        if (z_sector <= 0) {
            z_sector = 0;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > room->size.x - 2) {
                x_sector = room->size.x - 2;
            }
        } else if (z_sector >= room->size.z - 1) {
            z_sector = room->size.z - 1;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > room->size.x - 2) {
                x_sector = room->size.x - 2;
            }
        } else if (x_sector < 0) {
            x_sector = 0;
        } else if (x_sector >= room->size.x) {
            x_sector = room->size.x - 1;
        }

        sector = Room_GetUnitSector(room, x_sector, z_sector);
        if (sector->portal_room.wall == NO_ROOM) {
            break;
        }
        room_num = sector->portal_room.wall;
        room = Room_Get(room_num);
    }

    if (room->flags.underwater || room->flags.swamp) {
        while (sector->portal_room.sky != NO_ROOM) {
            room = Room_Get(sector->portal_room.sky);
            if (!room->flags.underwater && !room->flags.swamp) {
                const int32_t water_height = Room_GetWaterHeight(pos, room_num);
                sector = Room_GetSector(pos, &room_num);
                return Room_GetHeight(sector, pos) - water_height;
            }
            sector = Room_GetWorldSector(room, pos.x, pos.z);
        }
        return 0x7FFF;
    }

    while (sector->portal_room.pit != NO_ROOM) {
        room = Room_Get(sector->portal_room.pit);
        if (room->flags.underwater || room->flags.swamp) {
            const int32_t water_height = Room_GetWaterHeight(pos, room_num);
            sector = Room_GetSector(pos, &room_num);
            return Room_GetHeight(sector, pos) - water_height;
        }
        sector = Room_GetWorldSector(room, pos.x, pos.z);
    }
    return NO_HEIGHT;
}

bool Lara_IsMachineGunActive(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    ITEM *const lara_item = Lara_GetItem();
    if (lara->gun_item_num == NO_ITEM || lara_item->hit_points <= 0
        || !Gun_IsMachineGunType(lara->gun_type)) {
        return false;
    }

    const ITEM *const item = Item_Get(lara->gun_item_num);
    return item->current_anim_state == 0 || item->current_anim_state == 2
        || item->current_anim_state == 4;
}

void Lara_CatchFireEx(const FLAME_TYPE type)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->burn || lara_info->water_status == LWS_CHEAT) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    const int16_t effect_num = Effect_Create(O_FLAME, lara_item->room_num);
    if (effect_num == NO_EFFECT) {
        return;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    if (g_TRVersion == 3) {
        // TR3 effects use Collide_GetJointAbsPosition but only every x frames,
        // which lets Lara briefly catch fire even if she touches liquids
        // (for example, when running into the boiling water in Tony's room).
        effect->pos = (XYZ_32) {};
    } else {
        effect->pos = lara_item->pos;
    }
    effect->frame_num = g_TRVersion >= 3 ? type : 0;
    effect->counter = -1;
    lara_info->burn = true;
}

void Lara_CatchFire(void)
{
    Lara_CatchFireEx(FLAME_SMALL);
}

void Lara_Extinguish(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->electric = 0;

    if (!lara_info->burn) {
        return;
    }

    lara_info->burn = false;

    // put out flame objects
    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        const int16_t next_effect_num = effect->next_active;
        if (effect->object_id == O_FLAME && effect->counter < 0) {
            effect->counter = 0;
            Effect_Destroy(effect_num);
        }
        effect_num = next_effect_num;
    }
}

void Lara_Dry(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        lara_info->wet[mesh] = 0;
    }
}

bool Lara_IsWet(void)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        if (lara_info->wet[mesh] != 0) {
            return true;
        }
    }
    return false;
}

bool Lara_HasAnimation(const LARA_ANIMATION_ID *const test_arr)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return false;
    }

    if (Lara_GetAnimationObject() != O_LARA) {
        return false;
    }

    const LARA_ANIMATION_ID current_anim =
        LA_U(Item_GetRelativeAnim(lara_item));
    for (int32_t i = 0; test_arr[i] != NO_CATALOG_ID; i++) {
        if (test_arr[i] == current_anim) {
            return true;
        }
    }
    return false;
}

bool Lara_HasState(const LARA_STATE_ID *const test_arr)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->extra_anim) {
        return false;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return false;
    }

    for (int32_t i = 0; test_arr[i] != NO_CATALOG_ID; i++) {
        if (test_arr[i] == LS_U(lara_item->current_anim_state)) {
            return true;
        }
    }
    return false;
}

bool Lara_HasExtraState(const LARA_EXTRA_STATE *const test_arr)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (!lara_info->extra_anim) {
        return false;
    }

    const ITEM *const lara_item = Lara_GetItem();
    for (int32_t i = 0; test_arr[i] != (LARA_EXTRA_STATE)-1; i++) {
        if (test_arr[i] == (LARA_EXTRA_STATE)lara_item->current_anim_state) {
            return true;
        }
    }
    return false;
}

void Lara_SwitchToExtraState(const LARA_EXTRA_STATE goal_state)
{
    ITEM *const lara_item = Lara_GetItem();
    Item_SwitchToObjAnim(lara_item, LS_EXTRA_BREATH, 0, O_LARA_EXTRA);
    lara_item->current_anim_state = LS_EXTRA_BREATH;
    lara_item->goal_anim_state = goal_state;
    Item_Animate(lara_item);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->gun_status = LGS_HANDS_BUSY;
    lara->hit_direction = DIR_UNKNOWN;
    lara->extra_anim = true;
}
