#include <afxwin.h>
#include "MoviePlayerImpl.hpp"
#include "Dshow.h"
#include "d3d9.h"
#include "vmr9.h"
#include "evr.h"
#include "uuids.h"

MoviePlayerImpl::MoviePlayerImpl(void)
{
}

MoviePlayerImpl::~MoviePlayerImpl(void)
{
    m_cwnd.Detach();
}

HRESULT MoviePlayerImpl::Create(HWND hwnd)
{
    HRESULT hr;

    m_cwnd.Attach(hwnd);

    hr = m_builder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER);
    if(FAILED(hr))
        return hr;

    {
        CComPtr<IFilterGraph> graph;

        hr = m_builder.QueryInterface(&graph);
        if(FAILED(hr))
            return hr;

        hr = graph.QueryInterface(&m_control);
        if(FAILED(hr))
            return hr;

        hr = graph.QueryInterface(&m_event);
        if(FAILED(hr))
            return hr;
    }

    /* Try using the EVR */
    hr = CreateEVR(&m_cwnd);

    /* Use VMR9 as fallback */
    if(FAILED(hr))
        hr = CreateVMR9(&m_cwnd);

    return hr;
}

HRESULT MoviePlayerImpl::Play(LPCSTR path)
{
    HRESULT            hr;
    CA2W               wide_path(path);

    m_is_complete = false;
    hr = m_builder->RenderFile(wide_path, NULL);
    if(FAILED(hr))
        return hr;

    return m_control->Run();
}

HRESULT MoviePlayerImpl::GetEvent(HANDLE *handle)
{
    return m_event->GetEventHandle((OAEVENT *)handle);
}

bool MoviePlayerImpl::IsPlaying()
{
    long     lEventCode;
    LONG_PTR lParam1;
    LONG_PTR lParam2;
    HRESULT  hr;

    hr = m_event->GetEvent(&lEventCode, &lParam1, &lParam2, 0);
    if(hr == S_OK && lEventCode == EC_COMPLETE)
        m_is_complete = true;

    return !m_is_complete;
}

void MoviePlayerImpl::WaitForPlayback()
{
    HANDLE event;
    MSG msg;
    GetEvent(&event);
    bool playing = true;

    // Ignore any already pressed keys
    while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
    {
        if (::GetMessage(&msg, NULL, 0, 0))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
    
    while (playing)
    {
        MsgWaitForMultipleObjects(1, &event, false, INFINITE, QS_ALLINPUT);
        while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            if (::GetMessage(&msg, NULL, 0, 0))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_KEYDOWN)
                {
                    switch (msg.wParam)
                    {
                    case VK_ESCAPE:
                    case VK_RETURN:
                    case VK_SPACE:
                        playing = false;
                    }
                }
            }
        }

        if (!IsPlaying())
            playing = false;
    }

    m_control->Stop();
}

HRESULT MoviePlayerImpl::CreateVMR9(CWnd * wnd)
{
    CComPtr<IBaseFilter> vmr;
    HRESULT              hr;

    hr = vmr.CoCreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER);
    if(FAILED(hr))
        return hr;

    {
        CComPtr<IVMRFilterConfig9> config;

        hr = vmr.QueryInterface(&config);
        if(FAILED(hr))
            return hr;

        hr = config->SetRenderingMode(VMRMode_Windowless);
        if(FAILED(hr))
            return hr;

        hr = config->SetNumberOfStreams(1);
        if(FAILED(hr))
            return hr;
    }

    {
        CComPtr<IVMRWindowlessControl9> win;
        RECT rect;

        hr = vmr.QueryInterface(&win);
        if(FAILED(hr))
            return hr;

        hr = win->SetVideoClippingWindow(wnd->m_hWnd);
        if(FAILED(hr))
            return hr;

        hr = win->SetAspectRatioMode(VMR_ARMODE_LETTER_BOX);
        if(FAILED(hr))
            return hr;

        wnd->GetClientRect(&rect);

        hr = win->SetVideoPosition(NULL, &rect);
        if(FAILED(hr))
            return hr;
    }

    /* Add filter to the graph as the last step so that there
     * is no clean up needed in the error case */
    return m_builder->AddFilter(vmr, L"Video Mixing Renderer");
}

HRESULT MoviePlayerImpl::CreateEVR(CWnd * wnd)
{
    CComPtr<IBaseFilter> evr;
    HRESULT              hr;

    hr = evr.CoCreateInstance(CLSID_EnhancedVideoRenderer, NULL, CLSCTX_INPROC_SERVER);
    if(FAILED(hr))
        return hr;

    {
        CComPtr<IMFGetService>          service;
        CComPtr<IMFVideoDisplayControl> control;
        RECT                            rect;

        hr = evr.QueryInterface(&service);
        if(FAILED(hr))
            return hr;

        hr = service->GetService(MR_VIDEO_RENDER_SERVICE, IID_IMFVideoDisplayControl, (LPVOID *)&control);
        if(FAILED(hr))
            return hr;

        hr = control->SetVideoWindow(wnd->m_hWnd);
        if(FAILED(hr))
            return hr;

        wnd->GetClientRect(&rect);

        hr = control->SetVideoPosition(NULL, &rect);
        if(FAILED(hr))
            return hr;
    }

    /* Add filter to the graph as the last step so that there
     * is no clean up needed in the error case */
    return m_builder->AddFilter(evr, L"Enhanced Video Renderer");
}
