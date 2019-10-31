/*
    Contains input-related functions for Windows.
*/

#include <iostream>
#include <string>
#include <set>
#include <deque>

#include <windows.h>

#include "../keys.hpp"
#include "../platform.hpp"

#ifdef PROFILING
    #include "../profiling.hpp"
#else
    #define START_TIMER(desc)
    #define END_TIMER(desc)
#endif

unsigned int moveMouse(long dx, long dy) {
    INPUT input;

    input.type = INPUT_MOUSE;

    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.time = 0;

    std::unique_lock<std::mutex> lock(inputMutex);
    expectedMouseMovement.push_front(std::pair<long, long>(dx, dy));
    lock.unlock();

    return SendInput(1, &input, sizeof(INPUT));
}

unsigned int sendKey(std::string key, bool down, bool userOverride) {
    if (userOverride && isUserPressingKeys())
        return 0;

    INPUT input;

    // Handle mouse buttons separately
    if (key.find("mouse ") == 0) {
        input.type = INPUT_MOUSE;
        input.mi.dx = 0;
        input.mi.dy = 0;
        input.mi.time = 0;

        // Mouse buttons
        if (key == "mouse left" && down)
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        else if (key == "mouse left" && !down)
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        
        else if (key == "mouse right" && down)
            input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;

        else if (key == "mouse right" && !down)
            input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;

        else if (key == "mouse middle" && down)
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;

        else if (key == "mouse middle" && !down)
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;

        // Mouse wheel
        else if (key == "mouse up" && down) {
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = WHEEL_DELTA;
        } else if (key == "mouse down" && down) {
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = -WHEEL_DELTA;
        } else {
            return 0;
        }


    } else {
        unsigned int keyCode;
        // Get corresponding virtual key code
        try {
            keyCode = getKeyCode(key);
        } catch (std::out_of_range e) {
            return 0;
        }

        input.type = INPUT_KEYBOARD;

        // Convert virtual key code to scan code
        input.ki.wScan = MapVirtualKeyA(keyCode, MAPVK_VK_TO_VSC);

        if (down)
            input.ki.dwFlags = KEYEVENTF_SCANCODE;
        else
            input.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;

        input.ki.time = 0;
    }

    // Update expectedKeyDowns/Ups so we can ignore the input event caused
    // by the SendInput

    // Also update fakeKeysPressed so we can release this key if the user
    // overrides our input

    std::unique_lock<std::mutex> lock(inputMutex);

    if (down) {
        expectedKeyDowns.insert(key);
        fakeKeysPressed.insert(key);
    } else {
        expectedKeyUps.insert(key);
        fakeKeysPressed.erase(key);
    }

    lock.unlock();

    return SendInput(1, &input, sizeof(INPUT));
}

/*
    Returns the contents of pressedKeys and removes keys that were released.
 */
std::set<std::string> getKeys() {
    std::lock_guard<std::mutex> lock(inputMutex);

    // Return keys that were held down since last call
    auto keysReturn = std::set<std::string>(pressedKeys);

    // Remove released keys from pressedKeys
    for (auto key : releasedKeys) {
        pressedKeys.erase(key);
    }
    releasedKeys.clear();

    return keysReturn;
}

/*
    Returns the value of mouseDelta and resets the value back to (0, 0).
 */
std::pair<long, long> getMouse() {
    std::unique_lock<std::mutex> lock(inputMutex);

    long xd = mouseDelta.first;
    long yd = mouseDelta.second;

    mouseDelta.first = 0;
    mouseDelta.second = 0;

    lock.unlock();

    return std::pair<long, long>(xd, yd);
}

/*
    The window procedure that handles events for our hidden window.
    This captures the raw mouse input events.
 */
LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // http://keithditch.powweb.com/Games/html/raw_input.html
    if (uMsg == WM_INPUT) {
        // Get required buffer size
        UINT bufferSize;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize,
                        sizeof (RAWINPUTHEADER));
        
        // Allocate buffer
        BYTE* buffer = new BYTE[bufferSize];

        // Read data to buffer
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, (LPVOID)buffer,
                        &bufferSize, sizeof (RAWINPUTHEADER));
        
        RAWINPUT *raw = (RAWINPUT*) buffer;

        // Add relative mouse movement to the mouse delta
        if (raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
            LONG dx = raw->data.mouse.lLastX;
            LONG dy = raw->data.mouse.lLastY;

            mouseEvent(dx, dy);
        }
        delete[] buffer;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/*
    Windows will call this function every time there is a new mouse event.
 */
LRESULT CALLBACK mouseHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT* param = (MSLLHOOKSTRUCT*)lParam;
        switch (wParam) {
            case WM_LBUTTONDOWN:
                keyEvent("mouse left", true);
                break;

            case WM_LBUTTONUP:
                keyEvent("mouse left", false);
                break;

            case WM_RBUTTONDOWN:
                keyEvent("mouse right", true);
                break;

            case WM_RBUTTONUP:
                keyEvent("mouse right", false);
                break;
            
            case WM_MBUTTONDOWN:
                keyEvent("mouse middle", true);
                break;

            case WM_MBUTTONUP:
                keyEvent("mouse middle", false);
                break;

            case WM_MOUSEWHEEL:
                short delta = GET_WHEEL_DELTA_WPARAM(param->mouseData);
                if (delta > 0) {
                    keyEvent("mouse up", true);
                    keyEvent("mouse up", false);
                } else {
                    keyEvent("mouse down", true);
                    keyEvent("mouse down", false);
                }
                break;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

/*
    Windows will call this function every time there is a new keyboard event.
 */
LRESULT CALLBACK keyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* param = (KBDLLHOOKSTRUCT*)lParam;
        DWORD keyCode = param->vkCode;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            std::string keyName = getKeyName(keyCode);
            keyEvent(keyName, true);
        }
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            std::string keyName = getKeyName(keyCode);
            keyEvent(keyName, false);
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}