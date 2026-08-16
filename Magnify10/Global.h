#pragma once

#include "Config.h"

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
    inline POINT mouseLockPoint;
    inline BOOL showLens;
    inline BOOL panningEnabled;

    inline VOID UpdateScreenSize()
    {
        screenSize.cx = GetSystemMetrics(SM_CXSCREEN);
        screenSize.cy = GetSystemMetrics(SM_CYSCREEN);

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

    inline VOID UpdatePanningMousePoint(float magFactor, float newMagFactor, int dx, int dy)
    {
        if (!Global::panningEnabled) { return; }

        if (Config::proportionalPanning)
        {
            int newPanX = Global::mousePoint.x + dx;
            int newPanY = Global::mousePoint.y + dy;
            Global::mousePoint.x = max(0, min(newPanX, Global::screenSize.cx));
            Global::mousePoint.y = max(0, min(newPanY, Global::screenSize.cy));
        }
        else
        {
            float lensCenterX = Global::lensPosition.x + (Global::lensSize.cx / 2);
            float lensCenterY = Global::lensPosition.y + (Global::lensSize.cy / 2);

            if (magFactor != newMagFactor)
            {
                float zoomRatio = (newMagFactor / magFactor) * ((magFactor - 1.0f) / (newMagFactor - 1.0f));
                Global::mousePoint.x = lensCenterX + (Global::mousePoint.x - lensCenterX) * zoomRatio;
                Global::mousePoint.y = lensCenterY + (Global::mousePoint.y - lensCenterY) * zoomRatio;
            }

            float newPanX = Global::mousePoint.x + dx;
            float newPanY = Global::mousePoint.y + dy;

            float denom = newMagFactor - 1.0f;
            float scale = newMagFactor / denom;
            float minPanX = -lensCenterX / denom;
            float minPanY = -lensCenterY / denom;
            float maxPanX = minPanX + Global::screenSize.cx * scale;
            float maxPanY = minPanY + Global::screenSize.cy * scale;

            Global::mousePoint.x = max(minPanX, min(newPanX, maxPanX));
            Global::mousePoint.y = max(minPanY, min(newPanY, maxPanY));
        }
    }

    inline VOID UpdateMousePoint()
    {
        if (Global::panningEnabled) { return; }

        POINT newMousePoint;
        if (GetCursorPos(&newMousePoint))
        {
            Global::mousePoint = newMousePoint;
        }
    }

    inline BOOL UpdateLensPosition()
    {
        UpdateMousePoint();

        int newX = (Global::panningEnabled ? Global::mouseLockPoint.x : Global::mousePoint.x) - (Global::lensSize.cx / 2) - 1;
        int newY = (Global::panningEnabled ? Global::mouseLockPoint.y : Global::mousePoint.y) - (Global::lensSize.cy / 2) - 1;
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

}