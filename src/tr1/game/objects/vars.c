#include "game/objects/vars.h"

#include "global/const.h"

const GAME_OBJECT_ID g_EnemyObjects[] = {
    // clang-format off
    O_WOLF,
    O_BEAR,
    O_BAT,
    O_CROCODILE,
    O_ALLIGATOR,
    O_LION,
    O_LIONESS,
    O_PUMA,
    O_APE,
    O_RAT,
    O_VOLE,
    O_TREX,
    O_RAPTOR,
    O_WARRIOR_1,
    O_WARRIOR_2,
    O_WARRIOR_3,
    O_CENTAUR,
    O_MUMMY,
    O_TORSO,
    O_DINO_WARRIOR,
    O_FISH,
    O_LARSON,
    O_PIERRE,
    O_SKATEKID,
    O_COWBOY,
    O_BALDY,
    O_NATLA,
    O_SCION_ITEM_3,
    O_STATUE,
    NO_OBJECT,
    // clang-format on
};

const GAME_OBJECT_ID g_AllyObjects[] = {
    // clang-format off
    O_LARA,
    NO_OBJECT,
    // Lara's social skills: still loading...
    // clang-format on
};

const GAME_OBJECT_ID g_BossObjects[] = {
    // clang-format off
    O_TREX,
    O_LARSON,
    O_PIERRE,
    O_SKATEKID,
    O_COWBOY,
    O_BALDY,
    O_NATLA,
    O_TORSO,
    NO_OBJECT,
    // clang-format on
};

const GAME_OBJECT_ID g_PlaceholderObjects[] = {
    // clang-format off
    O_STATUE,
    O_PODS,
    O_BIG_POD,
    NO_OBJECT,
    // clang-format on
};

const GAME_OBJECT_ID g_PickupObjects[] = {
    // clang-format off
    O_PISTOL_ITEM,
    O_SHOTGUN_ITEM,
    O_MAGNUM_ITEM,
    O_UZI_ITEM,
    O_PISTOL_AMMO_ITEM,
    O_SG_AMMO_ITEM,
    O_MAG_AMMO_ITEM,
    O_UZI_AMMO_ITEM,
    O_MEDI_ITEM,
    O_BIGMEDI_ITEM,
    O_EXPLOSIVE_ITEM,
    O_PUZZLE_ITEM_1,
    O_PUZZLE_ITEM_2,
    O_PUZZLE_ITEM_3,
    O_PUZZLE_ITEM_4,
    O_KEY_ITEM_1,
    O_KEY_ITEM_2,
    O_KEY_ITEM_3,
    O_KEY_ITEM_4,
    O_PICKUP_ITEM_1,
    O_PICKUP_ITEM_2,
    O_LEADBAR_ITEM,
    O_SCION_ITEM_1,
    O_SCION_ITEM_2,
    NO_OBJECT,
    // clang-format on
};

const GAME_OBJECT_ID g_GunObjects[] = {
    // clang-format off
    O_PISTOL_ITEM,
    O_SHOTGUN_ITEM,
    O_MAGNUM_ITEM,
    O_UZI_ITEM,
    NO_OBJECT,
    // clang-format on
};

const GAME_OBJECT_ID g_DoorObjects[] = {
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

const GAME_OBJECT_ID g_TrapdoorObjects[] = {
    // clang-format off
    O_TRAPDOOR_1,
    O_TRAPDOOR_2,
    O_BIGTRAPDOOR,
    O_DRAWBRIDGE,
    NO_OBJECT,
    // clang-format on
};

const GAME_OBJECT_ID g_AnimObjects[] = {
    // clang-format off
    O_PISTOL_ANIM,
    O_SHOTGUN_ANIM,
    O_MAGNUM_ANIM,
    O_UZI_ANIM,
    O_HAIR,
    O_LARA_EXTRA,
    // clang-format on
};

const GAME_OBJECT_ID g_NullObjects[] = {
    // clang-format off
    O_EXPLOSION_1,
    O_EXPLOSION_2,
    O_SPLASH_1,
    O_SPLASH_2,
    O_BUBBLES_1,
    O_BUBBLES_2,
    O_BUBBLE_EMITTER,
    O_BLOOD_1,
    O_BLOOD_2,
    O_DART_EFFECT,
    O_RICOCHET_1,
    O_TWINKLE,
    O_GUN_FLASH,
    O_DUST,
    O_BODY_PART,
    O_CAMERA_TARGET,
    O_MISSILE_1,
    O_MISSILE_2,
    O_MISSILE_3,
    O_MISSILE_4,
    O_MISSILE_5,
    O_EARTHQUAKE,
    O_SKYBOX,
    O_ALPHABET,
    // clang-format on
};

const GAME_OBJECT_ID g_InvObjects[] = {
    // clang-format off
    O_PISTOL_OPTION,
    O_SHOTGUN_OPTION,
    O_MAGNUM_OPTION,
    O_UZI_OPTION,
    O_SG_AMMO_OPTION,
    O_MAG_AMMO_OPTION,
    O_UZI_AMMO_OPTION,
    O_EXPLOSIVE_OPTION,
    O_MEDI_OPTION,
    O_BIGMEDI_OPTION,
    O_PUZZLE_OPTION_1,
    O_PUZZLE_OPTION_2,
    O_PUZZLE_OPTION_3,
    O_PUZZLE_OPTION_4,
    O_LEADBAR_OPTION,
    O_KEY_OPTION_1,
    O_KEY_OPTION_2,
    O_KEY_OPTION_3,
    O_KEY_OPTION_4,
    O_PICKUP_OPTION_1,
    O_PICKUP_OPTION_2,
    O_SCION_OPTION,
    O_DETAIL_OPTION,
    O_SOUND_OPTION,
    O_CONTROL_OPTION,
    O_GAMMA_OPTION,
    O_PASSPORT_OPTION,
    O_MAP_OPTION,
    O_PHOTO_OPTION,
    NO_OBJECT,
    // clang-format on
};

const GAME_OBJECT_PAIR g_GunAmmoObjectMap[] = {
    // clang-format off
    { O_PISTOL_ITEM, O_PISTOL_AMMO_ITEM },
    { O_SHOTGUN_ITEM, O_SG_AMMO_ITEM },
    { O_MAGNUM_ITEM, O_MAG_AMMO_ITEM },
    { O_UZI_ITEM, O_UZI_AMMO_ITEM },
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};

const GAME_OBJECT_PAIR g_ItemToInvObjectMap[] = {
    // clang-format off
    { O_PISTOL_ITEM, O_PISTOL_OPTION },
    { O_SHOTGUN_ITEM, O_SHOTGUN_OPTION },
    { O_MAGNUM_ITEM, O_MAGNUM_OPTION },
    { O_UZI_ITEM, O_UZI_OPTION },
    { O_PISTOL_AMMO_ITEM, O_PISTOL_AMMO_OPTION },
    { O_SG_AMMO_ITEM, O_SG_AMMO_OPTION },
    { O_MAG_AMMO_ITEM, O_MAG_AMMO_OPTION },
    { O_UZI_AMMO_ITEM, O_UZI_AMMO_OPTION },
    { O_EXPLOSIVE_ITEM, O_EXPLOSIVE_OPTION },
    { O_MEDI_ITEM, O_MEDI_OPTION },
    { O_BIGMEDI_ITEM, O_BIGMEDI_OPTION },
    { O_PUZZLE_ITEM_1, O_PUZZLE_OPTION_1 },
    { O_PUZZLE_ITEM_2, O_PUZZLE_OPTION_2 },
    { O_PUZZLE_ITEM_3, O_PUZZLE_OPTION_3 },
    { O_PUZZLE_ITEM_4, O_PUZZLE_OPTION_4 },
    { O_LEADBAR_ITEM, O_LEADBAR_OPTION },
    { O_KEY_ITEM_1, O_KEY_OPTION_1 },
    { O_KEY_ITEM_2, O_KEY_OPTION_2 },
    { O_KEY_ITEM_3, O_KEY_OPTION_3 },
    { O_KEY_ITEM_4, O_KEY_OPTION_4 },
    { O_PICKUP_ITEM_1, O_PICKUP_OPTION_1 },
    { O_PICKUP_ITEM_2, O_PICKUP_OPTION_2 },
    { O_SCION_ITEM_1, O_SCION_OPTION },
    { O_SCION_ITEM_2, O_SCION_OPTION },
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};
