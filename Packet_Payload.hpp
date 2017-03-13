#ifndef PACKET_PAYLOAD_HPP
#define PACKET_PAYLOAD_HPP

#include <vector>

#include "Archiver.hpp"
#include "Force.hpp"
#include "Shot.hpp"
#include "Deltas.hpp"

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

    // defined in Deltas.hpp
    std::vector<Mom_Pos> vec_mom_pos;
    std::vector<AngMom_Ori> vec_angmom_ori;

    bool valid() const;

    template<class Archive>
        void serialize(Archive& archive) {
            archive(tick, forces, shots, vec_mom_pos, vec_angmom_ori, type, sequence_number);
            //archive(tick, forces, shots, packet_type);
        }
};

#endif
