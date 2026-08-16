
#include "stdafx.h"
#include <magnification.h>
#include <threadpoolapiset.h>
#include <shellapi.h>
#include <sstream>
#include <unordered_map>
#include <functional>
#include "Resource.h"
#include "Config.h"
#include "Global.h"
#include "MagWindow.h"
#include "MagWindowManager.h"

#pragma region Hotkey States

using HotkeyHandler = std::function<BOOL(WPARAM)>;
std::unordered_map<DWORD, HotkeyHandler> hotkeyHandlers;

BOOL KEYDOWN_TOGGLE_MAG = FALSE;
BOOL KEYDOWN_ZOOM_IN = FALSE;
BOOL KEYDOWN_ZOOM_OUT = FALSE;
BOOL KEYDOWN_INCREASE_LENS = FALSE;
BOOL KEYDOWN_DECREASE_LENS = FALSE;
BOOL KEYDOWN_PAN_MOUSE = FALSE;
BOOL KEYDOWN_TOGGLE_PAN_MOUSE = FALSE;

#pragma endregion

#pragma region Objects

// Main program handle 
const TCHAR         WindowClassName[] = TEXT("MagnifierWindow");

// Window handles
HINSTANCE           hMainInstance;
HWND                hwndHost;

MagWindowManager*   magManager;

// Keyboard/Mouse hook 
HHOOK               hkb;
HHOOK               hMouseHook;

// Timer interval structures
union FILETIME64
{
    INT64 quad;
    FILETIME ft;
};
FILETIME CreateRelativeFiletimeMS(DWORD milliseconds)
{
    FILETIME64 ft = { -static_cast<INT64>(milliseconds) * 10000 };
    return ft.ft;
}
FILETIME            timerDueTime;
FILETIME            timerDueTimeAfterEnable;
PTP_TIMER           refreshTimer;

#pragma endregion 

#pragma region Function Definitions

// Forward declarations. 
ATOM                RegisterHostWindowClass(HINSTANCE hInstance);
BOOL                SetupHostWindow(HINSTANCE hinst);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK    LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
VOID CALLBACK       TimerTickEvent(PTP_CALLBACK_INSTANCE, VOID* context, PTP_TIMER);

VOID                InitHotkeyMap();
VOID                UpdateHostSize();
VOID                RefreshMagnifier();
BOOL                HandleKeyStates();
VOID                StartPanMouse();
VOID                StopPanMouse();

VOID                ToggleMagnifier();
VOID                EnableMagnifier();
VOID                DisableMagnifier();

#pragma endregion

#pragma region Main

int APIENTRY WinMain(
    _In_ HINSTANCE     hInstance,
    _In_opt_ HINSTANCE /* hPrevInstance */,
    _In_ LPSTR         /* lpCmdLine */,
    _In_ int           /* nCmdShow */)
{
    hMainInstance = hInstance;

    Config::LoadConfiguration();
    timerDueTime = CreateRelativeFiletimeMS(Config::timerIntervalMs);
    timerDueTimeAfterEnable = CreateRelativeFiletimeMS(Config::timerIntervalAfterEnableMs);
    InitHotkeyMap();

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Global::UpdateScreenSize();

    if (!MagInitialize()) { return 0; }
    if (!SetupHostWindow(hInstance)) { return 0; }
    magManager = new MagWindowManager();
    if (!magManager->Create(hInstance, hwndHost)) { return 0; }
    if (!UpdateWindow(hwndHost)) { return 0; }

    // initialize lens as disabled
    ShowWindow(hwndHost, SW_HIDE);
    Global::lensEnabled = FALSE;
    Global::panningEnabled = FALSE;

    // Create notification object for the task tray icon
    NOTIFYICONDATA nid = {};
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.uVersion = NOTIFYICON_VERSION;
    nid.hWnd = hwndHost;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER;
    nid.hIcon = LoadIcon(hInstance, (LPCTSTR)(IDI_MAGNIFY10));
    lstrcpy(nid.szTip, TEXT("Magnify10 (Click to Close)"));

    // Add icon to the task tray
    Shell_NotifyIcon(NIM_ADD, &nid);
    Shell_NotifyIcon(NIM_SETVERSION, &nid);

    // Setup the keyboard hook to capture global hotkeys
    hkb = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

    // Create a timer to refresh the window. 
    refreshTimer = CreateThreadpoolTimer(TimerTickEvent, nullptr, nullptr);

    DisableMagnifier();
    if (Config::startEnabled)
    {
        EnableMagnifier();
    }

    // Main message loop. 
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DisableMagnifier();

    if (hkb != NULL)
    {
        UnhookWindowsHookEx(hkb);
        hkb = NULL;
    }
    if (hMouseHook != NULL)
    {
        UnhookWindowsHookEx(hMouseHook);
        hMouseHook = NULL;
    }

    SetThreadpoolTimer(refreshTimer, nullptr, 0, Config::timerToleranceMs);
    Shell_NotifyIcon(NIM_DELETE, &nid);
    MagUninitialize();
    magManager->DestroyWindows();
    DestroyWindow(hwndHost);

    delete magManager;
    return (int)msg.wParam;
}

#pragma endregion

#pragma region Host Window Proc

LRESULT CALLBACK HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_USER: // Exit on task tray icon click - very simple exit functionality
    {
        switch (lParam)
        {
        case WM_LBUTTONUP:
            PostMessage(hwndHost, WM_CLOSE, 0, 0);
            break;
        case WM_RBUTTONUP:
            PostMessage(hwndHost, WM_CLOSE, 0, 0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_QUERYENDSESSION:
        PostMessage(hwndHost, WM_DESTROY, 0, 0);
        break;
    case WM_CLOSE:
        PostMessage(hwndHost, WM_DESTROY, 0, 0);
        break;
    case WM_DESTROY:
        DisableMagnifier();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#pragma endregion


ATOM RegisterHostWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = HostWndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    wcex.lpszClassName = WindowClassName;

    return RegisterClassEx(&wcex);
}

BOOL SetupHostWindow(HINSTANCE hInst)
{
    // Create the host window. 
    RegisterHostWindowClass(hInst);

    hwndHost = CreateWindowEx(
        WS_EX_LAYERED | // Required style to render the magnification correctly
        WS_EX_TOPMOST | // Always-on-top
        WS_EX_TRANSPARENT | // Click-through
        WS_EX_TOOLWINDOW, // Do not show program on taskbar
        WindowClassName,
        TEXT("Screen Magnifier"),
        WS_CLIPCHILDREN |
        WS_POPUP | // Removes titlebar and borders - simply a bare window
        WS_BORDER, // Adds a 1-pixel border for tracking the edges - aesthetic
        Global::lensPosition.x, Global::lensPosition.y,
        Global::lensSize.cx, Global::lensSize.cy,
        nullptr, nullptr, hInst, nullptr);

    if (!hwndHost)
    {
        return FALSE;
    }

    // Make the window fully opaque.
    return SetLayeredWindowAttributes(hwndHost, 0, 255, LWA_ALPHA);
}

VOID CALLBACK TimerTickEvent(PTP_CALLBACK_INSTANCE, VOID* context, PTP_TIMER)
{
    RefreshMagnifier();
    if (Global::lensEnabled) // Reset timer to expire one time at next interval
    {
        SetThreadpoolTimer(refreshTimer, &timerDueTime, 0, Config::timerToleranceMs);
    }
}

VOID UpdateHostSize()
{
    Global::UpdateLensPosition();
    Global::UpdatePanningMousePoint(
       magManager->GetMagFactor(),
       magManager->GetMagFactor(),
       0, 0);

    magManager->RefreshMagnifier();
    magManager->RefreshMagnifier();
    SetWindowPos(hwndHost, HWND_TOPMOST,
        Global::lensPosition.x, Global::lensPosition.y,
        Global::lensSize.cx, Global::lensSize.cy, // width|height of window
        SWP_NOACTIVATE);
}

// Called in the timer tick event to refresh the magnification area drawn and lens (host window) position and size
VOID RefreshMagnifier()
{
    BOOL positionUpdated = Global::UpdateLensPosition();

    if (!HandleKeyStates())
    {
        if (!magManager->RefreshMagnifier())
        {
            return;
        }
    }

    if (positionUpdated)
    {
        SetWindowPos(hwndHost, HWND_TOPMOST,
            Global::lensPosition.x, Global::lensPosition.y, // x|y coordinate of top left corner
            0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | (SWP_NOMOVE * Global::panningEnabled));
    }
}

VOID DisableMagnifier()
{
    ShowWindow(hwndHost, SW_HIDE);
    Global::lensEnabled = FALSE;
    SetThreadpoolTimer(refreshTimer, nullptr, 0, Config::timerToleranceMs); // Stop the refresh timer

    // reset any panning that had been done
    StopPanMouse();
}

VOID EnableMagnifier()
{
    BOOL positionUpdated = Global::UpdateLensPosition();
    magManager->RefreshMagnifier();
    magManager->RefreshMagnifier();
    if (positionUpdated)
    {
        SetWindowPos(hwndHost, HWND_TOPMOST,
            Global::lensPosition.x, Global::lensPosition.y, // x|y coordinate of top left corner
            0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | (SWP_NOMOVE * Global::panningEnabled));
    }

    Global::lensEnabled = TRUE;
    SetThreadpoolTimer(refreshTimer, &timerDueTimeAfterEnable, 0, Config::timerToleranceMs); // Start the refresh timer
    ShowWindow(hwndHost, SW_SHOWNOACTIVATE);
}

VOID ToggleMagnifier()
{
    if (Global::lensEnabled) { DisableMagnifier(); }
    else { EnableMagnifier(); }
}

#pragma region Handle key states

BOOL HandleKeyStates()
{
    static int frameCounter = 0;
    if (frameCounter++ % Config::inputDelayFrames != 0)
    {
        return FALSE;
    }

    if (KEYDOWN_ZOOM_IN && !KEYDOWN_ZOOM_OUT)
    {
        magManager->IncreaseMagnification();
        return TRUE;
    }
    if (KEYDOWN_ZOOM_OUT && !KEYDOWN_ZOOM_IN)
    {
        if (!magManager->DecreaseMagnification())
        {
            DisableMagnifier();
        }
        return TRUE;
    }

    frameCounter = 0;
    return FALSE;
}

#pragma endregion

VOID StartPanMouse()
{
    if (hMouseHook == NULL)
    {
        hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hMainInstance, 0);
    }

    RECT mouseBoundary;
    mouseBoundary.left = Global::mousePoint.x;
    mouseBoundary.top = Global::mousePoint.y;
    mouseBoundary.right = Global::mousePoint.x;
    mouseBoundary.bottom = Global::mousePoint.y;

    ClipCursor(&mouseBoundary); // locks mouse movement
    MagShowSystemCursor(FALSE);
    Global::mouseLockPoint = Global::mousePoint;
    Global::panningEnabled = TRUE;
}

VOID StopPanMouse()
{
    if (hMouseHook != NULL)
    {
        UnhookWindowsHookEx(hMouseHook);
        hMouseHook = NULL;
    }

    Global::panningEnabled = FALSE;
    MagShowSystemCursor(TRUE);
    ClipCursor(NULL); // unlocks mouse movement
}

#pragma region Keyboard & Mouse Hook Callback

VOID InitHotkeyMap()
{
    hotkeyHandlers.clear();

    hotkeyHandlers[Config::hotkeyToggleMag] = [](WPARAM wParam) -> BOOL
        {
            bool keyDown = (wParam == WM_KEYDOWN);
            if (keyDown && !KEYDOWN_TOGGLE_MAG)
            {
                ToggleMagnifier();
            }
            KEYDOWN_TOGGLE_MAG = keyDown;
            return TRUE;
        };

    hotkeyHandlers[Config::hotkeyZoomIn] = [](WPARAM wParam) -> BOOL
        {
            KEYDOWN_ZOOM_IN = (wParam == WM_KEYDOWN);
            if (KEYDOWN_ZOOM_IN && !Global::lensEnabled) { EnableMagnifier(); }
            return TRUE;
        };

    hotkeyHandlers[Config::hotkeyZoomOut] = [](WPARAM wParam) -> BOOL
        {
            KEYDOWN_ZOOM_OUT = (wParam == WM_KEYDOWN);
            return TRUE;
        };

    hotkeyHandlers[Config::hotkeyIncreaseLens] = [](WPARAM wParam) -> BOOL
        {
            KEYDOWN_INCREASE_LENS = (wParam == WM_KEYDOWN);
            if (KEYDOWN_INCREASE_LENS && !KEYDOWN_DECREASE_LENS)
            {
                if (!Global::lensEnabled) { EnableMagnifier(); }
                else if (Global::UpdateLensSize(1.0f))
                {
                    UpdateHostSize();
                }
            }
            return TRUE;
        };

    hotkeyHandlers[Config::hotkeyDecreaseLens] = [](WPARAM wParam) -> BOOL
        {
            KEYDOWN_DECREASE_LENS = (wParam == WM_KEYDOWN);
            if (KEYDOWN_DECREASE_LENS && !KEYDOWN_INCREASE_LENS)
            {
                if (!Global::lensEnabled) { EnableMagnifier(); }
                else if (Global::UpdateLensSize(-1.0f))
                {
                    UpdateHostSize();
                }
            }
            return TRUE;
        };

    hotkeyHandlers[Config::hotkeyPanMouse] = [](WPARAM wParam) -> BOOL
        {
            BOOL keyDown = (wParam == WM_KEYDOWN);
            if (keyDown != KEYDOWN_PAN_MOUSE)
            {
                KEYDOWN_PAN_MOUSE = keyDown;
                if (Global::lensEnabled)
                {
                    if (KEYDOWN_PAN_MOUSE && !Global::panningEnabled)
                    {
                        StartPanMouse();
                    }
                    else if (!KEYDOWN_PAN_MOUSE && Global::panningEnabled)
                    {
                        StopPanMouse();
                    }
                }
            }
            return TRUE;
        };

    hotkeyHandlers[Config::hotkeyTogglePanMouse] = [](WPARAM wParam) -> BOOL
        {
            if (wParam == WM_KEYDOWN && !KEYDOWN_TOGGLE_PAN_MOUSE && !KEYDOWN_PAN_MOUSE && Global::lensEnabled)
            {
                if (!Global::panningEnabled) { StartPanMouse(); }
                else { StopPanMouse(); }
            }
            KEYDOWN_TOGGLE_PAN_MOUSE = (wParam == WM_KEYDOWN);
            return TRUE;
        };
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION) // do not process message 
    {
        return CallNextHookEx(hkb, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*)lParam;
    auto it = hotkeyHandlers.find(key->vkCode);
    if (it != hotkeyHandlers.end())
    {
        if (it->second(wParam)) { return TRUE; }
    }

    return CallNextHookEx(hkb, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION || wParam != WM_MOUSEMOVE ||
        !Global::panningEnabled || !Global::lensEnabled)
    {
        return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
    }

    MSLLHOOKSTRUCT* mouseInfo = (MSLLHOOKSTRUCT*)lParam;
    if (mouseInfo->pt.x != Global::mousePoint.x || mouseInfo->pt.y != Global::mousePoint.y)
    {
        Global::UpdatePanningMousePoint(
            magManager->GetMagFactor(),
            magManager->GetMagFactor(),
            mouseInfo->pt.x - Global::mouseLockPoint.x,
            mouseInfo->pt.y - Global::mouseLockPoint.y);
    }
    
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

#pragma endregion
