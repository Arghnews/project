#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include "Util.hpp"

#include <sstream>
#include <iostream>
#include <string>
#include <memory>

#include "Compress.hpp"
#include "Archiver.hpp"

class Receiver {
    private:
        io_service& io;
        unsigned short port;
        std::shared_ptr<udp_socket> socket;
        void read(std::stringstream& ss, int reply_size);

    public:
        Receiver(io_service& io, const std::shared_ptr<udp_socket>& socket);

        bool available();

        template <typename Serializable_Items>
            Serializable_Items static deserialize(std::stringstream& ss) {
                const std::string decomp(decompress_string(ss.str()));
                std::stringstream sss;
                sss.str(decomp);
                Serializable_Items items;
                {
                    cereal::PortableBinaryInputArchive iarchive(sss); // Create an input archive
                    iarchive(items); // Read the data from the archive
                } // flush
                return items;
            }

        template <typename Serializable_Items>
            Serializable_Items receive() {
                std::cout << "Receive called\n";
                Serializable_Items items;
                int reply_size = socket->available();

                if (reply_size > 0) {

                    std::stringstream ss;
                    read(ss,reply_size);

                    items = deserialize<Serializable_Items>(ss);

                } else {
                    ;
                }

                std::cout << "Receive end\n";
                return items;
            }

};

#endif
