
#include "stdafx.h"
#include <wincodec.h>
#include <magnification.h> 
#include <threadpoolapiset.h>
#include <shellapi.h>
#include "MagWindowManager.h"
#include "Resource.h"


#pragma region Constants

// Magnification lens refresh interval - Should be as low as possible to match monitor refresh rate.
const UINT          TIMER_INTERVAL_MS = 7;

// Magnification rates
const float         MAGNIFICATION_INCREMENT = 0.5f;
const float         MAGNIFICATION_LIMIT = 12.0f;

// lens sizing factors as a percent of screen resolution
const float         INIT_LENS_WIDTH_FACTOR = 0.5f;
const float         INIT_LENS_HEIGHT_FACTOR = 0.5f;
const float         INIT_LENS_RESIZE_HEIGHT_FACTOR = 0.0625f;
const float         INIT_LENS_RESIZE_WIDTH_FACTOR = 0.0625f;
const float         LENS_MAX_WIDTH_FACTOR = 1.2f;
const float         LENS_MAX_HEIGHT_FACTOR = 1.2f;

// lens shift/pan increments
const int           PAN_INCREMENT_HORIZONTAL = 75;
const int           PAN_INCREMENT_VERTICAL = 75;

#pragma endregion


#pragma region Variables

float               magnificationFactor = 2.0f;
float               newMagnificationFactor = magnificationFactor; // Temp mag factor to store change during update

SIZE                screenSize;
SIZE                lensSize; // Size in pixels of the lens (host window)
POINT               lensPosition; // Top left corner of the lens (host window)


SIZE                resizeIncrement;
SIZE                resizeLimit;

// Current mouse location
POINT               mousePoint;

#pragma endregion


#pragma region Objects

// Main program handle 
const TCHAR         WindowClassName[] = TEXT("MagnifierWindow");

// Window handles
HWND                hwndHost;

MagWindowManager    magManager;

// Show magnifier or not
BOOL                enabled;
BOOL                enableTimer;

// lens pan offset x|y
POINT               panOffset;

// Keyboard/Mouse hook 
HHOOK               hkb;
KBDLLHOOKSTRUCT*    key;
BOOL                wkDown = FALSE;

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

// Calculates a lens size value that is slightly larger than (lens + increment) to give an extra buffer area on the edges
#define LENS_SIZE_BUFFER_VALUE(LENS_SIZE_VALUE, RESIZE_INCREMENT_VALUE) (LENS_SIZE_VALUE + (2 * RESIZE_INCREMENT_VALUE))

// Forward declarations. 
ATOM                RegisterHostWindowClass(HINSTANCE hInstance);
BOOL                SetupMagnifierManager(HINSTANCE hInst);
BOOL                SetupHostWindow(HINSTANCE hinst);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
VOID CALLBACK       TimerTickEvent(PTP_CALLBACK_INSTANCE, VOID* context, PTP_TIMER);

VOID                InitScreenDimensions();

BOOL                UpdateLensPosition(LPPOINT mousePoint);
VOID                RefreshMagnifier();

VOID                ToggleMagnifier();

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

    if (!MagInitialize())
    {
        return 0;
    }
    if (!SetupHostWindow(hInstance)) 
    {
        return 0;
    }
    if (!SetupMagnifierManager(hInstance)) 
    {
        return 0;
    }
    if (!UpdateWindow(hwndHost)) 
    {
        return 0; 
    }


    // Start as disabled
    ShowWindow(hwndHost, SW_HIDE);
    enabled = FALSE;
    enableTimer = TRUE;

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

    SetThreadpoolTimer(refreshTimer, nullptr, 0, 0);
    Shell_NotifyIcon(NIM_DELETE, &nid);
    MagUninitialize();
    magManager.DestroyWindows();
    DestroyWindow(hwndHost);

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

    lensSize.cx = (int)(screenSize.cx * INIT_LENS_WIDTH_FACTOR);
    lensSize.cy = (int)(screenSize.cy * INIT_LENS_HEIGHT_FACTOR);

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
    UpdateLensPosition(&mousePoint);

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
        lensSize.cx, lensSize.cy,
        nullptr, nullptr, hInst, nullptr);

    if (!hwndHost)
    {
        return FALSE;
    }

    // Make the window fully opaque.
    return SetLayeredWindowAttributes(hwndHost, 0, 255, LWA_ALPHA);
}

BOOL SetupMagnifierManager(HINSTANCE hInst)
{
    magManager = MagWindowManager(
        { 0, 0 }, // position in the host window coordinates - top left corner
        lensSize);
    return magManager.Create(hInst, hwndHost);
}

VOID CALLBACK TimerTickEvent(PTP_CALLBACK_INSTANCE, void *context, PTP_TIMER)
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
    if (lensPosition.x == LENS_POSITION_VALUE(mousePosition->x, magManager._lensSize.cx) &&
        lensPosition.y == LENS_POSITION_VALUE(mousePosition->y, magManager._lensSize.cy))
    {
        return FALSE; // No change needed
    }

    lensPosition.x = LENS_POSITION_VALUE(mousePosition->x, magManager._lensSize.cx);
    lensPosition.y = LENS_POSITION_VALUE(mousePosition->y, magManager._lensSize.cy);
    return TRUE; // Values were changed
}

VOID UpdateLensSize(BOOL increase)
{

    SIZE newSize;

    if (increase)
    {
        UpdateLensPosition(&mousePoint);

        newSize.cx = magManager._lensSize.cx + resizeIncrement.cx;
        newSize.cy = magManager._lensSize.cy + resizeIncrement.cy;
        SetWindowPos(hwndHost, HWND_TOPMOST,
            lensPosition.x, lensPosition.y, // x|y coordinate of top left corner
            newSize.cx, newSize.cy, // width|height of window
            SWP_SHOWWINDOW);

        magManager.IncreaseLensSize(resizeIncrement, hwndHost);

        RefreshMagnifier();
    }
    else
    {
        UpdateLensPosition(&mousePoint);

        newSize.cx = magManager._lensSize.cx - resizeIncrement.cx;
        newSize.cy = magManager._lensSize.cy - resizeIncrement.cy;
        SetWindowPos(hwndHost, HWND_TOPMOST,
            lensPosition.x, lensPosition.y, // x|y coordinate of top left corner
            newSize.cx, newSize.cy, // width|height of window
            SWP_SHOWWINDOW);

        magManager.DecreaseLensSize(resizeIncrement, hwndHost);
        RefreshMagnifier();
    }

  
}

// Called in the timer tick event to refresh the magnification area drawn and lens (host window) position and size
VOID RefreshMagnifier() 
{
    GetCursorPos(&mousePoint);

    magManager.RefreshMagnifier(&mousePoint, panOffset);

    if (UpdateLensPosition(&mousePoint))
    {
        SetWindowPos(hwndHost, HWND_TOPMOST,
            lensPosition.x, lensPosition.y, // x|y coordinate of top left corner
            magManager._lensSize.cx, magManager._lensSize.cy, // width|height of window
            SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE);
    }
}


// Toggles showing the magnifier
VOID ToggleMagnifier()
{
    if (enabled)
    {
        ShowWindow(hwndHost, SW_HIDE);
        enabled = FALSE;
        SetThreadpoolTimer(refreshTimer, nullptr, 0, 0); // Stop the refresh timer

        // reset any panning that had been done
        panOffset.x = 0;
        panOffset.y = 0;
    }
    else
    {
        RefreshMagnifier(); // update position/rect before showing		
        enabled = TRUE;
        SetThreadpoolTimer(refreshTimer, &timerDueTime, 0, 0); // Start the refresh timer
        ShowWindow(hwndHost, SW_SHOWNOACTIVATE);
    }
}


#pragma region Keyboard Hook Callback

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION) // do not process message 
    {
        return CallNextHookEx(hkb, nCode, wParam, lParam);
    }

    if (((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_LWIN)
    {
        wkDown = wParam == WM_KEYDOWN;
    }
    else if (wkDown)
    {
        key = ((KBDLLHOOKSTRUCT*)lParam);

        if (key->vkCode == VK_OEM_3 /* tilde ` ~ key */ && wParam == WM_KEYDOWN)
        {
            ToggleMagnifier();
            return TRUE;
        }
        else if (enabled)
        {
            switch (wParam)
            {
            case WM_KEYDOWN:
                switch (key->vkCode)
                {
                case 0x74: // F5
                case 0x5A: // Z - decrease magnification
                    magManager.DecreaseMagnification();
                    return TRUE;    
                case 0x75: // F6
                case 0x51: // Q - increase magnification
                    magManager.IncreaseMagnification();
                    return TRUE;

                case 0x76: // F7
                case 0x43: // C - decrease lens size
                    UpdateLensSize(FALSE);
                    return TRUE;
                case 0x77: // F8
                case 0x56: // V - increase lens size
                    UpdateLensSize(TRUE);
                    return TRUE;

                case 0x57: // W - pan up
                    panOffset.y -= PAN_INCREMENT_VERTICAL;
                    return TRUE;
                case 0x58: // X - pan down	
                    panOffset.y += PAN_INCREMENT_VERTICAL;
                    return TRUE;
                case 0x41: // A - pan left
                    panOffset.x -= PAN_INCREMENT_HORIZONTAL;
                    return TRUE;
                case 0x53: // S - pan right
                    panOffset.x += PAN_INCREMENT_HORIZONTAL;
                    return TRUE;
  
                case 0x73: // F4
                    enableTimer = !enableTimer;
                    return TRUE;
                case 0x72: // F3
                    RefreshMagnifier();
                    return TRUE;

                default:
                    break;
                }
                break;

            default:
                break;
            }

        }
    }

    return CallNextHookEx(hkb, nCode, wParam, lParam);
}

#pragma endregion
