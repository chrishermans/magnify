#pragma once

#include "stdafx.h"
#include "MagWindow.h"
#include <iostream>
#include <thread>
#include <mutex>

// Calculates a lens size value that is slightly larger than (lens + increment) to give an extra buffer area on the edges
#define LENS_SIZE_BUFFER_VALUE(LENS_SIZE_VALUE, RESIZE_INCREMENT_VALUE) (LENS_SIZE_VALUE + (2 * RESIZE_INCREMENT_VALUE))

const int INIT_MAG_COUNT = 16;

class MagWindowManager
{

public:
    int _magCount;
    int _activeIndex;
    SIZE _screenSize;
    SIZE _lensSize;
    MagWindow* _mags;
    LPPOINT _mousePoint;
    POINT _panOffset;
    float _magFactorCurve[INIT_MAG_COUNT] = {
        1.5f, 1.75f, 2.0f, 2.5f, 3.0f, 3.5, 4.25f, 5.0f,
        6.0f, 7.0f, 8.5f, 10.0f, 12.0f, 15.0f, 20.0f, 26.0f
    };

    MagWindowManager(SIZE lensSize, SIZE screenSize)
    {
        _mags = nullptr;
        _mousePoint = nullptr;
        _magCount = INIT_MAG_COUNT;
        _activeIndex = 0;
        _panOffset = { 0, 0 };
        _lensSize = lensSize;
        _screenSize = screenSize;
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
            _mags[i] = MagWindow(_magFactorCurve[i], { 0, 0 }, _screenSize);
            if (!_mags[i].Create(hInst, hwndHost, i == 0))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    VOID UpdateParameters(LPPOINT mousePoint, POINT panOffset)
    {
        _mousePoint = mousePoint;
        _panOffset = panOffset;
    }

    BOOL RefreshMagnifier(LPPOINT mousePoint, POINT panOffset, POINT lensPosition)
    {
        _mousePoint = mousePoint;
        _panOffset = panOffset;
        return _mags[_activeIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize, lensPosition);
    }

    BOOL UpdateMagSize(SIZE newSize, SIZE resizeIncrement, SIZE limit)
    {
        if (newSize.cx > limit.cx || newSize.cy > limit.cy ||
            newSize.cx <= resizeIncrement.cx || newSize.cy <= resizeIncrement.cy)
        {
            return FALSE;
        }

        _lensSize = newSize;
        return TRUE;
    } 

    BOOL UpdateMagnification(int newIndex, POINT lensPosition)
    {
        if (newIndex >= _magCount || newIndex < 0) { return FALSE; }

        _mags[newIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize, lensPosition);
        _mags[newIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize, lensPosition);
        
        _activeIndex = newIndex;
        return SetWindowPos(_mags[newIndex].GetHandle(), HWND_TOP,
            0, 0, 0, 0,
            SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
    }

    BOOL IncreaseMagnification(POINT lensPosition)
    {
        return UpdateMagnification(_activeIndex + 1, lensPosition);
    }

    BOOL DecreaseMagnification(POINT lensPosition)
    {
        return UpdateMagnification(_activeIndex - 1, lensPosition);
    }

    BOOL IncreaseLensSize(SIZE increment, SIZE limit)
    {
        return UpdateMagSize(
            { _lensSize.cx + increment.cx,
              _lensSize.cy + increment.cy },
            increment, limit);
    }

    BOOL DecreaseLensSize(SIZE increment, SIZE limit)
    {
        return UpdateMagSize(
            { _lensSize.cx - increment.cx,
              _lensSize.cy - increment.cy },
            increment, limit);
    }

    float GetMagFactor()
    {
        return _mags[_activeIndex]._magFactor;
    }

};