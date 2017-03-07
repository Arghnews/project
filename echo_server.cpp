
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <sstream>
#include <string>
#include <cereal/types/deque.hpp>
#include "cereal/archives/portable_binary.hpp"
#include "Util.hpp"
#include "Force.hpp"
#include "Archiver.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#define ASIO_STANDALONE
#include "asio.hpp"
#include <unistd.h>
#include <vector>

class Receiver {
    private:
        io_service& io;
        unsigned short port;
        udp_socket socket;
        std::stringstream read(int reply_size) {
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

    public:
        Receiver(io_service& io, unsigned short port) :
            io(io),
            port(port),
            socket(io, udp_endpoint(asio::ip::udp::v4(), port))
    {

    }
        bool available() {
            return socket.available() > 0;
        }
        template <typename Serializable_Items>
            Serializable_Items static deserialize(std::stringstream& ss) {
                Serializable_Items items;
                {
                    cereal::PortableBinaryInputArchive iarchive(ss); // Create an input archive
                    iarchive(items); // Read the data from the archive
                } // flush
                return items;
            }

        template <typename Serializable_Items>
            Serializable_Items receive() {
                Serializable_Items items;
                int reply_size = socket.available();
                //std::deque<Serializable> items;

                if (reply_size > 0) {

                    std::stringstream ss = read(reply_size);

                    items = deserialize<Serializable_Items>(ss);

                } else {
                    ;
                }

                return items;
            }

};

int main(int argc, char* argv[]) {
    io_service io;
    Receiver receiver(io,2000);
    usleep(5 * 1000 * 1000);
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

