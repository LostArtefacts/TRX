#include "game/objects/vars.h"

const GAME_OBJECT_PAIR g_KeyItemToReceptacleMap[] = {
    // clang-format off
    { O_KEY_OPTION_1, O_KEY_HOLE_1 },
    { O_KEY_OPTION_2, O_KEY_HOLE_2 },
    { O_KEY_OPTION_3, O_KEY_HOLE_3 },
    { O_KEY_OPTION_4, O_KEY_HOLE_4 },
    { O_PUZZLE_OPTION_1, O_PUZZLE_HOLE_1 },
    { O_PUZZLE_OPTION_2, O_PUZZLE_HOLE_2 },
    { O_PUZZLE_OPTION_3, O_PUZZLE_HOLE_3 },
    { O_PUZZLE_OPTION_4, O_PUZZLE_HOLE_4 },
    { O_LEADBAR_OPTION, O_MIDAS_TOUCH },
    { O_KEY_OPTION_2, O_GONG },
    { O_KEY_OPTION_2, O_DETONATOR_BOX },
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};

const GAME_OBJECT_PAIR g_ReceptacleToReceptacleDoneMap[] = {
    // clang-format off
    { O_PUZZLE_HOLE_1, O_PUZZLE_DONE_1 },
    { O_PUZZLE_HOLE_2, O_PUZZLE_DONE_2 },
    { O_PUZZLE_HOLE_3, O_PUZZLE_DONE_3 },
    { O_PUZZLE_HOLE_4, O_PUZZLE_DONE_4 },
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};

const OBJECT_ID g_ReceptacleObjects[] = {
    // clang-format off
    O_KEY_HOLE_1,
    O_KEY_HOLE_2,
    O_KEY_HOLE_3,
    O_KEY_HOLE_4,
    O_PUZZLE_HOLE_1,
    O_PUZZLE_HOLE_2,
    O_PUZZLE_HOLE_3,
    O_PUZZLE_HOLE_4,
    O_PUZZLE_DONE_1,
    O_PUZZLE_DONE_2,
    O_PUZZLE_DONE_3,
    O_PUZZLE_DONE_4,
    O_MIDAS_TOUCH,
    O_GONG,
    O_DETONATOR_BOX,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_EnemyObjects[] = {
    // clang-format off
    O_ALLIGATOR,
    O_APE,
    O_BALDY,
    O_BANDIT_1,
    O_BANDIT_2,
    O_BANDIT_2B,
    O_BARRACUDA,
    O_BAT,
    O_BEAR,
    O_BIG_EEL,
    O_BIG_SPIDER,
    O_BIRD_GUARDIAN,
    O_CENTAUR,
    O_CENTAUR_STATUE,
    O_COWBOY,
    O_CROCODILE,
    O_CROW,
    O_CULT_1,
    O_CULT_1A,
    O_CULT_1B,
    O_CULT_2,
    O_CULT_3,
    O_DINO_WARRIOR,
    O_DIVER,
    O_DOG,
    O_DRAGON_FRONT,
    O_EAGLE,
    O_EEL,
    O_FISH,
    O_JELLY,
    O_LARSON,
    O_LION,
    O_LIONESS,
    O_MOUSE,
    O_MUMMY,
    O_NATLA,
    O_PIERRE,
    O_PUMA,
    O_RAPTOR,
    O_RAT,
    O_SHARK,
    O_SKATEKID,
    O_SKIDOO_DRIVER,
    O_SPIDER,
    O_TIGER,
    O_TORSO,
    O_TREX,
    O_VOLE,
    O_WARRIOR_1,
    O_WARRIOR_2,
    O_WARRIOR_3,
    O_WOLF,
    O_WORKER_1,
    O_WORKER_2,
    O_WORKER_3,
    O_WORKER_4,
    O_WORKER_5,
    O_XIAN_KNIGHT,
    O_XIAN_KNIGHT_STATUE,
    O_XIAN_SPEARMAN,
    O_XIAN_SPEARMAN_STATUE,
    O_YETI,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_WaterObjects[] = {
    // clang-format off
    O_ALLIGATOR,
    O_BARRACUDA,
    O_BIG_EEL,
    O_DIVER,
    O_EEL,
    O_FISH,
    O_GENERAL,
    O_JELLY,
    O_PROPELLER_2,
    O_SHARK,
    O_VOLE,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_AllyObjects[] = {
    // clang-format off
    O_MONK_1,
    O_MONK_2,
    O_MONK_3,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_LoyalObjects[] = {
    // clang-format off
    O_LARA,
    O_WINSTON,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_AllyTargetingEnemies[] = {
    // clang-format off
    O_BANDIT_1,
    O_BANDIT_2,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_BossObjects[] = {
    // clang-format off
    O_TREX,
    O_LARSON,
    O_PIERRE,
    O_SKATEKID,
    O_COWBOY,
    O_BALDY,
    O_NATLA,
    O_TORSO,
    O_CULT_3,
    O_DRAGON_FRONT,
    O_BARTOLI,
    O_BIRD_GUARDIAN,
    O_SKIDOO_DRIVER,
    O_SKIDOO_ARMED,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_MovableBlockObjects[] = {
    // clang-format off
    O_MOVABLE_BLOCK_1,
    O_MOVABLE_BLOCK_2,
    O_MOVABLE_BLOCK_3,
    O_MOVABLE_BLOCK_4,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_PlaceholderObjects[] = {
    // clang-format off
    O_CENTAUR_STATUE,
    O_PODS,
    O_BIG_POD,
    O_BARTOLI,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_SecretObjects[] = {
    // clang-format off
    O_SECRET_1,
    O_SECRET_2,
    O_SECRET_3,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_PickupObjects[] = {
    // clang-format off
    O_PISTOL_ITEM,
    O_PISTOL_AMMO_ITEM,
    O_SHOTGUN_ITEM,
    O_SHOTGUN_AMMO_ITEM,
    O_MAGNUM_ITEM,
    O_MAGNUM_AMMO_ITEM,
    O_UZI_AMMO_ITEM,
    O_UZI_ITEM,
    O_HARPOON_ITEM,
    O_HARPOON_AMMO_ITEM,
    O_M16_ITEM,
    O_M16_AMMO_ITEM,
    O_GRENADE_ITEM,
    O_GRENADE_AMMO_ITEM,
    O_EXPLOSIVE_ITEM,
    O_SMALL_MEDIPACK_ITEM,
    O_LARGE_MEDIPACK_ITEM,
    O_FLARES_ITEM,
    O_FLARE_ITEM,
    O_KEY_ITEM_1,
    O_KEY_ITEM_2,
    O_KEY_ITEM_3,
    O_KEY_ITEM_4,
    O_PICKUP_ITEM_1,
    O_PICKUP_ITEM_2,
    O_PUZZLE_ITEM_1,
    O_PUZZLE_ITEM_2,
    O_PUZZLE_ITEM_3,
    O_PUZZLE_ITEM_4,
    O_LEADBAR_ITEM,
    O_SCION_ITEM_1,
    O_SCION_ITEM_2,
    O_SECRET_1,
    O_SECRET_2,
    O_SECRET_3,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_SwitchObjects[] = {
    // clang-format off
    O_SWITCH_TYPE_AIRLOCK,
    O_SWITCH_TYPE_BUTTON,
    O_SWITCH_TYPE_NORMAL,
    O_SWITCH_TYPE_SMALL,
    O_SWITCH_TYPE_UW,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_GunObjects[] = {
    // clang-format off
    O_PISTOL_ITEM,
    O_SHOTGUN_ITEM,
    O_MAGNUM_ITEM,
    O_UZI_ITEM,
    O_HARPOON_ITEM,
    O_M16_ITEM,
    O_GRENADE_ITEM,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_DoorObjects[] = {
    // clang-format off
    O_DOOR_TYPE_1,
    O_DOOR_TYPE_2,
    O_DOOR_TYPE_3,
    O_DOOR_TYPE_4,
    O_DOOR_TYPE_5,
    O_DOOR_TYPE_6,
    O_DOOR_TYPE_7,
    O_DOOR_TYPE_8,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_TrapdoorObjects[] = {
    // clang-format off
    O_TRAPDOOR_TYPE_1,
    O_TRAPDOOR_TYPE_2,
    O_TRAPDOOR_TYPE_3,
    O_DRAWBRIDGE,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_AnimObjects[] = {
    // clang-format off
    O_LARA_PISTOLS,
    O_LARA_SHOTGUN,
    O_LARA_MAGNUMS,
    O_LARA_UZIS,
    O_LARA_HARPOON,
    O_LARA_M16,
    O_LARA_GRENADE,
    O_LARA_FLARE,
    O_LARA_HAIR,
    O_LARA_EXTRA,
    O_LARA_SKIDOO,
    O_LARA_BOAT,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_NullObjects[] = {
    // clang-format off
    O_ALPHABET,
    O_ASSAULT_DIGITS,
    O_BLOOD_1,
    O_BLOOD_2,
    O_BODY_PART,
    O_BUBBLE_1,
    O_BUBBLE_2,
    O_BUBBLE_EMITTER,
    O_CAMERA_TARGET,
    O_COMBAT_END,
    O_CUT_SHOTGUN,
    O_DART_EFFECT,
    O_DRAGON_BONES_2,
    O_DRAGON_BONES_3,
    O_DUST,
    O_EARTHQUAKE,
    O_EXPLOSION_1,
    O_EXPLOSION_2,
    O_FLARE_FIRE,
    O_FLARE_ITEM,
    O_FX_RESERVED,
    O_GLOW,
    O_GLOW_RESERVED,
    O_GONG_BONGER,
    O_GRENADE,
    O_GUN_FLASH,
    O_HARPOON_BOLT,
    O_HOT_LIQUID,
    O_INV_BACKGROUND,
    O_M16_FLASH,
    O_MISSILE_1,
    O_MISSILE_2,
    O_MISSILE_3,
    O_MISSILE_4,
    O_MISSILE_5,
    O_MISSILE_FLAME,
    O_MISSILE_HARPOON,
    O_MISSILE_KNIFE,
    O_PICKUP_AID,
    O_RICOCHET,
    O_SKYBOX,
    O_SNOW_SPRITE,
    O_SPHERE_OF_DOOM_1,
    O_SPHERE_OF_DOOM_2,
    O_SPHERE_OF_DOOM_3,
    O_SPLASH_1,
    O_SPLASH_2,
    O_TEXT_BOX,
    O_TWINKLE,
    O_WATER_SPRITE,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_InvObjects[] = {
    // clang-format off
    O_PISTOL_OPTION,
    O_PISTOL_AMMO_OPTION,
    O_MAGNUM_OPTION,
    O_MAGNUM_AMMO_OPTION,
    O_SHOTGUN_OPTION,
    O_SHOTGUN_AMMO_OPTION,
    O_UZI_OPTION,
    O_UZI_AMMO_OPTION,
    O_HARPOON_OPTION,
    O_HARPOON_AMMO_OPTION,
    O_M16_OPTION,
    O_M16_AMMO_OPTION,
    O_GRENADE_OPTION,
    O_GRENADE_AMMO_OPTION,
    O_EXPLOSIVE_OPTION,
    O_SMALL_MEDIPACK_OPTION,
    O_LARGE_MEDIPACK_OPTION,
    O_FLARES_OPTION,
    O_PUZZLE_OPTION_1,
    O_PUZZLE_OPTION_2,
    O_PUZZLE_OPTION_3,
    O_PUZZLE_OPTION_4,
    O_KEY_OPTION_1,
    O_KEY_OPTION_2,
    O_KEY_OPTION_3,
    O_KEY_OPTION_4,
    O_PICKUP_OPTION_1,
    O_PICKUP_OPTION_2,
    O_COMPASS_OPTION,
    O_CONTROL_OPTION,
    O_DETAIL_OPTION,
    O_GAMMA_OPTION,
    O_LEADBAR_OPTION,
    O_PASSPORT_OPTION,
    O_PHOTO_OPTION,
    O_SCION_OPTION,
    O_SOUND_OPTION,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_WaterSpriteObjects[] = {
    // clang-format off
    O_WATERFALL,
    O_SPLASH_1,
    O_SPLASH_2,
    O_BUBBLE_1,
    O_BUBBLE_2,
    NO_OBJECT,
    // clang-format on
};

const GAME_OBJECT_PAIR g_GunAmmoObjectMap[] = {
    // clang-format off
    { O_PISTOL_ITEM, O_PISTOL_AMMO_ITEM },
    { O_SHOTGUN_ITEM, O_SHOTGUN_AMMO_ITEM },
    { O_MAGNUM_ITEM, O_MAGNUM_AMMO_ITEM },
    { O_UZI_ITEM, O_UZI_AMMO_ITEM },
    { O_HARPOON_ITEM, O_HARPOON_AMMO_ITEM },
    { O_M16_ITEM, O_M16_AMMO_ITEM },
    { O_GRENADE_ITEM, O_GRENADE_AMMO_ITEM },
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};

const GAME_OBJECT_PAIR g_ItemToInvObjectMap[] = {
    // clang-format off
    { O_COMPASS_ITEM, O_COMPASS_OPTION },
    { O_PISTOL_ITEM, O_PISTOL_OPTION },
    { O_SHOTGUN_ITEM, O_SHOTGUN_OPTION },
    { O_MAGNUM_ITEM, O_MAGNUM_OPTION },
    { O_UZI_ITEM, O_UZI_OPTION },
    { O_HARPOON_ITEM, O_HARPOON_OPTION },
    { O_M16_ITEM, O_M16_OPTION },
    { O_GRENADE_ITEM, O_GRENADE_OPTION },
    { O_PISTOL_AMMO_ITEM, O_PISTOL_AMMO_OPTION },
    { O_SHOTGUN_AMMO_ITEM, O_SHOTGUN_AMMO_OPTION },
    { O_MAGNUM_AMMO_ITEM, O_MAGNUM_AMMO_OPTION },
    { O_UZI_AMMO_ITEM, O_UZI_AMMO_OPTION },
    { O_HARPOON_AMMO_ITEM,O_HARPOON_AMMO_OPTION },
    { O_M16_AMMO_ITEM, O_M16_AMMO_OPTION },
    { O_GRENADE_AMMO_ITEM, O_GRENADE_AMMO_OPTION },
    { O_EXPLOSIVE_ITEM, O_EXPLOSIVE_OPTION },
    { O_SMALL_MEDIPACK_ITEM, O_SMALL_MEDIPACK_OPTION },
    { O_LARGE_MEDIPACK_ITEM, O_LARGE_MEDIPACK_OPTION },
    { O_FLARE_ITEM, O_FLARES_OPTION },
    { O_FLARES_ITEM, O_FLARES_OPTION },
    { O_PUZZLE_ITEM_1, O_PUZZLE_OPTION_1 },
    { O_PUZZLE_ITEM_2, O_PUZZLE_OPTION_2 },
    { O_PUZZLE_ITEM_3, O_PUZZLE_OPTION_3 },
    { O_PUZZLE_ITEM_4, O_PUZZLE_OPTION_4 },
    { O_KEY_ITEM_1, O_KEY_OPTION_1 },
    { O_KEY_ITEM_2, O_KEY_OPTION_2 },
    { O_KEY_ITEM_3, O_KEY_OPTION_3 },
    { O_KEY_ITEM_4, O_KEY_OPTION_4 },
    { O_PICKUP_ITEM_1, O_PICKUP_OPTION_1 },
    { O_PICKUP_ITEM_2, O_PICKUP_OPTION_2 },
    { O_LEADBAR_ITEM, O_LEADBAR_OPTION },
    { O_SCION_ITEM_1, O_SCION_OPTION },
    { O_SCION_ITEM_2, O_SCION_OPTION },
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};

const OBJECT_ID g_ShatterableObjects[] = {
    // clang-format off
    O_WINDOW_1,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_SmashableObjects[] = {
    // clang-format off
    O_BELL,
    NO_OBJECT,
    // clang-format on
};
