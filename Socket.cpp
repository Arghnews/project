#include "Socket.hpp"

Socket::Socket() {
    if (!initialized) {
        initializeSockets();
    }
}

Socket::~Socket() {
    closeSocket();
}

bool Socket::initializeSockets() {
    if (!initialized) {
        auto f = [&] () -> bool {
            #if PLATFORM == PLATFORM_WINDOWS
            WSADATA WsaData;
            return WSAStartup( MAKEWORD(2,2), 
                               &WsaData ) 
                == NO_ERROR;
            #else
            return true;
            #endif
        };
        initialized = f();
    }
    return initialized;
}

void Socket::shutdownSockets() {
    #if PLATFORM == PLATFORM_WINDOWS
    WSACleanup();
    #endif
}

bool Socket::isOpen() const {
    return handle > 0;
}

bool Socket::open(unsigned short port) {
    // create socket
    int handle = socket( AF_INET,
            SOCK_DGRAM,
            IPPROTO_UDP );

    if (handle <= 0) {
        printf( "failed to create socket\n" );
        return false;
    }

    // bind socket
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( (unsigned short) port );

    if ( bind( handle, 
                (const sockaddr*) &address, 
                sizeof(sockaddr_in) ) < 0 ) {
        printf( "failed to bind socket\n" );
        return false;
    }
    std::cout << "Bound to port " << port << "\n";

    // set socket as non blocking
    #if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
    int nonBlocking = 1;
    if ( fcntl( handle, 
                F_SETFL, 
                O_NONBLOCK, 
                nonBlocking ) == -1 ) {
        printf( "failed to set non-blocking\n" );
        return false;
    }

    #elif PLATFORM == PLATFORM_WINDOWS

    DWORD nonBlocking = 1;
    if ( ioctlsocket( handle, 
                      FIONBIO, 
                      &nonBlocking ) != 0 ) {
        printf( "failed to set non-blocking\n" );
        return false;
    }
    #endif

    return true;
}

void Socket::closeSocket() {
    #if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
    close(handle);
    #elif PLATFORM == PLATFORM_WINDOWS
    closesocket(handle);
    #endif
}
