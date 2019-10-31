/*
    Contains the screen capture functions for Windows.
*/

#include <iostream>
#include <string>

#include <windows.h>
#include <gdiplus.h>
#include <psapi.h>
using namespace Gdiplus;

#include "../platform.hpp"

#ifdef PROFILING
    #include "../profiling.hpp"
#else
    #define START_TIMER(desc)
    #define END_TIMER(desc)
#endif

struct WindowEnumParams {
    std::string* processName;
    Bitmap* screenshot;
};

std::string cachedProcessName;
HWND cachedWindow;

unsigned long getJPGScreenshot(std::string* processName, char** imageBuffer,
                               unsigned int quality) {

    Bitmap* screenshot;

    // Parameters for EnumWindows callback
    WindowEnumParams params;      
    params.processName = processName;
    params.screenshot = screenshot;

    START_TIMER("takeScreenshot");

    // Empty process name: capture entire display
    if (processName->length() == 0) {
        params.screenshot = takeScreenshot(NULL);
    
    // Use saved window ID if process name is unchanged and window still exists
    } else if (*processName == cachedProcessName && IsWindow(cachedWindow)) {
        params.screenshot = takeScreenshot(cachedWindow);

    // Process name is new
    } else {
        // Enumerate through windows
        WINBOOL enumStatus =
            EnumWindows((WNDENUMPROC) enumWindowsCallback, (LPARAM) &params);
        if (enumStatus == TRUE) {
            END_TIMER("takeScreenshot");
            throw std::invalid_argument("window not found");
        }
    }

    END_TIMER("takeScreenshot");

    START_TIMER("bitmapToJPG");

    // Convert to JPG
    unsigned long bytes = bitmapToJPG(params.screenshot, imageBuffer, quality);

    END_TIMER("bitmapToJPG");

    return bytes;
}

Bitmap* takeScreenshot(HWND window) {
    /*
        Takes a screenshot of the screen and
        returns a pointer to a GDI+ Bitmap.

        Note: The returned Bitmap must be manually deleted to free memory

        Parameters:
            window: The window handle of the window to take screenshot of.
                    If this is NULL, takes a screenshot of the entire display.
    */
    // https://stackoverflow.com/questions/3291167/how-can-i-take-a-screenshot-in-a-windows-application

    // Get window/screen device context

    // Get client area (no title bar etc.)
    HDC dcScreen = GetDC(window);

    // Get window area (includes title bar)
    //HDC dcScreen = GetWindowDC(window);

    // Create target device context
    HDC dcTarget = CreateCompatibleDC(dcScreen);

    // Get window/screen width and height
    RECT windowSize;
    int width;
    int height;
    if (GetClientRect(window, &windowSize))
    {
        width = windowSize.right;
        height = windowSize.bottom;
    }
    else
    {
        width = GetDeviceCaps(dcScreen, HORZRES);
        height = GetDeviceCaps(dcScreen, VERTRES);
    }

    // Create a new bitmap
    HBITMAP bmpTarget = CreateCompatibleBitmap(dcScreen, width, height);

    // Select the new bitmap
    HGDIOBJ oldBmp = SelectObject(dcTarget, bmpTarget);

    // Copy from the screen
    BitBlt(dcTarget, 0, 0, width, height, dcScreen, 0, 0, SRCCOPY | CAPTUREBLT);

    // Deselect the new bitmap
    SelectObject(dcTarget, oldBmp);

    // Delete target DC and release screen DC
    DeleteDC(dcTarget);
    ReleaseDC(window, dcScreen);

    // Create a GDI+ bitmap
    Bitmap* bitmap = Bitmap::FromHBITMAP(bmpTarget, NULL);
    DeleteObject(bmpTarget);

    return bitmap;
}

ULONG bitmapToJPG(Bitmap* bitmap, char** imageBuffer, unsigned int quality) {
    /*
        Encodes a GDI+ bitmap as JPG.

        Parameters:
            bitmap: Pointer to the GDI+ Bitmap that is being encoded.
            imageBuffer: address of the pointer that will receive the new image
            quality: quality of the compressed jpg (0-100)
    */

    // Create IStream for the image
    IStream* istream;
    CreateStreamOnHGlobal(NULL, true, &istream);

    // Get jpg encoder
    CLSID encoderClsid;
    GetEncoderClsid(L"image/jpeg", &encoderClsid);

    // Set jpg quality
    Gdiplus::EncoderParameters params;
    params.Count = 1;
    params.Parameter[0].Guid = Gdiplus::EncoderQuality;
    params.Parameter[0].Type = EncoderParameterValueTypeLong;
    params.Parameter[0].NumberOfValues = 1;
    params.Parameter[0].Value = &quality;

    // Save compressed bitmap to the IStream
    Status stat = bitmap->Save(istream, &encoderClsid, &params);

    // Delete Bitmap object to free memory
    delete bitmap;

    if (stat != Ok)
        std::cout << "Saving image failed" << std::endl;

    // Allocate memory for the image
    STATSTG statstg;
    istream->Stat(&statstg, 0);
    ULONGLONG bufferLength = statstg.cbSize.QuadPart;
    *imageBuffer = new char[bufferLength];

    // Copy data from IStream to the allocated byte array
    LARGE_INTEGER seekPosition;
    seekPosition.QuadPart = 0;
    istream->Seek(seekPosition, STREAM_SEEK_SET, NULL);

    ULONG bytesRead;
    HRESULT readResult = istream->Read(*imageBuffer, bufferLength, &bytesRead);
    if (readResult != S_OK)
        std::cout << "Reading from IStream failed" << std::endl;

    istream->Release();

    return bytesRead;
}

bool CALLBACK enumWindowsCallback(HWND hwnd, LPARAM lParam) {
    /*
        Callback for enumerating through all open windows.
        
        Parameters:
            - hwnd: The window handle of the current window.

            - lParam:
            Pointer to a WindowEnumParams struct where:
            processName is the name of the process to take a screenshot of,
            screenshot is a GDI+ Bitmap pointer for saving the screenshot to.

    */

    // Parameters
    WindowEnumParams* params = (WindowEnumParams*) lParam;

    // Get ID of process that created the window
    DWORD pid;
    DWORD tid;
    tid = GetWindowThreadProcessId(hwnd, &pid);

    // Get executable name of the process
    TCHAR executable[MAX_PATH];
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                               false, pid);
    GetModuleFileNameEx((HANDLE)hProc, NULL, executable, MAX_PATH);
    CloseHandle(hProc);

    //std::cout << "Executable: " << executable << std::endl;

    std::string executable_string = executable;

    if (IsWindowVisible(hwnd) &&
        executable_string.find(*(params->processName), 0) != std::string::npos)
    {
        // Save process name and HWND
        cachedProcessName = *params->processName;
        cachedWindow = hwnd;

        // Take screenshot
        params->screenshot = takeScreenshot(hwnd);
        return FALSE;
    }
    return TRUE;
}