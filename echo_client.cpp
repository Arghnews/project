
//
// blocking_udp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

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

template <typename Serializable>
std::deque<Serializable> receive(udp_socket& s) {
    int reply_size = s.available();
    std::deque<Serializable> items;

    if (reply_size > 0) {

        std::vector<char> reply(reply_size);
        udp_endpoint sender_endpoint;

        int reply_length = s.receive_from(
                asio::buffer(reply.data(), reply_size), sender_endpoint);

        std::stringstream ss; // any stream can be used
        ss.write(reply.data(), reply_length); // reply data to stream

        {
            cereal::PortableBinaryInputArchive iarchive(ss); // Create an input archive
            iarchive(items); // Read the data from the archive
        } // flush

    } else {
        ;
    }

    return items;
}

class Sender {
    private:
        io_service& io;
        unsigned short local_port;
        std::string host;
        std::string port;
        udp_socket socket;
        udp_endpoint endpoint;
    public:
        Sender(io_service& io, unsigned short local_port, std::string host, std::string port) :
            io(io),
            host(host),
            port(port),
            local_port(local_port),
            socket(io, udp_endpoint(asio::ip::udp::v4(), local_port)),
            endpoint(*asio::ip::udp::resolver(io).resolve({asio::ip::udp::v4(), host, port}))
            {
            }

        template <typename Serializable_Items>
            void send(Serializable_Items& items) {
                send(Sender::serialize(items));
            }

        template <typename Serializable_Items>
            std::vector<char> static serialize(Serializable_Items& items) {
                std::stringstream ss; // any stream can be used
                {
                    cereal::PortableBinaryOutputArchive oarchive(ss); // Create an output archive
                    oarchive(items); // Write the data to the archive
                } // archive goes out of scope, ensuring all contents are flushed

                const std::string& tmp = ss.str();
                const char* cstr = tmp.c_str();

                return std::vector<char>(cstr, cstr+tmp.size());
            }

        void send(std::vector<char>& data) {
            int request_length = data.size();
            socket.send_to(asio::buffer(data.data(), request_length), endpoint);
        }
};

int main(int argc, char* argv[]) {

    io_service io;
    std::vector<Sender> senders;
    // send from port 2001 to address:port
    senders.emplace_back(Sender(io, 2001, "127.0.0.1", "2000"));

    Forces fs;
    fs.emplace_back(Force(69,v3(69.0f,72.0f,0.0f),Force::Type::Force));
    fs.emplace_back(Force(71,v3(69.0f,72.0f,0.0f),Force::Type::Force));

    auto serial = Sender::serialize(fs);
    for (auto& sender: senders) {
        sender.send(serial);
    }

  return 0;
}

