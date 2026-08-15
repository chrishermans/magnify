#pragma once

#include "MagWindow.h"
#include "Global.h"
#include "Config.h"


class MagWindowManager
{

public:
    int _magCount;
    int _activeIndex;
    MagWindow* _mags;

    MagWindowManager()
    {
        _mags = nullptr;
        _magCount = Config::magnificationCurveCount;
        _activeIndex = 0;
    }
    ~MagWindowManager()
    {
        delete[] _mags;
    }

    VOID DestroyWindows()
    {
        if (_mags == nullptr) { return; }
        for (int i = 0; i < _magCount; i++)
        {
            _mags[i].Destroy();
        }
    }

    BOOL Create(HINSTANCE hInst, HWND hwndHost)
    {
        _mags = new MagWindow[_magCount];

        for (int i = 0; i < _magCount; i++)
        {
            if (!_mags[i].Create(hInst, hwndHost, Config::magnificationCurve[i], i == 0))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    BOOL RefreshMagnifier()
    {
        return _mags[_activeIndex].RefreshMagnifier();
    }

    BOOL UpdateActiveIndex(int newIndex)
    {
        if (newIndex >= _magCount || newIndex < 0) { return FALSE; }

        _mags[newIndex].RefreshMagnifier();
        _mags[newIndex].RefreshMagnifier();
        
        _activeIndex = newIndex;
        return SetWindowPos(_mags[newIndex]._hwnd, HWND_TOP,
            0, 0, 0, 0,
            SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
    }

    BOOL IncreaseMagnification()
    {
        return UpdateActiveIndex(_activeIndex + 1);
    }

    BOOL DecreaseMagnification()
    {
        return UpdateActiveIndex(_activeIndex - 1);
    }

    float GetMagFactor()
    {
        return _mags[_activeIndex]._magFactor;
    }

};