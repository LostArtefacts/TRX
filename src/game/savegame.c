#include "game/inv.h"
#include "game/savegame.h"
#include "game/vars.h"
#include "specific/shed.h"
#include "util.h"

#define SAVE_CREATURE (1 << 7)

static int SGCount;
static char *SGPoint;

void InitialiseStartInfo()
{
    if (!SaveGame[0].bonus_flag) {
        for (int i = 0; i < LV_NUMBER_OF; i++) {
            ModifyStartInfo(i);
            SaveGame[0].start[i].available = 0;
        }
        SaveGame[0].start[LV_GYM].available = 1;
        SaveGame[0].start[LV_FIRSTLEVEL].available = 1;
    }
}

void ModifyStartInfo(int32_t level_num)
{
    START_INFO *start = &SaveGame[0].start[level_num];

    start->got_pistols = 1;
    start->gun_type = LGT_PISTOLS;
    start->pistol_ammo = 1000;

    switch (level_num) {
    case LV_GYM:
        start->num_medis = 0;
        start->num_big_medis = 0;
        start->num_scions = 0;
        start->pistol_ammo = 0;
        start->shotgun_ammo = 0;
        start->magnum_ammo = 0;
        start->uzi_ammo = 0;
        start->got_pistols = 0;
        start->got_shotgun = 0;
        start->got_magnums = 0;
        start->got_uzis = 0;
        start->gun_type = LGT_UNARMED;
        start->gun_status = LGS_ARMLESS;
        break;

    case LV_FIRSTLEVEL:
        start->num_medis = 0;
        start->num_big_medis = 0;
        start->num_scions = 0;
        start->shotgun_ammo = 0;
        start->magnum_ammo = 0;
        start->uzi_ammo = 0;
        start->got_shotgun = 0;
        start->got_magnums = 0;
        start->got_uzis = 0;
        start->gun_status = LGS_ARMLESS;
        break;

    case LV_LEVEL10A:
        start->num_scions = 0;
        start->got_pistols = 0;
        start->got_shotgun = 0;
        start->got_magnums = 0;
        start->got_uzis = 0;
        start->gun_type = LGT_UNARMED;
        start->gun_status = LGS_ARMLESS;
        break;
    }

    if (SaveGame[0].bonus_flag && level_num != LV_GYM) {
        start->got_pistols = 1;
        start->got_shotgun = 1;
        start->got_magnums = 1;
        start->got_uzis = 1;
        start->shotgun_ammo = 1234;
        start->magnum_ammo = 1234;
        start->uzi_ammo = 1234;
        start->gun_type = LGT_UZIS;
    }
}

void CreateStartInfo(int level_num)
{
    START_INFO *start = &SaveGame[0].start[level_num];

    start->available = 1;
    start->costume = 0;

    start->pistol_ammo = 1000;
    if (Inv_RequestItem(O_GUN_ITEM)) {
        start->got_pistols = 1;
    } else {
        start->got_pistols = 0;
    }

    if (Inv_RequestItem(O_MAGNUM_ITEM)) {
        start->magnum_ammo = Lara.magnums.ammo;
        start->got_magnums = 1;
    } else {
        start->magnum_ammo = Inv_RequestItem(O_MAG_AMMO_ITEM) * MAGNUM_AMMO_QTY;
        start->got_magnums = 0;
    }

    if (Inv_RequestItem(O_UZI_ITEM)) {
        start->uzi_ammo = Lara.uzis.ammo;
        start->got_uzis = 1;
    } else {
        start->uzi_ammo = Inv_RequestItem(O_UZI_AMMO_ITEM) * UZI_AMMO_QTY;
        start->got_uzis = 0;
    }

    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        start->shotgun_ammo = Lara.shotgun.ammo;
        start->got_shotgun = 1;
    } else {
        start->shotgun_ammo =
            Inv_RequestItem(O_SG_AMMO_ITEM) * SHOTGUN_AMMO_QTY;
        start->got_shotgun = 0;
    }

    start->num_medis = Inv_RequestItem(O_MEDI_ITEM);
    start->num_big_medis = Inv_RequestItem(O_BIGMEDI_ITEM);
    start->num_scions = Inv_RequestItem(O_SCION_ITEM);

    start->gun_type = Lara.gun_type;
    if (Lara.gun_status == LGS_READY) {
        start->gun_status = LGS_READY;
    } else {
        start->gun_status = LGS_ARMLESS;
    }
}

void CreateSaveGameInfo()
{
    SaveGame[0].current_level = CurrentLevel;

    CreateStartInfo(LV_CURRENT);

    SaveGame[0].num_pickup1 = Inv_RequestItem(O_PICKUP_ITEM1);
    SaveGame[0].num_pickup2 = Inv_RequestItem(O_PICKUP_ITEM2);
    SaveGame[0].num_puzzle1 = Inv_RequestItem(O_PUZZLE_ITEM1);
    SaveGame[0].num_puzzle2 = Inv_RequestItem(O_PUZZLE_ITEM2);
    SaveGame[0].num_puzzle3 = Inv_RequestItem(O_PUZZLE_ITEM3);
    SaveGame[0].num_puzzle4 = Inv_RequestItem(O_PUZZLE_ITEM4);
    SaveGame[0].num_key1 = Inv_RequestItem(O_KEY_ITEM1);
    SaveGame[0].num_key2 = Inv_RequestItem(O_KEY_ITEM2);
    SaveGame[0].num_key3 = Inv_RequestItem(O_KEY_ITEM3);
    SaveGame[0].num_key4 = Inv_RequestItem(O_KEY_ITEM4);
    SaveGame[0].num_leadbar = Inv_RequestItem(O_LEADBAR_ITEM);

    ResetSG();

    for (int i = 0; i < MAX_SAVEGAME_BUFFER; i++) {
        SGPoint[i] = 0;
    }

    WriteSG(&FlipStatus, sizeof(int32_t));
    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        int8_t flag = FlipMapTable[i] >> 8;
        WriteSG(&flag, sizeof(int8_t));
    }

    for (int i = 0; i < NumberCameras; i++) {
        WriteSG(&Camera.fixed[i].flags, sizeof(int16_t));
    }

    for (int i = 0; i < LevelItemCount; i++) {
        ITEM_INFO *item = &Items[i];
        OBJECT_INFO *obj = &Objects[item->object_number];

        if (obj->save_position) {
            WriteSG(&item->pos, sizeof(PHD_3DPOS));
            WriteSG(&item->room_number, sizeof(int16_t));
            WriteSG(&item->speed, sizeof(int16_t));
            WriteSG(&item->fall_speed, sizeof(int16_t));
        }

        if (obj->save_anim) {
            WriteSG(&item->current_anim_state, sizeof(int16_t));
            WriteSG(&item->goal_anim_state, sizeof(int16_t));
            WriteSG(&item->required_anim_state, sizeof(int16_t));
            WriteSG(&item->anim_number, sizeof(int16_t));
            WriteSG(&item->frame_number, sizeof(int16_t));
        }

        if (obj->save_hitpoints) {
            WriteSG(&item->hit_points, sizeof(int16_t));
        }

        if (obj->save_flags) {
            uint16_t flags = item->flags + item->active + (item->status << 1)
                + (item->gravity_status << 3) + (item->collidable << 4);
            if (obj->intelligent && item->data) {
                flags |= SAVE_CREATURE;
            }
            WriteSG(&flags, sizeof(uint16_t));
            WriteSG(&item->timer, sizeof(int16_t));
            if (flags & SAVE_CREATURE) {
                CREATURE_INFO *creature = item->data;
                WriteSG(&creature->head_rotation, sizeof(int16_t));
                WriteSG(&creature->neck_rotation, sizeof(int16_t));
                WriteSG(&creature->maximum_turn, sizeof(int16_t));
                WriteSG(&creature->flags, sizeof(int16_t));
                WriteSG(&creature->mood, sizeof(int32_t));
            }
        }
    }

    WriteSGLara(&Lara);

    WriteSG(&FlipEffect, sizeof(int32_t));
    WriteSG(&FlipTimer, sizeof(int32_t));
}

void WriteSG(void *pointer, int size)
{
    SGCount += size;
    if (SGCount >= MAX_SAVEGAME_BUFFER) {
        S_ExitSystem("FATAL: Savegame is too big to fit in buffer");
    }

    char *data = (char *)pointer;
    for (int i = 0; i < size; i++) {
        *SGPoint++ = *data++;
    }
}

void WriteSGLara(LARA_INFO *lara)
{
    int32_t tmp32 = 0;

    WriteSG(&lara->item_number, sizeof(int16_t));
    WriteSG(&lara->gun_status, sizeof(int16_t));
    WriteSG(&lara->gun_type, sizeof(int16_t));
    WriteSG(&lara->request_gun_type, sizeof(int16_t));
    WriteSG(&lara->calc_fall_speed, sizeof(int16_t));
    WriteSG(&lara->water_status, sizeof(int16_t));
    WriteSG(&lara->pose_count, sizeof(int16_t));
    WriteSG(&lara->hit_frame, sizeof(int16_t));
    WriteSG(&lara->hit_direction, sizeof(int16_t));
    WriteSG(&lara->air, sizeof(int16_t));
    WriteSG(&lara->dive_count, sizeof(int16_t));
    WriteSG(&lara->death_count, sizeof(int16_t));
    WriteSG(&lara->current_active, sizeof(int16_t));
    WriteSG(&lara->spaz_effect_count, sizeof(int16_t));

    // NOTE: OG just writes the pointer address (!)
    if (lara->spaz_effect) {
        tmp32 = (size_t)lara->spaz_effect - (size_t)Effects;
    }
    WriteSG(&tmp32, sizeof(int32_t));

    WriteSG(&lara->mesh_effects, sizeof(int32_t));

    for (int i = 0; i < LM_NUMBER_OF; i++) {
        tmp32 = (size_t)lara->mesh_ptrs[i] - (size_t)MeshBase;
        WriteSG(&tmp32, sizeof(int32_t));
    }

    // NOTE: og just writes the pointer address (!) assuming it's a
    // non-existing mesh 16 (!!) which happens to be Lara's current target.
    // Just write NULL.
    tmp32 = 0;
    WriteSG(&tmp32, sizeof(int32_t));

    WriteSG(&lara->target_angles[0], sizeof(PHD_ANGLE));
    WriteSG(&lara->target_angles[1], sizeof(PHD_ANGLE));
    WriteSG(&lara->turn_rate, sizeof(int16_t));
    WriteSG(&lara->move_angle, sizeof(int16_t));
    WriteSG(&lara->head_y_rot, sizeof(int16_t));
    WriteSG(&lara->head_x_rot, sizeof(int16_t));
    WriteSG(&lara->head_z_rot, sizeof(int16_t));
    WriteSG(&lara->torso_y_rot, sizeof(int16_t));
    WriteSG(&lara->torso_x_rot, sizeof(int16_t));
    WriteSG(&lara->torso_z_rot, sizeof(int16_t));

    WriteSGARM(&lara->left_arm);
    WriteSGARM(&lara->right_arm);
    WriteSG(&lara->pistols, sizeof(AMMO_INFO));
    WriteSG(&lara->magnums, sizeof(AMMO_INFO));
    WriteSG(&lara->uzis, sizeof(AMMO_INFO));
    WriteSG(&lara->shotgun, sizeof(AMMO_INFO));
    WriteSGLOT(&lara->LOT);
}

void ResetSG()
{
    SGCount = 0;
    SGPoint = SaveGame[0].buffer;
}

void WriteSGARM(LARA_ARM *arm)
{
    int32_t frame_base = (size_t)arm->frame_base - (size_t)AnimFrames;
    WriteSG(&frame_base, sizeof(int32_t));
    WriteSG(&arm->frame_number, sizeof(int16_t));
    WriteSG(&arm->lock, sizeof(int16_t));
    WriteSG(&arm->y_rot, sizeof(PHD_ANGLE));
    WriteSG(&arm->x_rot, sizeof(PHD_ANGLE));
    WriteSG(&arm->z_rot, sizeof(PHD_ANGLE));
    WriteSG(&arm->flash_gun, sizeof(int16_t));
}

void WriteSGLOT(LOT_INFO *lot)
{
    // it casually saves a pointer again!
    WriteSG(&lot->node, sizeof(int32_t));

    WriteSG(&lot->head, sizeof(int16_t));
    WriteSG(&lot->tail, sizeof(int16_t));
    WriteSG(&lot->search_number, sizeof(uint16_t));
    WriteSG(&lot->block_mask, sizeof(uint16_t));
    WriteSG(&lot->step, sizeof(int16_t));
    WriteSG(&lot->drop, sizeof(int16_t));
    WriteSG(&lot->fly, sizeof(int16_t));
    WriteSG(&lot->zone_count, sizeof(int16_t));
    WriteSG(&lot->target_box, sizeof(int16_t));
    WriteSG(&lot->required_box, sizeof(int16_t));
    WriteSG(&lot->target, sizeof(PHD_VECTOR));
}

void T1MInjectGameSaveGame()
{
    INJECT(0x004344D0, InitialiseStartInfo);
    INJECT(0x00434520, ModifyStartInfo);
    INJECT(0x004345E0, CreateStartInfo)
    INJECT(0x00434720, CreateSaveGameInfo);
}
