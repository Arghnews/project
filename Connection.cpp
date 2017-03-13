#include "Connection.hpp"

Instance_Id Connection::instance_id() const {
    return instance_id_;
}

Connection::Connection(io_service& io,
        const Connection_Address& connection_address,
        Instance_Id instance_id,
        int received_seqs_lim) :
    Connection(io,
            connection_address.local_port,
            connection_address.remote_host,
            connection_address.remote_port,
            instance_id,
            received_seqs_lim) {}
    Connection::Connection(io_service& io,
            std::string local_port, // port to listen on
            std::string host, // host to connect to to send stuff to
            std::string port, // port to connect to
            Instance_Id instance_id,
            int received_seqs_lim,
            long fake_delay_us) :
        socket_ptr(std::make_shared<udp_socket>(io, udp_endpoint(asio::ip::udp::v4(),std::stoi(local_port)))),
        receiver(io,socket_ptr),
        sender(io,socket_ptr,host,port),
        sequence_number(0),
        received_seqs_lim(received_seqs_lim),
        received(-1),
        tick(0),
        instance_id_(instance_id),
        fake_delay_us(fake_delay_us)
{

}

void Connection::toggle_fake_delay_us(const long& delay_us) {
    if (fake_delay_us == 0) {
        set_fake_delay_us(delay_us);
    } else {
        fake_delay_us = 0;
    }
}

void Connection::set_fake_delay_us(const long& delay_us) {
    fake_delay_us = delay_us;
}

void Connection::close() {
    socket_ptr->shutdown(asio::ip::udp::socket::shutdown_receive);
    socket_ptr->shutdown(asio::ip::udp::socket::shutdown_send);
}

bool Connection::available() {
    return receiver.available();
}

Packet_Payloads Connection::receive() {
    Packet_Payloads usable_payloads;
    Packet p = receiver.receive<Packet>();
    Packet_Header& header = p.header;
    Packet_Payloads& payloads = p.payloads;
    //assert(header.sender_id < 8);

    if (!Packet_Header::sequence_more_recent(header.sequence_number, received)) {
        ////std::cout << int(instance_id_) << " dropping duplicate/old packet " << int(header.sequence_number)  << " from " << int(header.sender_id) << "\n";
    } else {
        //if (header.sequence_number == 3 || header.sequence_number == 4
        //|| header.sequence_number == 5) return usable_payloads;
        ////std::cout << int(instance_id_) << " received new packet " << int(header.sequence_number) << " from " << int(header.sender_id) << ", last received was " << int(received) << "\n";
        //received.emplace_back(header.sequence_number);
        //std::cout << "packet:" << header.sequence_number << ", adding to received\n";
        received = header.sequence_number;
        // update received_seqs to say we have received this packet
        // now when we get packet, need to check through all of payloads to say "we got this packet in case it was piggybacked"
        // add for each payload seq_number to received_seqs so that when next send back a packet, the other end can see that did "get" packets say 3,4 even if instead they were on the back of packet 5
        Seqs duplicates;
        for (const auto& payload: payloads) {
            //std::cout << "Payload type " << int(payload.type) << "\n";
            assert(payload.valid() && "Invalid payload type - probably forgot to call constructor with type,tick!");
            const Seq seq_num = payload.sequence_number;
            const bool already_received = 
                contains(received_seqs, seq_num);
            // this payload is a duplicate
            if (already_received) {
                duplicates.emplace_back(seq_num);
            } else {
                usable_payloads.emplace_back(payload);
            }
        }

        //std::cout << "Duplicates " << pr(duplicates) << "\n";
        //std::cout << "Received seqs from " << pr(received_seqs);
        Seqs received_payload_seqs;
        for (const auto& payload: usable_payloads) {
            received_payload_seqs.emplace_back(payload.sequence_number);
        }
        std::sort(
                received_payload_seqs.begin(),
                received_payload_seqs.end(),
                [] (const Seq& s1, const Seq& s2) {
                // intentionally swapped
                return Packet_Header::sequence_more_recent(s2,s1);
                }
                );

        // no payloads! then just add this packet's sequence number to received_seqs
        if (received_payload_seqs.size() == 0) {
            const Seq& seq_num = header.sequence_number;
            received_seqs.emplace_back(seq_num);
            if (!received_seqs.empty() && received_seqs.size() > received_seqs_lim) {
                received_seqs.pop_front();
            }
        } else {
            for (const auto& seq_num: received_payload_seqs) {
                received_seqs.emplace_back(seq_num);
                if (!received_seqs.empty() && received_seqs.size() > received_seqs_lim) {
                    received_seqs.pop_front();
                }
            }
        }
        //std::cout << " to " << pr(received_seqs) << "\n";

        // acknowledge that we sent these packets and they have been acked
        // packets are in p.header.lost
        Seqs just_received_seqs;
        for (const Seq& seq_num: header.received_seqs) {
            //std::cout << "Sender knows other end received " << int(seq_num) << ", removing it from received_seqs\n";
            just_received_seqs.emplace_back(seq_num);
            // need to remove each packet with this seq_num from unacked_packets
        }
        //std::cout << int(instance_id_) << " (sender) knows remote end received " << pr(just_received_seqs) << "\n";

        // remove from unacked packets packets that have a sequence number
        // that is in "just_received_seqs", ie. we just got an ack for them
        // don't need to hold them in buffer anymore
        //std::cout << "Unacked packets before erase: ";
        //for (const auto& pack: unacked_packets) {
        //std::cout << int(pack.header.sequence_number) << ", ";
        //}
        //std::cout << "\n";
        unacked_packets.erase(std::remove_if(
                    unacked_packets.begin(),
                    unacked_packets.end(),
                    [&] (const Packet& p) -> bool {
                    return contains(just_received_seqs, p.header.sequence_number);
                    }),
                unacked_packets.end()
                );
    }
    return usable_payloads;
}

void Connection::send(Packet_Payload& payload) {
    // this packet
    Packet p;

    Packet_Header ph;
    ph.sequence_number = sequence_number;
    ph.received_seqs = received_seqs;
    ph.sender_id = instance_id_;
    //std::cout << int(instance_id_) << " setting sender_id to " << int(ph.sender_id) << "\n";
    assert(ph.sender_id < 8);

    payload.sequence_number = sequence_number;

    p.header = ph;
    assert(payload.valid() && "Invalid payload type on sending - probably forgot to call constructor with type,tick!");
    p.payloads.emplace_back(payload);
    // this packet built with this payload

    assert(p.header.sender_id < 8);
    Packet to_send = Packet::piggyback(p, unacked_packets);
    assert(to_send.header.sender_id < 8);

    // add packet to list of unacked packets
    unacked_packets.emplace_back(p);
    if (unacked_packets.size() >= received_seqs_lim/2) {
        //std::cout << int(instance_id_) << " unacked_packets queue is size " << unacked_packets.size();
        //std::cout << "Removing element from front \n";
        unacked_packets.pop_front();
    }
    assert(p.header.sender_id < 8);
    assert(unacked_packets.back().header.sender_id < 8);

    const int payload_size = to_send.payloads.size();
    if (payload_size > 10) {
        //std::cout << "Large payload size of " << payload_size << "\n";
    }

    //Packet::append_payloads(p, unacked_packets);
    data_to_send.emplace_back(
            std::make_pair(timeNowMicros()+fake_delay_us,Sender::serialize(to_send))
    );
    // DO NOT SWAP order of append_payloads and this line below

    long timeNow = timeNowMicros();
    while (!data_to_send.empty()) {
        const long& t = data_to_send.front().first;
        auto& serial_data = data_to_send.front().second;

        // if time now is later than or same as packet send time
        // then send the data
        if (timeNow >= t) {
            sender.send(serial_data);
            data_to_send.pop_front();
        } else {
            // otherwise if first item isn't recent enough stop
            // as other items can only be meant to be sent later
            break;
        }
    }

    // this should not really be a for loop
    // ie. should be one for client, and this whole
    // thing should be called once per client per server
    ////std::cout << int(instance_id_) << " sending packet seq_num:" << int(ph.sequence_number) << " (" << serial.size() << ") to " << sender.port << "\n";
    ++sequence_number;
    ++tick;
}

