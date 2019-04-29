# Magnify10
Magnification Lens.

## What is this?

Magnify10 is a screen magnification lens for Windows 10. It is designed to be a lightweight, stripped down magnifier that can be toggle on/off quickly and easily. It is an alternative the Windows default Magnifier app.

<img width="600" alt="mag10_lens" src="https://user-images.githubusercontent.com/49893805/56876155-3e54dc00-69fa-11e9-9c6f-252fb1983bf1.PNG">

## How does it work?

It uses the Windows [Magnification API](https://docs.microsoft.com/en-us/windows/desktop/api/_magapi/) for the main functionality, and Win32 APIs to control the window.

This program is designed to run in the background and consumes very little resources when not active. 
After you run it, an icon will appear in the notification area.

<img width="166" alt="mag10_icon" src="https://user-images.githubusercontent.com/49893805/56876198-94298400-69fa-11e9-8f84-bae10a52729c.PNG">

It relies a low level keyboard hook to set up global hotkeys that are used to interact with the functionality.

When it is toggled on, there is a periodic timer that refreshes the magnification lens.

## How do i use it?

Below is a list of global keyboard shortcut commands to control with the lens.

### Basic functionality
| Keyboard shortcut | Description |
| --- | --- |
| Clicking on the app's notification area icon | Exit |
| Left Windows Key + \` (tilde) | Toggle lens on/off |
| Left Windows Key + q | Increase magnification factor |
| Left Windows Key + z | Decrease magnification factor |
| Left Windows Key + v | Increase lens size (Down to 1/16th screen size) |
| Left Windows Key + c | Decrease lens size (Up to screen size) |

### Pan functionality
Sometimes it can be useful to move the lens source area without moving the mouse. This can be done by panning in any direction.

| Keyboard shortcut | Description |
| --- | --- |
| Left Windows Key + a | Pan lens source left |
| Left Windows Key + s | Pan lens source right |
| Left Windows Key + w | Pan lens source up |
| Left Windows Key + x | Pan lens source down |

This pan offset is reset after toggling the lens off.

## Installation

Because this program requires access to the Windows UI in order to properly magnify certain Windows 10 elements, it is compiled with a manifest file in order to set the uiAccess level to `true`.

However, windows will only allow such executables to run if they are signed, and reside within a secure location such as `%systemdrive%\Program Files\` for example. [More on this](https://docs.microsoft.com/en-us/windows/security/threat-protection/security-policy-settings/user-account-control-only-elevate-uiaccess-applications-that-are-installed-in-secure-locations).

### Signing and running an executable
To sign and run an executable, it requires a certificate to be generated and installed in the machine's `Trusted Root Certificate Authority`. To do this:

#### Create a certificate
1. Open an elevated command prompt
2. Browse to a folder that you want to create the certificate in
3. Execute: `makecert -r -pe -n "CN=Test Certificate - For Internal Use Only" -ss PrivateCertStore testcert.cer`
4. Execute: `certmgr.exe -add testcert.cer -s -r localMachine root`

#### Sign the executable
1. Browse to the location of the .exe file
2. Execute: `SignTool sign /v /s PrivateCertStore /n "Test Certificate - For Internal Use Only" /t http://timestamp.verisign.com/scripts/timestamp.dll app.exe` where app.exe is the executable to sign.

Once the executable is signed, place it in the `Program Files` directory or a subdirectory, and run it from there.
