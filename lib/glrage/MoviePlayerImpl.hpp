#pragma once

#include <afxwin.h>
#include "Dshow.h"
#include "MoviePlayer.hpp"

class MoviePlayerImpl : public MoviePlayer
{
private:
    CComPtr<IGraphBuilder>          m_builder;
    CComPtr<IMediaControl>          m_control;
    CComPtr<IMediaEvent>            m_event;
    bool                            m_is_complete;
    CWnd                            m_cwnd;

public:
    MoviePlayerImpl(void);
    ~MoviePlayerImpl(void);
    HRESULT Create(HWND hwnd);
    HRESULT Play(LPCSTR path);
    HRESULT GetEvent(HANDLE *handle);
    bool    IsPlaying();
    void    WaitForPlayback();
private:
    HRESULT CreateVMR9(CWnd * wnd);
    HRESULT CreateEVR(CWnd * wnd);
};
