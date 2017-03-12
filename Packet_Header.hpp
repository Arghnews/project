#ifndef PACKET_HEADER_HPP
#define PACKET_HEADER_HPP

#include "Util.hpp"

struct Packet_Header {
    // header
    Seq sequence_number;
    // DON'T PUT SENDER_ID BELOW RECEIVE_SEQS
    Instance_Id sender_id; // use last two digits of port
    // DON'T PUT SENDER_ID BELOW RECEIVE_SEQS
    // cannot properly read it after serialisation for some reason
    Seqs received_seqs; // corresponds to prior 32
    // packets
    template<class Archive>
        void serialize(Archive& archive) {
            archive(sequence_number, sender_id, received_seqs);
        }

    // returns true when s1 is a more recent Sequence number than s2
    // where Seq numbers are like ids that go up
    bool static sequence_more_recent(const Seq& s1, const Seq& s2,
            const Seq& max=-1);

    //Packet_Header() : Packet_Header(1, 0, short_max) {}
    Packet_Header(
            const Seq& sequence_number,
            const Seqs& received_seqs,
            const Instance_Id sender_id
            );
    Packet_Header();
};


#endif
