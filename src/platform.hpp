#include <string>
#include <set>

// Functions that are implemented by all platforms

/*
    Initializes some platform-specific libraries. Should be called once
    before using any of the other functions.
 */
void initialize();

/*
    Shuts down and cleans up after some platform-specific libraries.
    Should be called when any of the other functions will not be used anymore.
 */
void shutdown();

/*
    Captures a JPG screenshot of the entire display or a specific window.

    If processName is an empty string, the screenshot will be of the entire
    display. Otherwise a specific window will be captured.
    On Windows the name refers to the process that created the window.
    On Linux/X11 the name refers to the WM_NAME property of the window.

    The quality parameter is the encoding quality of the JPG, and should
    be between 0 and 100.

    The function will allocate memory for the screenshot and store its pointer
    in the address given by the imageBuffer parameter. The function returns
    the size of the image buffer in bytes.

    NOTE: The image buffer should be freed after it's no longer needed
    by calling delete[] on it.
 */
unsigned long getJPGScreenshot(std::string* processName, char** imageBuffer,
                               unsigned int quality);

/*
    Moves the mouse cursor by the given amount of pixels.
 */
unsigned int moveMouse(long dx, long dy);

/*
    Presses or releases the given key. The down parameter defines if
    the key should be pressed or released (true = press, false = release).

    If userOverride is set to true, the function will do nothing if the
    user is pressing any keys on the keyboard or mouse.

    Key names are defined in keys.cpp for the supported platforms.

    This function also handles mouse buttons and the scroll wheel.
    The key names for these are listed below:

    * Mouse buttons *
        mouse left
        mouse right
        mouse middle

    * Scroll wheel *
        mouse up
        mouse down
 */
unsigned int sendKey(std::string key, bool down, bool userOverride = false);

/*
    Returns a set with the names of the keys that were down at some point since
    the previous call to this function.

    Note: On Windows this only includes actual human inputs, on other platforms
    inputs sent by sendKey are also included.
 */
std::set<std::string> getKeys();

/*
    Returns how much the mouse cursor has moved since the last call to this
    function.

    Note: On Windows this only includes actual human inputs, on other platforms
    inputs sent by moveMouse are also included.
 */
std::pair<long, long> getMouse();

// Platform-specific functions
#ifdef _WIN32
    #include <windows.h>
    #include <gdiplus.h>

    // win.cpp
    int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

    DWORD WINAPI thread(LPVOID lpParameter);

    void createHiddenWindow(WNDPROC wndProc);

    void installHooks();

    // screen.cpp
    Gdiplus::Bitmap* takeScreenshot(HWND window);

    ULONG bitmapToJPG(Gdiplus::Bitmap* bitmap, char** imageBuffer,
                      unsigned int quality);

    bool CALLBACK enumWindowsCallback(HWND hwnd, LPARAM lParam);

    // inputs.cpp
    LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT CALLBACK mouseHook(int nCode, WPARAM wParam, LPARAM lParam);

    LRESULT CALLBACK keyboardHook(int nCode, WPARAM wParam, LPARAM lParam);
#endif
#ifdef __linux__
    #include <X11/Xlib.h>

    void initShm(Window window);

    Window findWindowRecursive(Window window, std::string* name);

    void eventThread();
#endif
#ifdef __APPLE__
    void eventThread();
#endif
