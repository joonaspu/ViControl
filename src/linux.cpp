#include <iostream>
#include <set>
#include <deque>
#include <cstring>
#include <thread>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/XInput2.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <turbojpeg.h>

#include "keys.hpp"
#include "platform.hpp"

XShmSegmentInfo shmInfo;
Display* display;
Display* eventDisplay;
XImage* image;
Window root;

Window cachedWindow;
std::string cachedName;

bool stopThread = false;

void initShm(Window window) {
    /*
        Initializes SHM for the given window
     */

    // Get window attributes
    XWindowAttributes windowAttributes;
    XGetWindowAttributes(display, window, &windowAttributes);
    Screen* screen = windowAttributes.screen;

    // Get window size and depth
    Window root_return;
    int x_return;
    int y_return;
    unsigned int width_return;
    unsigned int height_return;
    unsigned int border_width_return;
    unsigned int depth_return;
    XGetGeometry(display, window, &root_return, &x_return, &y_return,
                 &width_return, &height_return, &border_width_return,
                 &depth_return);

    int width = width_return;
    int height = height_return;

    // Create shared memory image
    image = XShmCreateImage(display, DefaultVisualOfScreen(screen),
                            depth_return, ZPixmap, NULL,
                            &shmInfo, width, height);

    // Create shared memory
    shmInfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height,
                           IPC_CREAT|0777);

    // Attach shared memory to our process
    shmInfo.shmaddr = image->data = (char*)shmat(shmInfo.shmid, 0, 0);

    // Allow writing to the memory segment
    shmInfo.readOnly = false;

    // Attach X to the shared memory
    Status status = XShmAttach(display, &shmInfo);
}

int xErrorHandler(Display* d, XErrorEvent* e) {
    return 0;
}

void initialize() {
    XInitThreads();

    display = XOpenDisplay(NULL);
    eventDisplay = XOpenDisplay(NULL);
    root = DefaultRootWindow(display);
    cachedWindow = root;
    initShm(root);

    // Set error handler to prevent X from crashing the application on errors
    XSetErrorHandler(xErrorHandler);

    // Test availability of XTest
    int eventBaseReturn;
    int errorBaseReturn;
    int majorVersionReturn;
    int minorVersionReturn;
    if (!XTestQueryExtension(display, &eventBaseReturn, &errorBaseReturn,
                             &majorVersionReturn, &minorVersionReturn)) {
        std::cout << "XTest extension not available!" << std::endl
                  << "You will not be able to send input events" << std::endl;
    }

    if (!XQueryExtension(display, "XInputExtension", &majorVersionReturn,
                         &eventBaseReturn, &errorBaseReturn)) {
        std::cout << "XInput extension not available!" << std::endl
                  << "You will not be able to record inputs" << std::endl;
    }

    // https://stackoverflow.com/a/37328659
    XIEventMask masks[1];
    unsigned char mask[(XI_LASTEVENT + 7)/8];

    memset(mask, 0, sizeof(mask));
    XISetMask(mask, XI_RawMotion);
    XISetMask(mask, XI_RawButtonPress);
    XISetMask(mask, XI_RawButtonRelease);
    XISetMask(mask, XI_RawKeyPress);
    XISetMask(mask, XI_RawKeyRelease);

    masks[0].deviceid = XIAllMasterDevices;
    masks[0].mask_len = sizeof(mask);
    masks[0].mask = mask;

    XISelectEvents(eventDisplay, root, masks, 1);
    XFlush(eventDisplay);

    std::thread(eventThread).detach();
}

void shutdown() {
    // Tell the event thread to stop
    stopThread = true;

    // Detach from shared memory, destroy XImage and shared memory
    XShmDetach(display, &shmInfo);
    XDestroyImage(image);
    shmdt(shmInfo.shmaddr);
    XCloseDisplay(display);
}

Window findWindowRecursive(Window window, std::string* name) {
    /*
        Find and return a window with the given name.
        Uses the WM_NAME property of the window. Use the xprop command
        to find the WM_NAME of a window you want to use.
        If no matching window was found, returns the root window.

        Parameters:
            window: the root window whose children are searched
            name: full or partial string to match with the window name
     */

    // Get window children
    Window root_return;
    Window parent_return;
    Window* children_return;
    unsigned int nchildren_return;
    if (XQueryTree(display, window, &root_return, &parent_return, 
                   &children_return, &nchildren_return) == 0) {
        std::cout << "XQueryTree failed" << std::endl;
        return root;
    }

    // Enumerate through every child
    for (int i = 0; i < nchildren_return; i++) {
        XWindowAttributes attrs;
        XGetWindowAttributes(display, children_return[i], &attrs);

        // Get name of the child window
        XTextProperty xText;
        if (XGetWMName(display, children_return[i], &xText) > 0
            && attrs.map_state == IsViewable) {
            std::string thisName = (char*)xText.value;

            if (thisName.find(*name) != std::string::npos)
                return children_return[i];
        }

        // Call recursively for the child
        Window child = findWindowRecursive(children_return[i], name);
        if (child != root)
            return child;
    }
    return root;
}

unsigned long getJPGScreenshot(std::string* processName, char** imageBuffer,
                               unsigned int quality) {
    /*
        Takes a screenshot of the display.

        Parameters:
            processName: currently not used
            imageBuffer: address of the pointer that will receive the new image
            quality: quality of the compressed jpg (0-100)
    */
   
    Window window;

    // If process name is unchanged from previous request
    if (processName->compare(cachedName) == 0) {
        window = cachedWindow;
    // If process name has changed
    } else {
        if (processName->length() == 0)
            window = root;
        else {
            window = findWindowRecursive(root, processName);

            // If the returned window is root, we didn't find the given window
            if (window == root)
                throw std::invalid_argument("window not found");
        }

        initShm(window);
        cachedName = *processName;
        cachedWindow = window;
    }

    /*  Get display image to shared memory
        If this fails it probably means the window doesn't exist anymore.
        Also seems to fail if the target window is partially outside the screen
        or resized without calling initShm on it
    */
    if (XShmGetImage(display, window, image, 0, 0, 0x00ffffff) == 0) {
        throw std::invalid_argument("window not found");
    }

    // Initialize libjpeg-turbo instance
    tjhandle tjInstance = tjInitCompress();
    if (tjInstance == NULL)
        return 0;

    int subsamp = TJSAMP_420;

    // Allocate a buffer
    ulong bufferLength = tjBufSize(image->width, image->height, subsamp);
    *imageBuffer = new char[bufferLength];

    // Compress image as jpg
    tjCompress2(tjInstance, (const unsigned char*)image->data, image->width, 0,
                image->height, TJPF_BGRX, (unsigned char**)imageBuffer,
                &bufferLength, subsamp, quality, 0);

    tjDestroy(tjInstance);
    return bufferLength;
}

unsigned int moveMouse(long dx, long dy) {
    int s = XTestFakeRelativeMotionEvent(display, dx, dy, CurrentTime);
    XFlush(display);

    std::lock_guard<std::mutex> lock(inputMutex);
    expectedMouseMovement.push_front(std::pair<long, long>(dx, dy));

    return s;
}

unsigned int sendKey(std::string key, bool down, bool userOverride) {
    int s;

    if (userOverride && isUserPressingKeys())
        return 0;

    // Handle mouse buttons separately
    if (key.find("mouse ") == 0) {
        unsigned int button = 1;

        // Mouse buttons
        if (key == "mouse left")
            button = 1;
        else if (key == "mouse right")
            button = 3;
        else if (key == "mouse middle")
            button = 2;

        // Mouse wheel
        else if (key == "mouse up")
            button = 4;
        else if (key == "mouse down")
            button = 5;

        s = XTestFakeButtonEvent(display, button, down, CurrentTime);

    } else {
        unsigned int keySym;
        try {
            keySym = getKeyCode(key);
        } catch (std::out_of_range e) {
            return 0;
        }

        unsigned int keyCode = XKeysymToKeycode(display, (KeySym)keySym);

        s = XTestFakeKeyEvent(display, keyCode, down, CurrentTime);
    }
    XFlush(display);

    // Update expectedKeyDowns/Ups so we can ignore the input event caused
    // by the fake event we send

    // Also update fakeKeysPressed so we can release this key if the user
    // overrides our input
    
    std::lock_guard<std::mutex> lock(inputMutex);

    if (down) {
        expectedKeyDowns.insert(key);
        fakeKeysPressed.insert(key);
    } else {
        expectedKeyUps.insert(key);
        fakeKeysPressed.erase(key);
    }

    return s;
}

/*
    Returns the contents of pressedKeys and removes keys that were released.
*/
std::set<std::string> getKeys() {
    std::lock_guard<std::mutex> lock(inputMutex);

    // Return keys that were held down since last call
    auto keysReturn = std::set<std::string>(pressedKeys);

    // Remove released keys from pressedKeys
    for (auto key : releasedKeys)
        pressedKeys.erase(key);

    releasedKeys.clear();

    return keysReturn;
}

/*
    Returns the value of mouseDelta and resets the value back to (0, 0).
*/
std::pair<long, long> getMouse() {
    std::lock_guard<std::mutex> lock(inputMutex);

    long xd = mouseDelta.first;
    long yd = mouseDelta.second;

    mouseDelta.first = 0;
    mouseDelta.second = 0;

    return std::pair<long, long>(xd, yd);
}

/*
    The thread that contains the X event loop
*/
void eventThread() {
    XEvent event;
    XGenericEventCookie* cookie;
    XIRawEvent* rawEvent;

    while (!stopThread) {
        XNextEvent(eventDisplay, &event);

        // Get the event data
        cookie = &event.xcookie;
        if (XGetEventData(eventDisplay, cookie) && cookie->type == GenericEvent) {
            rawEvent = (XIRawEvent*)cookie->data;

            int keyCode;
            KeySym keySym;
            std::string keyName;
            bool keyDown = false;

            long dx, dy;

            // Handle events
            switch (cookie->evtype) {
                // Mouse movement
                case XI_RawMotion:
                    dx = rawEvent->raw_values[0];
                    dy = rawEvent->raw_values[1];
                    
                    mouseEvent(dx, dy);
                    break;

                // Key press/release
                case XI_RawKeyPress:
                    keyDown = true;
                    
                case XI_RawKeyRelease:
                    keyCode = rawEvent->detail;
                    keySym = XkbKeycodeToKeysym(eventDisplay, keyCode, 0, 0);
                    keyName = getKeyName(keySym);

                    if (keyDown)
                        keyEvent(keyName, true);
                    else
                        keyEvent(keyName, false);
                        
                    break;

                // Mouse button press/release
                case XI_RawButtonPress:
                    keyDown = true;

                case XI_RawButtonRelease:
                    keyCode = rawEvent->detail;
                    if (keyDown) {
                        if (keyCode == 1)
                            keyEvent("mouse left", true);
                        else if (keyCode == 2)
                            keyEvent("mouse middle", true);
                        else if (keyCode == 3)
                            keyEvent("mouse right", true);
                        else if (keyCode == 4)
                            keyEvent("mouse up", true);
                        else if (keyCode == 5)
                            keyEvent("mouse down", true);
                    } else {
                        if (keyCode == 1)
                            keyEvent("mouse left", false);
                        else if (keyCode == 2)
                            keyEvent("mouse middle", false);
                        else if (keyCode == 3)
                            keyEvent("mouse right", false);
                        else if (keyCode == 4)
                            keyEvent("mouse up", false);
                        else if (keyCode == 5)
                            keyEvent("mouse down", false);
                    }
                    break;
            }
        }
        XFreeEventData(eventDisplay, cookie);
    }
    return;
}