#pragma once

class MoviePlayer
{
public:
    virtual HRESULT Create(HWND hwnd)=0;
    virtual HRESULT Play(LPCSTR path)=0;
    virtual HRESULT GetEvent(HANDLE *handle)=0;
    virtual bool    IsPlaying()=0;
    virtual void    WaitForPlayback() = 0;

    static MoviePlayer *GetPlayer();
};