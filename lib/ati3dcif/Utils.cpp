#include "Utils.hpp"

#include <sstream>

namespace glrage {
namespace cif {

const char* C3D_EC_NAMES[] = {
    "C3D_EC_OK",
    "C3D_EC_GENFAIL",
    "C3D_EC_MEMALLOCFAIL",
    "C3D_EC_BADPARAM",
    "C3D_EC_UNUSED0",
    "C3D_EC_BADSTATE",
    "C3D_EC_NOTIMPYET",
    "C3D_EC_UNUSED1",
    "C3D_EC_CHIPCAPABILITY",
    "C3D_EC_NUM",
    "C3D_EC_FORCE_U32",
};

const char* C3D_EVERTEX_NAMES[] = {
    "C3D_EV_VF",
    "C3D_EV_VCF",
    "C3D_EV_VTF",
    "C3D_EV_VTCF",
    "C3D_EV_TLVERTEX",
    "C3D_EV_NUM",
    "C3D_EV_FORCE_U32",
};

const char* C3D_EPRIM_NAMES[] = {
    "C3D_EPRIM_LINE",
    "C3D_EPRIM_TRI",
    "C3D_EPRIM_QUAD",
    "C3D_EPRIM_RECT",
    "C3D_EPRIM_POINT",
    "C3D_EPRIM_NUM",
    "C3D_EPRIM_FORCE_U32",
};

const char* C3D_ESHADE_NAMES[] = {
    "C3D_ESH_NONE",
    "C3D_ESH_SOLID",
    "C3D_ESH_FLAT",
    "C3D_ESH_SMOOTH",
    "C3D_ESH_NUM",
    "C3D_ESH_FORCE_U32",
};

const char* C3D_EASRC_NAMES[] = {
    "C3D_EASRC_ZERO",
    "C3D_EASRC_ONE",
    "C3D_EASRC_DSTCLR",
    "C3D_EASRC_INVDSTCLR",
    "C3D_EASRC_SRCALPHA",
    "C3D_EASRC_INVSRCALPHA",
    "C3D_EASRC_DSTALPHA",
    "C3D_EASRC_INVDSTALPHA",
    "C3D_EASRC_NUM",
    "C3D_EASRC_FORCE_U32",
};

const char* C3D_EADST_NAMES[] = {
    "C3D_EADST_ZERO",
    "C3D_EADST_ONE",
    "C3D_EADST_SRCCLR",
    "C3D_EADST_INVSRCCLR",
    "C3D_EADST_SRCALPHA",
    "C3D_EADST_INVSRCALPHA",
    "C3D_EADST_DSTALPHA",
    "C3D_EADST_INVDSTALPHA",
    "C3D_EADST_NUM",
    "C3D_EADST_FORCE_U32",
};

const char* C3D_EACMP_NAMES[] = {
    "C3D_EACMP_NEVER",
    "C3D_EACMP_LESS",
    "C3D_EACMP_LEQUAL",
    "C3D_EACMP_EQUAL",
    "C3D_EACMP_GEQUAL",
    "C3D_EACMP_GREATER",
    "C3D_EACMP_NOTEQUAL",
    "C3D_EACMP_ALWAYS",
    "C3D_EACMP_MAX",
    "C3D_EACMP_FORCE_U32",
};

const char* C3D_ETEXTILE_NAMES[] = {
    "C3D_ETEXTILE_DEFAULT",
    "C3D_ETEXTILE_OFF",
    "C3D_ETEXTILE_ON",
    "C3D_ETEXTILE_MAX",
    "C3D_ETEXTILE_FORCE_U32",
};

const char* C3D_ECI_TMAP_TYPE_NAMES[] = {
    "C3D_ECI_TMAP_TRUE_COLOR",
    "C3D_ECI_TMAP_4BIT_HI",
    "C3D_ECI_TMAP_4BIT_LOW",
    "C3D_ECI_TMAP_8BIT",
    "C3D_ECI_TMAP_VQ",
    "C3D_ECI_TMAP_NUM",
    "C3D_ECI_TMAP_FORCE_U32",
};

const char* C3D_ETLIGHT_NAMES[] = {
    "C3D_ETL_NONE",
    "C3D_ETL_MODULATE",
    "C3D_ETL_ALPHA_DECAL",
    "C3D_ETL_NUM",
    "C3D_ETL_FORCE_U32",
};

const char* C3D_ETPERSPCOR_NAMES[] = {
    "C3D_ETPC_NONE",
    "C3D_ETPC_ONE",
    "C3D_ETPC_TWO",
    "C3D_ETPC_THREE",
    "C3D_ETPC_FOUR",
    "C3D_ETPC_FIVE",
    "C3D_ETPC_SIX",
    "C3D_ETPC_SEVEN",
    "C3D_ETPC_EIGHT",
    "C3D_ETPC_NINE",
    "C3D_ETPC_NUM",
    "C3D_ETPC_FORCE_U32",
};

const char* C3D_ETEXFILTER_NAMES[] = {
    "C3D_ETFILT_MINPNT_MAGPNT",
    "C3D_ETFILT_MINPNT_MAG2BY2",
    "C3D_ETFILT_MIN2BY2_MAG2BY2",
    "C3D_ETFILT_MIPLIN_MAGPNT",
    "C3D_ETFILT_MIPLIN_MAG2BY2",
    "C3D_ETFILT_MIPTRI_MAG2BY2",
    "C3D_ETFILT_MIN2BY2_MAGPNT",
    "C3D_ETFILT_NUM",
    "C3D_ETFILT_FORCE_U32",
};

const char* C3D_ETEXOP_NAMES[] = {
    "C3D_ETEXOP_NONE",
    "C3D_ETEXOP_CHROMAKEY",
    "C3D_ETEXOP_ALPHA",
    "C3D_ETEXOP_ALPHA_MASK",
    "C3D_ETEXOP_NUM",
    "C3D_ETEXOP_FORCE_U32",
};

const char* C3D_ETEXCOMPFCN_NAMES[] = {
    "C3D_ETEXCOMPFCN_BLEND",
    "C3D_ETEXCOMPFCN_MOD",
    "C3D_ETEXCOMPFCN_ADD_SPEC",
    "C3D_ETEXCOMPFCN_MAX",
    "C3D_ETEXCOMPFCN_FORCE_U32",
};

const char* C3D_EZMODE_NAMES[] = {
    "C3D_EZMODE_OFF",
    "C3D_EZMODE_TESTON",
    "C3D_EZMODE_TESTON_WRITEZ",
    "C3D_EZMODE_MAX",
    "C3D_EZMODE_FORCE_U32",
};

const char* C3D_EZCMP_NAMES[] = {
    "C3D_EZCMP_NEVER",
    "C3D_EZCMP_LESS",
    "C3D_EZCMP_LEQUAL",
    "C3D_EZCMP_EQUAL",
    "C3D_EZCMP_GEQUAL",
    "C3D_EZCMP_GREATER",
    "C3D_EZCMP_NOTEQUAL",
    "C3D_EZCMP_ALWAYS",
    "C3D_EZCMP_MAX",
    "C3D_EZCMP_FORCE_U32",
};

const char* C3D_ERSID_NAMES[] = {
    "C3D_ERS_FG_CLR",
    "C3D_ERS_VERTEX_TYPE",
    "C3D_ERS_PRIM_TYPE",
    "C3D_ERS_SOLID_CLR",
    "C3D_ERS_SHADE_MODE",
    "C3D_ERS_TMAP_EN",
    "C3D_ERS_TMAP_SELECT",
    "C3D_ERS_TMAP_LIGHT",
    "C3D_ERS_TMAP_PERSP_COR",
    "C3D_ERS_TMAP_FILTER",
    "C3D_ERS_TMAP_TEXOP",
    "C3D_ERS_ALPHA_SRC",
    "C3D_ERS_ALPHA_DST",
    "C3D_ERS_SURF_DRAW_PTR",
    "C3D_ERS_SURF_DRAW_PITCH",
    "C3D_ERS_SURF_DRAW_PF",
    "C3D_ERS_SURF_VPORT",
    "C3D_ERS_FOG_EN",
    "C3D_ERS_DITHER_EN",
    "C3D_ERS_Z_CMP_FNC",
    "C3D_ERS_Z_MODE",
    "C3D_ERS_SURF_Z_PTR",
    "C3D_ERS_SURF_Z_PITCH",
    "C3D_ERS_SURF_SCISSOR",
    "C3D_ERS_COMPOSITE_EN",
    "C3D_ERS_COMPOSITE_SELECT",
    "C3D_ERS_COMPOSITE_FNC",
    "C3D_ERS_COMPOSITE_FACTOR",
    "C3D_ERS_COMPOSITE_FILTER",
    "C3D_ERS_COMPOSITE_FACTOR_ALPHA",
    "C3D_ERS_LOD_BIAS_LEVEL",
    "C3D_ERS_ALPHA_DST_TEST_ENABLE",
    "C3D_ERS_ALPHA_DST_TEST_FNC",
    "C3D_ERS_ALPHA_DST_WRITE_SELECT",
    "C3D_ERS_ALPHA_DST_REFERENCE",
    "C3D_ERS_SPECULAR_EN",
    "C3D_ERS_ENHANCED_COLOR_RANGE_EN",
    "C3D_ERS_NUM",
    "C3D_ERS_FORCE_U32",
};

const char* C3D_EASEL_NAMES[]{
    "C3D_EASEL_ZERO",
    "C3D_EASEL_ONE",
    "",
    "",
    "C3D_EASEL_SRCALPHA",
    "C3D_EASEL_INVSRCALPHA",
    "C3D_EASEL_DSTALPHA",
    "C3D_EASEL_INVDSTALPHA",
    "C3D_EASEL_FORCE_U32",
};

const char* C3D_EPIXFMT_NAMES[] = {
    "",
    "",
    "",
    "C3D_EPF_RGB1555",
    "C3D_EPF_RGB565",
    "",
    "C3D_EPF_RGB8888",
    "C3D_EPF_RGB332",
    "C3D_EPF_Y8",
    "",
    "",
    "C3D_EPF_YUV422",
    "C3D_EPF_FORCE_U32",
};

const char* C3D_ETEXFMT_NAMES[] = {
    "",
    "C3D_ETF_CI4",
    "C3D_ETF_CI8",
    "C3D_ETF_RGB1555",
    "C3D_ETF_RGB565",
    "",
    "C3D_ETF_RGB8888",
    "C3D_ETF_RGB332",
    "C3D_ETF_Y8",
    "",
    "",
    "C3D_ETF_YUV422",
    "",
    "",
    "",
    "C3D_ETF_RGB4444",
    "",
    "",
    "",
    "",
    "C3D_ETF_VQ",
    "C3D_ETF_FORCE_U32",
};

std::string Utils::dumpRenderStateData(C3D_ERSID eRStateID,
    C3D_PRSDATA pRStateData)
{
    std::stringstream ss;

    switch (eRStateID) {
        // C3D_PVOID / C3D_HTX
        case C3D_ERS_SURF_DRAW_PTR:
        case C3D_ERS_SURF_Z_PTR:
        case C3D_ERS_TMAP_SELECT:
        case C3D_ERS_COMPOSITE_SELECT:
            ss << std::hex << pRStateData;
            break;

        // C3D_PUINT32
        case C3D_ERS_SURF_DRAW_PITCH:
        case C3D_ERS_SURF_Z_PITCH:
        case C3D_ERS_COMPOSITE_FACTOR:
        case C3D_ERS_LOD_BIAS_LEVEL:
        case C3D_ERS_ALPHA_DST_REFERENCE:
            ss << *static_cast<C3D_PUINT32>(pRStateData);
            break;

        // C3D_BOOL
        case C3D_ERS_TMAP_EN:
        case C3D_ERS_FOG_EN:
        case C3D_ERS_DITHER_EN:
        case C3D_ERS_COMPOSITE_FACTOR_ALPHA:
        case C3D_ERS_COMPOSITE_EN:
        case C3D_ERS_ALPHA_DST_TEST_ENABLE:
        case C3D_ERS_SPECULAR_EN:
        case C3D_ERS_ENHANCED_COLOR_RANGE_EN: {
            ss << *static_cast<C3D_PBOOL>(pRStateData) ? "TRUE" : "FALSE";
            break;
        }

        // C3D_COLOR
        case C3D_ERS_FG_CLR:
        case C3D_ERS_SOLID_CLR: {
            C3D_PCOLOR color = static_cast<C3D_PCOLOR>(pRStateData);
            ss << "{" << color->r << "," << color->g << "," << color->b << ","
               << color->a << "}";
            break;
        }

        // C3D_EVERTEX
        case C3D_ERS_VERTEX_TYPE:
            // strncpy_s(str, size,
            // C3D_EVERTEX_NAMES[*static_cast<C3D_PEVERTEX>(pRStateData)].c_str(),
            // _TRUNCATE);
            ss << C3D_EVERTEX_NAMES[*static_cast<C3D_PEVERTEX>(pRStateData)];
            break;

        // C3D_EPRIM
        case C3D_ERS_PRIM_TYPE:
            ss << C3D_EPRIM_NAMES[*static_cast<C3D_PEPRIM>(pRStateData)];
            break;

        // C3D_ESHADE
        case C3D_ERS_SHADE_MODE:
            ss << C3D_ESHADE_NAMES[*static_cast<C3D_PESHADE>(pRStateData)];
            break;

        // C3D_ETLIGHT
        case C3D_ERS_TMAP_LIGHT:
            ss << C3D_ETLIGHT_NAMES[*static_cast<C3D_PETLIGHT>(pRStateData)];
            break;

        // C3D_ETPERSPCOR
        case C3D_ERS_TMAP_PERSP_COR:
            ss << C3D_ETPERSPCOR_NAMES[*static_cast<C3D_PETPERSPCOR>(
                pRStateData)];
            break;

        // C3D_ETEXFILTER
        case C3D_ERS_TMAP_FILTER:
        case C3D_ERS_COMPOSITE_FILTER:
            ss << C3D_ETEXFILTER_NAMES[*static_cast<C3D_PETEXFILTER>(
                pRStateData)];
            break;

        // C3D_ETEXOP
        case C3D_ERS_TMAP_TEXOP:
            ss << C3D_ETEXOP_NAMES[*static_cast<C3D_PETEXOP>(pRStateData)];
            break;

        // C3D_EASRC
        case C3D_ERS_ALPHA_SRC:
            ss << C3D_EASRC_NAMES[*static_cast<C3D_PEASRC>(pRStateData)];
            break;

        // C3D_EADST
        case C3D_ERS_ALPHA_DST:
            ss << C3D_EADST_NAMES[*static_cast<C3D_PEADST>(pRStateData)];
            break;

        // C3D_EPIXFMT
        case C3D_ERS_SURF_DRAW_PF:
            ss << C3D_EPIXFMT_NAMES[*static_cast<C3D_PEPIXFMT>(pRStateData)];
            break;

        // C3D_RECT
        case C3D_ERS_SURF_VPORT:
        case C3D_ERS_SURF_SCISSOR: {
            C3D_PRECT rect = (C3D_PRECT)pRStateData;
            ss << "{" << rect->top << "," << rect->left << "," << rect->bottom
               << "," << rect->right << "}";
            break;
        }

        // C3D_EZCMP
        case C3D_ERS_Z_CMP_FNC:
            ss << C3D_EZCMP_NAMES[*static_cast<C3D_PEZCMP>(pRStateData)];
            break;

        // C3D_EZMODE
        case C3D_ERS_Z_MODE:
            ss << C3D_EZMODE_NAMES[*static_cast<C3D_PEZMODE>(pRStateData)];
            break;

        // C3D_ETEXCOMPFCN
        case C3D_ERS_COMPOSITE_FNC:
            ss << C3D_ETEXCOMPFCN_NAMES[*static_cast<C3D_PETEXCOMPFCN>(
                pRStateData)];
            break;

        // C3D_EACMP
        case C3D_ERS_ALPHA_DST_TEST_FNC:
            ss << C3D_EACMP_NAMES[*static_cast<C3D_PEACMP>(pRStateData)];
            break;

        // C3D_EASEL
        case C3D_ERS_ALPHA_DST_WRITE_SELECT:
            ss << C3D_EASEL_NAMES[*static_cast<C3D_PEASEL>(pRStateData)];
            break;

        case C3D_ERS_NUM: {
            ss << "INVALID";
            break;
        }

        default:
            ss << "UNKNOWN (" << eRStateID << ")";
            break;
    }

    return ss.str();
}

} // namespace cif
} // namespace glrage
