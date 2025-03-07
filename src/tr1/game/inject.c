#include <libtrx/config.h>
#include <libtrx/game/inject.h>

bool Inject_IsRelevant(const INJECTION *const injection)
{
    switch (injection->type) {
    case IFT_GENERAL:
    case IFT_LARA_ANIMS:
        return true;
    case IFT_BRAID:
        return g_Config.visuals.enable_braid;
    case IFT_UZI_SFX:
        return g_Config.audio.enable_ps_uzi_sfx;
    case IFT_FLOOR_DATA:
        return g_Config.gameplay.fix_floor_data_issues;
    case IFT_TEXTURE_FIX:
        return g_Config.visuals.fix_texture_issues;
    case IFT_ITEM_POSITION:
        return g_Config.visuals.fix_item_rots;
    case IFT_PS1_ENEMY:
        return g_Config.gameplay.restore_ps1_enemies;
    case IFT_DISABLE_ANIM_SPRITE:
        return !g_Config.visuals.fix_animated_sprites;
    case IFT_SKYBOX:
        return g_Config.visuals.enable_skybox;
    case IFT_PS1_CRYSTAL:
        return g_Config.gameplay.enable_save_crystals
            && g_Config.visuals.enable_ps1_crystals;
    default:
        return false;
    }
}
