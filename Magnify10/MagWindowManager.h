#pragma once

#include "stdafx.h"
#include "MagWindow.h"

class MagWindowManager
{
private:
    unsigned int magCount;
    int activeIndex;
    MagWindow* mags;
    SIZE _windowSize;
    POINT _windowPosition;
    LPPOINT _mousePoint;
    POINT _panOffset;


    // Rectangle of screen that is centered at the mouse coordinates to be magnified.
    //RECT _sourceRect;

public:
    MagWindowManager() { }
    MagWindowManager(POINT windowPosition, SIZE windowSize)
    {
        mags = nullptr;
        magCount = 20;
        activeIndex = 0;
        _windowSize = windowSize;
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
            mags[i] = MagWindow(1 + (i * 1.5f), _windowPosition, _windowSize);
            if (!mags[i].Create(hInst, hwndHost, i == 0))
            {
                return FALSE;
            }
            mags[i].SetSize(_windowSize.cx, _windowSize.cy);


            //mag1.UpdateMagnifier(&mousePoint, panOffset, newSize);
            if (i == 0)
            {
                SetWindowPos(mags[i].GetHandle(), HWND_TOP, 0, 0, 0, 0,
                             SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW);
            }
            else
            {
                ShowWindow(mags[i].GetHandle(), SW_HIDE);
            }
        }




        return TRUE;
    }

    BOOL UpdateMagnifier(LPPOINT mousePoint, POINT panOffset, SIZE windowSize)
    {
        _mousePoint = mousePoint;
        _windowSize = windowSize;
        _panOffset = panOffset;
        return mags[activeIndex].UpdateMagnifier(_mousePoint, _panOffset, _windowSize);
    }





    VOID IncreaseMagnification()
    {
        if (activeIndex + 1 >= magCount)
        {
            return;
        }

        int newIndex = activeIndex + 1;
        int previousIndex = activeIndex;

        mags[newIndex].UpdateMagnifier(_mousePoint, _panOffset, _windowSize);

        SetWindowPos(mags[newIndex].GetHandle(), HWND_TOP, 0, 0, 0, 0,
            SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW);

        activeIndex = newIndex;
        ShowWindow(mags[previousIndex].GetHandle(), SW_HIDE);
    }

    VOID DecreaseMagnification()
    {
        if (activeIndex - 1 < 0)
        {
            return;
        }

        int previousIndex = activeIndex;
        int newIndex = activeIndex - 1;

        mags[newIndex].UpdateMagnifier(_mousePoint, _panOffset, _windowSize);

        SetWindowPos(mags[newIndex].GetHandle(), HWND_TOP, 0, 0, 0, 0,
            SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW);
        
        activeIndex = newIndex;
        ShowWindow(mags[previousIndex].GetHandle(), SW_HIDE);
    }



};