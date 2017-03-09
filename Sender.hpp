#ifndef SENDER_HPP
#define SENDER_HPP

#include "Util.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>

#include "Archiver.hpp"
#define ASIO_STANDALONE
#include "asio.hpp"
#include "Compress.hpp"

class Sender {

    private:
        io_service& io;
        unsigned short local_port;
        std::shared_ptr<udp_socket> socket;
        udp_endpoint endpoint;

    public:
        std::string host;
        std::string port;

        Sender(io_service& io, const std::shared_ptr<udp_socket>& socket, std::string host, std::string port);

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

                const std::string tmp(ss.str());
                const std::string compressed = compress_string(tmp, 1);
                const char* cstr = compressed.c_str();
                //std::cout << "Compression ratio " << (float)compressed.size()/(float)tmp.size() << " orig size " << tmp.size() << "\n";

                return std::vector<char>(cstr, cstr+compressed.size());
            }

        void send(std::vector<char>& data);
};

#endif
