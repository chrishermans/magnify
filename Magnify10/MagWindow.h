#pragma once

#include "stdafx.h"

class MagWindow
{

public:

    HWND _hwnd;
    float _magFactor;
    POINT _windowPosition;
    SIZE _screenSize;

    // Rectangle of screen that is centered at the mouse coordinates to be magnified.
    RECT _sourceRect;

    MagWindow()
    {
        _hwnd = nullptr;
        _magFactor = 1;
        _windowPosition = { 0, 0 };
        _sourceRect = { 0, 0 };
        _screenSize = SIZE();
    }
    MagWindow(float magFactor, POINT windowPosition, SIZE screenSize)
    {
        _hwnd = nullptr;
        _magFactor = magFactor;
        _windowPosition = windowPosition;
        _sourceRect = { 0, 0 };
        _screenSize = screenSize;
    }
    ~MagWindow() {}

    BOOL Destroy()
    {
        return _hwnd == nullptr || DestroyWindow(_hwnd);
    }

    HWND GetHandle() { return _hwnd; }

    BOOL Create(HINSTANCE hInst, HWND hwndHost, BOOL visible)
    {
        DWORD dwStyle =
            WS_CHILD | // Required for magnification window
            WS_EX_COMPOSITED; // Double-buffered

        _hwnd = CreateWindow(
            WC_MAGNIFIER, // Magnifier window class name defined in magnification.h
            TEXT("MagnifierWindow2"),
            dwStyle | (WS_VISIBLE * visible),
            _windowPosition.x, _windowPosition.y,
            _screenSize.cx, _screenSize.cy,
            hwndHost, nullptr, hInst, nullptr);

        if (_hwnd == nullptr)
        {
            return FALSE;
        }

        return SetMagnificationFactorInternal(_magFactor);
    }

    BOOL SetMagnificationFactor(float magFactor)
    {
        if (_magFactor != 0 && _magFactor == magFactor) { return FALSE; }
        return SetMagnificationFactorInternal(magFactor);
    }

    BOOL SetMagnificationFactorInternal(float magFactor)
    {
        MAGTRANSFORM matrix;
        memset(&matrix, 0, sizeof(matrix));
        matrix.v[0][0] = magFactor;
        matrix.v[1][1] = magFactor;
        matrix.v[2][2] = 1.0f;

        _magFactor = magFactor;
        return MagSetWindowTransform(_hwnd, &matrix);
    }

    VOID UpdateSourceRect(LPPOINT mousePoint, POINT panOffset, SIZE lensSize, POINT lensPosition)
    {
        // Proportional viewport | Bounded lens window
        // Sync the viewport origin based on the lensPosition
        // This maintains 1:1 cursor-to-content alignment even when lens is bounded by screen edges
        int left = mousePoint->x + panOffset.x - static_cast<int>((mousePoint->x - lensPosition.x) / _magFactor);
        int top = mousePoint->y + panOffset.y - static_cast<int>((mousePoint->y - lensPosition.y) / _magFactor);

        _sourceRect.left = left;
        _sourceRect.top = top;
        _sourceRect.right = left + (lensSize.cx / _magFactor);
        _sourceRect.bottom = top + (lensSize.cy / _magFactor);
    }

    BOOL RefreshMagnifier(LPPOINT mousePoint, POINT panOffset, SIZE lensSize, POINT lensPosition)
    {
        UpdateSourceRect(mousePoint, panOffset, lensSize, lensPosition);

        // Set the source rectangle for the magnifier control.
        return MagSetWindowSource(_hwnd, _sourceRect);
    }

};
