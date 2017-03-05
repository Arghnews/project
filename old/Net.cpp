#include <iostream>
#include <glm/glm.hpp>
#include "Socket.hpp"
#include "Address.hpp"

int main() {

    std::cout << "Hi!\n";
    glm::vec3 ve;
    std::cout << sizeof(ve) << "\n";

    unsigned short port = 9669;
    Socket s;
    s.open(port);

    /*
    int sent_bytes = 
        sendto( handle, 
                (const char*)packet_data, 
                packet_size,
                0, 
                (sockaddr*)&address, 
                sizeof(sockaddr_in) );

    if ( sent_bytes != packet_size ) {
        printf( "failed to send packet\n" );
        return false;
    }
    */

}

