// Type information for the IDA importer.
//
// This is unrelated to the Tomb1Main codebase which is a subject to changes
// and improvements with regard to the original game. This file documents the
// TombATI.exe layout.

#pragma pack(push, 8)
typedef unsigned int C3D_BOOL;
typedef int C3D_INT32;
typedef unsigned int C3D_UINT32;
typedef unsigned short C3D_UINT16;
typedef unsigned char C3D_UINT8;
typedef float C3D_FLOAT32;

typedef unsigned int *C3D_PBOOL;
typedef int *C3D_PINT32;
typedef unsigned int *C3D_PUINT32;
typedef unsigned short *C3D_PUINT16;
typedef unsigned char *C3D_PUINT8;
typedef float *C3D_PFLOAT32;
typedef void *C3D_PVOID;

typedef enum {
    C3D_EC_OK = 0,
    C3D_EC_GENFAIL = 1,
    C3D_EC_MEMALLOCFAIL = 2,
    C3D_EC_BADPARAM = 3,
    C3D_EC_UNUSED0 = 4,
    C3D_EC_BADSTATE = 5,
    C3D_EC_NOTIMPYET = 6,
    C3D_EC_UNUSED1 = 7,
    C3D_EC_CHIPCAPABILITY = 8,
    C3D_EC_NUM = 9,
    C3D_EC_FORCE_U32 = 0x7FFFFFFF
} C3D_EC,
    *C3D_PEC;

typedef struct {
    C3D_INT32 top;
    C3D_INT32 left;
    C3D_INT32 bottom;
    C3D_INT32 right;
} C3D_RECT, *C3D_PRECT;

typedef union {
    struct {
        unsigned r : 8;
        unsigned g : 8;
        unsigned b : 8;
        unsigned a : 8;
    };
    C3D_UINT32 u32All;
} C3D_COLOR, *C3D_PCOLOR;

#define C3D_LOAD_PALETTE_ENTRY 0x40
#define C3D_NO_LOAD_PALETTE_ENTRY 0x00

typedef union {
    struct {
        unsigned r : 8;
        unsigned g : 8;
        unsigned b : 8;
        unsigned flags : 8;
    };
    C3D_UINT32 u32All;
} C3D_PALETTENTRY, *C3D_PPALETTENTRY;

typedef struct {
    C3D_UINT16 ul;
    C3D_UINT16 ur;
    C3D_UINT16 ll;
    C3D_UINT16 lr;
} C3D_CODEBOOKENTRY, *C3D_PCODEBOOKENTRY;

#define C3D_CAPS1_BASE 0x00000000
#define C3D_CAPS1_FOG 0x00000001
#define C3D_CAPS1_POINT 0x00000002
#define C3D_CAPS1_RECT 0x00000004
#define C3D_CAPS1_Z_BUFFER 0x00000008
#define C3D_CAPS1_CI4_TMAP 0x00000010
#define C3D_CAPS1_CI8_TMAP 0x00000020
#define C3D_CAPS1_LOAD_OBJECT 0x00000040
#define C3D_CAPS1_DITHER_EN 0x00000080
#define C3D_CAPS1_ENH_PERSP 0x00000100
#define C3D_CAPS1_SCISSOR 0x00000200
#define C3D_CAPS1_PROFILE_IF 0x01000000
#define C3D_CAPS2_TEXTURE_COMPOSITE 0x00000001
#define C3D_CAPS2_TEXTURE_CLAMP 0x00000002
#define C3D_CAPS2_DESTINATION_ALPHA_BLEND 0x00000004
#define C3D_CAPS2_TEXTURE_TILING 0x00000008

typedef struct {
    uint32_t u32Size;
    uint32_t u32FrameBuffBase;
    uint32_t u32OffScreenHeap;
    uint32_t u32OffScreenSize;
    uint32_t u32TotalRAM;
    uint32_t u32ASICID;
    uint32_t u32ASICRevision;
    uint32_t u32CIFCaps1;
    uint32_t u32CIFCaps2;
    uint32_t u32CIFCaps3;
    uint32_t u32CIFCaps4;
    uint32_t u32CIFCaps5;
} C3D_3DCIFINFO, *PC3D_3DCIFINFO;

typedef enum {
    C3D_EV_VTCF = 3,
    C3D_EV_TLVERTEX = 4,
    C3D_EV_NUM = 5,
    C3D_EV_FORCE_U32 = 0x7FFFFFFF
} C3D_EVERTEX,
    *C3D_PEVERTEX;

typedef struct {
    C3D_FLOAT32 x, y, z;
    C3D_FLOAT32 s, t, w;
    C3D_FLOAT32 r, g, b, a;
} C3D_VTCF, *C3D_PVTCF;

typedef struct {
    union {
        C3D_FLOAT32 sx;
        C3D_FLOAT32 x;
    };
    union {
        C3D_FLOAT32 sy;
        C3D_FLOAT32 y;
    };
    union {
        C3D_FLOAT32 sz;
        C3D_FLOAT32 z;
    };
    union {
        C3D_FLOAT32 rhw;
        C3D_FLOAT32 w;
    };
    union {
        C3D_UINT32 color;
        struct {
            C3D_UINT8 b;
            C3D_UINT8 g;
            C3D_UINT8 r;
            C3D_UINT8 a;
        };
    };
    union {
        C3D_UINT32 specular;
        struct {
            C3D_UINT8 spec_b;
            C3D_UINT8 spec_g;
            C3D_UINT8 spec_r;
            C3D_UINT8 spec_a;
        };
    };
    union {
        C3D_FLOAT32 tu;
        C3D_FLOAT32 s;
    };
    union {
        C3D_FLOAT32 tv;
        C3D_FLOAT32 t;
    };
    struct {
        C3D_FLOAT32 rhw;
        C3D_FLOAT32 tu;
        C3D_FLOAT32 tv;
    } composite;
} C3D_TLVERTEX;

typedef void *C3D_VSTRIP;
typedef void **C3D_VLIST;
typedef void **C3D_PVARRAY;

typedef enum {
    C3D_EPRIM_LINE = 0,
    C3D_EPRIM_TRI = 1,
    C3D_EPRIM_QUAD = 2,
    C3D_EPRIM_RECT = 3,
    C3D_EPRIM_POINT = 4,
    C3D_EPRIM_NUM = 5,
    C3D_EPRIM_FORCE_U32 = 0x7FFFFFFF
} C3D_EPRIM,
    *C3D_PEPRIM;

typedef enum {
    C3D_ESH_NONE = 0,
    C3D_ESH_SOLID = 1,
    C3D_ESH_FLAT = 2,
    C3D_ESH_SMOOTH = 3,
    C3D_ESH_NUM = 4,
    C3D_ESH_FORCE_U32 = 0x7FFFFFFF
} C3D_ESHADE,
    *C3D_PESHADE;

typedef enum {
    C3D_EASRC_ZERO = 0,
    C3D_EASRC_ONE = 1,
    C3D_EASRC_DSTCLR = 2,
    C3D_EASRC_INVDSTCLR = 3,
    C3D_EASRC_SRCALPHA = 4,
    C3D_EASRC_INVSRCALPHA = 5,
    C3D_EASRC_DSTALPHA = 6,
    C3D_EASRC_INVDSTALPHA = 7,
    C3D_EASRC_NUM = 8,
    C3D_EASRC_FORCE_U32 = 0x7FFFFFFF
} C3D_EASRC,
    *C3D_PEASRC;

typedef enum {
    C3D_EADST_ZERO = 0,
    C3D_EADST_ONE = 1,
    C3D_EADST_SRCCLR = 2,
    C3D_EADST_INVSRCCLR = 3,
    C3D_EADST_SRCALPHA = 4,
    C3D_EADST_INVSRCALPHA = 5,
    C3D_EADST_DSTALPHA = 6,
    C3D_EADST_INVDSTALPHA = 7,
    C3D_EADST_NUM = 8,
    C3D_EADST_FORCE_U32 = 0x7FFFFFFF
} C3D_EADST,
    *C3D_PEADST;

typedef enum {
    C3D_EASEL_ZERO = 0,
    C3D_EASEL_ONE = 1,
    C3D_EASEL_SRCALPHA = 4,
    C3D_EASEL_INVSRCALPHA = 5,
    C3D_EASEL_DSTALPHA = 6,
    C3D_EASEL_INVDSTALPHA = 7,
    C3D_EASEL_FORCE_U32 = 0x7FFFFFFF
} C3D_EASEL,
    *C3D_PEASEL;

typedef enum {
    C3D_EACMP_NEVER = 0,
    C3D_EACMP_LESS = 1,
    C3D_EACMP_LEQUAL = 2,
    C3D_EACMP_EQUAL = 3,
    C3D_EACMP_GEQUAL = 4,
    C3D_EACMP_GREATER = 5,
    C3D_EACMP_NOTEQUAL = 6,
    C3D_EACMP_ALWAYS = 7,
    C3D_EACMP_MAX = 8,
    C3D_EACMP_FORCE_U32 = 0x7FFFFFFF
} C3D_EACMP,
    *C3D_PEACMP;

typedef enum {
    C3D_ETEXTILE_DEFAULT = 0,
    C3D_ETEXTILE_OFF = 1,
    C3D_ETEXTILE_ON = 2,
    C3D_ETEXTILE_MAX = 3,
    C3D_ETEXTILE_FORCE_U32 = 0x7FFFFFFF
} C3D_ETEXTILE,
    *C3D_PETEXTILE;

typedef void *C3D_HTX;
typedef C3D_HTX *C3D_PHTX;
typedef void *C3D_HTXPAL;
typedef C3D_HTXPAL *C3D_PHTXPAL;

typedef enum {
    C3D_ECI_TMAP_TRUE_COLOR = 0,
    C3D_ECI_TMAP_4BIT_HI = 1,
    C3D_ECI_TMAP_4BIT_LOW = 2,
    C3D_ECI_TMAP_8BIT = 3,
    C3D_ECI_TMAP_VQ = 4,
    C3D_ECI_TMAP_NUM = 5,
    C3D_ECI_TMAP_FORCE_U32 = 0x7FFFFFFF
} C3D_ECI_TMAP_TYPE;

typedef enum {
    C3D_ETL_NONE = 0,
    C3D_ETL_MODULATE = 1,
    C3D_ETL_ALPHA_DECAL = 2,
    C3D_ETL_NUM = 3,
    C3D_ETL_FORCE_U32 = 0x7FFFFFFF
} C3D_ETLIGHT,
    *C3D_PETLIGHT;

typedef enum {
    C3D_ETPC_NONE = 0,
    C3D_ETPC_ONE = 1,
    C3D_ETPC_TWO = 2,
    C3D_ETPC_THREE = 3,
    C3D_ETPC_FOUR = 4,
    C3D_ETPC_FIVE = 5,
    C3D_ETPC_SIX = 6,
    C3D_ETPC_SEVEN = 7,
    C3D_ETPC_EIGHT = 8,
    C3D_ETPC_NINE = 9,
    C3D_ETPC_NUM = 10,
    C3D_ETPC_FORCE_U32 = 0x7FFFFFFF
} C3D_ETPERSPCOR,
    *C3D_PETPERSPCOR;

typedef enum {
    C3D_ETFILT_MINPNT_MAGPNT = 0,
    C3D_ETFILT_MINPNT_MAG2BY2 = 1,
    C3D_ETFILT_MIN2BY2_MAG2BY2 = 2,
    C3D_ETFILT_MIPLIN_MAGPNT = 3,
    C3D_ETFILT_MIPLIN_MAG2BY2 = 4,
    C3D_ETFILT_MIPTRI_MAG2BY2 = 5,
    C3D_ETFILT_MIN2BY2_MAGPNT = 6,
    C3D_ETFILT_NUM = 7,
    C3D_ETFILT_FORCE_U32 = 0x7FFFFFFF
} C3D_ETEXFILTER,
    *C3D_PETEXFILTER;

typedef enum {
    C3D_ETEXOP_NONE = 0,
    C3D_ETEXOP_CHROMAKEY = 1,
    C3D_ETEXOP_ALPHA = 2,
    C3D_ETEXOP_ALPHA_MASK = 3,
    C3D_ETEXOP_NUM = 4,
    C3D_ETEXOP_FORCE_U32 = 0x7FFFFFFF
} C3D_ETEXOP,
    *C3D_PETEXOP;

typedef enum {
    C3D_EPF_RGB1555 = 3,
    C3D_EPF_RGB565 = 4,
    C3D_EPF_RGB8888 = 6,
    C3D_EPF_RGB332 = 7,
    C3D_EPF_Y8 = 8,
    C3D_EPF_YUV422 = 11,
    C3D_EPF_FORCE_U32 = 0x7FFFFFFF
} C3D_EPIXFMT,
    *C3D_PEPIXFMT;

typedef enum {
    C3D_ETF_CI4 = 1,
    C3D_ETF_CI8 = 2,
    C3D_ETF_RGB1555 = 3,
    C3D_ETF_RGB565 = 4,
    C3D_ETF_RGB8888 = 6,
    C3D_ETF_RGB332 = 7,
    C3D_ETF_Y8 = 8,
    C3D_ETF_YUV422 = 11,
    C3D_ETF_RGB4444 = 15,
    C3D_ETF_VQ = 20,
    C3D_ETF_FORCE_U32 = 0x7FFFFFFF
} C3D_ETEXFMT,
    *C3D_PETEXFMT;

#define cu32MAX_TMAP_LEV 11

typedef struct {
    C3D_UINT32 u32Size;
    C3D_BOOL bMipMap;
    C3D_PVOID apvLevels[cu32MAX_TMAP_LEV];
    C3D_UINT32 u32MaxMapXSizeLg2;
    C3D_UINT32 u32MaxMapYSizeLg2;
    C3D_ETEXFMT eTexFormat;
    C3D_COLOR clrTexChromaKey;
    C3D_HTXPAL htxpalTexPalette;
    C3D_BOOL bClampS;
    C3D_BOOL bClampT;
    C3D_BOOL bAlphaBlend;
} C3D_TMAP, *C3D_PTMAP;

typedef enum {
    C3D_ETEXCOMPFCN_BLEND = 0,
    C3D_ETEXCOMPFCN_MOD = 1,
    C3D_ETEXCOMPFCN_ADD_SPEC = 2,
    C3D_ETEXCOMPFCN_MAX = 3,
    C3D_ETEXCOMPFCN_FORCE_U32 = 0x7FFFFFFF
} C3D_ETEXCOMPFCN,
    *C3D_PETEXCOMPFCN;

typedef enum {
    C3D_EZMODE_OFF = 0,
    C3D_EZMODE_TESTON = 1,
    C3D_EZMODE_TESTON_WRITEZ = 2,
    C3D_EZMODE_MAX = 3,
    C3D_EZMODE_FORCE_U32 = 0x7FFFFFFF
} C3D_EZMODE,
    *C3D_PEZMODE;

typedef enum {
    C3D_EZCMP_NEVER = 0,
    C3D_EZCMP_LESS = 1,
    C3D_EZCMP_LEQUAL = 2,
    C3D_EZCMP_EQUAL = 3,
    C3D_EZCMP_GEQUAL = 4,
    C3D_EZCMP_GREATER = 5,
    C3D_EZCMP_NOTEQUAL = 6,
    C3D_EZCMP_ALWAYS = 7,
    C3D_EZCMP_MAX = 8,
    C3D_EZCMP_FORCE_U32 = 0x7FFFFFFF
} C3D_EZCMP,
    *C3D_PEZCMP;

typedef void *C3D_HRC;
typedef void *C3D_PRSDATA;

typedef enum {
    C3D_ERS_FG_CLR = 0,
    C3D_ERS_VERTEX_TYPE = 1,
    C3D_ERS_PRIM_TYPE = 2,
    C3D_ERS_SOLID_CLR = 3,
    C3D_ERS_SHADE_MODE = 4,
    C3D_ERS_TMAP_EN = 5,
    C3D_ERS_TMAP_SELECT = 6,
    C3D_ERS_TMAP_LIGHT = 7,
    C3D_ERS_TMAP_PERSP_COR = 8,
    C3D_ERS_TMAP_FILTER = 9,
    C3D_ERS_TMAP_TEXOP = 10,
    C3D_ERS_ALPHA_SRC = 11,
    C3D_ERS_ALPHA_DST = 12,
    C3D_ERS_SURF_DRAW_PTR = 13,
    C3D_ERS_SURF_DRAW_PITCH = 14,
    C3D_ERS_SURF_DRAW_PF = 15,
    C3D_ERS_SURF_VPORT = 16,
    C3D_ERS_FOG_EN = 17,
    C3D_ERS_DITHER_EN = 18,
    C3D_ERS_Z_CMP_FNC = 19,
    C3D_ERS_Z_MODE = 20,
    C3D_ERS_SURF_Z_PTR = 21,
    C3D_ERS_SURF_Z_PITCH = 22,
    C3D_ERS_SURF_SCISSOR = 23,
    C3D_ERS_COMPOSITE_EN = 24,
    C3D_ERS_COMPOSITE_SELECT = 25,
    C3D_ERS_COMPOSITE_FNC = 26,
    C3D_ERS_COMPOSITE_FACTOR = 27,
    C3D_ERS_COMPOSITE_FILTER = 28,
    C3D_ERS_COMPOSITE_FACTOR_ALPHA = 29,
    C3D_ERS_LOD_BIAS_LEVEL = 30,
    C3D_ERS_ALPHA_DST_TEST_ENABLE = 31,
    C3D_ERS_ALPHA_DST_TEST_FNC = 32,
    C3D_ERS_ALPHA_DST_WRITE_SELECT = 33,
    C3D_ERS_ALPHA_DST_REFERENCE = 34,
    C3D_ERS_SPECULAR_EN = 35,
    C3D_ERS_ENHANCED_COLOR_RANGE_EN = 36,
    C3D_ERS_NUM = 37,
    C3D_ERS_FORCE_U32 = 0x7FFFFFFF,
} C3D_ERSID,
    *C3D_PERSID;

#pragma pack(pop)

enum CAMERA_TYPE {
    CAM_CHASE = 0,
    CAM_FIXED = 1,
    CAM_LOOK = 2,
    CAM_COMBAT = 3,
    CAM_CINEMATIC = 4,
    CAM_HEAVY = 5,
};

enum GAME_OBJECT_ID {
    O_LARA = 0,
    O_PISTOLS = 1,
    O_SHOTGUN = 2,
    O_MAGNUM = 3,
    O_UZI = 4,
    O_LARA_EXTRA = 5,
    O_EVIL_LARA = 6,
    O_WOLF = 7,
    O_BEAR = 8,
    O_BAT = 9,
    O_CROCODILE = 10,
    O_ALLIGATOR = 11,
    O_LION = 12,
    O_LIONESS = 13,
    O_PUMA = 14,
    O_APE = 15,
    O_RAT = 16,
    O_VOLE = 17,
    O_DINOSAUR = 18,
    O_RAPTOR = 19,
    O_WARRIOR1 = 20,
    O_WARRIOR2 = 21,
    O_WARRIOR3 = 22,
    O_CENTAUR = 23,
    O_MUMMY = 24,
    O_DINO_WARRIOR = 25,
    O_FISH = 26,
    O_LARSON = 27,
    O_PIERRE = 28,
    O_SKATEBOARD = 29,
    O_MERCENARY1 = 30,
    O_MERCENARY2 = 31,
    O_MERCENARY3 = 32,
    O_NATLA = 33,
    O_ABORTION = 34,
    O_FALLING_BLOCK = 35,
    O_PENDULUM = 36,
    O_SPIKES = 37,
    O_ROLLING_BALL = 38,
    O_DARTS = 39,
    O_DART_EMITTER = 40,
    O_DRAW_BRIDGE = 41,
    O_TEETH_TRAP = 42,
    O_DAMOCLES_SWORD = 43,
    O_THORS_HANDLE = 44,
    O_THORS_HEAD = 45,
    O_LIGHTNING_EMITTER = 46,
    O_MOVING_BAR = 47,
    O_MOVABLE_BLOCK = 48,
    O_MOVABLE_BLOCK2 = 49,
    O_MOVABLE_BLOCK3 = 50,
    O_MOVABLE_BLOCK4 = 51,
    O_ROLLING_BLOCK = 52,
    O_FALLING_CEILING1 = 53,
    O_FALLING_CEILING2 = 54,
    O_SWITCH_TYPE1 = 55,
    O_SWITCH_TYPE2 = 56,
    O_DOOR_TYPE1 = 57,
    O_DOOR_TYPE2 = 58,
    O_DOOR_TYPE3 = 59,
    O_DOOR_TYPE4 = 60,
    O_DOOR_TYPE5 = 61,
    O_DOOR_TYPE6 = 62,
    O_DOOR_TYPE7 = 63,
    O_DOOR_TYPE8 = 64,
    O_TRAPDOOR = 65,
    O_TRAPDOOR2 = 66,
    O_BIGTRAPDOOR = 67,
    O_BRIDGE_FLAT = 68,
    O_BRIDGE_TILT1 = 69,
    O_BRIDGE_TILT2 = 70,
    O_PASSPORT_OPTION = 71,
    O_MAP_OPTION = 72,
    O_PHOTO_OPTION = 73,
    O_COG_1 = 74,
    O_COG_2 = 75,
    O_COG_3 = 76,
    O_PLAYER_1 = 77,
    O_PLAYER_2 = 78,
    O_PLAYER_3 = 79,
    O_PLAYER_4 = 80,
    O_PASSPORT_CLOSED = 81,
    O_MAP_CLOSED = 82,
    O_SAVEGAME_ITEM = 83,
    O_GUN_ITEM = 84,
    O_SHOTGUN_ITEM = 85,
    O_MAGNUM_ITEM = 86,
    O_UZI_ITEM = 87,
    O_GUN_AMMO_ITEM = 88,
    O_SG_AMMO_ITEM = 89,
    O_MAG_AMMO_ITEM = 90,
    O_UZI_AMMO_ITEM = 91,
    O_EXPLOSIVE_ITEM = 92,
    O_MEDI_ITEM = 93,
    O_BIGMEDI_ITEM = 94,
    O_DETAIL_OPTION = 95,
    O_SOUND_OPTION = 96,
    O_CONTROL_OPTION = 97,
    O_GAMMA_OPTION = 98,
    O_GUN_OPTION = 99,
    O_SHOTGUN_OPTION = 100,
    O_MAGNUM_OPTION = 101,
    O_UZI_OPTION = 102,
    O_GUN_AMMO_OPTION = 103,
    O_SG_AMMO_OPTION = 104,
    O_MAG_AMMO_OPTION = 105,
    O_UZI_AMMO_OPTION = 106,
    O_EXPLOSIVE_OPTION = 107,
    O_MEDI_OPTION = 108,
    O_BIGMEDI_OPTION = 109,
    O_PUZZLE_ITEM1 = 110,
    O_PUZZLE_ITEM2 = 111,
    O_PUZZLE_ITEM3 = 112,
    O_PUZZLE_ITEM4 = 113,
    O_PUZZLE_OPTION1 = 114,
    O_PUZZLE_OPTION2 = 115,
    O_PUZZLE_OPTION3 = 116,
    O_PUZZLE_OPTION4 = 117,
    O_PUZZLE_HOLE1 = 118,
    O_PUZZLE_HOLE2 = 119,
    O_PUZZLE_HOLE3 = 120,
    O_PUZZLE_HOLE4 = 121,
    O_PUZZLE_DONE1 = 122,
    O_PUZZLE_DONE2 = 123,
    O_PUZZLE_DONE3 = 124,
    O_PUZZLE_DONE4 = 125,
    O_LEADBAR_ITEM = 126,
    O_LEADBAR_OPTION = 127,
    O_MIDAS_TOUCH = 128,
    O_KEY_ITEM1 = 129,
    O_KEY_ITEM2 = 130,
    O_KEY_ITEM3 = 131,
    O_KEY_ITEM4 = 132,
    O_KEY_OPTION1 = 133,
    O_KEY_OPTION2 = 134,
    O_KEY_OPTION3 = 135,
    O_KEY_OPTION4 = 136,
    O_KEY_HOLE1 = 137,
    O_KEY_HOLE2 = 138,
    O_KEY_HOLE3 = 139,
    O_KEY_HOLE4 = 140,
    O_PICKUP_ITEM1 = 141,
    O_PICKUP_ITEM2 = 142,
    O_SCION_ITEM = 143,
    O_SCION_ITEM2 = 144,
    O_SCION_ITEM3 = 145,
    O_SCION_ITEM4 = 146,
    O_SCION_HOLDER = 147,
    O_PICKUP_OPTION1 = 148,
    O_PICKUP_OPTION2 = 149,
    O_SCION_OPTION = 150,
    O_EXPLOSION1 = 151,
    O_EXPLOSION2 = 152,
    O_SPLASH1 = 153,
    O_SPLASH2 = 154,
    O_BUBBLES1 = 155,
    O_BUBBLES2 = 156,
    O_BUBBLE_EMITTER = 157,
    O_BLOOD1 = 158,
    O_BLOOD2 = 159,
    O_DART_EFFECT = 160,
    O_STATUE = 161,
    O_PORTACABIN = 162,
    O_PODS = 163,
    O_RICOCHET1 = 164,
    O_TWINKLE = 165,
    O_GUN_FLASH = 166,
    O_DUST = 167,
    O_BODY_PART = 168,
    O_CAMERA_TARGET = 169,
    O_WATERFALL = 170,
    O_MISSILE1 = 171,
    O_MISSILE2 = 172,
    O_MISSILE3 = 173,
    O_MISSILE4 = 174,
    O_MISSILE5 = 175,
    O_LAVA = 176,
    O_LAVA_EMITTER = 177,
    O_FLAME = 178,
    O_FLAME_EMITTER = 179,
    O_LAVA_WEDGE = 180,
    O_BIG_POD = 181,
    O_BOAT = 182,
    O_EARTHQUAKE = 183,
    O_TEMP5 = 184,
    O_TEMP6 = 185,
    O_TEMP7 = 186,
    O_TEMP8 = 187,
    O_TEMP9 = 188,
    O_TEMP10 = 189,
    O_HAIR = O_TEMP10,
    O_ALPHABET = 190,
    O_NUMBER_OF = 191,
};

enum GAME_STATIC_ID {
    STATIC_PLANT0 = 0,
    STATIC_PLANT1 = 1,
    STATIC_PLANT2 = 2,
    STATIC_PLANT3 = 3,
    STATIC_PLANT4 = 4,
    STATIC_PLANT5 = 5,
    STATIC_PLANT6 = 6,
    STATIC_PLANT7 = 7,
    STATIC_PLANT8 = 8,
    STATIC_PLANT9 = 9,
    STATIC_FURNITURE0 = 10,
    STATIC_FURNITURE1 = 11,
    STATIC_FURNITURE2 = 12,
    STATIC_FURNITURE3 = 13,
    STATIC_FURNITURE4 = 14,
    STATIC_FURNITURE5 = 15,
    STATIC_FURNITURE6 = 16,
    STATIC_FURNITURE7 = 17,
    STATIC_FURNITURE8 = 18,
    STATIC_FURNITURE9 = 19,
    STATIC_ROCK0 = 20,
    STATIC_ROCK1 = 21,
    STATIC_ROCK2 = 22,
    STATIC_ROCK3 = 23,
    STATIC_ROCK4 = 24,
    STATIC_ROCK5 = 25,
    STATIC_ROCK6 = 26,
    STATIC_ROCK7 = 27,
    STATIC_ROCK8 = 28,
    STATIC_ROCK9 = 29,
    STATIC_ARCHITECTURE0 = 30,
    STATIC_ARCHITECTURE1 = 31,
    STATIC_ARCHITECTURE2 = 32,
    STATIC_ARCHITECTURE3 = 33,
    STATIC_ARCHITECTURE4 = 34,
    STATIC_ARCHITECTURE5 = 35,
    STATIC_ARCHITECTURE6 = 36,
    STATIC_ARCHITECTURE7 = 37,
    STATIC_ARCHITECTURE8 = 38,
    STATIC_ARCHITECTURE9 = 39,
    STATIC_DEBRIS0 = 40,
    STATIC_DEBRIS1 = 41,
    STATIC_DEBRIS2 = 42,
    STATIC_DEBRIS3 = 43,
    STATIC_DEBRIS4 = 44,
    STATIC_DEBRIS5 = 45,
    STATIC_DEBRIS6 = 46,
    STATIC_DEBRIS7 = 47,
    STATIC_DEBRIS8 = 48,
    STATIC_DEBRIS9 = 49,
    STATIC_NUMBER_OF = 50,
};

enum SOUND_EFFECT_ID {
    SFX_LARA_FEET = 0,
    SFX_LARA_CLIMB2 = 1,
    SFX_LARA_NO = 2,
    SFX_LARA_SLIPPING = 3,
    SFX_LARA_LAND = 4,
    SFX_LARA_CLIMB1 = 5,
    SFX_LARA_DRAW = 6,
    SFX_LARA_HOLSTER = 7,
    SFX_LARA_FIRE = 8,
    SFX_LARA_RELOAD = 9,
    SFX_LARA_RICOCHET = 10,
    SFX_BEAR_GROWL = 11,
    SFX_BEAR_FEET = 12,
    SFX_BEAR_ATTACK = 13,
    SFX_BEAR_SNARL = 14,
    SFX_BEAR_HURT = 16,
    SFX_BEAR_DEATH = 18,
    SFX_WOLF_JUMP = 19,
    SFX_WOLF_HURT = 20,
    SFX_WOLF_DEATH = 22,
    SFX_WOLF_HOWL = 24,
    SFX_WOLF_ATTACK = 25,
    SFX_LARA_CLIMB3 = 26,
    SFX_LARA_BODYSL = 27,
    SFX_LARA_SHIMMY2 = 28,
    SFX_LARA_JUMP = 29,
    SFX_LARA_FALL = 30,
    SFX_LARA_INJURY = 31,
    SFX_LARA_ROLL = 32,
    SFX_LARA_SPLASH = 33,
    SFX_LARA_GETOUT = 34,
    SFX_LARA_SWIM = 35,
    SFX_LARA_BREATH = 36,
    SFX_LARA_BUBBLES = 37,
    SFX_LARA_SWITCH = 38,
    SFX_LARA_KEY = 39,
    SFX_LARA_OBJECT = 40,
    SFX_LARA_GENERAL_DEATH = 41,
    SFX_LARA_KNEES_DEATH = 42,
    SFX_LARA_UZI_FIRE = 43,
    SFX_LARA_MAGNUMS = 44,
    SFX_LARA_SHOTGUN = 45,
    SFX_LARA_BLOCK_PUSH1 = 46,
    SFX_LARA_BLOCK_PUSH2 = 47,
    SFX_LARA_EMPTY = 48,
    SFX_LARA_BULLETHIT = 50,
    SFX_LARA_BLKPULL = 51,
    SFX_LARA_FLOATING = 52,
    SFX_LARA_FALLDETH = 53,
    SFX_LARA_GRABHAND = 54,
    SFX_LARA_GRABBODY = 55,
    SFX_LARA_GRABFEET = 56,
    SFX_LARA_SWITCHUP = 57,
    SFX_BAT_SQK = 58,
    SFX_BAT_FLAP = 59,
    SFX_UNDERWATER = 60,
    SFX_UNDERWATER_SWITCH = 61,
    SFX_BLOCK_SOUND = 63,
    SFX_DOOR = 64,
    SFX_PENDULUM_BLADES = 65,
    SFX_ROCK_FALL_CRUMBLE = 66,
    SFX_ROCK_FALL_FALL = 67,
    SFX_ROCK_FALL_LAND = 68,
    SFX_T_REX_DEATH = 69,
    SFX_T_REX_FOOTSTOMP = 70,
    SFX_T_REX_ROAR = 71,
    SFX_T_REX_ATTACK = 72,
    SFX_RAPTOR_ROAR = 73,
    SFX_RAPTOR_ATTACK = 74,
    SFX_RAPTOR_FEET = 75,
    SFX_MUMMY_GROWL = 76,
    SFX_LARSON_FIRE = 77,
    SFX_LARSON_RICOCHET = 78,
    SFX_WATERFALL_LOOP = 79,
    SFX_WATER_LOOP = 80,
    SFX_WATERFALL_BIG = 81,
    SFX_CHAINDOOR_UP = 82,
    SFX_CHAINDOOR_DOWN = 83,
    SFX_COGS = 84,
    SFX_LION_HURT = 85,
    SFX_LION_ATTACK = 86,
    SFX_LION_ROAR = 87,
    SFX_LION_DEATH = 88,
    SFX_GORILLA_FEET = 89,
    SFX_GORILLA_PANT = 90,
    SFX_GORILLA_DEATH = 91,
    SFX_CROC_FEET = 92,
    SFX_CROC_ATTACK = 93,
    SFX_RAT_FEET = 94,
    SFX_RAT_CHIRP = 95,
    SFX_RAT_ATTACK = 96,
    SFX_RAT_DEATH = 97,
    SFX_THUNDER = 98,
    SFX_EXPLOSION = 99,
    SFX_GORILLA_GRUNT = 100,
    SFX_GORILLA_GRUNTS = 101,
    SFX_CROC_DEATH = 102,
    SFX_DAMOCLES_SWORD = 103,
    SFX_ATLANTEAN_EXPLODE = 104,
    SFX_MENU_ROTATE = 108,
    SFX_MENU_CHOOSE = 109,
    SFX_MENU_GAMEBOY = 110,
    SFX_MENU_SPININ = 111,
    SFX_MENU_SPINOUT = 112,
    SFX_MENU_COMPASS = 113,
    SFX_MENU_GUNS = 114,
    SFX_MENU_PASSPORT = 115,
    SFX_MENU_MEDI = 116,
    SFX_RAISINGBLOCK_FX = 117,
    SFX_SAND_FX = 118,
    SFX_STAIRS2SLOPE_FX = 119,
    SFX_ATLANTEAN_WALK = 120,
    SFX_ATLANTEAN_ATTACK = 121,
    SFX_ATLANTEAN_JUMP_ATTACK = 122,
    SFX_ATLANTEAN_NEEDLE = 123,
    SFX_ATLANTEAN_BALL = 124,
    SFX_ATLANTEAN_WINGS = 125,
    SFX_ATLANTEAN_RUN = 126,
    SFX_SLAMDOOR_CLOSE = 127,
    SFX_SLAMDOOR_OPEN = 128,
    SFX_SKATEBOARD_MOVE = 129,
    SFX_SKATEBOARD_STOP = 130,
    SFX_SKATEBOARD_SHOOT = 131,
    SFX_SKATEBOARD_HIT = 132,
    SFX_SKATEBOARD_START = 133,
    SFX_SKATEBOARD_DEATH = 134,
    SFX_SKATEBOARD_HIT_GROUND = 135,
    SFX_ABORTION_HIT_GROUND = 136,
    SFX_ABORTION_ATTACK1 = 137,
    SFX_ABORTION_ATTACK2 = 138,
    SFX_ABORTION_DEATH = 139,
    SFX_ABORTION_ARM_SWING = 140,
    SFX_ABORTION_MOVE = 141,
    SFX_ABORTION_HIT = 142,
    SFX_CENTAUR_FEET = 143,
    SFX_CENTAUR_ROAR = 144,
    SFX_LARA_SPIKE_DEATH = 145,
    SFX_LARA_DEATH3 = 146,
    SFX_ROLLING_BALL = 147,
    SFX_LAVA_LOOP = 148,
    SFX_LAVA_FOUNTAIN = 149,
    SFX_FIRE = 150,
    SFX_DARTS = 151,
    SFX_METAL_DOOR_CLOSE = 152,
    SFX_METAL_DOOR_OPEN = 153,
    SFX_ALTAR_LOOP = 154,
    SFX_POWERUP_FX = 155,
    SFX_COWBOY_DEATH = 156,
    SFX_BLACK_GOON_DEATH = 157,
    SFX_LARSON_DEATH = 158,
    SFX_PIERRE_DEATH = 159,
    SFX_NATLA_DEATH = 160,
    SFX_TRAPDOOR_OPEN = 161,
    SFX_TRAPDOOR_CLOSE = 162,
    SFX_ATLANTEAN_EGG_LOOP = 163,
    SFX_ATLANTEAN_EGG_HATCH = 164,
    SFX_DRILL_ENGINE_START = 165,
    SFX_DRILL_ENGINE_LOOP = 166,
    SFX_CONVEYOR_BELT = 167,
    SFX_HUT_LOWERED = 168,
    SFX_HUT_HIT_GROUND = 169,
    SFX_EXPLOSION_FX = 170,
    SFX_ATLANTEAN_DEATH = 171,
    SFX_CHAINBLOCK_FX = 172,
    SFX_SECRET = 173,
    SFX_GYM_HINT_01 = 174,
    SFX_GYM_HINT_02 = 175,
    SFX_GYM_HINT_03 = 176,
    SFX_GYM_HINT_04 = 177,
    SFX_GYM_HINT_05 = 178,
    SFX_GYM_HINT_06 = 179,
    SFX_GYM_HINT_07 = 180,
    SFX_GYM_HINT_08 = 181,
    SFX_GYM_HINT_09 = 182,
    SFX_GYM_HINT_10 = 183,
    SFX_GYM_HINT_11 = 184,
    SFX_GYM_HINT_12 = 185,
    SFX_GYM_HINT_13 = 186,
    SFX_GYM_HINT_14 = 187,
    SFX_GYM_HINT_15 = 188,
    SFX_GYM_HINT_16 = 189,
    SFX_GYM_HINT_17 = 190,
    SFX_GYM_HINT_18 = 191,
    SFX_GYM_HINT_19 = 192,
    SFX_GYM_HINT_20 = 193,
    SFX_GYM_HINT_21 = 194,
    SFX_GYM_HINT_22 = 195,
    SFX_GYM_HINT_23 = 196,
    SFX_GYM_HINT_24 = 197,
    SFX_GYM_HINT_25 = 198,
    SFX_BALDY_SPEECH = 199,
    SFX_COWBOY_SPEECH = 200,
    SFX_LARSON_SPEECH = 201,
    SFX_NATLA_SPEECH = 202,
    SFX_PIERRE_SPEECH = 203,
    SFX_SKATEKID_SPEECH = 204,
    SFX_LARA_SETUP = 205,
};

enum LARA_ANIMATION_FRAME {
    AF_VAULT12 = 759,
    AF_VAULT34 = 614,
    AF_FASTFALL = 481,
    AF_STOP = 185,
    AF_FALLDOWN = 492,
    AF_STOP_LEFT = 58,
    AF_STOP_RIGHT = 74,
    AF_HITWALLLEFT = 800,
    AF_HITWALLRIGHT = 815,
    AF_RUNSTEPUP_LEFT = 837,
    AF_RUNSTEPUP_RIGHT = 830,
    AF_WALKSTEPUP_LEFT = 844,
    AF_WALKSTEPUP_RIGHT = 858,
    AF_WALKSTEPD_LEFT = 887,
    AF_WALKSTEPD_RIGHT = 874,
    AF_BACKSTEPD_LEFT = 899,
    AF_BACKSTEPD_RIGHT = 930,
    AF_LANDFAR = 358,
    AF_GRABLEDGE = 1493,
    AF_GRABLEDGE_OLD = 621,
    AF_SWIMGLIDE = 1431,
    AF_FALLBACK = 1473,
    AF_HANG = 1514,
    AF_HANG_OLD = 642,
    AF_STARTHANG = 1505,
    AF_STARTHANG_OLD = 634,
    AF_STOPHANG = 448,
    AF_SLIDE = 1133,
    AF_SLIDEBACK = 1677,
    AF_TREAD = 1736,
    AF_SURFTREAD = 1937,
    AF_SURFDIVE = 2041,
    AF_SURFCLIMB = 1849,
    AF_JUMPIN = 1895,
    AF_ROLL = 3857,
    AF_RBALL_DEATH = 3561,
    AF_SPIKE_DEATH = 3887,
    AF_GRABLEDGEIN = 3974,
    AF_PPREADY = 2091,
    AF_PICKUP = 3443,
    AF_PICKUP_UW = 2970,
    AF_PICKUPSCION = 44,
    AF_USEPUZZLE = 3372,
};

enum LARA_SHOTGUN_ANIMATION_FRAME {
    AF_SG_AIM = 0,
    AF_SG_DRAW = 13,
    AF_SG_RECOIL = 47,
    AF_SG_UNDRAW = 80,
    AF_SG_UNAIM = 114,
    AF_SG_END = 127,
};

enum LARA_GUN_ANIMATION_FRAME {
    AF_G_AIM = 0,
    AF_G_AIM_L = 4,
    AF_G_DRAW1 = 5,
    AF_G_DRAW1_L = 12,
    AF_G_DRAW2 = 13,
    AF_G_DRAW2_L = 23,
    AF_G_RECOIL = 24,
};

enum LARA_ANIMATION {
    AA_VAULT12 = 50,
    AA_VAULT34 = 42,
    AA_FASTFALL = 32,
    AA_STOP = 11,
    AA_FALLDOWN = 34,
    AA_STOP_LEFT = 2,
    AA_STOP_RIGHT = 3,
    AA_HITWALLLEFT = 53,
    AA_HITWALLRIGHT = 54,
    AA_RUNSTEPUP_LEFT = 56,
    AA_RUNSTEPUP_RIGHT = 55,
    AA_WALKSTEPUP_LEFT = 57,
    AA_WALKSTEPUP_RIGHT = 58,
    AA_WALKSTEPD_LEFT = 60,
    AA_WALKSTEPD_RIGHT = 59,
    AA_BACKSTEPD_LEFT = 61,
    AA_BACKSTEPD_RIGHT = 62,
    AA_LANDFAR = 24,
    AA_GRABLEDGE = 96,
    AA_GRABLEDGE_OLD = 32,
    AA_SWIMGLIDE = 87,
    AA_FALLBACK = 93,
    AA_HANG = 96,
    AA_HANG_OLD = 33,
    AA_STARTHANG = 96,
    AA_STARTHANG_OLD = 33,
    AA_STOPHANG = 28,
    AA_SLIDE = 70,
    AA_SLIDEBACK = 104,
    AA_TREAD = 108,
    AA_SURFTREAD = 114,
    AA_SURFDIVE = 119,
    AA_SURFCLIMB = 111,
    AA_JUMPIN = 112,
    AA_ROLL = 146,
    AA_RBALL_DEATH = 139,
    AA_SPIKE_DEATH = 149,
    AA_GRABLEDGEIN = 150,
    AA_SPAZ_FORWARD = 125,
    AA_SPAZ_BACK = 126,
    AA_SPAZ_RIGHT = 127,
    AA_SPAZ_LEFT = 128,
};

enum LARA_WATER_STATUS {
    LWS_ABOVEWATER = 0,
    LWS_UNDERWATER = 1,
    LWS_SURFACE = 2,
    LWS_CHEAT = 3,
};

enum LARA_STATE {
    AS_WALK = 0,
    AS_RUN = 1,
    AS_STOP = 2,
    AS_FORWARDJUMP = 3,
    AS_POSE = 4,
    AS_FASTBACK = 5,
    AS_TURN_R = 6,
    AS_TURN_L = 7,
    AS_DEATH = 8,
    AS_FASTFALL = 9,
    AS_HANG = 10,
    AS_REACH = 11,
    AS_SPLAT = 12,
    AS_TREAD = 13,
    AS_LAND = 14,
    AS_COMPRESS = 15,
    AS_BACK = 16,
    AS_SWIM = 17,
    AS_GLIDE = 18,
    AS_NULL = 19,
    AS_FASTTURN = 20,
    AS_STEPRIGHT = 21,
    AS_STEPLEFT = 22,
    AS_HIT = 23,
    AS_SLIDE = 24,
    AS_BACKJUMP = 25,
    AS_RIGHTJUMP = 26,
    AS_LEFTJUMP = 27,
    AS_UPJUMP = 28,
    AS_FALLBACK = 29,
    AS_HANGLEFT = 30,
    AS_HANGRIGHT = 31,
    AS_SLIDEBACK = 32,
    AS_SURFTREAD = 33,
    AS_SURFSWIM = 34,
    AS_DIVE = 35,
    AS_PUSHBLOCK = 36,
    AS_PULLBLOCK = 37,
    AS_PPREADY = 38,
    AS_PICKUP = 39,
    AS_SWITCHON = 40,
    AS_SWITCHOFF = 41,
    AS_USEKEY = 42,
    AS_USEPUZZLE = 43,
    AS_UWDEATH = 44,
    AS_ROLL = 45,
    AS_SPECIAL = 46,
    AS_SURFBACK = 47,
    AS_SURFLEFT = 48,
    AS_SURFRIGHT = 49,
    AS_USEMIDAS = 50,
    AS_DIEMIDAS = 51,
    AS_SWANDIVE = 52,
    AS_FASTDIVE = 53,
    AS_GYMNAST = 54,
    AS_WATEROUT = 55,
};

enum LARA_GUN_STATE {
    LGS_ARMLESS = 0,
    LGS_HANDSBUSY = 1,
    LGS_DRAW = 2,
    LGS_UNDRAW = 3,
    LGS_READY = 4,
};

enum LARA_GUN_TYPE {
    LGT_UNARMED = 0,
    LGT_PISTOLS = 1,
    LGT_MAGNUMS = 2,
    LGT_UZIS = 3,
    LGT_SHOTGUN = 4,
    NUM_WEAPONS = 5
};

enum LARA_MESH {
    LM_HIPS = 0,
    LM_THIGH_L = 1,
    LM_CALF_L = 2,
    LM_FOOT_L = 3,
    LM_THIGH_R = 4,
    LM_CALF_R = 5,
    LM_FOOT_R = 6,
    LM_TORSO = 7,
    LM_UARM_R = 8,
    LM_LARM_R = 9,
    LM_HAND_R = 10,
    LM_UARM_L = 11,
    LM_LARM_L = 12,
    LM_HAND_L = 13,
    LM_HEAD = 14,
    LM_NUMBER_OF = 15,
};

enum MOOD_TYPE {
    MOOD_BORED = 0,
    MOOD_ATTACK = 1,
    MOOD_ESCAPE = 2,
    MOOD_STALK = 3,
};

enum TARGET_TYPE {
    TARGET_NONE = 0,
    TARGET_PRIMARY = 1,
    TARGET_SECONDARY = 2,
};

enum GAMEALLOC_BUFFER {
    GBUF_TEXTURE_PAGES = 0,
    GBUF_OBJECT_TEXTURES = 1,
    GBUF_MESH_POINTERS = 2,
    GBUF_MESHES = 3,
    GBUF_ANIMS = 4,
    GBUF_ANIM_CHANGES = 5,
    GBUF_ANIM_RANGES = 6,
    GBUF_ANIM_COMMANDS = 7,
    GBUF_ANIM_BONES = 8,
    GBUF_ANIM_FRAMES = 9,
    GBUF_ROOM_TEXTURES = 10,
    GBUF_ROOM_INFOS = 11,
    GBUF_ROOM_MESH = 12,
    GBUF_ROOM_DOOR = 13,
    GBUF_ROOM_FLOOR = 14,
    GBUF_ROOM_LIGHTS = 15,
    GBUF_ROOM_STATIC_MESH_INFOS = 16,
    GBUF_FLOOR_DATA = 17,
    GBUF_ITEMS = 18,
    GBUF_CAMERAS = 19,
    GBUF_SOUND_FX = 20,
    GBUF_BOXES = 21,
    GBUF_OVERLAPS = 22,
    GBUF_GROUNDZONE = 23,
    GBUF_FLYZONE = 24,
    GBUF_ANIMATING_TEXTURE_RANGES = 25,
    GBUF_CINEMATIC_FRAMES = 26,
    GBUF_LOADDEMO_BUFFER = 27,
    GBUF_SAVEDEMO_BUFFER = 28,
    GBUF_CINEMATIC_EFFECTS = 29,
    GBUF_MUMMY_HEAD_TURN = 30,
    GBUF_EXTRA_DOOR_STUFF = 31,
    GBUF_EFFECTS = 32,
    GBUF_CREATURE_DATA = 33,
    GBUF_CREATURE_LOT = 34,
    GBUF_SAMPLE_INFOS = 35,
    GBUF_SAMPLES = 36,
    GBUF_SAMPLE_OFFSETS = 37,
    GBUF_ROLLINGBALL_STUFF = 38,
};

enum KEY_NUMBER {
    KEY_UP = 0,
    KEY_DOWN = 1,
    KEY_LEFT = 2,
    KEY_RIGHT = 3,
    KEY_STEP_L = 4,
    KEY_STEP_R = 5,
    KEY_SLOW = 6,
    KEY_JUMP = 7,
    KEY_ACTION = 8,
    KEY_DRAW = 9,
    KEY_LOOK = 10,
    KEY_ROLL = 11,
    KEY_OPTION = 12,
    KEY_FLY_CHEAT = 13,
    KEY_ITEM_CHEAT = 14,
    KEY_LEVEL_SKIP_CHEAT = 15,
    KEY_PAUSE = 16,
    KEY_NUMBER_OF = 17,
};

enum INPUT_STATE {
    IN_FORWARD = 1 << 0,
    IN_BACK = 1 << 1,
    IN_LEFT = 1 << 2,
    IN_RIGHT = 1 << 3,
    IN_JUMP = 1 << 4,
    IN_DRAW = 1 << 5,
    IN_ACTION = 1 << 6,
    IN_SLOW = 1 << 7,
    IN_OPTION = 1 << 8,
    IN_LOOK = 1 << 9,
    IN_STEPL = 1 << 10,
    IN_STEPR = 1 << 11,
    IN_ROLL = 1 << 12,
    IN_PAUSE = 1 << 13,
    IN_A = 1 << 14,
    IN_B = 1 << 15,
    IN_C = 1 << 16,
    IN_MENUBACK = 1 << 17,
    IN_UP = 1 << 18,
    IN_DOWN = 1 << 19,
    IN_SELECT = 1 << 20,
    IN_DESELECT = 1 << 21,
    IN_SAVE = 1 << 22,
    IN_LOAD = 1 << 23,
    IN_FLY_CHEAT = 1 << 24,
    IN_ITEM_CHEAT = 1 << 25,
};

enum TEXTSTRING_FLAG {
    TF_ACTIVE = 1 << 0,
    TF_FLASH = 1 << 1,
    TF_ROTATE_H = 1 << 2,
    TF_ROTATE_V = 1 << 3,
    TF_CENTRE_H = 1 << 4,
    TF_CENTRE_V = 1 << 5,
    TF_RIGHT = 1 << 7,
    TF_BOTTOM = 1 << 8,
    TF_BGND = 1 << 9,
    TF_OUTLINE = 1 << 10,
};

enum RENDER_SETTINGS_FLAG {
    RSF_PERSPECTIVE = 1 << 0,
    RSF_BILINEAR = 1 << 1,
    RSF_FPS = 1 << 2,
};

enum COLL_TYPE {
    COLL_NONE = 0,
    COLL_FRONT = 1,
    COLL_LEFT = 2,
    COLL_RIGHT = 4,
    COLL_TOP = 8,
    COLL_TOPFRONT = 16,
    COLL_CLAMP = 32,
};

enum HEIGHT_TYPE {
    HT_WALL = 0,
    HT_SMALL_SLOPE = 1,
    HT_BIG_SLOPE = 2,
};

enum DIRECTION {
    DIR_NORTH = 0,
    DIR_EAST = 1,
    DIR_SOUTH = 2,
    DIR_WEST = 3,
};

enum ANIM_COMMAND {
    AC_NULL = 0,
    AC_MOVE_ORIGIN = 1,
    AC_JUMP_VELOCITY = 2,
    AC_ATTACK_READY = 3,
    AC_DEACTIVATE = 4,
    AC_SOUND_FX = 5,
    AC_EFFECT = 6,
};

enum BONE_EXTRA_BITS {
    BEB_POP = 1 << 0,
    BEB_PUSH = 1 << 1,
    BEB_ROT_X = 1 << 2,
    BEB_ROT_Y = 1 << 3,
    BEB_ROT_Z = 1 << 4,
};

enum ITEM_STATUS {
    IS_NOT_ACTIVE = 0,
    IS_ACTIVE = 1,
    IS_DEACTIVATED = 2,
    IS_INVISIBLE = 3,
};

enum ROOM_FLAG {
    RF_UNDERWATER = 1,
};

enum FLOOR_TYPE {
    FT_FLOOR = 0,
    FT_DOOR = 1,
    FT_TILT = 2,
    FT_ROOF = 3,
    FT_TRIGGER = 4,
    FT_LAVA = 5,
};

enum TRIGGER_TYPE {
    TT_TRIGGER = 0,
    TT_PAD = 1,
    TT_SWITCH = 2,
    TT_KEY = 3,
    TT_PICKUP = 4,
    TT_HEAVY = 5,
    TT_ANTIPAD = 6,
    TT_COMBAT = 7,
    TT_DUMMY = 8,
};

enum TRIGGER_OBJECT {
    TO_OBJECT = 0,
    TO_CAMERA = 1,
    TO_SINK = 2,
    TO_FLIPMAP = 3,
    TO_FLIPON = 4,
    TO_FLIPOFF = 5,
    TO_TARGET = 6,
    TO_FINISH = 7,
    TO_CD = 8,
    TO_FLIPEFFECT = 9,
    TO_SECRET = 10,
};

enum ITEM_FLAG {
    IF_ONESHOT = 0x0100,
    IF_CODE_BITS = 0x3E00,
    IF_REVERSE = 0x4000,
    IF_NOT_VISIBLE = 0x0100,
    IF_KILLED_ITEM = 0x8000,
};

enum FMV_SEQUENCE {
    FMV_INTRO = 0,
    FMV_GYM = 1,
    FMV_SNOW = 2,
    FMV_LIFT = 3,
    FMV_VISION = 4,
    FMV_CANYON = 5,
    FMV_PYRAMID = 6,
    FMV_PRISON = 7,
    FMV_ENDSEQ = 8,
    FMV_CORE = 9,
    FMV_ESCAPE = 10,
    FMV_NUMBER_OF = 11,
};

enum INV_MODE {
    INV_GAME_MODE = 0,
    INV_TITLE_MODE = 1,
    INV_KEYS_MODE = 2,
    INV_SAVE_MODE = 3,
    INV_LOAD_MODE = 4,
    INV_DEATH_MODE = 5,
    INV_SAVE_CRYSTAL_MODE = 6,
    INV_PAUSE_MODE = 7,
};

enum INV_TEXT {
    IT_NAME = 0,
    IT_QTY = 1,
    IT_NUMBER_OF = 2,
};

enum INV_COLOUR {
    IC_BLACK = 0,
    IC_GREY = 1,
    IC_WHITE = 2,
    IC_RED = 3,
    IC_ORANGE = 4,
    IC_YELLOW = 5,
    IC_GREEN1 = 6,
    IC_GREEN2 = 7,
    IC_GREEN3 = 8,
    IC_GREEN4 = 9,
    IC_GREEN5 = 10,
    IC_GREEN6 = 11,
    IC_DARKGREEN = 12,
    IC_GREEN = 13,
    IC_CYAN = 14,
    IC_BLUE = 15,
    IC_MAGENTA = 16,
    IC_NUMBER_OF = 17,
};

enum RING_STATUS {
    RNG_OPENING = 0,
    RNG_OPEN = 1,
    RNG_CLOSING = 2,
    RNG_MAIN2OPTION = 3,
    RNG_MAIN2KEYS = 4,
    RNG_KEYS2MAIN = 5,
    RNG_OPTION2MAIN = 6,
    RNG_SELECTING = 7,
    RNG_SELECTED = 8,
    RNG_DESELECTING = 9,
    RNG_DESELECT = 10,
    RNG_CLOSING_ITEM = 11,
    RNG_EXITING_INVENTORY = 12,
    RNG_DONE = 13,
};

enum RING_TYPE {
    RT_MAIN = 0,
    RT_OPTION = 1,
    RT_KEYS = 2,
};

enum SHAPE { SHAPE_SPRITE = 1, SHAPE_LINE = 2, SHAPE_BOX = 3, SHAPE_FBOX = 4 };

enum DOOR_ANIM {
    DOOR_CLOSED = 0,
    DOOR_OPEN = 1,
};

enum TRAP_ANIM {
    TRAP_SET = 0,
    TRAP_ACTIVATE = 1,
    TRAP_WORKING = 2,
    TRAP_FINISHED = 3,
};

enum ROLLING_BLOCK_STATE {
    RBS_START = 0,
    RBS_END = 1,
    RBS_MOVING = 2,
};

enum REQUEST_INFO_FLAGS {
    RIF_BLOCKED = 1 << 0,
    RIF_BLOCKABLE = 1 << 1,
};

enum SOUND_PLAY_MODE {
    SPM_NORMAL = 0,
    SPM_UNDERWATER = 1,
    SPM_ALWAYS = 2,
};

#pragma pack(push, 1)

struct KEYSTUFF {
    uint8_t keymap[128];
    uint8_t oldkeymap[128];
    uint8_t keybuf[64];
    uint8_t bufin;
    uint8_t bufout;
    uint8_t bufchars;
    uint8_t extended;
    uint8_t last_key;
    uint8_t keys_held;
};

struct RGB888 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct PCX_HEADER {
    uint8_t manufacturer;
    uint8_t version;
    uint8_t rle;
    uint8_t bpp;
    uint16_t x_min;
    uint16_t y_min;
    uint16_t x_max;
    uint16_t y_max;
    uint16_t h_dpi;
    uint16_t v_dpi;
    struct RGB888 header_palette[16];
    uint8_t reserved;
    uint8_t planes;
    uint16_t bytes_per_line;
    uint16_t pal_interpret;
    uint16_t h_res;
    uint16_t v_res;
    uint8_t reserved_data[54];
};

struct POS_2D {
    uint16_t x;
    uint16_t y;
};

struct POS_3D {
    uint16_t x;
    uint16_t y;
    uint16_t z;
};

struct PHD_VECTOR {
    int32_t x;
    int32_t y;
    int32_t z;
};

struct PHD_MATRIX {
    int32_t _00;
    int32_t _01;
    int32_t _02;
    int32_t _03;
    int32_t _10;
    int32_t _11;
    int32_t _12;
    int32_t _13;
    int32_t _20;
    int32_t _21;
    int32_t _22;
    int32_t _23;
};

struct PHD_3DPOS {
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t x_rot;
    int16_t y_rot;
    int16_t z_rot;
};

struct PHD_VBUF {
    int32_t xv;
    int32_t yv;
    int32_t zv;
    int32_t xs;
    int32_t ys;
    int32_t dist;
    int16_t clip;
    int16_t g;
    uint16_t u;
    uint16_t v;
};

struct PHD_UV {
    uint16_t u1;
    uint16_t v1;
};

struct PHD_TEXTURE {
    uint16_t drawtype;
    uint16_t tpage;
    PHD_UV uv[4];
};

struct PHD_SPRITE {
    uint16_t tpage;
    uint16_t offset;
    uint16_t width;
    uint16_t height;
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
};

struct DOOR_INFO {
    int16_t room_num;
    int16_t x;
    int16_t y;
    int16_t z;
    struct POS_3D vertex[4];
};

struct DOOR_INFOS {
    uint16_t count;
    struct DOOR_INFO door[];
};

struct FLOOR_INFO {
    uint16_t index;
    int16_t box;
    uint8_t pit_room;
    int8_t floor;
    uint8_t sky_room;
    int8_t ceiling;
};

struct DOORPOS_DATA {
    struct FLOOR_INFO *floor;
    struct FLOOR_INFO old_floor;
    int16_t block;
};

struct DOOR_DATA {
    struct DOORPOS_DATA d1;
    struct DOORPOS_DATA d1flip;
    struct DOORPOS_DATA d2;
    struct DOORPOS_DATA d2flip;
};

struct LIGHT_INFO {
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t intensity;
    int32_t falloff;
};

struct MESH_INFO {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    int16_t y_rot;
    uint16_t shade;
    uint16_t static_number;
};

struct ROOM_INFO {
    int16_t *data;
    struct DOOR_INFOS *doors;
    struct FLOOR_INFO *floor;
    struct LIGHT_INFO *light;
    struct MESH_INFO *mesh;
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t min_floor;
    int32_t max_ceiling;
    int16_t x_size;
    int16_t y_size;
    int16_t ambient;
    int16_t num_lights;
    int16_t num_meshes;
    int16_t left;
    int16_t right;
    int16_t top;
    int16_t bottom;
    int16_t bound_active;
    int16_t item_number;
    int16_t fx_number;
    int16_t flipped_room;
    uint16_t flags;
};

struct ITEM_INFO {
    int32_t floor;
    uint32_t touch_bits;
    uint32_t mesh_bits;
    int16_t object_number;
    int16_t current_anim_state;
    int16_t goal_anim_state;
    int16_t required_anim_state;
    int16_t anim_number;
    int16_t frame_number;
    int16_t room_number;
    int16_t next_item;
    int16_t next_active;
    int16_t speed;
    int16_t fall_speed;
    int16_t hit_points;
    int16_t box_number;
    int16_t timer;
    int16_t flags;
    int16_t shade;
    void *data;
    struct PHD_3DPOS pos;
    uint16_t active : 1;
    uint16_t status : 2;
    uint16_t gravity_status : 1;
    uint16_t hit_status : 1;
    uint16_t collidable : 1;
    uint16_t looked_at : 1;
};

struct LARA_ARM {
    int16_t *frame_base;
    int16_t frame_number;
    int16_t lock;
    int16_t y_rot;
    int16_t x_rot;
    int16_t z_rot;
    uint16_t flash_gun;
};

struct AMMO_INFO {
    int32_t ammo;
    int32_t hit;
    int32_t miss;
};

struct BOX_NODE {
    int16_t exit_box;
    uint16_t search_number;
    int16_t next_expansion;
    int16_t box_number;
};

struct LOT_INFO {
    struct BOX_NODE *node;
    int16_t head;
    int16_t tail;
    uint16_t search_number;
    uint16_t block_mask;
    int16_t step;
    int16_t drop;
    int16_t fly;
    int16_t zone_count;
    int16_t target_box;
    int16_t required_box;
    struct PHD_VECTOR target;
};

struct FX_INFO {
    struct PHD_3DPOS pos;
    int16_t room_number;
    int16_t object_number;
    int16_t next_fx;
    int16_t next_active;
    int16_t speed;
    int16_t fall_speed;
    int16_t frame_number;
    int16_t counter;
    int16_t shade;
};

struct LARA_INFO {
    int16_t item_number;
    int16_t gun_status;
    int16_t gun_type;
    int16_t request_gun_type;
    int16_t calc_fall_speed;
    int16_t water_status;
    int16_t pose_count;
    int16_t hit_frame;
    int16_t hit_direction;
    int16_t air;
    int16_t dive_count;
    int16_t death_count;
    int16_t current_active;
    int16_t spaz_effect_count;
    struct FX_INFO *spaz_effect;
    int32_t mesh_effects;
    int16_t *mesh_ptrs[LM_NUMBER_OF];
    struct ITEM_INFO *target;
    int16_t target_angles[2];
    int16_t turn_rate;
    int16_t move_angle;
    int16_t head_y_rot;
    int16_t head_x_rot;
    int16_t head_z_rot;
    int16_t torso_y_rot;
    int16_t torso_x_rot;
    int16_t torso_z_rot;
    struct LARA_ARM left_arm;
    struct LARA_ARM right_arm;
    struct AMMO_INFO pistols;
    struct AMMO_INFO magnums;
    struct AMMO_INFO uzis;
    struct AMMO_INFO shotgun;
    struct LOT_INFO LOT;
};

struct START_INFO {
    uint16_t pistol_ammo;
    uint16_t magnum_ammo;
    uint16_t uzi_ammo;
    uint16_t shotgun_ammo;
    uint8_t num_medis;
    uint8_t num_big_medis;
    uint8_t num_scions;
    int8_t gun_status;
    int8_t gun_type;
    uint16_t available : 1;
    uint16_t got_pistols : 1;
    uint16_t got_magnums : 1;
    uint16_t got_uzis : 1;
    uint16_t got_shotgun : 1;
    uint16_t costume : 1;
};

struct SAVEGAME_INFO {
    struct START_INFO *start;
    uint32_t timer;
    uint32_t kills;
    uint16_t secrets;
    uint16_t current_level;
    uint8_t pickups;
    uint8_t bonus_flag;
    uint8_t num_pickup1;
    uint8_t num_pickup2;
    uint8_t num_puzzle1;
    uint8_t num_puzzle2;
    uint8_t num_puzzle3;
    uint8_t num_puzzle4;
    uint8_t num_key1;
    uint8_t num_key2;
    uint8_t num_key3;
    uint8_t num_key4;
    uint8_t num_leadbar;
    uint8_t challenge_failed;
    char buffer[10240];
};

struct CREATURE_INFO {
    int16_t head_rotation;
    int16_t neck_rotation;
    int16_t maximum_turn;
    uint16_t flags;
    int16_t item_num;
    int32_t mood;
    struct LOT_INFO LOT;
    struct PHD_VECTOR target;
};

struct TEXTSTRING {
    uint32_t flags;
    uint16_t text_flags;
    uint16_t bgnd_flags;
    uint16_t outl_flags;
    int16_t xpos;
    int16_t ypos;
    int16_t zpos;
    int16_t letter_spacing;
    int16_t word_spacing;
    int16_t flash_rate;
    int16_t flash_count;
    int16_t bgnd_colour;
    uint32_t *bgnd_gour;
    int16_t outl_colour;
    uint32_t *outl_gour;
    int16_t bgnd_size_x;
    int16_t bgnd_size_y;
    int16_t bgnd_off_x;
    int16_t bgnd_off_y;
    int16_t bgnd_off_z;
    int32_t scale_h;
    int32_t scale_v;
    char *string;
};

struct DISPLAYPU {
    int16_t duration;
    int16_t sprnum;
};

struct COLL_INFO {
    int32_t mid_floor;
    int32_t mid_ceiling;
    int32_t mid_type;
    int32_t front_floor;
    int32_t front_ceiling;
    int32_t front_type;
    int32_t left_floor;
    int32_t left_ceiling;
    int32_t left_type;
    int32_t right_floor;
    int32_t right_ceiling;
    int32_t right_type;
    int32_t radius;
    int32_t bad_pos;
    int32_t bad_neg;
    int32_t bad_ceiling;
    struct PHD_VECTOR shift;
    struct PHD_VECTOR old;
    int16_t facing;
    int16_t quadrant;
    int16_t coll_type;
    int16_t *trigger;
    int8_t tilt_x;
    int8_t tilt_z;
    int8_t hit_by_baddie;
    int8_t hit_static;
    uint16_t slopes_are_walls : 1;
    uint16_t slopes_are_pits : 1;
    uint16_t lava_is_pit : 1;
    uint16_t enable_baddie_push : 1;
    uint16_t enable_spaz : 1;
};

struct OBJECT_INFO {
    int16_t nmeshes;
    int16_t mesh_index;
    int32_t bone_index;
    int16_t *frame_base;
    void (*initialise)(int16_t item_num);
    void (*control)(int16_t item_num);
    void (*floor)(
        struct ITEM_INFO *item, int32_t x, int32_t y, int32_t z,
        int16_t *height);
    void (*ceiling)(
        struct ITEM_INFO *item, int32_t x, int32_t y, int32_t z,
        int16_t *height);
    void (*draw_routine)(struct ITEM_INFO *item);
    void (*collision)(
        int16_t item_num, struct ITEM_INFO *lara_item, struct COLL_INFO *coll);
    int16_t anim_index;
    int16_t hit_points;
    int16_t pivot_length;
    int16_t radius;
    int16_t smartness;
    int16_t shadow_size;
    uint16_t loaded : 1;
    uint16_t intelligent : 1;
    uint16_t save_position : 1;
    uint16_t save_hitpoints : 1;
    uint16_t save_flags : 1;
    uint16_t save_anim : 1;
};

struct SHADOW_INFO {
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t radius;
    int16_t poly_count;
    int16_t vertex_count;
    POS_3D vertex[8];
};

struct STATIC_INFO {
    int16_t mesh_number;
    int16_t flags;
    int16_t x_minp;
    int16_t x_maxp;
    int16_t y_minp;
    int16_t y_maxp;
    int16_t z_minp;
    int16_t z_maxp;
    int16_t x_minc;
    int16_t x_maxc;
    int16_t y_minc;
    int16_t y_maxc;
    int16_t z_minc;
    int16_t z_maxc;
};

struct GAME_VECTOR {
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t room_number;
    int16_t box_number;
};

struct OBJECT_VECTOR {
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t data;
    int16_t flags;
};

struct CAMERA_INFO {
    struct GAME_VECTOR pos;
    struct GAME_VECTOR target;
    int32_t type;
    int32_t shift;
    int32_t flags;
    int32_t fixed_camera;
    int32_t number_frames;
    int32_t bounce;
    int32_t underwater;
    int32_t target_distance;
    int32_t target_square;
    int16_t target_angle;
    int16_t actual_angle;
    int16_t target_elevation;
    int16_t box;
    int16_t number;
    int16_t last;
    int16_t timer;
    int16_t speed;
    struct ITEM_INFO *item;
    struct ITEM_INFO *last_item;
    struct OBJECT_VECTOR *fixed;
};

struct ANIM_STRUCT {
    int16_t *frame_ptr;
    int16_t interpolation;
    int16_t current_anim_state;
    int32_t velocity;
    int32_t acceleration;
    int16_t frame_base;
    int16_t frame_end;
    int16_t jump_anim_num;
    int16_t jump_frame_num;
    int16_t number_changes;
    int16_t change_index;
    int16_t number_commands;
    int16_t command_index;
};

struct ANIM_CHANGE_STRUCT {
    int16_t goal_anim_state;
    int16_t number_ranges;
    int16_t range_index;
};

struct ANIM_RANGE_STRUCT {
    int16_t start_frame;
    int16_t end_frame;
    int16_t link_anim_num;
    int16_t link_frame_num;
};

struct DOOR_VBUF {
    int32_t xv;
    int32_t yv;
    int32_t zv;
};

struct WEAPON_INFO {
    int16_t lock_angles[4];
    int16_t left_angles[4];
    int16_t right_angles[4];
    int16_t aim_speed;
    int16_t shot_accuracy;
    int32_t gun_height;
    int32_t damage;
    int32_t target_dist;
    int16_t recoil_frame;
    int16_t flash_time;
    int16_t sample_num;
};

struct SPHERE {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t r;
};

struct BITE_INFO {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t mesh_num;
};

struct AI_INFO {
    int16_t zone_number;
    int16_t enemy_zone;
    int32_t distance;
    int32_t ahead;
    int32_t bite;
    int16_t angle;
    int16_t enemy_facing;
};

struct BOX_INFO {
    int32_t left;
    int32_t right;
    int32_t top;
    int32_t bottom;
    int16_t height;
    int16_t overlap_index;
};

struct REQUEST_INFO {
    uint16_t items;
    uint16_t requested;
    uint16_t vis_lines;
    uint16_t line_offset;
    uint16_t line_old_offset;
    uint16_t pix_width;
    uint16_t line_height;
    int16_t x;
    int16_t y;
    int16_t z;
    uint16_t flags;
    const char *heading_text;
    char *item_texts;
    int16_t item_text_len;
    struct TEXTSTRING *heading;
    struct TEXTSTRING *background;
    struct TEXTSTRING *moreup;
    struct TEXTSTRING *moredown;
    struct TEXTSTRING *texts[16];
    uint16_t item_flags[16];
};

struct IMOTION_INFO {
    int16_t count;
    int16_t status;
    int16_t status_target;
    int16_t radius_target;
    int16_t radius_rate;
    int16_t camera_ytarget;
    int16_t camera_yrate;
    int16_t camera_pitch_target;
    int16_t camera_pitch_rate;
    int16_t rotate_target;
    int16_t rotate_rate;
    int16_t item_ptxrot_target;
    int16_t item_ptxrot_rate;
    int16_t item_xrot_target;
    int16_t item_xrot_rate;
    int32_t item_ytrans_target;
    int32_t item_ytrans_rate;
    int32_t item_ztrans_target;
    int32_t item_ztrans_rate;
    int32_t misc;
};

struct INVENTORY_SPRITE {
    int16_t shape;
    int16_t x;
    int16_t y;
    int16_t z;
    int32_t param1;
    int32_t param2;
    uint32_t *grdptr;
    int16_t sprnum;
};

struct INVENTORY_ITEM {
    char *string;
    int16_t object_number;
    int16_t frames_total;
    int16_t current_frame;
    int16_t goal_frame;
    int16_t open_frame;
    int16_t anim_direction;
    int16_t anim_speed;
    int16_t anim_count;
    int16_t pt_xrot_sel;
    int16_t pt_xrot;
    int16_t x_rot_sel;
    int16_t x_rot;
    int16_t y_rot_sel;
    int16_t y_rot;
    int32_t ytrans_sel;
    int32_t ytrans;
    int32_t ztrans_sel;
    int32_t ztrans;
    uint32_t which_meshes;
    uint32_t drawn_meshes;
    int16_t inv_pos;
    struct INVENTORY_SPRITE **sprlist;
};

struct RING_INFO {
    struct INVENTORY_ITEM **list;
    int16_t type;
    int16_t radius;
    int16_t camera_pitch;
    int16_t rotating;
    int16_t rot_count;
    int16_t current_object;
    int16_t target_object;
    int16_t number_of_objects;
    int16_t angle_adder;
    int16_t rot_adder;
    int16_t rot_adder_l;
    int16_t rot_adder_r;
    struct PHD_3DPOS ringpos;
    struct PHD_3DPOS camera;
    struct PHD_VECTOR light;
    struct IMOTION_INFO *imo;
};

struct MN_SFX_PLAY_INFO {
    void *handle;
    struct PHD_3DPOS *pos;
    uint32_t loudness;
    int16_t volume;
    int16_t pan;
    int16_t fxnum;
    int16_t mn_flags;
};

struct SAMPLE_INFO {
    int16_t number;
    int16_t volume;
    int16_t randomness;
    int16_t flags;
};

struct SAMPLE_DATA {
    char *data;
    int32_t length;
    int16_t bits_per_sample;
    int16_t channels;
    int16_t sample_rate;
    int16_t block_align;
    int16_t volume;
    int32_t pan;
    void *handle;
};

struct VERTEX_INFO {
    float x;
    float y;
    float ooz;
    float u;
    float v;
    float g;
    int32_t vr;
    int32_t vg;
    int32_t vb;
};

struct LIGHTNING {
    int32_t onstate;
    int32_t count;
    int32_t zapped;
    int32_t notarget;
    PHD_VECTOR target;
    PHD_VECTOR main[8];
    PHD_VECTOR wibble[8];
    int start[2];
    PHD_VECTOR end[2];
    PHD_VECTOR shoot[2][8];
};

#pragma pack(pop)
