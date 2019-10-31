#include <iostream>
#include <string>
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <netdb.h>
#endif

#include "messages.pb.h"

#ifdef PROFILING
    #include "profiling.hpp"
#else
    #define START_TIMER(desc)
    #define END_TIMER(desc)
#endif

using namespace google::protobuf;

int initSocket() {
    #ifdef _WIN32
        WSADATA wsa_data;
        return WSAStartup(MAKEWORD(2,2), &wsa_data);
    #else
        return 0;
    #endif

}

int shutdownSocket() {
    #ifdef _WIN32
        return WSACleanup();
    #else
        return 0;
    #endif
}

int createListenSocket(std::string address, int port) {
    struct addrinfo *result = NULL, *ptr = NULL, hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hints, &result);

    // Create socket
    int listenSocket = socket(result->ai_family, result->ai_socktype,
                              result->ai_protocol);
    if (listenSocket < 0) {
        throw std::runtime_error("Socket creation failed");
    }

    // Bind address to socket
    bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    // Listen on socket
    listen(listenSocket, SOMAXCONN);

    return listenSocket;
}

int getClientSocket(int listenSocket) {
    int socket = accept(listenSocket, NULL, NULL);
    char value = 1;
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
    return socket;
}

Request getRequest(int clientSocket) {
    int bytesReceived = 0;

    // Receive message length
    uint32_t netLen;
    while (bytesReceived < sizeof(netLen)) {
        int received = recv(clientSocket, (char*)((&netLen) + bytesReceived),
                            sizeof(netLen) - bytesReceived, 0);

        if (received == 0)
            throw std::runtime_error("Socket was closed");
        else if (received < 0)
            throw std::runtime_error("Socket error");

        bytesReceived += received;
    }
    int msgLen = ntohl(netLen);

    char* clientInput = new char[msgLen];

    START_TIMER("Recv message contents");

    // Receive message contents
    bytesReceived = 0;
    while (bytesReceived < msgLen) {
        int received = recv(clientSocket, clientInput + bytesReceived,
                            msgLen - bytesReceived, 0);

        if (received == 0)
            throw std::runtime_error("Socket was closed");
        else if (received < 0)
            throw std::runtime_error("Socket error");

        bytesReceived += received;
    }

    END_TIMER("Recv message contents");

    Request reqMsg;
    if (!reqMsg.ParseFromArray(clientInput, msgLen))
        throw std::invalid_argument("Could not parse received bytes");

    delete[] clientInput;

    return reqMsg;
}

int sendResponse(Response respMsg, int clientSocket) {
    int msgLen = respMsg.ByteSize();
    char* responseBuffer = new char[msgLen];
    respMsg.SerializeWithCachedSizesToArray((uint8*)responseBuffer);
    
    int sentBytes = 0;

    // Send message length
    uint32_t netLen = htonl(msgLen);
    while (sentBytes < sizeof(netLen)) {
        int sent = send(clientSocket, (char*)((&netLen) + sentBytes), 
                        sizeof(netLen) - sentBytes, 0);

        if (sent < 0)
            throw std::runtime_error("Socket error");

        sentBytes += sent;
    }

    // Send message contents
    sentBytes = 0;
    while (sentBytes < msgLen) {
        int sent = send(clientSocket, responseBuffer + sentBytes,
                        msgLen - sentBytes, 0);

        if (sent < 0)
            throw std::runtime_error("Socket error");

        sentBytes += sent;
    }
    
    delete[] responseBuffer;
    return sentBytes;
}