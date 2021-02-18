#include "game/data.h"
#include "game/effects.h"
#include "game/inv.h"
#include "game/lara.h"
#include "game/lot.h"
#include "mod.h"
#include "util.h"

void __cdecl InitialiseLara()
{
    TRACE("");
    LaraItem->collidable = 0;
    LaraItem->data = &Lara;
    LaraItem->hit_points = Tomb1MConfig.disable_healing_between_levels
        ? Tomb1MData.stored_lara_health
        : LARA_HITPOINTS;

    Lara.air = LARA_AIR;
    Lara.torso_y_rot = 0;
    Lara.torso_x_rot = 0;
    Lara.torso_z_rot = 0;
    Lara.head_y_rot = 0;
    Lara.head_x_rot = 0;
    Lara.head_z_rot = 0;
    Lara.calc_fallspeed = 0;
    Lara.mesh_effects = 0;
    Lara.hit_frames = 0;
    Lara.hit_direction = 0;
    Lara.death_count = 0;
    Lara.target = 0;
    Lara.spaz_effect = 0;
    Lara.spaz_effect_count = 0;
    Lara.turn_rate = 0;
    Lara.move_angle = 0;
    Lara.right_arm.flash_gun = 0;
    Lara.left_arm.flash_gun = 0;
    Lara.right_arm.lock = 0;
    Lara.left_arm.lock = 0;

    if (RoomInfo[LaraItem->room_number].flags & 1) {
        Lara.water_status = LWS_UNDERWATER;
        LaraItem->fall_speed = 0;
        LaraItem->goal_anim_state = AS_TREAD;
        LaraItem->current_anim_state = AS_TREAD;
        LaraItem->anim_number = AA_TREAD;
        LaraItem->frame_number = AF_TREAD;
    } else {
        Lara.water_status = LWS_ABOVEWATER;
        LaraItem->goal_anim_state = AS_STOP;
        LaraItem->current_anim_state = AS_STOP;
        LaraItem->anim_number = AA_STOP;
        LaraItem->frame_number = AF_STOP;
    }

    Lara.current_active = 0;

    InitialiseLOT(&Lara.LOT);
    Lara.LOT.step = WALL_L * 20;
    Lara.LOT.drop = -WALL_L * 20;
    Lara.LOT.fly = STEP_L;

    InitialiseLaraInventory(CurrentLevel);
}

void __cdecl UseItem(__int16 object_num)
{
    TRACE("%d", object_num);
    switch (object_num) {
    case O_GUN_ITEM:
    case O_GUN_OPTION:
        Lara.request_gun_type = LGT_PISTOLS;
        if (!Lara.gun_status && Lara.gun_type == LGT_PISTOLS) {
            Lara.gun_type = LGT_UNARMED;
        }
        break;
    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        Lara.request_gun_type = LGT_SHOTGUN;
        if (!Lara.gun_status && Lara.gun_type == LGT_SHOTGUN) {
            Lara.gun_type = LGT_UNARMED;
        }
        break;
    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        Lara.request_gun_type = LGT_MAGNUMS;
        if (!Lara.gun_status && Lara.gun_type == LGT_MAGNUMS) {
            Lara.gun_type = LGT_UNARMED;
        }
        break;
    case O_UZI_ITEM:
    case O_UZI_OPTION:
        Lara.request_gun_type = LGT_UZIS;
        if (!Lara.gun_status && Lara.gun_type == LGT_UZIS) {
            Lara.gun_type = LGT_UNARMED;
        }
        break;
    case O_MEDI_ITEM:
    case O_MEDI_OPTION:
        if (LaraItem->hit_points <= 0
            || LaraItem->hit_points >= LARA_HITPOINTS) {
            return;
        }
        LaraItem->hit_points += LARA_HITPOINTS / 2;
        if (LaraItem->hit_points > LARA_HITPOINTS)
            LaraItem->hit_points = LARA_HITPOINTS;
        Inv_RemoveItem(O_MEDI_ITEM);
        SoundEffect(116, 0, SFX_ALWAYS);
        break;
    case O_BIGMEDI_ITEM:
    case O_BIGMEDI_OPTION:
        if (LaraItem->hit_points > 0 && LaraItem->hit_points < LARA_HITPOINTS) {
            LaraItem->hit_points = LaraItem->hit_points + LARA_HITPOINTS;
            if (LaraItem->hit_points > LARA_HITPOINTS)
                LaraItem->hit_points = LARA_HITPOINTS;
            Inv_RemoveItem(O_BIGMEDI_ITEM);
            SoundEffect(116, 0, SFX_ALWAYS);
        }
        break;
    }
}

void Tomb1MInjectGameLaraMisc()
{
    INJECT(0x00428020, InitialiseLara);
    INJECT(0x00427E80, UseItem);
}
