#include "State.hpp"

namespace glrage {
namespace cif {

State::State()
    : m_vars{{
          // clang-format off
          {C3D_COLOR{0, 0, 0, 0}},                      // C3D_ERS_FG_CLR
          {C3D_EVERTEX{C3D_EV_VTCF}},                   // C3D_ERS_VERTEX_TYPE
          {C3D_EPRIM{C3D_EPRIM_TRI}},                   // C3D_ERS_PRIM_TYPE
          {C3D_COLOR{0, 0, 0, 0}},                      // C3D_ERS_SOLID_CLR
          {C3D_ESHADE{C3D_ESH_SMOOTH}},                 // C3D_ERS_SHADE_MODE
          {C3D_BOOL{C3D_FALSE}},                        // C3D_ERS_TMAP_EN
          {C3D_HTX{NULL}},                              // C3D_ERS_TMAP_SELECT
          {C3D_ETLIGHT{C3D_ETL_NONE}},                  // C3D_ERS_TMAP_LIGHT
          {C3D_ETPERSPCOR{C3D_ETPC_THREE}},             // C3D_ERS_TMAP_PERSP_COR
          {C3D_ETEXFILTER{C3D_ETFILT_MINPNT_MAG2BY2}},  // C3D_ERS_TMAP_FILTER
          {C3D_ETEXOP{C3D_ETEXOP_NONE}},                // C3D_ERS_TMAP_TEXOP
          {C3D_EASRC{C3D_EASRC_ONE}},                   // C3D_ERS_ALPHA_SRC
          {C3D_EADST{C3D_EADST_ZERO}},                  // C3D_ERS_ALPHA_DST
          {C3D_PVOID{NULL}},                            // C3D_ERS_SURF_DRAW_PTR
          {C3D_UINT32{NULL}},                           // C3D_ERS_SURF_DRAW_PITCH
          {C3D_EPIXFMT{C3D_EPF_RGB8888}},               // C3D_ERS_SURF_DRAW_PF
          {C3D_RECT{NULL}},                             // C3D_ERS_SURF_VPORT
          {C3D_BOOL{C3D_FALSE}},                        // C3D_ERS_FOG_EN
          {C3D_BOOL{C3D_TRUE}},                         // C3D_ERS_DITHER_EN
          {C3D_EZCMP{C3D_EZCMP_ALWAYS}},                // C3D_ERS_Z_CMP_FNC
          {C3D_EZMODE{C3D_EZMODE_OFF}},                 // C3D_ERS_Z_MODE
          {C3D_PVOID{NULL}},                            // C3D_ERS_SURF_Z_PTR
          {C3D_UINT32{NULL}},                           // C3D_ERS_SURF_Z_PITCH
          {C3D_RECT{NULL}},                             // C3D_ERS_SURF_SCISSOR
          {C3D_BOOL{C3D_FALSE}},                        // C3D_ERS_COMPOSITE_EN
          {C3D_HTX{NULL}},                              // C3D_ERS_COMPOSITE_SELECT
          {C3D_ETEXCOMPFCN{C3D_ETEXCOMPFCN_MAX}},       // C3D_ERS_COMPOSITE_FNC
          {C3D_UINT32{8}},                              // C3D_ERS_COMPOSITE_FACTOR
          {C3D_ETEXFILTER{C3D_ETFILT_MIN2BY2_MAG2BY2}}, // C3D_ERS_COMPOSITE_FILTER
          {C3D_BOOL{C3D_FALSE}},                        // C3D_ERS_COMPOSITE_FACTOR_ALPHA
          {C3D_UINT32{0}},                              // C3D_ERS_LOD_BIAS_LEVEL
          {C3D_BOOL{C3D_FALSE}},                        // C3D_ERS_ALPHA_DST_TEST_ENABLE
          {C3D_EACMP{C3D_EACMP_ALWAYS}},                // C3D_ERS_ALPHA_DST_TEST_FNC
          {C3D_EASEL{C3D_EASEL_ZERO}},                  // C3D_ERS_ALPHA_DST_WRITE_SELECT
          {C3D_UINT32{0}},                              // C3D_ERS_ALPHA_DST_REFERENCE
          {C3D_BOOL{C3D_FALSE}},                        // C3D_ERS_SPECULAR_EN
          {C3D_BOOL{C3D_FALSE}},                        // C3D_ERS_ENHANCED_COLOR_RANGE_EN
          // clang-format on
      }}
{
}

void State::set(C3D_ERSID id, C3D_PRSDATA data)
{
    m_vars[id].set(data);
}

const StateVar::Value& State::get(C3D_ERSID id)
{
    return m_vars[id].get();
}

void State::set(C3D_ERSID id, const StateVar::Value& value)
{
    m_vars[id].set(value);
}

void State::reset()
{
    for (auto& var : m_vars) {
        var.reset();
    }
}

void State::registerObserver(const StateVar::Observer& observer)
{
    for (auto& var : m_vars) {
        var.registerObserver(observer);
    }
}

void State::registerObserver(const StateVar::Observer& observer, C3D_ERSID id)
{
    m_vars[id].registerObserver(observer);
}

} // namespace cif
} // namespace glrage