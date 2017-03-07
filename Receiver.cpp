#include "Util.hpp"

#include <sstream>
#include <iostream>
#include <string>
#define ASIO_STANDALONE
#include "asio.hpp"
#include <vector>
#include "Receiver.hpp"

std::stringstream Receiver::read(int reply_size) {
    std::vector<char> reply(reply_size);
    udp_endpoint sender_endpoint;

    int reply_length = socket.receive_from(
            asio::buffer(reply.data(), reply_size), sender_endpoint);

    auto addr = sender_endpoint.address();
    auto port = sender_endpoint.port();
    std::cout << addr << ":" << port << "\n";

    std::stringstream ss; // any stream can be used
    ss.write(reply.data(), reply_length); // reply data to stream
    return ss;
}

Receiver::Receiver(io_service& io, unsigned short port) :
    io(io),
    port(port),
    socket(io, udp_endpoint(asio::ip::udp::v4(), port))
{

}
bool Receiver::available() {
    return socket.available() > 0;
}

/*
int main(int argc, char* argv[]) {
    io_service io;
    Receiver receiver(io,2000);
    while (1) {
        if (receiver.available()) {
            Forces fs;
            fs = receiver.receive<Forces>();
            std::cout << "Server received:\n";
            for (const auto& f: fs) {
                std::cout << "Server reads " << f.id << "\n";
            }
            std::cout << "Server receive end\n";
        }
    }
  return 0;
}
*/
