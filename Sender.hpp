#ifndef SENDER_HPP
#define SENDER_HPP

#include "Util.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include "Archiver.hpp"
#include "cereal/types/deque.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/portable_binary.hpp"
#define ASIO_STANDALONE
#include "asio.hpp"

class Sender {
    private:
        io_service& io;
        unsigned short local_port;
        std::string host;
        std::string port;
        udp_socket socket;
        udp_endpoint endpoint;
    public:
        Sender(io_service& io, unsigned short local_port, std::string host, std::string port);

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

        void send(std::vector<char>& data);
};

#endif
