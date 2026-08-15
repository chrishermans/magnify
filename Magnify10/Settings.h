#pragma once

#include <windows.h>
#include <string>
#include <sstream>

class Settings
{
public:
    static Settings& Get()
    {
        static Settings instance;
        return instance;
    }

    static const int MAX_CURVE_SIZE = 32;

    int magnificationCurveCount;
    float magnificationCurve[MAX_CURVE_SIZE];

    DWORD timerIntervalMs;
    DWORD timerIntervalAfterEnableMs;
    DWORD timerToleranceMs;
    int inputDelayFrames;
    BOOL startEnabled;

    DWORD hotkeyToggleMag;
    DWORD hotkeyZoomIn;
    DWORD hotkeyZoomOut;
    DWORD hotkeyIncreaseLens;
    DWORD hotkeyDecreaseLens;
    DWORD hotkeyPanMouse;
    DWORD hotkeyTogglePanMouse;

    static void LoadSettings()
    {
        Get().magnificationCurveCount = 0;

        wchar_t binaryPath[MAX_PATH];
        GetModuleFileNameW(NULL, binaryPath, MAX_PATH);
        std::wstring pathStr(binaryPath);
        size_t lastSlash = pathStr.find_last_of(L"\\/");
        std::wstring iniPath = pathStr.substr(0, lastSlash) + L"\\Magnify10.ini";

        Get().timerIntervalMs              = ReadDword(L"Settings", L"TimerIntervalMs", L"7", iniPath, 10);
        Get().timerIntervalAfterEnableMs   = ReadDword(L"Settings", L"TimerIntervalAfterEnableMs", L"50", iniPath, 10);
        Get().timerToleranceMs             = ReadDword(L"Settings", L"TimerToleranceMs", L"2", iniPath, 10);
        Get().inputDelayFrames = GetPrivateProfileIntW(L"Settings", L"InputDelayFrames", 1, iniPath.c_str());
        Get().startEnabled     = GetPrivateProfileIntW(L"Settings", L"StartEnabled", 0, iniPath.c_str());

        Get().timerIntervalMs             = max(0, min(Get().timerIntervalMs, 10000));
        Get().timerIntervalAfterEnableMs  = max(0, min(Get().timerIntervalAfterEnableMs, 10000));
        Get().timerToleranceMs            = max(0, min(Get().timerToleranceMs, 10000));
        Get().inputDelayFrames            = max(0, min(Get().inputDelayFrames, 10)) + 1;
        Get().startEnabled                = max(0, min(Get().startEnabled, 1));

        wchar_t curveBuffer[256];
        GetPrivateProfileStringW(L"Settings", L"MagnificationCurve",
            L"1.5, 1.75, 2.0, 2.5, 3.0, 3.5, 4.25, 5.0, 6.0, 7.0, 8.5, 10.0, 12.0, 15.0, 20.0, 26.0",
            curveBuffer, 256, iniPath.c_str());
        Get().magnificationCurveCount = ParseFloatArray(curveBuffer, L',', Get().magnificationCurve, MAX_CURVE_SIZE);

        Get().hotkeyToggleMag       = ReadDword(L"Keybinds", L"ToggleMag", L"0x7C", iniPath, 16);
        Get().hotkeyZoomIn          = ReadDword(L"Keybinds", L"ZoomIn", L"0x7E", iniPath, 16);
        Get().hotkeyZoomOut         = ReadDword(L"Keybinds", L"ZoomOut", L"0x7D", iniPath, 16);
        Get().hotkeyIncreaseLens    = ReadDword(L"Keybinds", L"IncreaseLens", L"0x81", iniPath, 16);
        Get().hotkeyDecreaseLens    = ReadDword(L"Keybinds", L"DecreaseLens", L"0x80", iniPath, 16);
        Get().hotkeyPanMouse        = ReadDword(L"Keybinds", L"PanMouse", L"0x82", iniPath, 16);
        Get().hotkeyTogglePanMouse  = ReadDword(L"Keybinds", L"TogglePanMouse", L"0x83", iniPath, 16);
    }

private:

    static int ParseFloatArray(const std::wstring& wstr, wchar_t delimiter, float* outArray, int maxSize)
    {
        std::wstring token;
        std::wstringstream tokenStream(wstr);
        int count = 0;

        while (std::getline(tokenStream, token, delimiter) && count < maxSize)
        {
            try
            {
                outArray[count] = std::stof(token);
                count++;
            }
            catch (...) { /* skip invalid elements */ }
        }
        return count;
    }

    static DWORD ReadDword(const wchar_t* section, const wchar_t* keyName, const wchar_t* defaultString, const std::wstring& iniPath, int base)
    {
        const int BUFFER_SIZE = 16;
        wchar_t buffer[BUFFER_SIZE];
        GetPrivateProfileStringW(section, keyName, defaultString, buffer, BUFFER_SIZE, iniPath.c_str());
        return static_cast<DWORD>(wcstoul(buffer, nullptr, base));
    }
};
