#include "Packet_Header.hpp"

// returns true when s1 is a more recent Sequence number than s2
// where Seq numbers are like ids that go up
bool Packet_Header::sequence_more_recent(
        const Seq& s1,
        const Seq& s2,
        const Seq& max) {
    return (s1 > s2) && (s1 - s2 <= max/2) ||
        (s2 > s1) && (s2 - s1 > max/2);
}

//Packet_Header() : Packet_Header(1, 0, short_max) {}
Packet_Header::Packet_Header(
        const Seq& sequence_number,
        const Seqs& received_seqs,
        const Instance_Id sender_id
        ) :
    sequence_number(sequence_number),
    received_seqs(received_seqs),
    sender_id(sender_id) {
    }
Packet_Header::Packet_Header() {}
