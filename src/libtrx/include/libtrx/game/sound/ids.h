#pragma once

typedef enum {
    SFX_INVALID = -1,
} SAMPLE_ID;

#if TR_VERSION == 1
    #define SFX_NUMBER_OF 256
#elif TR_VERSION == 2
    #define SFX_NUMBER_OF 370
#endif

typedef enum {
    SFX_TRX_INVALID = -1,
#define X_CATALOG_ID(enum_value) enum_value,
#include "../catalog_samples.def"
#undef X_CATALOG_ID
} SAMPLE_TRX_ID;

SAMPLE_ID Sound_ToGameID(SAMPLE_TRX_ID sample_id);
