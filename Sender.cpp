#include "Sender.hpp"

#include <string>
#include <vector>

#define ASIO_STANDALONE
#include "asio.hpp"

Sender::Sender(io_service& io, unsigned short local_port, std::string host, std::string port) :
    io(io),
    host(host),
    port(port),
    local_port(local_port),
    socket(io, udp_endpoint(asio::ip::udp::v4(), local_port)),
    endpoint(*asio::ip::udp::resolver(io).resolve({asio::ip::udp::v4(), host, port}))
{
}

void Sender::send(std::vector<char>& data) {
    int request_length = data.size();
    socket.send_to(asio::buffer(data.data(), request_length), endpoint);
}
