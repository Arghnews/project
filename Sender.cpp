#include "Sender.hpp"

#include <string>
#include <vector>
#include <memory>
#include "Compress.hpp"

#define ASIO_STANDALONE
#include "asio.hpp"

Sender::Sender(io_service& io, const std::shared_ptr<udp_socket>& socket, std::string host, std::string port) :
    io(io),
    host(host),
    port(port),
    socket(socket),
    endpoint(*asio::ip::udp::resolver(io).resolve({asio::ip::udp::v4(), host, port}))
{
}

void Sender::send(std::vector<char>& data) {
    int request_length = data.size();
    socket->send_to(asio::buffer(data.data(), request_length), endpoint);
}
