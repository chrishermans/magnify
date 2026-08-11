
#include "stdafx.h"
#include <wincodec.h>
#include <magnification.h> 
#include <threadpoolapiset.h>
#include <shellapi.h>
#include "MagWindowManager.h"
#include "Resource.h"


#pragma region Hotkey definitions

const DWORD HOTKEY_TOGGLE_MAG = VK_F13;
const DWORD HOTKEY_ZOOM_IN = VK_F15;
const DWORD HOTKEY_ZOOM_OUT = VK_F14;
const DWORD HOTKEY_INCREASE_LENS = VK_F18;
const DWORD HOTKEY_DECREASE_LENS = VK_F17;
const DWORD HOTKEY_PAN_MOUSE = VK_F19;
const DWORD HOTKEY_TOGGLE_PAN_MOUSE = VK_F20;

BOOL KEYDOWN_TOGGLE_MAG = FALSE;
BOOL KEYDOWN_ZOOM_IN = FALSE;
BOOL KEYDOWN_ZOOM_OUT = FALSE;
BOOL KEYDOWN_INCREASE_LENS = FALSE;
BOOL KEYDOWN_DECREASE_LENS = FALSE;
BOOL KEYDOWN_PAN_MOUSE = FALSE;
BOOL KEYDOWN_TOGGLE_PAN_MOUSE = FALSE;

#pragma endregion

#pragma region Constants

// Magnification lens refresh interval - Should be as low as possible to match monitor refresh rate.
const UINT          TIMER_INTERVAL_MS = 5;

// lens sizing factors as a percent of screen resolution
const float         INIT_LENS_WIDTH_FACTOR = 0.5f;
const float         INIT_LENS_HEIGHT_FACTOR = 0.5f;
const float         INIT_LENS_RESIZE_HEIGHT_FACTOR = 0.1f; 
const float         INIT_LENS_RESIZE_WIDTH_FACTOR = 0.1f;
const float         LENS_MAX_WIDTH_FACTOR = 1.0f;
const float         LENS_MAX_HEIGHT_FACTOR = 1.0f;

// lens shift/pan increments
const int           PAN_INCREMENT_HORIZONTAL = 20;
const int           PAN_INCREMENT_VERTICAL = 15;

#pragma endregion


#pragma region Variables

SIZE                screenSize;
SIZE                initLensSize; // Size in pixels of the lens (host window)
POINT               lensPosition; // Top left corner of the lens (host window)

SIZE                resizeIncrement;
SIZE                resizeLimit;

// Current mouse location
POINT               mousePoint;
RECT                mouseLockPoint;

#pragma endregion


#pragma region Objects

// Main program handle 
const TCHAR         WindowClassName[] = TEXT("MagnifierWindow");

// Window handles
HWND                hwndHost;

MagWindowManager*   magManager;

// Show magnifier or not
BOOL                enabled;
BOOL                enableTimer;

// lens pan offset x|y
POINT               panOffset;
BOOL                panningEnabled;

// Keyboard/Mouse hook 
HHOOK               hkb;
KBDLLHOOKSTRUCT*    key;
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
FILETIME            timerDueTime = CreateRelativeFiletimeMS(TIMER_INTERVAL_MS);
PTP_TIMER           refreshTimer;

#pragma endregion 


#pragma region Function Definitions

// Calculates an X or Y value where the lens (host window) should be relative to mouse position. i.e. top left corner of a window centered on mouse
#define LENS_POSITION_VALUE(MOUSEPOINT_VALUE, LENSSIZE_VALUE) (MOUSEPOINT_VALUE - (LENSSIZE_VALUE / 2) - 1)

// Forward declarations. 
ATOM                RegisterHostWindowClass(HINSTANCE hInstance);
BOOL                SetupHostWindow(HINSTANCE hinst);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK    LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
VOID CALLBACK       TimerTickEvent(PTP_CALLBACK_INSTANCE, VOID* context, PTP_TIMER);

VOID                InitScreenDimensions();

VOID                UpdateHostSize();
BOOL                UpdateLensPosition(LPPOINT mousePoint);
VOID                RefreshMagnifier();
BOOL                HandleKeyStates();
VOID                StartPanMouse();
VOID                StopPanMouse();

VOID                ToggleMagnifier();
BOOL                EnableMagnifier();
VOID                DisableMagnifier();

#pragma endregion


#pragma region Main

int APIENTRY WinMain(
    _In_ HINSTANCE     hInstance,
    _In_opt_ HINSTANCE /* hPrevInstance */,
    _In_ LPSTR         /* lpCmdLine */,
    _In_ int           /* nCmdShow */)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    InitScreenDimensions();

    if (!MagInitialize()) { return 0; }
    if (!SetupHostWindow(hInstance)) { return 0; }
    magManager = new MagWindowManager(initLensSize, screenSize);
    if (!magManager->Create(hInstance, hwndHost)) { return 0; }
    if (!UpdateWindow(hwndHost)) { return 0; }

    // Start as disabled
    ShowWindow(hwndHost, SW_HIDE);
    enabled = FALSE;
    enableTimer = TRUE;
    panningEnabled = FALSE;

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
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);

    // Create and start a timer to refresh the window. 
    refreshTimer = CreateThreadpoolTimer(TimerTickEvent, nullptr, nullptr);
    SetThreadpoolTimer(refreshTimer, &timerDueTime, 0, 0); // TODO: this only needs to be started if enabled at start

    // Main message loop. 
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    // Shut down.
    enabled = FALSE; 

    UnhookWindowsHookEx(hkb);
    hkb = 0;
    delete hkb;
    key = 0;
    delete key;

    UnhookWindowsHookEx(hMouseHook);
    hMouseHook = NULL;

    SetThreadpoolTimer(refreshTimer, nullptr, 0, 0);
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

    case WM_QUERYENDSESSION:
        PostMessage(hwndHost, WM_DESTROY, 0, 0);
        break;
    case WM_CLOSE:
        PostMessage(hwndHost, WM_DESTROY, 0, 0);
        break;
    case WM_DESTROY:
        enabled = FALSE;
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#pragma endregion


// Set initial values for screen, lens, and resizing dimensions.
VOID InitScreenDimensions()
{
    screenSize.cx = GetSystemMetrics(SM_CXSCREEN);
    screenSize.cy = GetSystemMetrics(SM_CYSCREEN);

    initLensSize.cx = (int)(screenSize.cx * INIT_LENS_WIDTH_FACTOR);
    initLensSize.cy = (int)(screenSize.cy * INIT_LENS_HEIGHT_FACTOR);

    resizeIncrement.cx = (int)(screenSize.cx * INIT_LENS_RESIZE_WIDTH_FACTOR);
    resizeIncrement.cy = (int)(screenSize.cy * INIT_LENS_RESIZE_HEIGHT_FACTOR);
    resizeLimit.cx = (int)(screenSize.cx * LENS_MAX_WIDTH_FACTOR);
    resizeLimit.cy = (int)(screenSize.cy * LENS_MAX_HEIGHT_FACTOR);
}

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
    GetCursorPos(&mousePoint);
    lensPosition = { 0, 0 };

    // Create the host window. 
    RegisterHostWindowClass(hInst);

    hwndHost = CreateWindowEx(
        WS_EX_LAYERED | // Required style to render the magnification correctly
        WS_EX_TOPMOST | // Always-on-top
        WS_EX_TRANSPARENT | // Click-through
        WS_EX_TOOLWINDOW, // Do not show program on taskbar
        WindowClassName,
        TEXT("Screen Magnifier"),
        WS_CLIPCHILDREN | // ???
        WS_POPUP | // Removes titlebar and borders - simply a bare window
        WS_BORDER, // Adds a 1-pixel border for tracking the edges - aesthetic
        lensPosition.x, lensPosition.y,
        initLensSize.cx, initLensSize.cy,
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
    if (enableTimer)
    {
        RefreshMagnifier();
    }

    if (enabled) // Reset timer to expire one time at next interval
    {
        SetThreadpoolTimer(refreshTimer, &timerDueTime, 0, 0);
    }
}

BOOL UpdateLensPosition(LPPOINT mousePosition)
{
    int newX = LENS_POSITION_VALUE(mousePosition->x, magManager->_lensSize.cx);
    int newY = LENS_POSITION_VALUE(mousePosition->y, magManager->_lensSize.cy);
    if (lensPosition.x == newX && lensPosition.y == newY)
    {
        return FALSE;
    }

    // Limit the new x|y by screen dimensions
    newX = max(0, min(newX, screenSize.cx - magManager->_lensSize.cx));
    newY = max(0, min(newY, screenSize.cy - magManager->_lensSize.cy));

    if (lensPosition.x == newX && lensPosition.y == newY)
    {
        return FALSE;
    }

    lensPosition.x = newX;
    lensPosition.y = newY;
    return TRUE;
}


VOID UpdateHostSize()
{
    UpdateLensPosition(&mousePoint);
    magManager->RefreshMagnifier(&mousePoint, panOffset, lensPosition);
    magManager->RefreshMagnifier(&mousePoint, panOffset, lensPosition);

    SetWindowPos(hwndHost, HWND_TOPMOST,
        lensPosition.x, lensPosition.y,
        magManager->_lensSize.cx, magManager->_lensSize.cy, // width|height of window
        SWP_NOACTIVATE);
}

// Called in the timer tick event to refresh the magnification area drawn and lens (host window) position and size
VOID RefreshMagnifier()
{
    if (!panningEnabled)
    {
        GetCursorPos(&mousePoint);
    }
    BOOL positionUpdated = UpdateLensPosition(&mousePoint);
    magManager->UpdateParameters(&mousePoint, panOffset);

    if (!HandleKeyStates())
    {
        magManager->RefreshMagnifier(&mousePoint, panOffset, lensPosition);
    }

    if (positionUpdated)
    {
        SetWindowPos(hwndHost, HWND_TOPMOST,
            lensPosition.x, lensPosition.y, // x|y coordinate of top left corner
            0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | (SWP_NOMOVE * panningEnabled));
    }
}

VOID DisableMagnifier()
{
    ShowWindow(hwndHost, SW_HIDE);
    enabled = FALSE;
    SetThreadpoolTimer(refreshTimer, nullptr, 0, 0); // Stop the refresh timer

    // reset any panning that had been done
    StopPanMouse();
}

BOOL EnableMagnifier()
{
    RefreshMagnifier(); // update position/rect before showing		
    SetThreadpoolTimer(refreshTimer, &timerDueTime, 0, 0); // Start the refresh timer
    enabled = TRUE;
    ShowWindow(hwndHost, SW_SHOWNOACTIVATE);
    return TRUE;
}

// Toggles showing the magnifier
VOID ToggleMagnifier()
{
    if (enabled) { DisableMagnifier(); }
    else { EnableMagnifier(); }
}

#pragma region Handle key states

BOOL HandleKeyStates()
{
    if (KEYDOWN_ZOOM_IN && !KEYDOWN_ZOOM_OUT)
    {
        magManager->IncreaseMagnification(lensPosition);
        return TRUE;
    }
    if (KEYDOWN_ZOOM_OUT && !KEYDOWN_ZOOM_IN)
    {
        if (!magManager->DecreaseMagnification(lensPosition))
        {
            DisableMagnifier();
        }
        return TRUE;
    }

    return FALSE;
}

#pragma endregion

VOID StartPanMouse()
{
    mouseLockPoint.left = mousePoint.x;
    mouseLockPoint.top = mousePoint.y;
    mouseLockPoint.right = mousePoint.x;
    mouseLockPoint.bottom = mousePoint.y;

    ClipCursor(&mouseLockPoint); // locks mouse movement
    MagShowSystemCursor(FALSE);
    panningEnabled = TRUE;
}

VOID StopPanMouse()
{
    panningEnabled = FALSE;
    panOffset.x = 0;
    panOffset.y = 0;
    MagShowSystemCursor(TRUE);
    ClipCursor(NULL); // unlocks mouse movement
}

#pragma region Keyboard & Mouse Hook Callback

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION) // do not process message 
    {
        return CallNextHookEx(hkb, nCode, wParam, lParam);
    }

    key = ((KBDLLHOOKSTRUCT*)lParam);
    if (key->vkCode < VK_F13 || key->vkCode > VK_F24)
    {
        return CallNextHookEx(hkb, nCode, wParam, lParam);
    }

    switch (key->vkCode)
    {
    case HOTKEY_TOGGLE_MAG:
    {
        bool keyDown = (wParam == WM_KEYDOWN);
        if (keyDown && !KEYDOWN_TOGGLE_MAG)
        {
            ToggleMagnifier();
        }
        KEYDOWN_TOGGLE_MAG = keyDown;
        return TRUE;
    }

    case HOTKEY_ZOOM_IN:
        KEYDOWN_ZOOM_IN = wParam == WM_KEYDOWN;
        if (KEYDOWN_ZOOM_IN && !enabled) { EnableMagnifier(); }
        return TRUE;
    case HOTKEY_ZOOM_OUT:
        KEYDOWN_ZOOM_OUT = wParam == WM_KEYDOWN;
        return TRUE;

    case HOTKEY_INCREASE_LENS:
        KEYDOWN_INCREASE_LENS = wParam == WM_KEYDOWN;
        if (KEYDOWN_INCREASE_LENS && !KEYDOWN_DECREASE_LENS)
        {
            if (!enabled) { EnableMagnifier(); }
            else if (magManager->IncreaseLensSize(resizeIncrement, resizeLimit))
            {
                UpdateHostSize();
            }
        }
        return TRUE;
    case HOTKEY_DECREASE_LENS:
        KEYDOWN_DECREASE_LENS = wParam == WM_KEYDOWN;
        if (KEYDOWN_DECREASE_LENS && !KEYDOWN_INCREASE_LENS)
        {
            if (!enabled) { EnableMagnifier(); }
            else  if (magManager->DecreaseLensSize(resizeIncrement, resizeLimit))
            {
                UpdateHostSize();
            }
        }
        return TRUE;

    case HOTKEY_PAN_MOUSE:
    {
        BOOL keyDown = wParam == WM_KEYDOWN;
        if (keyDown != KEYDOWN_PAN_MOUSE)
        {
            KEYDOWN_PAN_MOUSE = keyDown;
            if (KEYDOWN_PAN_MOUSE && enabled && !panningEnabled)
            {
                StartPanMouse();
            }
            else if (!KEYDOWN_PAN_MOUSE && enabled && panningEnabled)
            {
                StopPanMouse();
            }
        }
        return TRUE;
    }
    case HOTKEY_TOGGLE_PAN_MOUSE:
        if (wParam == WM_KEYDOWN && !KEYDOWN_TOGGLE_PAN_MOUSE && !KEYDOWN_PAN_MOUSE && enabled)
        {
            if (!panningEnabled) { StartPanMouse(); }
            else { StopPanMouse(); }
        }
        KEYDOWN_TOGGLE_PAN_MOUSE = wParam == WM_KEYDOWN;
        return TRUE;

    default:
        break;
    }

    return CallNextHookEx(hkb, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode != HC_ACTION || wParam != WM_MOUSEMOVE ||
        !panningEnabled || !enabled)
    {
        return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
    }

    MSLLHOOKSTRUCT* mouseInfo = (MSLLHOOKSTRUCT*)lParam;
    if (mouseInfo->pt.x != mousePoint.x || mouseInfo->pt.y != mousePoint.y)
    {
        int newPanX = panOffset.x + mouseInfo->pt.x - mousePoint.x;
        int newPanY = panOffset.y + mouseInfo->pt.y - mousePoint.y;

        int halfSrcWidth = (magManager->_lensSize.cx / magManager->GetMagFactor()) / -2;
        int halfSrcHeight = (magManager->_lensSize.cy / magManager->GetMagFactor()) / -2;
        int trackingAdjustmentX = (mousePoint.x - lensPosition.x) / magManager->GetMagFactor();
        int trackingAdjustmentY = (mousePoint.y - lensPosition.y) / magManager->GetMagFactor();

        int minPanX = halfSrcWidth - mousePoint.x + trackingAdjustmentX;
        int minPanY = halfSrcHeight - mousePoint.y + trackingAdjustmentY;
        int maxPanX = screenSize.cx + halfSrcWidth - mousePoint.x + trackingAdjustmentX;
        int maxPanY = screenSize.cy + halfSrcHeight - mousePoint.y + trackingAdjustmentY;

        panOffset.x = max(minPanX, min(newPanX, maxPanX));
        panOffset.y = max(minPanY, min(newPanY, maxPanY));
    }
    
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

#pragma endregion
