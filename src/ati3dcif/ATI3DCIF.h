#pragma once

#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} C3D_COLOR;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} C3D_PALETTENTRY;

typedef struct {
    float x, y, z;
    float s, t, w;
    float r, g, b, a;
} C3D_VTCF;

typedef void *C3D_VSTRIP;
typedef void **C3D_VLIST;

typedef enum {
    C3D_EPRIM_LINE = 0,
    C3D_EPRIM_TRI = 1,
} C3D_EPRIM;

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
} C3D_EASRC;

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
} C3D_EADST;

typedef int C3D_HTX;
typedef C3D_HTX *C3D_PHTX;

typedef enum {
    C3D_ETL_NONE = 0,
    C3D_ETL_MODULATE = 1,
    C3D_ETL_ALPHA_DECAL = 2,
    C3D_ETL_NUM = 3,
} C3D_ETLIGHT;

typedef enum {
    C3D_ETFILT_MINPNT_MAGPNT = 0,
    C3D_ETFILT_MINPNT_MAG2BY2 = 1,
    C3D_ETFILT_MIN2BY2_MAG2BY2 = 2,
    C3D_ETFILT_MIPLIN_MAGPNT = 3,
    C3D_ETFILT_MIPLIN_MAG2BY2 = 4,
    C3D_ETFILT_MIPTRI_MAG2BY2 = 5,
    C3D_ETFILT_MIN2BY2_MAGPNT = 6,
    C3D_ETFILT_NUM = 7,
} C3D_ETEXFILTER;

typedef void *C3D_PRSDATA;

typedef enum {
    C3D_ERS_PRIM_TYPE = 2,
    C3D_ERS_TMAP_EN = 5,
    C3D_ERS_TMAP_SELECT = 6,
    C3D_ERS_TMAP_FILTER = 9,
    C3D_ERS_ALPHA_SRC = 11,
    C3D_ERS_ALPHA_DST = 12,
} C3D_ERSID;
