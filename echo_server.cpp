#define ASIO_STANDALONE


//
// async_udp_echo_Server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include "asio.hpp"
#include <unistd.h>
#include "Archiver.hpp"
#include "Force.hpp"

using asio::ip::udp;

class Server {
    public:
        Server(asio::io_service& io_service, unsigned short port)
            : socket_(io_service, udp::endpoint(udp::v4(), port)) {
                std::cout << "Server constructed\n";
            }

        void do_receive() {
            socket_.async_receive_from(
                    asio::buffer(data_, max_length),
                    sender_endpoint_,
                    [this](std::error_code ec, std::size_t bytes_recvd)
                    {
                        if (!ec && bytes_recvd > 0) {
                            std::cout << "Server sending back start\n";
                            std::cout << "Server Received message of size " << bytes_recvd << "\n";
                            do_send(bytes_recvd);
                            std::cout << "Server sending back end\n";
                        } else {
                            std::cout << "Server received nothing\n";
                        }
                    });
        }

        void do_send(std::size_t length) {
            socket_.async_send_to(
                    asio::buffer(data_, length),
                    sender_endpoint_,
                    [this](std::error_code /*ec*/, std::size_t bs/*bytes_sent*/)
                    {
                        std::cout << "Server do_send size " << " and " << bs << "\n";
                    });
        }

    private:
        udp::socket socket_;
        udp::endpoint sender_endpoint_;
        enum { max_length = 65000 };
        unsigned char data_[max_length];
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: async_udp_echo_Server <port>\n";
            return 1;
        }

        asio::io_service io_service;

        Server s(io_service, std::atoi(argv[1]));

        std::cout << "Ioservice run\n";
        s.do_receive();
        io_service.run();
        io_service.reset();
        std::cout << "Ioservice run done\n";

        /*
        std::cout << "Ioservice run\n";
        s.do_send(10);
        io_service.run();
        io_service.reset();
        std::cout << "Ioservice run done\n";
        */

    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

