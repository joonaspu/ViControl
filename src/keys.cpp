#include <unordered_map>
#include <string>
#include <set>
#include <deque>
#include <mutex>

#include "platform.hpp"
#include "keys.hpp"

// Mutex that should be used before accessing the variables defined here
std::mutex inputMutex;

std::set<std::string> pressedKeys;
std::set<std::string> releasedKeys;
std::pair<long, long> mouseDelta = std::pair<long, long>(0, 0);

// Keep track of fake input events we are expecting so they can be filtered out
std::multiset<std::string> expectedKeyDowns;
std::multiset<std::string> expectedKeyUps;
std::deque<std::pair<long, long>> expectedMouseMovement;

// Contains keys that are currently held down as a result of calling sendKey
std::set<std::string> fakeKeysPressed;

#ifdef _WIN32
    #include <windows.h>

    //https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    std::unordered_map<std::string, unsigned int> keys = {
        // Misc
        {"backspace", VK_BACK},
        {"tab", VK_TAB},
        {"clear", VK_CLEAR},
        {"enter", VK_RETURN},
        {"shift", VK_SHIFT},
        {"ctrl", VK_CONTROL},
        {"alt", VK_MENU},
        {"pause", VK_PAUSE},
        {"caps lock", VK_CAPITAL},
        {"esc", VK_ESCAPE},
        {"space", VK_SPACE},
        {"page up", VK_PRIOR},
        {"page down", VK_NEXT},
        {"end", VK_END},
        {"home", VK_HOME},

        {"left", VK_LEFT},
        {"up", VK_UP},
        {"right", VK_RIGHT},
        {"down", VK_DOWN},

        {"select", VK_SELECT},
        {"print", VK_PRINT},
        {"execute", VK_EXECUTE},
        {"print screen", VK_SNAPSHOT},
        {"insert", VK_INSERT},
        {"delete", VK_DELETE},
        {"help", VK_HELP},

        // Numbers
        {"0", '0'},
        {"1", '1'},
        {"2", '2'},
        {"3", '3'},
        {"4", '4'},
        {"5", '5'},
        {"6", '6'},
        {"7", '7'},
        {"8", '8'},
        {"9", '9'},

        // Letters
        {"a", 'A'},
        {"b", 'B'},
        {"c", 'C'},
        {"d", 'D'},
        {"e", 'E'},
        {"f", 'F'},
        {"g", 'G'},
        {"h", 'H'},
        {"i", 'I'},
        {"j", 'J'},
        {"k", 'K'},
        {"l", 'L'},
        {"m", 'M'},
        {"n", 'N'},
        {"o", 'O'},
        {"p", 'P'},
        {"q", 'Q'},
        {"r", 'R'},
        {"s", 'S'},
        {"t", 'T'},
        {"u", 'U'},
        {"v", 'V'},
        {"w", 'W'},
        {"x", 'X'},
        {"y", 'Y'},
        {"z", 'Z'},

        {"left windows", VK_LWIN},
        {"right windows", VK_RWIN},
        {"applications", VK_APPS},

        // Numpad
        {"numpad 0", VK_NUMPAD0},
        {"numpad 1", VK_NUMPAD1},
        {"numpad 2", VK_NUMPAD2},
        {"numpad 3", VK_NUMPAD3},
        {"numpad 4", VK_NUMPAD4},
        {"numpad 5", VK_NUMPAD5},
        {"numpad 6", VK_NUMPAD6},
        {"numpad 7", VK_NUMPAD7},
        {"numpad 8", VK_NUMPAD8},
        {"numpad 9", VK_NUMPAD9},

        {"multiply", VK_MULTIPLY},
        {"add", VK_ADD},
        {"separator", VK_SEPARATOR},
        {"subtract", VK_SUBTRACT},
        {"decimal", VK_DECIMAL},
        {"divide", VK_DIVIDE},

        // Function keys
        {"f1", VK_F1},
        {"f2", VK_F2},
        {"f3", VK_F3},
        {"f4", VK_F4},
        {"f5", VK_F5},
        {"f6", VK_F6},
        {"f7", VK_F7},
        {"f8", VK_F8},
        {"f9", VK_F9},
        {"f10", VK_F10},
        {"f11", VK_F11},
        {"f12", VK_F12},

        {"num lock", VK_NUMLOCK},
        {"scroll lock", VK_SCROLL},

        {"left shift", VK_LSHIFT},
        {"right shift", VK_RSHIFT},
        {"left ctrl", VK_LCONTROL},
        {"right ctrl", VK_RCONTROL},
        {"left alt", VK_LMENU},
        {"right alt", VK_RMENU},

        // Regional keys
        // Always corresponds to this character, physical location may vary
        {"plus", VK_OEM_PLUS},
        {"comma", VK_OEM_COMMA},
        {"minus", VK_OEM_MINUS},
        {"period", VK_OEM_PERIOD},

        // Always corresponds to the same physical key, character may vary
        // Key names based on the US layout
        {"semicolon", VK_OEM_1},
        {"slash", VK_OEM_2},
        {"backtick", VK_OEM_3},
        {"square bracket open", VK_OEM_4},
        {"backslash", VK_OEM_5},
        {"square bracket close", VK_OEM_6},
        {"quote", VK_OEM_7},

        // "Either the angle bracket key or the backslash key",
        // between left shift and Z
        {"angle bracket", VK_OEM_102}
    };
#endif

#ifdef __linux__
    #include <X11/keysym.h>

    // From X11/keysymdef.h
    std::unordered_map<std::string, unsigned int> keys = {
        // Misc
        {"backspace", XK_BackSpace},
        {"tab", XK_Tab},
        {"clear", XK_Clear},
        {"enter", XK_Return},
        {"shift", XK_Shift_L},
        {"ctrl", XK_Control_L},
        {"alt", XK_Alt_L},
        {"pause", XK_Pause},
        {"caps lock", XK_Caps_Lock},
        {"esc", XK_Escape},
        {"space", XK_space},
        {"page up", XK_Page_Up},
        {"page down", XK_Page_Down},
        {"end", XK_End},
        {"home", XK_Home},

        {"left", XK_Left},
        {"up", XK_Up},
        {"right", XK_Right},
        {"down", XK_Down},

        {"select", XK_Select},
        {"print", XK_Print},
        {"execute", XK_Execute},
        {"insert", XK_Insert},
        {"delete", XK_Delete},
        {"help", XK_Help},

        // Numbers
        {"0", XK_0},
        {"1", XK_1},
        {"2", XK_2},
        {"3", XK_3},
        {"4", XK_4},
        {"5", XK_5},
        {"6", XK_6},
        {"7", XK_7},
        {"8", XK_8},
        {"9", XK_9},

        // Letters
        {"a", XK_a},
        {"b", XK_b},
        {"c", XK_c},
        {"d", XK_d},
        {"e", XK_e},
        {"f", XK_f},
        {"g", XK_g},
        {"h", XK_h},
        {"i", XK_i},
        {"j", XK_j},
        {"k", XK_k},
        {"l", XK_l},
        {"m", XK_m},
        {"n", XK_n},
        {"o", XK_o},
        {"p", XK_p},
        {"q", XK_q},
        {"r", XK_r},
        {"s", XK_s},
        {"t", XK_t},
        {"u", XK_u},
        {"v", XK_v},
        {"w", XK_w},
        {"x", XK_x},
        {"y", XK_y},
        {"z", XK_z},

        {"left windows", XK_Super_L},
        {"right windows", XK_Super_R},

        // Numpad
        {"numpad 0", XK_KP_0},
        {"numpad 1", XK_KP_1},
        {"numpad 2", XK_KP_2},
        {"numpad 3", XK_KP_3},
        {"numpad 4", XK_KP_4},
        {"numpad 5", XK_KP_5},
        {"numpad 6", XK_KP_6},
        {"numpad 7", XK_KP_7},
        {"numpad 8", XK_KP_8},
        {"numpad 9", XK_KP_9},

        {"multiply", XK_KP_Multiply},
        {"add", XK_KP_Add},
        {"separator", XK_KP_Separator},
        {"subtract", XK_KP_Subtract},
        {"decimal", XK_KP_Decimal},
        {"divide", XK_KP_Divide},

        // Function keys
        {"f1", XK_F1},
        {"f2", XK_F2},
        {"f3", XK_F3},
        {"f4", XK_F4},
        {"f5", XK_F5},
        {"f6", XK_F6},
        {"f7", XK_F7},
        {"f8", XK_F8},
        {"f9", XK_F9},
        {"f10", XK_F10},
        {"f11", XK_F11},
        {"f12", XK_F12},

        //{"num lock", },
        {"scroll lock", XK_Scroll_Lock},

        {"left shift", XK_Shift_L},
        {"right shift", XK_Shift_R},
        {"left ctrl", XK_Control_L},
        {"right ctrl", XK_Control_R},
        {"left alt", XK_Alt_L},
        {"right alt", XK_Alt_R},

        // Regional keys
        {"plus", XK_plus},
        {"comma", XK_comma},
        {"minus", XK_minus},
        {"period", XK_period},
        {"semicolon", XK_semicolon},
        {"slash", XK_slash},
        {"backtick", XK_grave},
        {"square bracket open", XK_bracketleft},
        {"backslash", XK_backslash},
        {"square bracket close", XK_bracketright},
        {"quote", XK_quoteleft},
        {"angle bracket", XK_bracketleft}
    };
#endif

#ifdef __APPLE__
    #include <Carbon/Carbon.h>
    std::unordered_map<std::string, unsigned int> keys = {
        // Misc
        {"backspace", kVK_Delete},
        {"tab", kVK_Tab},
        {"enter", kVK_Return},
        {"shift", kVK_Shift},
        {"ctrl", kVK_Control},
        {"alt", kVK_Option},
        {"caps lock", kVK_CapsLock},
        {"esc", kVK_Escape},
        {"space", kVK_Space},
        {"page up", kVK_PageUp},
        {"page down", kVK_PageDown},
        {"end", kVK_End},
        {"home", kVK_Home},

        {"left", kVK_LeftArrow},
        {"up", kVK_UpArrow},
        {"right", kVK_RightArrow},
        {"down", kVK_DownArrow},

        {"delete", kVK_ForwardDelete},
        {"help", kVK_Help},

        // Numbers
        {"0", kVK_ANSI_0},
        {"1", kVK_ANSI_1},
        {"2", kVK_ANSI_2},
        {"3", kVK_ANSI_3},
        {"4", kVK_ANSI_4},
        {"5", kVK_ANSI_5},
        {"6", kVK_ANSI_6},
        {"7", kVK_ANSI_7},
        {"8", kVK_ANSI_8},
        {"9", kVK_ANSI_9},

        // Letters
        {"a", kVK_ANSI_A},
        {"b", kVK_ANSI_B},
        {"c", kVK_ANSI_C},
        {"d", kVK_ANSI_D},
        {"e", kVK_ANSI_E},
        {"f", kVK_ANSI_F},
        {"g", kVK_ANSI_G},
        {"h", kVK_ANSI_H},
        {"i", kVK_ANSI_I},
        {"j", kVK_ANSI_J},
        {"k", kVK_ANSI_K},
        {"l", kVK_ANSI_L},
        {"m", kVK_ANSI_M},
        {"n", kVK_ANSI_N},
        {"o", kVK_ANSI_O},
        {"p", kVK_ANSI_P},
        {"q", kVK_ANSI_Q},
        {"r", kVK_ANSI_R},
        {"s", kVK_ANSI_S},
        {"t", kVK_ANSI_T},
        {"u", kVK_ANSI_U},
        {"v", kVK_ANSI_V},
        {"w", kVK_ANSI_W},
        {"x", kVK_ANSI_X},
        {"y", kVK_ANSI_Y},
        {"z", kVK_ANSI_Z},

        {"left windows", kVK_Command},
        {"right windows", kVK_RightCommand},

        // Numpad
        {"numpad 0", kVK_ANSI_Keypad0},
        {"numpad 1", kVK_ANSI_Keypad1},
        {"numpad 2", kVK_ANSI_Keypad2},
        {"numpad 3", kVK_ANSI_Keypad3},
        {"numpad 4", kVK_ANSI_Keypad4},
        {"numpad 5", kVK_ANSI_Keypad5},
        {"numpad 6", kVK_ANSI_Keypad6},
        {"numpad 7", kVK_ANSI_Keypad7},
        {"numpad 8", kVK_ANSI_Keypad8},
        {"numpad 9", kVK_ANSI_Keypad9},

        {"multiply", kVK_ANSI_KeypadMultiply},
        {"add", kVK_ANSI_KeypadPlus},
        {"subtract", kVK_ANSI_KeypadMinus},
        {"decimal", kVK_ANSI_KeypadDecimal},
        {"divide", kVK_ANSI_KeypadDivide},

        // Function keys
        {"f1", kVK_F1},
        {"f2", kVK_F2},
        {"f3", kVK_F3},
        {"f4", kVK_F4},
        {"f5", kVK_F5},
        {"f6", kVK_F6},
        {"f7", kVK_F7},
        {"f8", kVK_F8},
        {"f9", kVK_F9},
        {"f10", kVK_F10},
        {"f11", kVK_F11},
        {"f12", kVK_F12},

        {"left shift", kVK_Shift},
        {"right shift", kVK_RightShift},
        {"left ctrl", kVK_Control},
        {"right ctrl", kVK_RightControl},
        {"left alt", kVK_Option},
        {"right alt", kVK_RightOption},

        // Regional keys
        {"plus", kVK_ANSI_Equal},
        {"comma", kVK_ANSI_Comma},
        {"minus", kVK_ANSI_Minus},
        {"period", kVK_ANSI_Period},
        {"semicolon", kVK_ANSI_Semicolon},
        {"slash", kVK_ANSI_Slash},
        {"backtick", kVK_ANSI_Grave},
        {"square bracket open", kVK_ANSI_LeftBracket},
        {"backslash", kVK_ANSI_Backslash},
        {"square bracket close", kVK_ANSI_RightBracket},
        {"quote", kVK_ANSI_Quote},
    };
#endif

// Reversed version of the map
std::unordered_map<unsigned int, std::string> rev_keys = {};

std::string getKeyName(unsigned int key) {
    // TODO: Better way to create a reversed version of the map?
    if (rev_keys.size() < 1) {
        for (auto key : keys) {
            rev_keys[key.second] = key.first;
        }
    }

    try {
        return rev_keys.at(key);
    } catch (std::out_of_range) {
        return "";
    }
}

unsigned int getKeyCode(std::string key) {
    return keys.at(key);
}

std::unordered_map<std::string, unsigned int>* getKeysMap() {
    return &keys;
}

void keyEvent(std::string name, bool down) {
    std::unique_lock<std::mutex> lock(inputMutex);

    if (down) {
        // If the key is in expectedKeyDowns, we know this is a fake event
        // that can be ignored
        auto keyIter = expectedKeyDowns.find(name);
        if (keyIter != expectedKeyDowns.end())
            expectedKeyDowns.erase(keyIter);
        else {
            pressedKeys.insert(name);
            releasedKeys.erase(name);
            lock.unlock();
            releaseFakeInputs();
            lock.lock();
        }
    } else {
        // If the key is in expectedKeyUps, we know this is a fake event
        // that can be ignored
        auto keyIter = expectedKeyUps.find(name);
        if (keyIter != expectedKeyUps.end())
            expectedKeyUps.erase(keyIter);
        else
            releasedKeys.insert(name);
    }

    return;
}

bool isUserPressingKeys() {
    for (auto key : pressedKeys) {
        if (releasedKeys.count(key) == 0)
            return true;
    }

    return false;
}

void releaseFakeInputs() {
    // Make a copy of fakeKeysPressed to use in the for loop,
    // because sendKey removes elements from fakeKeysPressed,
    // which can break the iterator and cause the loop to never end
    auto fakeKeysPressedCopy = std::set<std::string>(fakeKeysPressed);
    for (auto key : fakeKeysPressedCopy)
        sendKey(key, false, false);

    std::lock_guard<std::mutex> lock(inputMutex);
    fakeKeysPressed.clear();
}

void mouseEvent(long dx, long dy) {
    std::lock_guard<std::mutex> lock(inputMutex);

    if (!expectedMouseMovement.empty() &&
        expectedMouseMovement.back() == std::pair<long, long>(dx, dy))
    {
        expectedMouseMovement.pop_back();
    } else {
        mouseDelta.first += dx;
        mouseDelta.second += dy;
    }
}