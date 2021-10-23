#ifndef T1M_SPECIFIC_ATI_H
#define T1M_SPECIFIC_ATI_H

#include <stdint.h>

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

#define C3D_FORCE_SIZE 0x7FFFFFFF

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
    C3D_EC_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_EV_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_EPRIM_FORCE_U32 = C3D_FORCE_SIZE
} C3D_EPRIM,
    *C3D_PEPRIM;

typedef enum {
    C3D_ESH_NONE = 0,
    C3D_ESH_SOLID = 1,
    C3D_ESH_FLAT = 2,
    C3D_ESH_SMOOTH = 3,
    C3D_ESH_NUM = 4,
    C3D_ESH_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_EASRC_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_EADST_FORCE_U32 = C3D_FORCE_SIZE
} C3D_EADST,
    *C3D_PEADST;

typedef enum {
    C3D_EASEL_ZERO = 0,
    C3D_EASEL_ONE = 1,
    C3D_EASEL_SRCALPHA = 4,
    C3D_EASEL_INVSRCALPHA = 5,
    C3D_EASEL_DSTALPHA = 6,
    C3D_EASEL_INVDSTALPHA = 7,
    C3D_EASEL_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_EACMP_FORCE_U32 = C3D_FORCE_SIZE
} C3D_EACMP,
    *C3D_PEACMP;

typedef enum {
    C3D_ETEXTILE_DEFAULT = 0,
    C3D_ETEXTILE_OFF = 1,
    C3D_ETEXTILE_ON = 2,
    C3D_ETEXTILE_MAX = 3,
    C3D_ETEXTILE_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_ECI_TMAP_FORCE_U32 = C3D_FORCE_SIZE
} C3D_ECI_TMAP_TYPE;

typedef enum {
    C3D_ETL_NONE = 0,
    C3D_ETL_MODULATE = 1,
    C3D_ETL_ALPHA_DECAL = 2,
    C3D_ETL_NUM = 3,
    C3D_ETL_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_ETPC_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_ETFILT_FORCE_U32 = C3D_FORCE_SIZE
} C3D_ETEXFILTER,
    *C3D_PETEXFILTER;

typedef enum {
    C3D_ETEXOP_NONE = 0,
    C3D_ETEXOP_CHROMAKEY = 1,
    C3D_ETEXOP_ALPHA = 2,
    C3D_ETEXOP_ALPHA_MASK = 3,
    C3D_ETEXOP_NUM = 4,
    C3D_ETEXOP_FORCE_U32 = C3D_FORCE_SIZE
} C3D_ETEXOP,
    *C3D_PETEXOP;

typedef enum {
    C3D_EPF_RGB1555 = 3,
    C3D_EPF_RGB565 = 4,
    C3D_EPF_RGB8888 = 6,
    C3D_EPF_RGB332 = 7,
    C3D_EPF_Y8 = 8,
    C3D_EPF_YUV422 = 11,
    C3D_EPF_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_ETF_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_ETEXCOMPFCN_FORCE_U32 = C3D_FORCE_SIZE
} C3D_ETEXCOMPFCN,
    *C3D_PETEXCOMPFCN;

typedef enum {
    C3D_EZMODE_OFF = 0,
    C3D_EZMODE_TESTON = 1,
    C3D_EZMODE_TESTON_WRITEZ = 2,
    C3D_EZMODE_MAX = 3,
    C3D_EZMODE_FORCE_U32 = C3D_FORCE_SIZE
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
    C3D_EZCMP_FORCE_U32 = C3D_FORCE_SIZE
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

C3D_EC InitATI3DCIF();
C3D_EC ShutdownATI3DCIF();

C3D_EC __stdcall ATI3DCIF_NullSub(void);
C3D_EC __stdcall ATI3DCIF_GetInfo(C3D_3DCIFINFO *info);
C3D_EC __stdcall ATI3DCIF_TextureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap);
C3D_EC __stdcall ATI3DCIF_TextureUnreg(C3D_HTX htxToUnreg);
C3D_EC __stdcall ATI3DCIF_TexturePaletteCreate(
    C3D_ECI_TMAP_TYPE epalette, void *pPalette, C3D_PHTXPAL phtpalCreated);
C3D_EC __stdcall ATI3DCIF_TexturePaletteDestroy(C3D_HTXPAL htxpalToDestroy);
C3D_HRC __stdcall ATI3DCIF_ContextCreate(void);
C3D_EC __stdcall ATI3DCIF_ContextDestroy(C3D_HRC hRC);
C3D_EC __stdcall ATI3DCIF_ContextSetState(
    C3D_HRC hRC, C3D_ERSID eRStateID, C3D_PRSDATA pRStateData);
C3D_EC __stdcall ATI3DCIF_RenderBegin(C3D_HRC hRC);
C3D_EC __stdcall ATI3DCIF_RenderEnd(void);
C3D_EC __stdcall ATI3DCIF_RenderPrimStrip(
    C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert);
C3D_EC __stdcall ATI3DCIF_RenderPrimList(
    C3D_VLIST vList, C3D_UINT32 u32NumVert);

void T1MInjectSpecificATI();

#endif
