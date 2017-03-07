#include "Util.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include "cereal/types/deque.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <sstream>
#include <string>
#include <memory>

#include "Force.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"

int main() {

    io_service io;

    unsigned short port = 2000;

    auto socket_ptr = std::make_shared<udp_socket>(io, udp_endpoint(asio::ip::udp::v4(), port));

    Receiver receiver(io,socket_ptr);

    std::vector<Sender> senders;
    // send from port 2001 to address:port
    senders.emplace_back(Sender(io, socket_ptr, "127.0.0.1", "2000"));

    Forces fs;
    fs.emplace_back(Force(69,v3(69.0f,72.0f,0.0f),Force::Type::Force));
    fs.emplace_back(Force(71,v3(69.0f,72.0f,0.0f),Force::Type::Force));

    auto serial = Sender::serialize(fs);
    for (auto& sender: senders) {
        sender.send(serial);
    }

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
}
