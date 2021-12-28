#pragma once

#include "stdafx.h"
#include "MagWindow.h"
#include <iostream>
#include <thread>
#include <mutex>

// Calculates a lens size value that is slightly larger than (lens + increment) to give an extra buffer area on the edges
#define LENS_SIZE_BUFFER_VALUE(LENS_SIZE_VALUE, RESIZE_INCREMENT_VALUE) (LENS_SIZE_VALUE + (2 * RESIZE_INCREMENT_VALUE))
#define LENS_POSITION_VALUE(MOUSEPOINT_VALUE, LENSSIZE_VALUE) (MOUSEPOINT_VALUE - (LENSSIZE_VALUE / 2) - 1)


class MagWindowManager
{
private:
    int magCount;
    int activeIndex;
    MagWindow* mags;
    POINT _windowPosition;
    LPPOINT _mousePoint;
    POINT _panOffset;
    float _magFactor;

public:
    SIZE _lensSize;
    
    MagWindowManager() { }
    MagWindowManager(POINT windowPosition, SIZE windowSize)
    {
        _magFactor = 1.0f;
        mags = nullptr;
        magCount = 10;
        activeIndex = 0;
        _lensSize = windowSize;
        _windowPosition = windowPosition;
        
    }
    ~MagWindowManager()
    {
        delete mags;
    }

    VOID DestroyWindows()
    {
        if (mags == nullptr)
        {
            return;
        }
        for (int i = 0; i < magCount; i++)
        {
            DestroyWindow(mags[i].GetHandle());
        }
    }

    BOOL Create(HINSTANCE hInst, HWND hwndHost)
    {
        mags = new MagWindow[magCount];

        for (int i = 0; i < magCount; i++)
        {
            mags[i] = MagWindow(1 + (i * 1.25f), _windowPosition, _lensSize);
            if (!mags[i].Create(hInst, hwndHost, i == 0))
            {
                return FALSE;
            }
            mags[i].SetSize(_lensSize.cx, _lensSize.cy);
        }

        return TRUE;
    }

    VOID RefreshMagnifier(LPPOINT mousePoint, POINT panOffset)
    {
        _mousePoint = mousePoint;
        _panOffset = panOffset;
        mags[activeIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);
    }


    VOID UpdateMagSize(SIZE newSize, SIZE resizeIncrement)
    {
        _lensSize = newSize;
        
        //mags[activeIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);

        mags[activeIndex]._windowSize.cx = _lensSize.cx;
        mags[activeIndex]._windowSize.cy = _lensSize.cy;

        SetWindowPos(mags[activeIndex].GetHandle(), HWND_TOP,
            0, 0, //_windowPosition.x, _windowPosition.y,
            _lensSize.cx, _lensSize.cy,
            SWP_SHOWWINDOW);
        
        mags[activeIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);


    }


    VOID UpdateMagnification(int previousIndex, int newIndex)
    {
        mags[newIndex].RefreshMagnifier(_mousePoint, _panOffset, _lensSize);
        mags[newIndex]._windowSize.cx = _lensSize.cx;
        mags[newIndex]._windowSize.cy = _lensSize.cy;

        SetWindowPos(mags[newIndex].GetHandle(), HWND_TOP,
            _windowPosition.x, _windowPosition.y,
            _lensSize.cx, _lensSize.cy,
            SWP_SHOWWINDOW);
        activeIndex = newIndex;
    }


    VOID IncreaseMagnification()
    {
        if (activeIndex + 1 >= magCount) { return; }
        UpdateMagnification(activeIndex, activeIndex + 1);
    }

    VOID DecreaseMagnification()
    {
        if (activeIndex - 1 < 0) { return; }
        UpdateMagnification(activeIndex, activeIndex - 1);
    }


    VOID IncreaseLensSize(SIZE resizeIncrement, HWND hwndHost)
    {
        SIZE newSize;
        newSize.cx = _lensSize.cx + resizeIncrement.cx;
        newSize.cy = _lensSize.cy + resizeIncrement.cy;
        UpdateMagSize(newSize, resizeIncrement);
    }

    VOID DecreaseLensSize(SIZE resizeIncrement, HWND hwndHost)
    {
        SIZE newSize;
        newSize.cx = _lensSize.cx - resizeIncrement.cx;
        newSize.cy = _lensSize.cy - resizeIncrement.cy;
        UpdateMagSize(newSize, resizeIncrement);
    }

};