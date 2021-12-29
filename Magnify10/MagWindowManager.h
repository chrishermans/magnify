#pragma once

#include "stdafx.h"
#include "MagWindow.h"
#include <iostream>
#include <thread>
#include <mutex>

// Calculates a lens size value that is slightly larger than (lens + increment) to give an extra buffer area on the edges
#define LENS_SIZE_BUFFER_VALUE(LENS_SIZE_VALUE, RESIZE_INCREMENT_VALUE) (LENS_SIZE_VALUE + (2 * RESIZE_INCREMENT_VALUE))


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
        _magCount = 8;
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
            _mags[i] = MagWindow((float)pow(1.5f, i + 1), { 0, 0 }, _lensSize);
            if (!_mags[i].Create(hInst, hwndHost, i == 0))
            {
                return FALSE;
            }
            _mags[i].SetSize(_lensSize.cx, _lensSize.cy);
        }

        return TRUE;
    }

    VOID RefreshMagnifier(LPPOINT mousePoint, POINT panOffset)
    {
        _mousePoint = mousePoint;
        _panOffset = panOffset;
        _mags[_activeIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);
    }


    VOID UpdateMagSize(SIZE newSize, SIZE resizeIncrement)
    {
        _lensSize = newSize;
        _mags[_activeIndex].SetSize(
            LENS_SIZE_BUFFER_VALUE(newSize.cx, resizeIncrement.cx),
            LENS_SIZE_BUFFER_VALUE(newSize.cy, resizeIncrement.cy)
        );

        _mags[_activeIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);
        _mags[_activeIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);
    } 

    VOID UpdateMagnification(int newIndex)
    {
        _mags[newIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);
        _mags[newIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);
        
        _mags[newIndex]._windowSize.cx = _lensSize.cx;
        _mags[newIndex]._windowSize.cy = _lensSize.cy;

        _activeIndex = newIndex;
        SetWindowPos(_mags[newIndex].GetHandle(), HWND_TOP,
            0, 0, _lensSize.cx, _lensSize.cy,
            SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOMOVE);
    }


    BOOL IncreaseMagnification()
    {
        if (_activeIndex + 1 >= _magCount) { return FALSE; }
        UpdateMagnification(_activeIndex + 1);
        return TRUE;
    }

    BOOL DecreaseMagnification()
    {
        if (_activeIndex - 1 < 0) { return FALSE; }
        UpdateMagnification(_activeIndex - 1);
        return TRUE;
    }


    VOID IncreaseLensSize(SIZE resizeIncrement)
    {
        SIZE newSize;
        newSize.cx = _lensSize.cx + resizeIncrement.cx;
        newSize.cy = _lensSize.cy + resizeIncrement.cy;
        UpdateMagSize(newSize, resizeIncrement);
    }

    VOID DecreaseLensSize(SIZE resizeIncrement)
    {
        SIZE newSize;
        newSize.cx = _lensSize.cx - resizeIncrement.cx; 
        newSize.cy = _lensSize.cy - resizeIncrement.cy;
        UpdateMagSize(newSize, resizeIncrement);
    }

};