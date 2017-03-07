#define ASIO_STANDALONE

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
#include "asio.hpp"
#include <unistd.h>
#include <vector>

typedef asio::io_service io_service;
typedef asio::ip::udp::socket udp_socket;
typedef asio::ip::udp::endpoint udp_endpoint;
typedef std::vector<udp_endpoint> udp_endpoints;

template <typename Serializable>
void send(const udp_endpoint& endpoint, udp_socket& s, const std::deque<Serializable>& items) {
    send(udp_endpoints{endpoint},s,items);
}

// serializes and sends data to udp endpoint (eg. server)
template <typename Serializable>
void send(const udp_endpoints& udp_endpoints, udp_socket& s, const std::deque<Serializable>& items) {
    std::stringstream ss; // any stream can be used

    {
        cereal::PortableBinaryOutputArchive oarchive(ss); // Create an output archive
        oarchive(items); // Write the data to the archive
    } // archive goes out of scope, ensuring all contents are flushed

    const std::string& tmp = ss.str();
    const char* cstr = tmp.c_str();

    const std::vector<char> request(cstr, cstr+tmp.size());
    int request_length = request.size();
    for (auto& endpoint: udp_endpoints) {
        s.send_to(asio::buffer(request.data(), request_length), endpoint);
    }
}

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

using asio::ip::udp;

class Sender {
    io_service& io;
    std::string host;
    std::string port;
    udp_socket socket;
    udp_endpoint endpoint;
    public:
    Sender(io_service& io, std::string host, std::string port) :
        io(io),
        host(host),
        port(port),
        socket(io, udp_endpoint(udp::v4(), 0)),
        endpoint(*udp::resolver(io).resolve({udp::v4(), host, port}))
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
  try {

    io_service io;
    std::vector<Sender> senders;
    senders.emplace_back(Sender(io, "127.0.0.1", "2000"));

    Forces fs;
    fs.emplace_back(Force(69,v3(69.0f,72.0f,0.0f),Force::Type::Force));
    fs.emplace_back(Force(69,v3(69.0f,72.0f,0.0f),Force::Type::Force));

    auto serial = Sender::serialize(fs);
    for (auto& sender: senders) {
        sender.send(serial);
    }

    //sender.send(fs);
    /*
    std::string host = "127.0.0.1";
    std::string port = "2000";

    udp::socket s(io_service, udp::endpoint(udp::v4(), 0));

    udp::resolver resolver(io_service);
    udp::endpoint endpoint = *resolver.resolve({udp::v4(), host, port});
    
    Forces fs;
    fs.emplace_back(Force(69,v3(69.0f,72.0f,0.0f),Force::Type::Force));

    send(endpoint,s,fs);

    //usleep(1000000);
    std::deque<Force> items(receive<Force>(s)); // non-blocking receive
    for (auto& i: items) {
        std::cout << i.id << "\n";
    }
    */

  }
  catch (std::exception& e) {
      std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

