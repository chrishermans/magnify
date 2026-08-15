#pragma once

#include "Global.h"

class MagWindow
{

public:
    
    HWND _hwnd;
    float _magFactor;

    // Rectangle of screen that is centered at the mouse coordinates to be magnified.
    RECT _sourceRect;

    MagWindow()
    {
        _hwnd = nullptr;
        _magFactor = 1;
        _sourceRect = { 0, 0 };
    }
    ~MagWindow() {}

    BOOL Destroy()
    {
        return _hwnd == nullptr || DestroyWindow(_hwnd);
    }

    BOOL Create(HINSTANCE hInst, HWND hwndHost, float magFactor, BOOL visible)
    {
        DWORD dwStyle =
            WS_CHILD | // Required for magnification window
            WS_EX_COMPOSITED; // Double-buffered

        _hwnd = CreateWindow(
            WC_MAGNIFIER, // Magnifier window class name defined in magnification.h
            TEXT("MagnifierWindow2"),
            dwStyle | (WS_VISIBLE * visible),
            0, 0,
            Global::screenSize.cx, Global::screenSize.cy,
            hwndHost, nullptr, hInst, nullptr);

        if (_hwnd == nullptr)
        {
            return FALSE;
        }

        MAGTRANSFORM matrix;
        memset(&matrix, 0, sizeof(matrix));
        matrix.v[0][0] = magFactor;
        matrix.v[1][1] = magFactor;
        matrix.v[2][2] = 1.0f;

        _magFactor = magFactor;
        return MagSetWindowTransform(_hwnd, &matrix);
    }

    BOOL RefreshMagnifier()
    {
        // Proportional viewport | Bounded lens window
        // Sync the viewport origin based on the lensPosition
        // This maintains 1:1 cursor-to-content alignment even when lens is bounded by screen edges
        int left = Global::mousePoint.x + Global::panOffset.x - static_cast<int>((Global::mousePoint.x - Global::lensPosition.x) / _magFactor);
        int top = Global::mousePoint.y + Global::panOffset.y - static_cast<int>((Global::mousePoint.y - Global::lensPosition.y) / _magFactor);

        _sourceRect.left = left;
        _sourceRect.top = top;
        _sourceRect.right = left + (Global::lensSize.cx / _magFactor);
        _sourceRect.bottom = top + (Global::lensSize.cy / _magFactor);

        // Set the source rectangle for the magnifier control.
        return MagSetWindowSource(_hwnd, _sourceRect);
    }

};
