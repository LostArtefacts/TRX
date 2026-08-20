#include <trx/game/objects/vars.h>

const GAME_OBJECT_PAIR g_KeyItemToReceptacleMap[] = {
#define X_RECEPTACLE(option, receptacle) { option, receptacle },
#include <trx/game/objects/pickups.def>
#undef X_RECEPTACLE
    // clang-format off
    { O_LEADBAR_OPTION, O_MIDAS_TOUCH },
    { O_KEY_OPTION_2, O_GONG },
    { O_KEY_OPTION_2, O_DETONATOR_BOX },
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};

const GAME_OBJECT_PAIR g_ReceptacleToReceptacleDoneMap[] = {
#define X_RECEPTACLE_DONE(receptacle, done) { receptacle, done },
#include <trx/game/objects/pickups.def>
#undef X_RECEPTACLE_DONE
    // clang-format off
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};

const OBJECT_ID g_ReceptacleObjects[] = {
#define X_RECEPTACLE(option, receptacle) receptacle,
#define X_RECEPTACLE_DONE(receptacle, done) done,
#include <trx/game/objects/pickups.def>
#undef X_RECEPTACLE_DONE
#undef X_RECEPTACLE
    // clang-format off
    O_MIDAS_TOUCH,
    O_GONG,
    O_DETONATOR_BOX,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_CreatureObjects[] = {
    // clang-format off
    O_ALLIGATOR,
    O_APE,
    O_ATLANTEAN_GROUND,
    O_ATLANTEAN_SHOOTER,
    O_ATLANTEAN_WINGED,
    O_BALDY,
    O_BANDIT_1,
    O_BANDIT_2,
    O_BANDIT_2B,
    O_BARRACUDA,
    O_BAT,
    O_BEAR,
    O_BOAR,
    O_BIG_EEL,
    O_BIG_SPIDER,
    O_BIRD_GUARDIAN,
    O_CENTAUR,
    O_CENTAUR_STATUE,
    O_CIVILIAN,
    O_CLAW_MUTANT,
    O_COBRA,
    O_COMPY,
    O_COWBOY,
    O_CRAWLER_MUTANT,
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
    O_DOLPHIN,
    O_DRAGON_FRONT,
    O_EAGLE,
    O_EEL,
    O_FISH,
    O_HUSKIE,
    O_HYBRID_MUTANT,
    O_JELLY,
    O_LARSON,
    O_LION,
    O_LIONESS,
    O_LIZARD,
    O_MONKEY,
    O_MONK_1,
    O_MONK_2,
    O_MONK_3,
    O_MOUSE,
    O_MP_1,
    O_MP_2,
    O_MUMMY,
    O_NATLA,
    O_ORCA,
    O_PATROL_DOG,
    O_PIERRE,
    O_PRISONER,
    O_PUMA,
    O_PUNK_1,
    O_PUNK_2,
    O_RAPTOR,
    O_RAT,
    O_RX_WORKER_1,
    O_RX_WORKER_2,
    O_RX_WORKER_3,
    O_SECURITY_GUARD,
    O_SENTRY_GUN,
    O_SHARK,
    O_SHIVA,
    O_SKATEKID,
    O_SKIDOO_DRIVER,
    O_SOPHIA,
    O_SPIDER,
    O_STHPAC_MERCENARY,
    O_SWAT_1,
    O_SWAT_2,
    O_SWAT_3,
    O_TIGER,
    O_TONY,
    O_TORSO,
    O_TREX,
    O_TREX_ALPHA,
    O_TRIBE_AXEMAN,
    O_TRIBE_BOSS,
    O_TRIBE_PIPEMAN,
    O_VOLE,
    O_VULTURE,
    O_WASP_MUTANT,
    O_WILLARD,
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

const OBJECT_ID g_ProjectileObjects[] = {
    // clang-format off
    O_HARPOON_BOLT,
    O_GRENADE,
    O_ROCKET,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_WaterObjects[] = {
    // clang-format off
    O_ALLIGATOR,
    O_BARRACUDA,
    O_BIG_EEL,
    O_DIVER,
    O_DOLPHIN,
    O_EEL,
    O_FISH,
    O_GENERAL,
    O_JELLY,
    O_PROPELLER_2,
    O_SHARK,
    O_VOLE,
    O_ORCA,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_LoyalObjects[] = {
    // clang-format off
    O_LARA,
    O_WINSTON,
    O_WINSTON_ARMY,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_BossObjects[] = {
    // clang-format off
    O_TREX,
    O_TREX_ALPHA,
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
    O_TONY,
    O_TRIBE_BOSS,
    O_SOPHIA,
    O_WILLARD,
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

const OBJECT_ID g_SecretObjects[] = {
    // clang-format off
    O_SECRET_1,
    O_SECRET_2,
    O_SECRET_3,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_QuestObjects[] = {
#define X_PICKUP_QUEST(n) O_QUEST_ITEM_##n,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_QUEST
    // clang-format off
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_PickupObjects[] = {
#define X_PICKUP(item, option) item,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP
    NO_OBJECT,
};

const OBJECT_ID g_ElevatedPickupObjects[] = {
    // clang-format off
    O_SCION_ITEM_1,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_SwitchObjects[] = {
    // clang-format off
    O_SWITCH_TYPE_AIRLOCK,
    O_SWITCH_TYPE_BUTTON,
    O_SWITCH_TYPE_CROWBAR,
    O_SWITCH_TYPE_FLOOR,
    O_SWITCH_TYPE_GENERIC_1,
    O_SWITCH_TYPE_GENERIC_2,
    O_SWITCH_TYPE_GENERIC_3,
    O_SWITCH_TYPE_GENERIC_4,
    O_SWITCH_TYPE_GENERIC_5,
    O_SWITCH_TYPE_GENERIC_6,
    O_SWITCH_TYPE_JUMP,
    O_SWITCH_TYPE_NORMAL,
    O_SWITCH_TYPE_PULLEY,
    O_SWITCH_TYPE_SMALL,
    O_SWITCH_TYPE_UW,
    O_SWITCH_TYPE_UW_CEILING,
    O_SWITCH_TYPE_WHEEL,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_GunObjects[] = {
#define X_PICKUP_GUN(item, option) item,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_GUN
    NO_OBJECT,
};

const OBJECT_ID g_GunAmmoObjects[] = {
#define X_PICKUP_GUN_AMMO(gun_item, item, option) item,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_GUN_AMMO
    NO_OBJECT,
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
    O_LARA_AUTOS,
    O_LARA_ROCKET_GUN,
    O_LARA_VEHICLE_ANIM,
    O_LARA_SHOTGUN,
    O_LARA_MAGNUMS,
    O_LARA_DESERT_EAGLE,
    O_LARA_UZIS,
    O_LARA_HARPOON_GUN,
    O_LARA_M16,
    O_LARA_MP5,
    O_LARA_GRENADE_GUN,
    O_LARA_CROSSBOW,
    O_LARA_REVOLVER,
    O_LARA_FLARE,
    O_LARA_HAIR,
    O_LARA_EXTRA,
    O_LARA_SKIDOO,
    O_LARA_BOAT,
    O_LARA_QUAD_BIKE,
    O_LARA_MOUNTED_GUN,
    O_LARA_KAYAK,
    O_LARA_UPV,
    O_LARA_RIB,
    O_LARA_MINE_CART,
    O_MESH_SWAP_1,
    O_MESH_SWAP_2,
    O_MESH_SWAP_3,
    O_CROWBAR_ANIM,
    O_LARA_SKIN_SWAP_1,
    O_LARA_SKIN_SWAP_2,
    O_LARA_SKIN_JOINTS_1,
    O_LARA_SKIN_JOINTS_2,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_NullObjects[] = {
    // clang-format off
    O_AI_AMBUSH,
    O_AI_FOLLOW,
    O_AI_GUARD,
    O_AI_MODIFY,
    O_AI_PATROL_1,
    O_AI_PATROL_2,
    O_AI_X1,
    O_AI_X2,
    O_AI_X3,
    O_ALPHABET,
    O_ALPHABET_SMALL,
    O_ASSAULT_DIGITS,
    O_BAT_GFX,
    O_BINOCULAR_GFX,
    O_BLOOD,
    O_BLOOD_PINK,
    O_BODY_PART,
    O_BUBBLE_1,
    O_BUBBLE_2,
    O_BUBBLE_EMITTER,
    O_CAMERA_TARGET,
    O_COMBAT_END,
    O_CROSSBOW_BOLT,
    O_CUT_SHOTGUN,
    O_DART,
    O_DART_EFFECT,
    O_DRAGON_BONES_1,
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
    O_GUN_SHELL,
    O_HARPOON_BOLT,
    O_HEAVY_ROCKET,
    O_HOT_LIQUID,
    O_INV_BACKGROUND,
    O_KILL_ALL_TRIGGERED,
    O_LENS_FLARE,
    O_M16_FLASH,
    O_MISSILE_ATLANTEAN_BOMB,
    O_MISSILE_ATLANTEAN_SHARD,
    O_MISSILE_FLAME,
    O_MISSILE_HARPOON,
    O_MISSILE_KNIFE,
    O_MISSILE_POISON,
    O_NATLA_GUN,
    O_PICKUP_AID,
    O_PIRAHNA_GFX,
    O_POISON_DART,
    O_RICOCHET,
    O_ROCKET,
    O_SHADOW,
    O_SHOTGUN_SHELL,
    O_SKYBOX,
    O_SNOWFLAKE,
    O_SNOW_SPRITE,
    O_SOPHIA_LASER_BOLT,
    O_SPARKS_GFX,
    O_SPHERE_OF_DOOM_1,
    O_SPHERE_OF_DOOM_2,
    O_SPHERE_OF_DOOM_3,
    O_SPLASH_1,
    O_SPLASH_2,
    O_TELEPORTER,
    O_TEXT_BOX,
    O_TONY_FIRE_BALL,
    O_TROPICAL_FISH_GFX,
    O_TWINKLE,
    O_WATER_SPRITE,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_InvObjects[] = {
#define X_PICKUP(item, option) option,
#define X_PICKUP_VARIANT(item, option)
#define X_PICKUP_GUN_AMMO_VARIANT(gun_item, item, option)
#define X_PICKUP_SECRET(item, option)
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_SECRET
#undef X_PICKUP_GUN_AMMO_VARIANT
#undef X_PICKUP_VARIANT
#undef X_PICKUP
    // clang-format off
    O_COMPASS_OPTION,
    O_STOPWATCH_OPTION,
    O_CONTROL_OPTION,
    O_DETAIL_OPTION,
    O_GAMMA_OPTION,
    O_GLOBE_SELECT_OPTION,
    O_PASSPORT_OPTION,
    O_PHOTO_OPTION,
    O_SOUND_OPTION,
    O_PDA_OPTION,
    O_PASSPORT_CLOSED,
    O_SAVE_CRYSTAL_OPTION,
    NO_OBJECT,
    // clang-format on
};

// Inventory ring options for generic pickups: shown with a quantity,
// selectable, but with no dedicated behavior of their own.
const OBJECT_ID g_GenericInvOptions[] = {
#define X_PICKUP_NUMBERED(item, option) option,
#define X_PICKUP_MISC(item, option) option,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_MISC
#undef X_PICKUP_NUMBERED
    NO_OBJECT,
};

const OBJECT_ID g_WaterSpriteObjects[] = {
    // clang-format off
    O_WATERFALL_MIST,
    O_SPLASH_1,
    O_SPLASH_2,
    O_BUBBLE_1,
    O_BUBBLE_2,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_GameSpriteObjects[] = {
#define X_PICKUP(item, option) item,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP
    // clang-format off
    O_EXPLOSION_1,
    O_EXPLOSION_2,
    O_MISSILE_FLAME,
    O_SPLASH_1,
    O_SPLASH_2,
    O_BUBBLE_1,
    O_BUBBLE_2,
    O_BLOOD,
    O_BLOOD_PINK,
    O_DART_EFFECT,
    O_RICOCHET,
    O_TWINKLE,
    O_DUST,
    O_EMBER,
    O_FLAME,
    O_PICKUP_AID,
    O_GLOW,
    O_WATER_SPRITE,
    O_SNOW_SPRITE,
    O_HOT_LIQUID,
    O_SHADOW,
    O_SPARKS_GFX,
    O_GLOW_RESERVED,
    O_FX_RESERVED,
    O_PIRAHNA_GFX,
    O_TROPICAL_FISH_GFX,
    O_BAT_GFX,
    O_ALPHABET,
    O_ALPHABET_SMALL,
    O_TEXT_BOX,
    O_ASSAULT_DIGITS,
    NO_OBJECT,
    // clang-format on
};

const GAME_OBJECT_PAIR g_GunAmmoObjectMap[] = {
#define X_PICKUP_GUN_AMMO(gun_item, item, option) { gun_item, item },
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_GUN_AMMO
    { NO_OBJECT, NO_OBJECT },
};

const GAME_OBJECT_PAIR g_ItemToInvObjectMap[] = {
#define X_PICKUP(item, option) { item, option },
#include <trx/game/objects/pickups.def>
#undef X_PICKUP
    // The crystal is collected by its own control routine rather than the
    // generic pickup code, so it is not part of pickups.def.
    { O_SAVE_CRYSTAL_ITEM, O_SAVE_CRYSTAL_OPTION },
    { NO_OBJECT, NO_OBJECT },
};

const OBJECT_ID g_ShatterableObjects[] = {
    // clang-format off
    O_SMASH_OBJECT_1,
    O_SMASH_OBJECT_4,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_HeavyShatterableObjects[] = {
    // clang-format off
    O_SMASH_OBJECT_2,
    O_SMASH_OBJECT_3,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_HeavyMissileObjects[] = {
    // clang-format off
    O_HEAVY_ROCKET,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_SmashableObjects[] = {
    // clang-format off
    O_BELL,
    O_SCION_ITEM_3,
    O_CARCASS,
    O_FUSE_BOX,
    NO_OBJECT,
    // clang-format on
};

const OBJECT_ID g_ShoalObjects[] = {
    // clang-format off
    O_TROPICAL_FISH,
    O_PIRAHNAS,
    NO_OBJECT,
    // clang-format on
};
