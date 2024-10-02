#include "game/hwr.h"

#include "decomp/decomp.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <d3d.h>

HRESULT HWR_DrawPrimitive(
    D3DPRIMITIVETYPE primitive_type, LPVOID vertices, DWORD vtx_count,
    bool is_no_clip)
{
    return g_D3DDev->lpVtbl->DrawPrimitive(
        g_D3DDev, primitive_type, D3DVT_TLVERTEX, vertices, vtx_count,
        is_no_clip ? D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTCLIP : 0);
}

void __cdecl HWR_InitState(void)
{
    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);

    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);

    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);

    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);

    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);

    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_TEXTUREPERSPECTIVE,
        g_SavedAppSettings.perspective_correct ? TRUE : FALSE);

    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_DITHERENABLE,
        g_SavedAppSettings.dither ? TRUE : FALSE);

    g_AlphaBlendEnabler = g_CurrentDisplayAdapter.shade_restricted
        ? D3DRENDERSTATE_STIPPLEDALPHA
        : D3DRENDERSTATE_ALPHABLENDENABLE;

    const DWORD texture_filter = g_SavedAppSettings.bilinear_filtering
        ? D3DFILTER_LINEAR
        : D3DFILTER_NEAREST;
    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_TEXTUREMAG, texture_filter);
    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_TEXTUREMIN, texture_filter);

    const DWORD blend_mode =
        (g_CurrentDisplayAdapter.hw_device_desc.dpcTriCaps.dwTextureBlendCaps
         & D3DPTBLENDCAPS_MODULATEALPHA)
        ? D3DTBLEND_MODULATEALPHA
        : D3DTBLEND_MODULATE;
    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_TEXTUREMAPBLEND, blend_mode);

    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_TEXTUREADDRESS, D3DTADDRESS_CLAMP);

    HWR_ResetTexSource();
    HWR_ResetColorKey();
    HWR_ResetZBuffer();
}

void __cdecl HWR_ResetTexSource(void)
{
    g_CurrentTexSource = 0;
    g_D3DDev->lpVtbl->SetRenderState(g_D3DDev, D3DRENDERSTATE_TEXTUREHANDLE, 0);
    g_D3DDev->lpVtbl->SetRenderState(g_D3DDev, D3DRENDERSTATE_FLUSHBATCH, 0);
}

void __cdecl HWR_ResetColorKey(void)
{
    g_ColorKeyState = FALSE;
    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev,
        g_TexturesAlphaChannel ? D3DRENDERSTATE_ALPHABLENDENABLE
                               : D3DRENDERSTATE_COLORKEYENABLE,
        FALSE);
}

void __cdecl HWR_ResetZBuffer(void)
{
    g_ZEnableState = FALSE;
    g_ZWriteEnableState = FALSE;
    if (g_ZBufferSurface != NULL) {
        g_D3DDev->lpVtbl->SetRenderState(
            g_D3DDev, D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
        g_D3DDev->lpVtbl->SetRenderState(
            g_D3DDev, D3DRENDERSTATE_ZENABLE,
            g_SavedAppSettings.zbuffer ? TRUE : FALSE);
    } else {
        g_D3DDev->lpVtbl->SetRenderState(
            g_D3DDev, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
        g_D3DDev->lpVtbl->SetRenderState(
            g_D3DDev, D3DRENDERSTATE_ZENABLE, FALSE);
    }
    g_D3DDev->lpVtbl->SetRenderState(
        g_D3DDev, D3DRENDERSTATE_ZWRITEENABLE, FALSE);
}

void __cdecl HWR_TexSource(const HWR_TEXTURE_HANDLE tex_source)
{
    if (g_CurrentTexSource != tex_source) {
        g_D3DDev->lpVtbl->SetRenderState(
            g_D3DDev, D3DRENDERSTATE_TEXTUREHANDLE, tex_source);
        g_CurrentTexSource = tex_source;
    }
}

void __cdecl HWR_EnableColorKey(const bool state)
{
    if (g_ColorKeyState != state) {
        g_D3DDev->lpVtbl->SetRenderState(
            g_D3DDev,
            g_TexturesAlphaChannel ? D3DRENDERSTATE_ALPHABLENDENABLE
                                   : D3DRENDERSTATE_COLORKEYENABLE,
            state ? TRUE : FALSE);
        g_ColorKeyState = state;
    }
}

void __cdecl HWR_EnableZBuffer(const bool z_write_enable, const bool z_enable)
{
    if (!g_SavedAppSettings.zbuffer) {
        return;
    }

    if (g_ZWriteEnableState != z_write_enable) {
        g_D3DDev->lpVtbl->SetRenderState(
            g_D3DDev, D3DRENDERSTATE_ZWRITEENABLE,
            z_write_enable ? TRUE : FALSE);
        g_ZWriteEnableState = z_write_enable;
    }

    if (g_ZEnableState != z_enable) {
        if (g_ZBufferSurface != NULL) {
            g_D3DDev->lpVtbl->SetRenderState(
                g_D3DDev, D3DRENDERSTATE_ZFUNC,
                z_enable ? D3DCMP_LESSEQUAL : D3DCMP_ALWAYS);
        } else {
            g_D3DDev->lpVtbl->SetRenderState(
                g_D3DDev, D3DRENDERSTATE_ZENABLE, z_enable ? TRUE : FALSE);
        }
        g_ZEnableState = z_enable;
    }
}

void __cdecl HWR_BeginScene(void)
{
    HWR_GetPageHandles();
    WaitPrimaryBufferFlip();
    g_D3DDev->lpVtbl->BeginScene(g_D3DDev);
}

void __cdecl HWR_DrawPolyList(void)
{
    HWR_EnableZBuffer(false, true);

    for (int32_t i = 0; i < g_SurfaceCount; i++) {
        uint16_t *buf_ptr = (uint16_t *)g_SortBuffer[i]._0;

        uint16_t poly_type = *buf_ptr++;
        uint16_t tex_page =
            (poly_type == POLY_HWR_GTMAP || poly_type == POLY_HWR_WGTMAP)
            ? *buf_ptr++
            : 0;
        uint16_t vtx_count = *buf_ptr++;
        D3DTLVERTEX *vtx_ptr = *(D3DTLVERTEX **)buf_ptr;

        switch (poly_type) {
        // triangle fan (texture)
        case POLY_HWR_GTMAP:
        // triangle fan (texture + colorkey)
        case POLY_HWR_WGTMAP:
            HWR_TexSource(g_HWR_PageHandles[tex_page]);
            HWR_EnableColorKey(poly_type == POLY_HWR_WGTMAP);
            HWR_DrawPrimitive(D3DPT_TRIANGLEFAN, vtx_ptr, vtx_count, true);
            break;

        // triangle fan (color)
        case POLY_HWR_GOURAUD:
            HWR_TexSource(0);
            HWR_EnableColorKey(false);
            HWR_DrawPrimitive(D3DPT_TRIANGLEFAN, vtx_ptr, vtx_count, true);
            break;

        // line strip (color)
        case POLY_HWR_LINE:
            HWR_TexSource(0);
            HWR_EnableColorKey(false);
            HWR_DrawPrimitive(D3DPT_LINESTRIP, vtx_ptr, vtx_count, true);
            break;

        // triangle fan (color + semitransparent)
        case POLY_HWR_TRANS: {
            DWORD alpha_state;
            HWR_TexSource(0);
            g_D3DDev->lpVtbl->GetRenderState(
                g_D3DDev, g_AlphaBlendEnabler, &alpha_state);
            g_D3DDev->lpVtbl->SetRenderState(
                g_D3DDev, g_AlphaBlendEnabler, TRUE);
            HWR_DrawPrimitive(D3DPT_TRIANGLEFAN, vtx_ptr, vtx_count, true);
            g_D3DDev->lpVtbl->SetRenderState(
                g_D3DDev, g_AlphaBlendEnabler, alpha_state);
            break;
        }
        }
    }
}

void __cdecl HWR_LoadTexturePages(
    const int32_t pages_count, const void *const pages_buffer,
    const RGB_888 *const palette)
{
    int32_t page_idx = -1;
    const BYTE *buffer_ptr = (const BYTE *)pages_buffer;

    HWR_FreeTexturePages();

    if (palette != NULL) {
        g_PaletteIndex = CreateTexturePalette(palette);
    }

    for (int32_t i = 0; i < pages_count; i++) {
        if (palette != NULL) {
            page_idx = AddTexturePage8(256, 256, buffer_ptr, g_PaletteIndex);
            buffer_ptr += 256 * 256 * 1;
        } else {
            page_idx = AddTexturePage16(256, 256, buffer_ptr);
            buffer_ptr += 256 * 256 * 2;
        }
        g_HWR_TexturePageIndexes[i] = page_idx < 0 ? -1 : page_idx;
    }

    HWR_GetPageHandles();
}

void __cdecl HWR_FreeTexturePages(void)
{
    for (int32_t i = 0; i < MAX_TEXTURE_PAGES; i++) {
        if (g_HWR_TexturePageIndexes[i] >= 0) {
            SafeFreeTexturePage(g_HWR_TexturePageIndexes[i]);
            g_HWR_TexturePageIndexes[i] = -1;
        }
        g_HWR_PageHandles[i] = 0;
    }
    if (g_PaletteIndex >= 0) {
        SafeFreePalette(g_PaletteIndex);
    }
}

void __cdecl HWR_GetPageHandles(void)
{
    for (int32_t i = 0; i < MAX_TEXTURE_PAGES; i++) {
        if (g_HWR_TexturePageIndexes[i] < 0)
            g_HWR_PageHandles[i] = 0;
        else
            g_HWR_PageHandles[i] =
                GetTexturePageHandle(g_HWR_TexturePageIndexes[i]);
    }
}

bool __cdecl HWR_VertexBufferFull(void)
{
    const int32_t index =
        (g_HWR_VertexPtr - g_HWR_VertexBuffer) / sizeof(D3DTLVERTEX);
    return index >= MAX_VERTICES;
}

bool __cdecl HWR_Init(void)
{
    memset(g_HWR_VertexBuffer, 0, sizeof(g_HWR_VertexBuffer));
    memset(
        g_HWR_TexturePageIndexes, (uint8_t)-1,
        sizeof(g_HWR_TexturePageIndexes));
    return true;
}
