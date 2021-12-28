#pragma once

#include "stdafx.h"
#include "MagWindow.h"
#include <iostream>
#include <thread>
#include <mutex>

class MagWindowManager
{
private:
    int _magCount;
    int _activeIndex;
    MagWindow* _mags;
    LPPOINT _mousePoint;
    POINT _panOffset;

public:
    SIZE _lensSize;
    
    MagWindowManager(SIZE lensSize)
    {
        _mags = nullptr;
        _mousePoint = nullptr;
        _magCount = 10;
        _activeIndex = 0;
        _panOffset = { 0, 0 };
        _lensSize = lensSize;
    }
    ~MagWindowManager()
    {
        delete _mags;
    }

    VOID DestroyWindows()
    {
        if (_mags == nullptr)
        {
            return;
        }
        for (int i = 0; i < _magCount; i++)
        {
            DestroyWindow(_mags[i].GetHandle());
        }
    }

    BOOL Create(HINSTANCE hInst, HWND hwndHost)
    {
        _mags = new MagWindow[_magCount];

        for (int i = 0; i < _magCount; i++)
        {
            _mags[i] = MagWindow(1 + (i * 1.25f), { 0, 0 }, _lensSize);
            if (!_mags[i].Create(hInst, hwndHost, i == 0))
            {
                return FALSE;
            }
            _mags[i].SetSize(_lensSize);
        }

        return TRUE;
    }

    VOID RefreshMagnifier(LPPOINT mousePoint, POINT panOffset)
    {
        _mousePoint = mousePoint;
        _panOffset = panOffset;
        _mags[_activeIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);
    }


    VOID UpdateMagSize(SIZE newSize)
    {
        _lensSize = newSize;
        _mags[_activeIndex].SetSize(_lensSize);
        _mags[_activeIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);
    }


    VOID UpdateMagnification(int previousIndex, int newIndex)
    {
        _mags[newIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);
        _mags[newIndex]._windowSize.cx = _lensSize.cx;
        _mags[newIndex]._windowSize.cy = _lensSize.cy;

        _activeIndex = newIndex;
        SetWindowPos(_mags[newIndex].GetHandle(), HWND_TOP,
            0, 0, _lensSize.cx, _lensSize.cy,
            SWP_SHOWWINDOW | SWP_NOMOVE);
    }


    VOID IncreaseMagnification()
    {
        if (_activeIndex + 1 >= _magCount) { return; }
        UpdateMagnification(_activeIndex, _activeIndex + 1);
    }

    VOID DecreaseMagnification()
    {
        if (_activeIndex - 1 < 0) { return; }
        UpdateMagnification(_activeIndex, _activeIndex - 1);
    }


    VOID IncreaseLensSize(SIZE resizeIncrement, HWND hwndHost)
    {
        SIZE newSize;
        newSize.cx = _lensSize.cx + resizeIncrement.cx;
        newSize.cy = _lensSize.cy + resizeIncrement.cy;
        UpdateMagSize(newSize);
    }

    VOID DecreaseLensSize(SIZE resizeIncrement, HWND hwndHost)
    {
        SIZE newSize;
        newSize.cx = _lensSize.cx - resizeIncrement.cx;
        newSize.cy = _lensSize.cy - resizeIncrement.cy;
        UpdateMagSize(newSize);
    }

};