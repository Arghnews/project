#ifndef PACKET_PAYLOAD_HPP
#define PACKET_PAYLOAD_HPP

#include <vector>

#include "Archiver.hpp"
#include "Force.hpp"
#include "Shot.hpp"

#include "Util.hpp"

struct Packet_Payload {

    enum class Type : uint8_t {
        Invalid, Input, State
    };
    Packet_Payload(
            const Packet_Payload::Type& t,
            const Tick& tick
            );
    Packet_Payload();

    Seq sequence_number;

    Tick tick; // initially tied to Seq number
    // application level number, so can tell at what tick this was sent on
    // can tell if too old or not etc // think I need this? maybe not really

    Type type;

    Forces forces;
    Shots shots;

    std::vector<Id_v3> positions;
    std::vector<Id_fq> orients;
    std::vector<Id_v3> momentums;
    std::vector<Id_v3> ang_momentums;

    bool valid() const;

    template<class Archive>
        void serialize(Archive& archive) {
            archive(tick, forces, shots, positions, orients, momentums, ang_momentums, type, sequence_number);
            //archive(tick, forces, shots, packet_type);
        }
};

#endif
