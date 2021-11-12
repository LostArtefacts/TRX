#pragma once

#include "ati3dcif.hpp"

#include <string>

namespace glrage {
namespace cif {

const char* C3D_EC_NAMES[];
const char* C3D_EVERTEX_NAMES[];
const char* C3D_EPRIM_NAMES[];
const char* C3D_ESHADE_NAMES[];
const char* C3D_EASRC_NAMES[];
const char* C3D_EADST_NAMES[];
const char* C3D_EACMP_NAMES[];
const char* C3D_ETEXTILE_NAMES[];
const char* C3D_ECI_TMAP_TYPE_NAMES[];
const char* C3D_ETLIGHT_NAMES[];
const char* C3D_ETPERSPCOR_NAMES[];
const char* C3D_ETEXFILTER_NAMES[];
const char* C3D_ETEXOP_NAMES[];
const char* C3D_ETEXCOMPFCN_NAMES[];
const char* C3D_EZMODE_NAMES[];
const char* C3D_EZCMP_NAMES[];
const char* C3D_ERSID_NAMES[];
const char* C3D_EASEL_NAMES[];
const char* C3D_EPIXFMT_NAMES[];
const char* C3D_ETEXFMT_NAMES[];

class Utils
{
public:
    static std::string dumpRenderStateData(
        C3D_ERSID eRStateID, C3D_PRSDATA pRStateData);
};

} // namespace cif
} // namespace glrage