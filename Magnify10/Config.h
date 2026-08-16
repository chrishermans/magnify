#pragma once


namespace Config
{
    inline constexpr int MAX_CURVE_SIZE = 32;

    inline int magnificationCurveCount = 0;
    inline float magnificationCurve[MAX_CURVE_SIZE];

    inline DWORD timerIntervalMs = 0;
    inline DWORD timerIntervalAfterEnableMs = 0;
    inline DWORD timerToleranceMs = 0;
    inline int inputDelayFrames = 0;
    inline BOOL startEnabled = FALSE;
    inline BOOL proportionalPanning = FALSE;

    inline DWORD hotkeyToggleMag = 0;
    inline DWORD hotkeyZoomIn = 0;
    inline DWORD hotkeyZoomOut = 0;
    inline DWORD hotkeyIncreaseLens = 0;
    inline DWORD hotkeyDecreaseLens = 0;
    inline DWORD hotkeyPanMouse = 0;
    inline DWORD hotkeyTogglePanMouse = 0;

    inline int ParseFloatArray(const std::wstring& wstr, wchar_t delimiter, float* outArray, int maxSize)
    {
        std::wstring token;
        std::wstringstream tokenStream(wstr);
        int count = 0;

        while (std::getline(tokenStream, token, delimiter) && count < maxSize)
        {
            try
            {
                float magFactor = std::stof(token);
                if (magFactor <= 1.0f || magFactor > 100.0f) { continue; } // ignore nonsense magnification factors
                outArray[count++] = magFactor;
            }
            catch (...) { /* Skip parsing errors */ }
        }
        return count;
    }

    inline DWORD ReadDword(const wchar_t* section, const wchar_t* keyName, const wchar_t* defaultString, const std::wstring& iniPath, int base)
    {
        const int BUFFER_SIZE = 16;
        wchar_t buffer[BUFFER_SIZE];
        GetPrivateProfileStringW(section, keyName, defaultString, buffer, BUFFER_SIZE, iniPath.c_str());
        return static_cast<DWORD>(wcstoul(buffer, nullptr, base));
    }

    inline void LoadConfiguration()
    {
        magnificationCurveCount = 0;

        wchar_t binaryPath[MAX_PATH];
        GetModuleFileNameW(NULL, binaryPath, MAX_PATH);
        std::wstring pathStr(binaryPath);
        size_t lastSlash = pathStr.find_last_of(L"\\/");
        std::wstring iniPath = pathStr.substr(0, lastSlash) + L"\\Magnify10.ini";

        timerIntervalMs              = ReadDword(L"Settings", L"TimerIntervalMs", L"7", iniPath, 10);
        timerIntervalAfterEnableMs   = ReadDword(L"Settings", L"TimerIntervalAfterEnableMs", L"50", iniPath, 10);
        timerToleranceMs             = ReadDword(L"Settings", L"TimerToleranceMs", L"2", iniPath, 10);
        inputDelayFrames = GetPrivateProfileIntW(L"Settings", L"InputDelayFrames", 1, iniPath.c_str());
        startEnabled     = GetPrivateProfileIntW(L"Settings", L"StartEnabled", 0, iniPath.c_str());
        proportionalPanning = GetPrivateProfileIntW(L"Settings", L"ProportionalPanning", 0, iniPath.c_str());

        timerIntervalMs             = max(0, min(timerIntervalMs, 10000));
        timerIntervalAfterEnableMs  = max(0, min(timerIntervalAfterEnableMs, 10000));
        timerToleranceMs            = max(0, min(timerToleranceMs, 10000));
        inputDelayFrames            = max(0, min(inputDelayFrames, 10)) + 1;
        startEnabled                = max(0, min(startEnabled, 1));
        proportionalPanning         = max(0, min(proportionalPanning, 1));

        wchar_t curveBuffer[256];
        GetPrivateProfileStringW(L"Settings", L"MagnificationCurve",
            L"1.5, 1.75, 2.0, 2.5, 3.0, 3.5, 4.25, 5.0, 6.0, 7.0, 8.5, 10.0, 12.0, 15.0, 20.0, 26.0",
            curveBuffer, 256, iniPath.c_str());
        magnificationCurveCount = ParseFloatArray(curveBuffer, L',', magnificationCurve, MAX_CURVE_SIZE);

        hotkeyToggleMag       = ReadDword(L"Keybinds", L"ToggleMag", L"0x7C", iniPath, 16);
        hotkeyZoomIn          = ReadDword(L"Keybinds", L"ZoomIn", L"0x7E", iniPath, 16);
        hotkeyZoomOut         = ReadDword(L"Keybinds", L"ZoomOut", L"0x7D", iniPath, 16);
        hotkeyIncreaseLens    = ReadDword(L"Keybinds", L"IncreaseLens", L"0x81", iniPath, 16);
        hotkeyDecreaseLens    = ReadDword(L"Keybinds", L"DecreaseLens", L"0x80", iniPath, 16);
        hotkeyPanMouse        = ReadDword(L"Keybinds", L"PanMouse", L"0x82", iniPath, 16);
        hotkeyTogglePanMouse  = ReadDword(L"Keybinds", L"TogglePanMouse", L"0x83", iniPath, 16);
    }
}
