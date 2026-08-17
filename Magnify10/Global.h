#pragma once


#pragma region Lens Size Constants

// lens sizing factors as a percent of screen resolution
const float INIT_LENS_WIDTH_FACTOR = 0.5f;
const float INIT_LENS_HEIGHT_FACTOR = 0.5f;
const float INIT_LENS_RESIZE_HEIGHT_FACTOR = 0.1f;
const float INIT_LENS_RESIZE_WIDTH_FACTOR = 0.1f;
const float LENS_MAX_WIDTH_FACTOR = 1.0f;
const float LENS_MAX_HEIGHT_FACTOR = 1.0f;

#pragma endregion

namespace Global {
    inline SIZE screenSize;
    inline SIZE lensSize;
    inline SIZE lensSizeLimit;
    inline SIZE lensSizeIncrement;
    inline POINT lensPosition;
    inline POINT mousePoint;
    inline POINT mouseLockPoint;
    inline POINT panOffset;
    inline BOOL lensEnabled;
    inline BOOL panningEnabled;

    inline VOID UpdateScreenSize()
    {
        screenSize.cx = GetSystemMetrics(SM_CXSCREEN);
        screenSize.cy = GetSystemMetrics(SM_CYSCREEN);

        lensSize.cx = (int)(screenSize.cx * INIT_LENS_WIDTH_FACTOR);
        lensSize.cy = (int)(screenSize.cy * INIT_LENS_HEIGHT_FACTOR);
        lensSizeIncrement.cx = (int)(screenSize.cx * INIT_LENS_RESIZE_WIDTH_FACTOR);
        lensSizeIncrement.cy = (int)(screenSize.cy * INIT_LENS_RESIZE_HEIGHT_FACTOR);
        lensSizeLimit.cx = (int)(screenSize.cx * LENS_MAX_WIDTH_FACTOR);
        lensSizeLimit.cy = (int)(screenSize.cy * LENS_MAX_HEIGHT_FACTOR);
    }

    inline BOOL UpdateLensSize(float incrementFactor)
    {
        SIZE newSize = { lensSize.cx + lensSizeIncrement.cx * incrementFactor,
                         lensSize.cy + lensSizeIncrement.cy * incrementFactor };
        if (newSize.cx > lensSizeLimit.cx || newSize.cy > lensSizeLimit.cy ||
            newSize.cx <= lensSizeIncrement.cx || newSize.cy <= lensSizeIncrement.cy)
        {
            return FALSE;
        }

        lensSize = newSize;
        return TRUE;
    }

    inline VOID UpdatePanningMousePoint(float magFactor, float newMagFactor, int dx, int dy)
    {
        if (!panningEnabled) { return; }

        float denom = newMagFactor - 1.0f;
        float scale = newMagFactor / denom;

        if (Config::proportionalPanning)
        {
            float newPanX = mousePoint.x + dx;
            float newPanY = mousePoint.y + dy;
            float minPanX = -lensPosition.x / denom;
            float minPanY = -lensPosition.y / denom;
            float maxPanX = minPanX + (screenSize.cx * scale) - (lensSize.cx / denom);
            float maxPanY = minPanY + (screenSize.cy * scale) - (lensSize.cy / denom);

            mousePoint.x = max(minPanX, min(newPanX, maxPanX));
            mousePoint.y = max(minPanY, min(newPanY, maxPanY));
        }
        else
        {
            float lensCenterX = lensPosition.x + (lensSize.cx / 2);
            float lensCenterY = lensPosition.y + (lensSize.cy / 2);
            float magFactorRatio = (newMagFactor / magFactor) * ((magFactor - 1.0f) / (newMagFactor - 1.0f));
            mousePoint.x = lensCenterX + (mousePoint.x - lensCenterX) * magFactorRatio;
            mousePoint.y = lensCenterY + (mousePoint.y - lensCenterY) * magFactorRatio;

            float newPanX = mousePoint.x + dx;
            float newPanY = mousePoint.y + dy;
            float minPanX = -lensCenterX / denom;
            float minPanY = -lensCenterY / denom;
            float maxPanX = minPanX + screenSize.cx * scale;
            float maxPanY = minPanY + screenSize.cy * scale;

            mousePoint.x = max(minPanX, min(newPanX, maxPanX));
            mousePoint.y = max(minPanY, min(newPanY, maxPanY));
        }
    }

    inline VOID UpdateMousePoint()
    {
        if (panningEnabled) { return; }

        POINT newMousePoint;
        if (GetCursorPos(&newMousePoint))
        {
            mousePoint = newMousePoint;
        }
    }

    inline BOOL UpdateLensPosition()
    {
        UpdateMousePoint();

        int newX = (panningEnabled ? mouseLockPoint.x : mousePoint.x) - (lensSize.cx / 2) - 1;
        int newY = (panningEnabled ? mouseLockPoint.y : mousePoint.y) - (lensSize.cy / 2) - 1;
        if (lensPosition.x == newX && lensPosition.y == newY)
        {
            return FALSE;
        }

        // Limit the new x|y by screen dimensions
        newX = max(0, min(newX, screenSize.cx - lensSize.cx));
        newY = max(0, min(newY, screenSize.cy - lensSize.cy));

        if (lensPosition.x == newX && lensPosition.y == newY)
        {
            return FALSE;
        }

        lensPosition.x = newX;
        lensPosition.y = newY;
        return TRUE;
    }

}