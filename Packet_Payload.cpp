#include "Packet_Payload.hpp"

Packet_Payload::Packet_Payload(
        const Packet_Payload::Type& t,
        const Tick& tick
        ) :
    type(t),
    tick(tick)
{
}

Packet_Payload::Packet_Payload() : Packet_Payload(Packet_Payload::Type::Invalid, 0) {}

bool Packet_Payload::valid() const {
    return type != Packet_Payload::Type::Invalid;
}

