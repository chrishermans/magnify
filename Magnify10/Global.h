#pragma once

#pragma region Lens Size Constants

// lens sizing factors as a percent of screen resolution
const float         INIT_LENS_WIDTH_FACTOR = 0.5f;
const float         INIT_LENS_HEIGHT_FACTOR = 0.5f;
const float         INIT_LENS_RESIZE_HEIGHT_FACTOR = 0.1f;
const float         INIT_LENS_RESIZE_WIDTH_FACTOR = 0.1f;
const float         LENS_MAX_WIDTH_FACTOR = 1.0f;
const float         LENS_MAX_HEIGHT_FACTOR = 1.0f;

#pragma endregion

namespace Global {
    inline SIZE screenSize;
    inline SIZE lensSize;
    inline SIZE lensSizeLimit;
    inline SIZE lensSizeIncrement;
    inline POINT lensPosition;
    inline POINT mousePoint;
    inline POINT panOffset;
    inline BOOL enabled;
    inline BOOL panningEnabled;

    inline VOID UpdateScreenSize()
    {
        Global::screenSize.cx = GetSystemMetrics(SM_CXSCREEN);
        Global::screenSize.cy = GetSystemMetrics(SM_CYSCREEN);

        Global::lensSize.cx = (int)(Global::screenSize.cx * INIT_LENS_WIDTH_FACTOR);
        Global::lensSize.cy = (int)(Global::screenSize.cy * INIT_LENS_HEIGHT_FACTOR);
        Global::lensSizeIncrement.cx = (int)(Global::screenSize.cx * INIT_LENS_RESIZE_WIDTH_FACTOR);
        Global::lensSizeIncrement.cy = (int)(Global::screenSize.cy * INIT_LENS_RESIZE_HEIGHT_FACTOR);
        Global::lensSizeLimit.cx = (int)(Global::screenSize.cx * LENS_MAX_WIDTH_FACTOR);
        Global::lensSizeLimit.cy = (int)(Global::screenSize.cy * LENS_MAX_HEIGHT_FACTOR);
    }

    inline BOOL UpdateLensSize(float incrementFactor = 1.0f)
    {
        SIZE newSize = { Global::lensSize.cx + Global::lensSizeIncrement.cx * incrementFactor,
                         Global::lensSize.cy + Global::lensSizeIncrement.cy * incrementFactor };
        if (newSize.cx > Global::lensSizeLimit.cx || newSize.cy > Global::lensSizeLimit.cy ||
            newSize.cx <= Global::lensSizeIncrement.cx || newSize.cy <= Global::lensSizeIncrement.cy)
        {
            return FALSE;
        }

        Global::lensSize = newSize;
        return TRUE;
    }

    inline VOID UpdateMousePoint()
    {
        if (!Global::panningEnabled)
        {
            POINT newMousePoint;
            if (GetCursorPos(&newMousePoint))
            {
                Global::mousePoint = newMousePoint;
            }
        }
    }

    inline BOOL UpdateLensPosition()
    {
        UpdateMousePoint();

        int newX = Global::mousePoint.x - (Global::lensSize.cx / 2) - 1;
        int newY = Global::mousePoint.y - (Global::lensSize.cy / 2) - 1;
        if (Global::lensPosition.x == newX && Global::lensPosition.y == newY)
        {
            return FALSE;
        }

        // Limit the new x|y by screen dimensions
        newX = max(0, min(newX, Global::screenSize.cx - Global::lensSize.cx));
        newY = max(0, min(newY, Global::screenSize.cy - Global::lensSize.cy));

        if (Global::lensPosition.x == newX && Global::lensPosition.y == newY)
        {
            return FALSE;
        }

        Global::lensPosition.x = newX;
        Global::lensPosition.y = newY;
        return TRUE;
    }

    inline VOID UpdatePanOffset(float magFactor, int newMousePointX, int newMousePointY)
    {
        int newPanX = Global::panOffset.x + newMousePointX - Global::mousePoint.x;
        int newPanY = Global::panOffset.y + newMousePointY - Global::mousePoint.y;
        int minPanX = (Global::mousePoint.x - Global::lensPosition.x - Global::lensSize.cx / 2) / magFactor - Global::mousePoint.x;
        int minPanY = (Global::mousePoint.y - Global::lensPosition.y - Global::lensSize.cy / 2) / magFactor - Global::mousePoint.y;
        int maxPanX = minPanX + Global::screenSize.cx;
        int maxPanY = minPanY + Global::screenSize.cy;

        Global::panOffset.x = max(minPanX, min(newPanX, maxPanX));
        Global::panOffset.y = max(minPanY, min(newPanY, maxPanY));
    }
}