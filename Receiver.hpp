#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include "Util.hpp"

#include <sstream>
#include <iostream>
#include <string>

#include "Archiver.hpp"
#include "cereal/types/deque.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/portable_binary.hpp"

class Receiver {
    private:
        io_service& io;
        unsigned short port;
        udp_socket socket;
        void read(std::stringstream& ss, int reply_size);

    public:
        Receiver(io_service& io, unsigned short port);

        bool available();

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

                if (reply_size > 0) {

                    std::stringstream ss;
                    read(ss,reply_size);

                    items = deserialize<Serializable_Items>(ss);

                } else {
                    ;
                }

                return items;
            }

};


#endif
