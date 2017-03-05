#include "Address.hpp"

Address::Address() : Address(0, 0) {}

Address::Address( unsigned char a, 
        unsigned char b, 
        unsigned char c, 
        unsigned char d, 
        unsigned short port ) :
    Address(createAddress(a,b,c,d), port) {}

    Address::Address(unsigned int address, unsigned short port) :
        address(address), port(port) {
        }

unsigned int Address::getAddress() const {
    return address;   
}

unsigned short Address::getPort() const {
    return port;
}

unsigned int Address::createAddress(
        unsigned char a, 
        unsigned char b, 
        unsigned char c, 
        unsigned char d) {
    return ( a << 24 ) | 
        ( b << 16 ) |
        ( c << 8  ) |
        d;
}
