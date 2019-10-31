/*
    Contains the initialize and shutdown functions as well as some
    utility functions that don't belong in any of the other source files.
*/

#include <iostream>
#include <string>
#include <set>
#include <deque>

#include <windows.h>
#include <gdiplus.h>
using namespace Gdiplus;

#include "../platform.hpp"

#ifdef PROFILING
    #include "../profiling.hpp"
#else
    #define START_TIMER(desc)
    #define END_TIMER(desc)
#endif

ULONG_PTR gdiplusToken;

void initialize() {
    /*
        Initializes GDI+ and starts a thread for capturing events
     */

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    CreateThread(NULL, 0, thread, NULL, 0, NULL);
}

void shutdown() {
    /*
        Shuts down GDI+
     */

    GdiplusShutdown(gdiplusToken);
}

/*
    A thread that calls GetMessage in a loop. This is required for the
    mouse and keyboard hooks to work.
 */
DWORD WINAPI thread(LPVOID lpParameter) {   
    installHooks();
    createHiddenWindow(windowProc);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

/*
    Creates a hidden window, since using raw input requires a window.
 */
void createHiddenWindow(WNDPROC wndProc) {
    // https://docs.microsoft.com/en-us/windows/win32/winmsg/using-window-classes

    // Create a window class
    WNDCLASSEX wcx;
    HINSTANCE hinstance = GetModuleHandle(NULL);
    wcx.cbSize = sizeof(wcx);            // size of structure
    wcx.style = CS_HREDRAW | CS_VREDRAW; // redraw if size changes
    wcx.lpfnWndProc = wndProc;           // points to window procedure
    wcx.cbClsExtra = 0;                  // no extra class memory
    wcx.cbWndExtra = 0;                  // no extra window memory
    wcx.hInstance = hinstance;           // handle to instance
    wcx.hIcon = LoadIcon(NULL,
        IDI_APPLICATION);                // predefined app. icon
    wcx.hCursor = LoadCursor(NULL,
        IDC_ARROW);                      // predefined arrow
    wcx.hbrBackground = (HBRUSH)GetStockObject(
        WHITE_BRUSH);                    // white background brush
    wcx.lpszMenuName =  "MainMenu";      // name of menu resource
    wcx.lpszClassName = "MainWClass";    // name of window class
    wcx.hIconSm = (HICON)LoadImage(      // small class icon
        hinstance,
        MAKEINTRESOURCE(5),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);

    // Register the window class.
    RegisterClassEx(&wcx);

    // Create a window
    HWND hwnd = CreateWindow( 
        "MainWClass",        // name of window class
        "Sample",            // title-bar string
        0,                   // top-level window
        CW_USEDEFAULT,       // default horizontal position
        CW_USEDEFAULT,       // default vertical position
        CW_USEDEFAULT,       // default width
        CW_USEDEFAULT,       // default height
        (HWND) NULL,         // no owner window
        (HMENU) NULL,        // use class menu
        hinstance,           // handle to application instance
        (LPVOID) NULL);      // no window-creation data

    // Register the mouse as a raw input device
    // http://keithditch.powweb.com/Games/html/raw_input.html
    RAWINPUTDEVICE ridev[1];
    ridev[0].usUsagePage = 0x01;
    ridev[0].usUsage = 0x02;
    ridev[0].dwFlags = RIDEV_INPUTSINK; // Capture all input (not just this window)
    ridev[0].hwndTarget = hwnd;
    RegisterRawInputDevices(ridev, 1, sizeof(ridev[0]));
}

/*
    Sets the keyboard and mouse hooks.
 */
void installHooks() {
    HHOOK hook = SetWindowsHookEx(WH_MOUSE_LL, mouseHook, NULL, 0);
    if (hook == NULL)
        std::cout << "Mouse hook failed" << std::endl;

    hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHook, NULL, 0);
    if (hook == NULL)
        std::cout << "Keyboard hook failed" << std::endl;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    // https://docs.microsoft.com/en-us/windows/desktop/gdiplus/-gdiplus-retrieving-the-class-identifier-for-an-encoder-use

    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;  // Failure

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;  // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}