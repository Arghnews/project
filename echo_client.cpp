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

typedef asio::ip::udp::socket udp_socket;
typedef asio::ip::udp::endpoint udp_endpoint;
typedef std::vector<udp_endpoint> udp_endpoints;

using asio::ip::udp;

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

int main(int argc, char* argv[]) {
  try {

    std::string host = "127.0.0.1";
    std::string port = "2000";

    asio::io_service io_service;

    udp::socket s(io_service, udp::endpoint(udp::v4(), 0));

    udp::resolver resolver(io_service);
    udp::endpoint endpoint = *resolver.resolve({udp::v4(), host, port});
    
    Forces fs;
    fs.emplace_back(Force(69,v3(69.0f,72.0f,0.0f),Force::Type::Force));

    send(endpoint,s,fs);

    usleep(1000000);
    std::deque<Force> items(receive<Force>(s)); // non-blocking receive
    for (auto& i: items) {
        std::cout << i.id << "\n";
    }

  }
  catch (std::exception& e) {
      std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

