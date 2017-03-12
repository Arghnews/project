#ifndef PACKET_HPP
#define PACKET_HPP

#include "Util.hpp"
#include "Archiver.hpp"
#include "Packet_Header.hpp"
#include "Packet_Payload.hpp"

struct Packet {
    Packet_Header header;
    Packet_Payloads payloads; // a packet may have multiple payloads
    Packet();
    Packet(Packet_Header& header,
            Packet_Payloads& payloads);
    template<class Archive>
        void serialize(Archive& archive) {
            archive(header, payloads);       
        }

    // takes a packet and a container of packets,
    // and appends all the payloads from the packets in the container
    // to that of the packet
    // should have a way to set limit on number here!
    Packet static piggyback(const Packet& p, const Packets& packets);
};

#endif
