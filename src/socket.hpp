#include <string>
#ifdef _WIN32
    #include <winsock2.h>
#endif
#ifdef __linux__
    #include <sys/types.h>
    #include <sys/socket.h>
#endif
#include "messages.pb.h"

/*
    Initializes the socket API (if needed). Should be called before using
    any of the other functions.

    Returns zero if initialization was successful.
 */
int initSocket();

/*
    Shuts down the socket API (if needed).

    Returns zero if shutdown was successful.
 */
int shutdownSocket();

/*
    Creates and returns a listen socket on the given address and port.
    Throws runtime_error if the socket creation failed.
*/
int createListenSocket(std::string address, int port);

/*
    Accepts the first incoming connection
    to the given listen socket and
    returns the new client socket.
*/
int getClientSocket(int listenSocket);

/*
    Waits for and parses incoming requests to the given socket.
    Attempts to parse the message as a protobuf Request message
    and if successful, returns it.

    Throws runtime_error if the connection was closed or there was
    a socket error.
    Throws invalid_argument if the received bytes could not be parsed
    as a protobuf message.
*/
Request getRequest(int clientSocket);

/*
    Sends the given protobuf Response message to the given socket.
    Returns the number of bytes sent to the socket.
*/
int sendResponse(Response respMsg, int clientSocket);