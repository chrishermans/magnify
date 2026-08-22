# Magnify10
Magnification Lens.

## What is this?

Magnify10 is a screen magnification lens for Windows 10. It is designed to be a lightweight, stripped down magnifier that can be toggled on/off quickly and easily. It is an alternative the Windows default Magnifier app.

<img width="600" alt="mag10_lens" src="https://user-images.githubusercontent.com/49893805/56876155-3e54dc00-69fa-11e9-9c6f-252fb1983bf1.PNG">

## How does it work?

It uses the Windows [Magnification API](https://docs.microsoft.com/en-us/windows/desktop/api/_magapi/) for the main functionality, and Win32 APIs to control the window.

This program is designed to run in the background and consumes very little resources when not active. 
After you run it, an icon will appear in the notification area.

<img width="166" alt="mag10_icon" src="https://user-images.githubusercontent.com/49893805/56876198-94298400-69fa-11e9-8f84-bae10a52729c.PNG">

It relies a low level keyboard & mouse hook to set up global hotkeys that are used to interact with the functionality.

When it is toggled on, there is a periodic timer that refreshes the magnification lens.

## How do i use it?

Below is a list of global keyboard shortcut commands to control the lens.

### Basic functionality

| Default Keyboard shortcut | Description |
| --- | --- |
| Clicking on the app's notification icon | Exit |
| F13 | Toggle lens on/off |
| F15 | Increase magnification factor |
| F14 | Decrease magnification factor |
| F18 | Increase lens size (Down to 1/16th screen size) |
| F17 | Decrease lens size (Up to screen size) |
| F21 | Reset screen size |

### Pan functionality
Sometimes it can be useful to move the lens source area without moving the mouse. This can be done by panning in any direction.

| Default Keyboard shortcut | Description |
| --- | --- |
| F22 | Pan lens source left |
| F24 | Pan lens source right |
| F23 | Pan lens source up |
| F20 | Pan lens source down |
| F16 | Pan with mouse movement |
| F19 | Toggle panning with mouse |

This pan offset is reset after toggling the lens off or exiting mouse panning.

## Configuration File

Hotkeys and settings can be changed in the `Magnify10.ini` file which goes in the same directory as the executable. All configuration is loaded at startup.

[See Magnify10.ini](Magnify10/Magnify10.ini) for available settings

## Building & Installation

Because this program requires access to the Windows UI in order to properly magnify certain Windows 10 elements, it should be compiled with a manifest file in order to set the `uiAccess` level to `true`.

When building the project through Visual Studio, set the `uiAccess` to `true` by going to the project Properties -> Linker -> Manifest File, and setting UAC Bypass UI Protection to `Yes (/uiAccess='true')`

However, windows will only allow such executables to run if they are signed, and reside within a secure location such as `%systemdrive%\Program Files\`. [More on this](https://docs.microsoft.com/en-us/windows/security/threat-protection/security-policy-settings/user-account-control-only-elevate-uiaccess-applications-that-are-installed-in-secure-locations).

[See SigningInstructions](SigningInstructions.md) for details on how to sign a new build

Once signed, move the executable and ini file under `%systemdrive%\Program Files\`