#include <iostream>
#include <string>
#include <set>
#include <thread>

#include <ApplicationServices/ApplicationServices.h>

#include "platform.hpp"
#include "keys.hpp"

CGWindowID cachedWindow = kCGNullWindowID;
std::string cachedName;

void initialize() {
    std::thread(eventThread).detach();
    return;
}

void shutdown() {
    return;
}

bool findWindow(std::string* processName, CGWindowID* window) {
    // Get list of windows
    CFArrayRef list = CGWindowListCopyWindowInfo(kCGWindowListOptionAll,
                                                kCGNullWindowID);

    // Loop through every window
    CFIndex count = CFArrayGetCount(list);
    for (CFIndex i = 0; i < count; i++) {
        CFDictionaryRef dict = (CFDictionaryRef)CFArrayGetValueAtIndex(list, i);

        // Get window name
        // TODO: May fail if there are non-ASCII characters in the name
        CFStringRef name = (CFStringRef)CFDictionaryGetValue(dict, kCGWindowName);
        const char* cstr = CFStringGetCStringPtr(name, kCFStringEncodingUTF8);

        // Check if name was found
        if (cstr != NULL) {
            std::string string (cstr);

            // Check if name matches
            if (string.find(*processName) != std::string::npos) {
                CFNumberRef number = (CFNumberRef)CFDictionaryGetValue(
                    dict, kCGWindowNumber
                );
                CFNumberGetValue(number, kCFNumberSInt32Type, window);
                CFRelease(list);
                CFRelease(number);
                return true;
            }
        }
    }
    CFRelease(list);
    return false;
}

unsigned long getJPGScreenshot(std::string* processName, char** imageBuffer,
                               unsigned int quality) {
    CGWindowID window = kCGNullWindowID;
    CGImageRef image;

    if (processName->length() > 0) {
        if (*processName == cachedName) {
            window = cachedWindow;
        } else {
            findWindow(processName, &window);
            cachedWindow = window;
            cachedName = *processName;
        }
    }

    // Capture image
    if (window == kCGNullWindowID) {
        image = CGWindowListCreateImage(
            CGRectNull, kCGWindowListOptionAll, window,
            kCGWindowImageBoundsIgnoreFraming
        );
    } else {
        image = CGWindowListCreateImage(
            CGRectNull, kCGWindowListOptionIncludingWindow, window,
            kCGWindowImageBoundsIgnoreFraming
        );
    }
    
    // Save image to a data object
    CFStringRef type = CFSTR("public.jpeg");
    CFMutableDataRef data = CFDataCreateMutable(NULL, 0);
    CGImageDestinationRef dest = CGImageDestinationCreateWithData(
        data, type, 1, NULL
    );

    // Set quality
    double qualityDouble = 0.01 * quality;
    CFMutableDictionaryRef props = CFDictionaryCreateMutable(NULL, 1, NULL,
                                                             NULL);
    CFNumberRef qualityNumber = CFNumberCreate(NULL, kCFNumberDoubleType,
                                               &qualityDouble);
    CFDictionarySetValue(props, kCGImageDestinationLossyCompressionQuality,
                         qualityNumber);

    CGImageDestinationAddImage(dest, image, props);
    CGImageDestinationFinalize(dest);
    CGImageRelease(image);
    CFRelease(dest);
    CFRelease(qualityNumber);
    CFRelease(props);

    // Copy to a new buffer
    unsigned long bufferLength = CFDataGetLength(data);
    *imageBuffer = new char[bufferLength];
    CFDataGetBytes(data, CFRangeMake(0, bufferLength),
                   (uint8*)*imageBuffer);
    CFRelease(data);

    return bufferLength;
}

unsigned int moveMouse(long dx, long dy) {
    // Get current position
    CGEventRef posEvent =  CGEventCreate(NULL);
    CGPoint position = CGEventGetLocation(posEvent);

    CFRelease(posEvent);

    // Change position
    position.x += dx;
    position.y += dy;

    // Set new position
    CGEventSourceRef eventSrc = CGEventSourceCreate(
        kCGEventSourceStateHIDSystemState
    );

    CGEventRef event = CGEventCreateMouseEvent(eventSrc, kCGEventMouseMoved,
                                               position, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, event);

    CFRelease(event);
    CFRelease(eventSrc);
    return 0;
}

unsigned int sendKey(std::string key, bool down, bool userOverride) {
    CGEventSourceRef eventSrc;
    CGEventRef event;

    // Handle mouse buttons separately
    if (key.find("mouse ") == 0) {
        // Scroll wheel
        if (key == "mouse up" || key == "mouse down") {
            uint32_t scrollDist = 0;
            if (key == "mouse up")
                scrollDist = -1;
            else if (key == "mouse down")
                scrollDist = 1;

            eventSrc = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

            event = CGEventCreateScrollWheelEvent(
                eventSrc, kCGScrollEventUnitLine, 1, scrollDist
            );
            CGEventPost(kCGHIDEventTap, event);
        } else {
            // Get current position
            CGEventRef posEvent =  CGEventCreate(NULL);
            CGPoint position = CGEventGetLocation(posEvent);

            CFRelease(posEvent);

            CGEventType eventType = kCGEventNull;
            CGMouseButton mouseButton = kCGMouseButtonLeft;

            // Mouse buttons
            if (key == "mouse left" && down)
                eventType = kCGEventLeftMouseDown;
            else if (key == "mouse left" && !down)
                eventType = kCGEventLeftMouseUp;
            else if (key == "mouse right" && down)
                eventType = kCGEventRightMouseDown;
            else if (key == "mouse right" && !down)
                eventType = kCGEventRightMouseUp;
            else if (key == "mouse middle" && down) {
                eventType = kCGEventOtherMouseDown;
                mouseButton = kCGMouseButtonCenter;
            } else if (key == "mouse middle" && !down) {
                eventType = kCGEventOtherMouseUp;
                mouseButton = kCGMouseButtonCenter;
            }

            // Post event
            eventSrc = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

            event = CGEventCreateMouseEvent(
                eventSrc, eventType, position, mouseButton
            );
            CGEventPost(kCGHIDEventTap, event);
        }

    } else {
        eventSrc = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

        CGKeyCode keyCode = getKeyCode(key);

        event = CGEventCreateKeyboardEvent(eventSrc, keyCode, down);
        CGEventPost(kCGHIDEventTap, event);
    }

    CFRelease(event);
    CFRelease(eventSrc);

    // Update expectedKeyDowns/Ups so we can ignore the input event caused
    // by the SendInput

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

    return 0;
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
    std::lock_guard<std::mutex> lock(inputMutex);

    long xd = mouseDelta.first;
    long yd = mouseDelta.second;

    mouseDelta.first = 0;
    mouseDelta.second = 0;

    return std::pair<long, long>(xd, yd);
}

/*
    Callback for new input events
    https://developer.apple.com/documentation/coregraphics/cgeventtapcallback?language=objc
*/
CGEventRef eventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *userInfo) {
    int dx = 0;
    int dy = 0;
    int keycode = 0;
    int scrollDelta = 0;
    std::string keyname = "";
    CGEventFlags flags;

    switch (type) {
        // Mouse buttons
        case kCGEventLeftMouseDown:
            keyEvent("mouse left", true);
            break;
        case kCGEventLeftMouseUp:
            keyEvent("mouse left", false);
            break;
        case kCGEventRightMouseDown:
            keyEvent("mouse right", true);
            break;
        case kCGEventRightMouseUp:
            keyEvent("mouse right", false);
            break;
        case kCGEventOtherMouseDown:
            if (CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber)
                == kCGMouseButtonCenter)
            {
                keyEvent("mouse middle", true);
            }
            break;
        case kCGEventOtherMouseUp:
            if (CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber)
                == kCGMouseButtonCenter)
            {
                keyEvent("mouse middle", false);
            }
            break;

        // Mouse moved
        case kCGEventLeftMouseDragged:
        case kCGEventRightMouseDragged:
        case kCGEventOtherMouseDragged:
        case kCGEventMouseMoved:
            dx = CGEventGetIntegerValueField(event, kCGMouseEventDeltaX);
            dy = CGEventGetIntegerValueField(event, kCGMouseEventDeltaY);

            mouseEvent(dx, dy);
            break;

        // Keyboard keys
        case kCGEventKeyDown:
            keycode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            keyname = getKeyName(keycode);
            
            keyEvent(keyname, true);
            break;
        case kCGEventKeyUp:
            keycode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            keyname = getKeyName(keycode);

            keyEvent(keyname, false);
            break;
        case kCGEventFlagsChanged:
            // All changes in modifier keys get bunched into this event
            // We have to check the state of each modifier key in case it changed

            flags = CGEventGetFlags(event);

            if ((flags & kCGEventFlagMaskAlphaShift) == kCGEventFlagMaskAlphaShift)
                keyEvent("caps lock", true);
            else
                keyEvent("caps lock", false);

            if ((flags & kCGEventFlagMaskShift) == kCGEventFlagMaskShift)
                keyEvent("shift", true);
            else
                keyEvent("shift", false);

            if ((flags & kCGEventFlagMaskControl) == kCGEventFlagMaskControl)
                keyEvent("ctrl", true);
            else
                keyEvent("ctrl", false);

            if ((flags & kCGEventFlagMaskAlternate) == kCGEventFlagMaskAlternate)
                keyEvent("alt", true);
            else
                keyEvent("alt", false);

            // "left windows" is probably the closest match to the Cmd key
            if ((flags & kCGEventFlagMaskCommand) == kCGEventFlagMaskCommand)
                keyEvent("left windows", true);
            else
                keyEvent("left windows", false);

            break;

        // Sroll wheel
        case kCGEventScrollWheel:
            scrollDelta = CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1);

            if (scrollDelta > 0) {
                keyEvent("mouse up", true);
                keyEvent("mouse up", false);
            } else if (scrollDelta < 0) {
                keyEvent("mouse down", true);
                keyEvent("mouse down", false);
            }
            break;

        default:
            break;
    }
    //std::cout << "New event" << std::endl;
    return event;
}

void eventThread() {
    std::cout << "Starting event thread" << std::endl;

    // We are interested in all mouse and keyboard events
    CGEventMask eventMask = CGEventMaskBit(kCGEventLeftMouseDown) |
                            CGEventMaskBit(kCGEventLeftMouseUp) |
                            CGEventMaskBit(kCGEventRightMouseDown) |
                            CGEventMaskBit(kCGEventRightMouseUp) |
                            CGEventMaskBit(kCGEventOtherMouseDown) |
                            CGEventMaskBit(kCGEventOtherMouseUp) |
                            CGEventMaskBit(kCGEventMouseMoved) |
                            CGEventMaskBit(kCGEventLeftMouseDragged) |
                            CGEventMaskBit(kCGEventRightMouseDragged) |
                            CGEventMaskBit(kCGEventOtherMouseDragged) |
                            CGEventMaskBit(kCGEventKeyDown) |
                            CGEventMaskBit(kCGEventKeyUp) |
                            CGEventMaskBit(kCGEventFlagsChanged) |
                            CGEventMaskBit(kCGEventScrollWheel);

    // Create an event tap
    CFMachPortRef eventTap =
        CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, 
                         kCGEventTapOptionListenOnly, eventMask, eventCallback,
                         nullptr);

    // Create a run loop source and add it to the current run loop
    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);

    // Start the run loop
    std::cout << "RunLoopRun" << std::endl;
    CFRunLoopRun();
}