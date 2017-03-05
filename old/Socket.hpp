#ifndef SOCKET_HPP
#define SOCKET_HPP

// platform detection

#define PLATFORM_WINDOWS  1
#define PLATFORM_MAC      2
#define PLATFORM_UNIX     3

#if defined(_WIN32)
    #define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
    #define PLATFORM PLATFORM_MAC
#else
    #define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_WINDOWS
    #include <winsock2.h>

#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <unistd.h> // to close socket/fd

#endif

#if PLATFORM == PLATFORM_WINDOWS
    #pragma comment( lib, "wsock32.lib" )
#endif

#include "Address.hpp"
#include <iostream>

static bool initialized = false;

class Socket {
    private:
        int handle;
    public:
        bool static initializeSockets();
        void static shutdownSockets();
        bool open(unsigned short port);
        bool isOpen() const;
        void closeSocket();
        Socket();
        ~Socket();
};

#endif
