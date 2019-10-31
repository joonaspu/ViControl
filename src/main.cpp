#include <iostream>
#include <string>

#include "socket.hpp"
#include "platform.hpp"

#include "messages.pb.h"

#ifdef PROFILING
    #include "profiling.hpp"
#else
    #define START_TIMER(desc)
    #define END_TIMER(desc)
#endif

int main(int argc, char** argv) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::string address = "localhost";
    int port = 12345;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.compare("-a") == 0 || arg.compare("--address") == 0) {
            if ((i + 1) < argc)
                address = argv[i + 1];
        }
        if (arg.compare("-p") == 0 || arg.compare("--port") == 0) {
            if ((i + 1) < argc)
                port = std::stoi(argv[i + 1]);
        }
        if (arg.compare("-h") == 0 || arg.compare("--help") == 0) {
            std::cout << "Usage: [-a ADDRESS] [-p PORT]" << std::endl;
            std::cout << "\t-a, --address \taddress to listen at, "
                      << "default: localhost, "
                      << "set to 0.0.0.0 to allow connections from other machines"
                      << std::endl;
            std::cout << "\t-p, --port \tport to listen at, default: 12345"
                      << std::endl;

            return 0;
        }
    }

    // Initialize platform-specific code
    initialize();

    // Initialize sockets
    initSocket();

    // Create a listen socket
    int listenSocket;
    try {
        listenSocket = createListenSocket(address, port);
    } catch (std::runtime_error e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    // Get the client socket for the first client to connect
    int clientSocket = getClientSocket(listenSocket);
    
    do {
        // Get a request from the client
        Request reqMsg;
        try {
            reqMsg = getRequest(clientSocket);
        } catch (std::runtime_error e) {
            std::cout << e.what() << std::endl;
            shutdownSocket();
            shutdown();
            return 1;
        } catch (std::invalid_argument e) {
            std::cout << e.what() << std::endl;
            continue;
        }

        // Create a response message
        Response respMsg;

        bool userOverride = reqMsg.allow_user_override();

        // Press/release requested keys
        for (int i = 0; i < reqMsg.press_keys_size(); i++)
            sendKey(reqMsg.press_keys(i), true, userOverride);

        for (int i = 0; i < reqMsg.release_keys_size(); i++)
            sendKey(reqMsg.release_keys(i), false, userOverride);

        // Move mouse cursor according to request
        if (!(reqMsg.mouse().x() == 0 && reqMsg.mouse().y() == 0))
            moveMouse(reqMsg.mouse().x(), reqMsg.mouse().y());

        // If client requested an image
        if (reqMsg.get_image()) {
            // Take screenshot
            std::string processName = reqMsg.process_name();

            char* imageBuffer = NULL;

            START_TIMER("getJPGScreenshot");

            try {
                unsigned long imageBytes = getJPGScreenshot(&processName,
                                                            &imageBuffer,
                                                            reqMsg.quality());
                respMsg.set_image(imageBuffer, imageBytes);
            } catch (const std::invalid_argument& e) {
                std::cout << "Exception in getJPGScreenshot: " 
                          << e.what() << std::endl;
            }
            END_TIMER("getJPGScreenshot");

            delete[] imageBuffer;
        }

        // If client requested key states
        if (reqMsg.get_keys()) {
            auto keys = getKeys();
            for (auto key : keys)
                respMsg.add_pressed_keys(key);
        }

        // If client requested mouse position
        if (reqMsg.get_mouse()) {
            auto mouse = getMouse();
            respMsg.mutable_mouse()->set_x(mouse.first);
            respMsg.mutable_mouse()->set_y(mouse.second);
        }

        START_TIMER("sendResponse");

        // Send the response
        int bytesSent = sendResponse(respMsg, clientSocket);

        END_TIMER("sendResponse");
    } while (true);

    // Shut down sockets
    shutdownSocket();

    // Shut down platform-specific code
    shutdown();

    return 0;
}