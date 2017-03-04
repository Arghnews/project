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
#include <iostream>
#include <sstream>
#include <string>
#include <cereal/types/vector.hpp>
#include "cereal/archives/portable_binary.hpp"

// Portable binary checks endianness etc, for small overhead

namespace cereal {
    template<class Archive>
        void serialize(Archive& archive, glm::vec3& v) {
            archive(v.x, v.y, v.z);
        }
}

struct Classy {
   int x;
   glm::vec3 y;
   std::vector<glm::vec3> vec;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(x, y, vec);
    }
    friend class cereal::access;

};

#include <cstdlib>
#include <cstring>
#include <iostream>
#include "asio.hpp"

using asio::ip::udp;

enum { max_length = 1024 };

int main(int argc, char* argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: blocking_udp_echo_client <host> <port>\n";
      return 1;
    }

    asio::io_service io_service;

    udp::socket s(io_service, udp::endpoint(udp::v4(), 0));

    udp::resolver resolver(io_service);
    udp::endpoint endpoint = *resolver.resolve({udp::v4(), argv[1], argv[2]});


    std::stringstream ss; // any stream can be used

    {
        cereal::PortableBinaryOutputArchive oarchive(ss); // Create an output archive
        Classy c1_in;
        c1_in.x = 0.0f;
        c1_in.y.x = 71.0f;
        c1_in.vec.push_back(glm::vec3(97.0f));
        oarchive(c1_in); // Write the data to the archive
    } // archive goes out of scope, ensuring all contents are flushed


    std::cout << "Enter message: ";
    //char request[max_length];
    //std::cin.getline(request, max_length);
    //size_t request_length = std::strlen(request);
    //std::vector<char> request = {'a','b'};
    const std::string& tmp = ss.str();
    const char* cstr = tmp.c_str();
    std::vector<char> request(cstr, cstr+tmp.size());
    size_t request_length = request.size();
    std::cout << "Size of req back " << request_length << "\n";
    s.send_to(asio::buffer(request.data(), request_length), endpoint);

    char reply[max_length];
    udp::endpoint sender_endpoint;
    size_t reply_length = s.receive_from(
        asio::buffer(reply, max_length), sender_endpoint);


    std::stringstream ss_out; // any stream can be used
    /*
    std::cout << "Reply is: \n";
    std::cout.write(reply, reply_length);
    std::cout << "\n";
    */
    for (const auto& c: reply) {
        ss_out << c;
    }

    // insert networked magic here
    Classy c2_in;
    {
        cereal::PortableBinaryInputArchive iarchive(ss_out); // Create an input archive
        iarchive(c2_in); // Read the data from the archive
    }

    std::cout << c2_in.y.x << "\n";
    for (const auto& v: c2_in.vec) {
        std::cout << "vec " << v.x << "," << v.y << "," << v.z << "\n";
    }

  }
  catch (std::exception& e) {
      std::cerr << "Exception: " << e.what() << "\n";
  }


  return 0;
}
