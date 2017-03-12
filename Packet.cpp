#include "Packet.hpp"

Packet::Packet() {}
Packet::Packet(Packet_Header& header,
        Packet_Payloads& payloads) :
    header(header), payloads(payloads) {
        //std::cout << "Copy constructor called\n";

    }

// takes a packet and a container of packets,
// and appends all the payloads from the packets in the container
// to that of the packet
// should have a way to set limit on number here!
Packet Packet::piggyback(const Packet& p, const Packets& packets) {
    Packet ret = p;
    Packet_Payloads& payloads = ret.payloads;
    for (const auto& packet: packets) {
        concat(payloads, packet.payloads);
    }
    // convert 6,3,4,5 -> 3,4,5,6
    if (payloads.size() > 1) {
        const Packet_Payload payload = payloads.front();
        payloads.pop_front();
        payloads.emplace_back(payload);
    }
    assert(ret.header.sender_id < 8 && "IN PIGGYBACK\n");

    return ret;
}

