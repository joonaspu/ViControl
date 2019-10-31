#include <string>
#include <unordered_map>
#include <deque>
#include <mutex>

// Mutex that should be used before accessing the variables defined here
extern std::mutex inputMutex;

// Keep track of keys that have been pressed or released since last request
extern std::set<std::string> pressedKeys;
extern std::set<std::string> releasedKeys;

// Keep track of the total mouse delta since last request
extern std::pair<long, long> mouseDelta;

// Keep track of fake input events we are expecting so they can be filtered out
extern std::multiset<std::string> expectedKeyDowns;
extern std::multiset<std::string> expectedKeyUps;
extern std::deque<std::pair<long, long>> expectedMouseMovement;

// Contains keys that are currently held down as a result of calling sendKey
extern std::set<std::string> fakeKeysPressed;

/*
    Returns the platform-specific key code for the given key name.
 */
unsigned int getKeyCode(std::string key);

/*
    Returns the key name for the given platform-specific key code.
 */
std::string getKeyName(unsigned int key);

/*
    Returns a pointer to the unordered_map that contains the key names and
    corresponding key codes.
 */
std::unordered_map<std::string, unsigned int>* getKeysMap();

/*
    This function should be called whenever there is a new key up/down event.
    Maintains the pressedKeys and releasedKeys sets.
    Also ignores known fake events.
*/
void keyEvent(std::string name, bool down);

/*
    Returns true if the real user is holding any keys on the keyboard or mouse.
*/
bool isUserPressingKeys();

/*
    Releases all keys that are held down by the sendKey function.
*/
void releaseFakeInputs();

/*
    This function should be called whenever there is a new mouse movement event.
    Maintains mouseDelta and filters out fake events.
*/
void mouseEvent(long dx, long dy);