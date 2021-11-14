#pragma once

#include "ati3dcif.hpp"

#include <string>

namespace glrage {
namespace cif {

extern const char* C3D_EC_NAMES[];
extern const char* C3D_EVERTEX_NAMES[];
extern const char* C3D_EPRIM_NAMES[];
extern const char* C3D_ESHADE_NAMES[];
extern const char* C3D_EASRC_NAMES[];
extern const char* C3D_EADST_NAMES[];
extern const char* C3D_EACMP_NAMES[];
extern const char* C3D_ETEXTILE_NAMES[];
extern const char* C3D_ECI_TMAP_TYPE_NAMES[];
extern const char* C3D_ETLIGHT_NAMES[];
extern const char* C3D_ETPERSPCOR_NAMES[];
extern const char* C3D_ETEXFILTER_NAMES[];
extern const char* C3D_ETEXOP_NAMES[];
extern const char* C3D_ETEXCOMPFCN_NAMES[];
extern const char* C3D_EZMODE_NAMES[];
extern const char* C3D_EZCMP_NAMES[];
extern const char* C3D_ERSID_NAMES[];
extern const char* C3D_EASEL_NAMES[];
extern const char* C3D_EPIXFMT_NAMES[];
extern const char* C3D_ETEXFMT_NAMES[];

class Utils
{
public:
    static std::string dumpRenderStateData(
        C3D_ERSID eRStateID, C3D_PRSDATA pRStateData);
};

} // namespace cif
} // namespace glrage
