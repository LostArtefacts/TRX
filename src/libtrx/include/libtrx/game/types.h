#pragma once

#if TR_VERSION == 1
    #define __PACKING
#elif TR_VERSION == 2
    #define __PACKING __attribute__((packed))
#endif
