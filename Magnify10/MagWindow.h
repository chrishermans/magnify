#pragma once

#include "stdafx.h"

class MagWindow
{

public:

    HWND _hwnd;
    float _magFactor;
    POINT _windowPosition;
    SIZE _windowSize;

    // Rectangle of screen that is centered at the mouse coordinates to be magnified.
    RECT _sourceRect;

    VOID UpdateSourceRect(LPPOINT mousePoint, POINT panOffset, SIZE windowSize)
    {
        _sourceRect.left = mousePoint->x + panOffset.x - (int)((windowSize.cx / 2) / _magFactor);
        _sourceRect.top = mousePoint->y + panOffset.y - (int)((windowSize.cy / 2) / _magFactor);
        _sourceRect.right = mousePoint->x + (int)((windowSize.cx / 2) / _magFactor);
        _sourceRect.bottom = mousePoint->y + (int)((windowSize.cy / 2) / _magFactor);
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

    MagWindow()
    {
        _hwnd = nullptr;
        _magFactor = 1;
        _windowSize = { 0, 0 };
        _windowPosition = { 0, 0 };
        _sourceRect = { 0, 0 };
    }
    MagWindow(float magFactor, POINT windowPosition, SIZE windowSize)
    {
        _hwnd = nullptr;
        _magFactor = magFactor;
        _windowSize = windowSize;
        _windowPosition = windowPosition;
        _sourceRect = { 0, 0 };
    }
    ~MagWindow() {}

    BOOL Create(HINSTANCE hInst, HWND hwndHost, BOOL visible)
    {
        DWORD dwStyle =
            WS_CHILD | // Required for magnification window
            WS_EX_COMPOSITED; // Double-buffered
        if (visible) { dwStyle |= WS_VISIBLE; }

        _hwnd = CreateWindow(
            WC_MAGNIFIER, // Magnifier window class name defined in magnification.h
            TEXT("MagnifierWindow2"),
            dwStyle,
            _windowPosition.x, _windowPosition.y,
            _windowSize.cx, _windowSize.cy,
            hwndHost, nullptr, hInst, nullptr);

        if (_hwnd == nullptr)
        {
            return FALSE;
        }

        return SetMagnificationFactorInternal(_magFactor);
    }

    BOOL Destroy()
    {
        return _hwnd == nullptr || DestroyWindow(_hwnd);
    }

    HWND GetHandle() { return _hwnd; }

    BOOL SetMagnificationFactor(float magFactor)
    {
        if (_magFactor != 0 && _magFactor == magFactor) { return FALSE; }
        return SetMagnificationFactorInternal(magFactor);
    }

    BOOL SetSize(int width, int height)
    {
        if (_windowSize.cx == width && _windowSize.cy == height) { return FALSE; }

        _windowSize.cx = width;
        _windowSize.cy = height;
        return SetWindowPos(_hwnd, HWND_TOP,
                _windowPosition.x, _windowPosition.y,
                _windowSize.cx, _windowSize.cy,
                SWP_NOREDRAW | SWP_NOMOVE);
    }

    BOOL RefreshMagnifier(LPPOINT mousePoint, POINT panOffset, SIZE windowSize)
    {
        UpdateSourceRect(mousePoint, panOffset, windowSize);

        // Set the source rectangle for the magnifier control.
        return MagSetWindowSource(_hwnd, _sourceRect);
    }

};
