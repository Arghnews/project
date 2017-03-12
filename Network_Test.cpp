#include "Util.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <sstream>
#include <string>
#include <memory>
#include <algorithm>
#include <chrono>
#include <thread>

#include "Archiver.hpp"
#include "Force.hpp"
#include "Shot.hpp"
#include "P_State.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"

struct Packet;
struct Packet_Header;
struct Packet_Payload;

void sleep_ms(long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(std::max(0l,ms)));
}

void sleep_us(long us) {
    std::this_thread::sleep_for(std::chrono::microseconds(std::max(0l,us)));
}


typedef uint8_t Seq;
typedef std::deque<Seq> Seqs;

typedef std::deque<Packet_Payload> Packet_Payloads;

typedef std::deque<Packet> Packets;

std::string pr(Seqs& q) {
    std::stringstream buf;
    buf << "(" << q.size() << ") [";
    for (const auto& i: q) {
        buf << int(i) << ", ";
    }
    buf << "]";
    return buf.str();
};

struct Connection_Address {
    std::string local_port; // port to listen on
    std::string remote_host; // remote host
    std::string remote_port; // remote port
};

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
            const Seq& max=-1) {
        return (s1 > s2) && (s1 - s2 <= max/2) ||
            (s2 > s1) && (s2 - s1 > max/2);
    }

    //Packet_Header() : Packet_Header(1, 0, short_max) {}
    Packet_Header(
            const Seq& sequence_number,
            const Seqs& received_seqs,
            const Instance_Id sender_id
            ) :
        sequence_number(sequence_number),
        received_seqs(received_seqs),
        sender_id(sender_id) {
        }
    Packet_Header() {}
    //friend std::ostream& operator<<(std::ostream&, const Packet_Header&);
};

/*
   std::ostream& operator<<(std::ostream& stream, const Packet_Header& h) {
   stream << "Seq:" << h.sequence_number << ", ack:" << h.remote_sequence_number << ", sender_id:" << h.sender_id << " and bitfield:" << h.lost;
   */

class Packet_Payload {

    private:
    public:
        enum class Type : uint8_t {
            Invalid, Input, State
        };
        Packet_Payload(
                const Packet_Payload::Type& t,
                const Tick& tick
                ) :
            type(t),
            tick(tick)
    {

    }
        Packet_Payload() : Packet_Payload(Packet_Payload::Type::Invalid, 0) {}

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

        bool valid() const {
            return type != Packet_Payload::Type::Invalid;
        }

        template<class Archive>
            void serialize(Archive& archive) {
                archive(tick, forces, shots, positions, orients, momentums, ang_momentums, type, sequence_number);
                //archive(tick, forces, shots, packet_type);
            }
};

struct Packet {
    Packet_Header header;
    Packet_Payloads payloads; // a packet may have multiple payloads
    Packet() {}
    Packet(Packet_Header& header,
            Packet_Payloads& payloads) :
        header(header), payloads(payloads) {
            //std::cout << "Copy constructor called\n";

        }
    template<class Archive>
        void serialize(Archive& archive) {
            archive(header, payloads);       
        }

    // takes a packet and a container of packets,
    // and appends all the payloads from the packets in the container
    // to that of the packet
    // should have a way to set limit on number here!
    Packet static piggyback(const Packet& p, const Packets& packets) {
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

};

class Connection {
    private:
        std::shared_ptr<udp_socket> socket_ptr;
        Receiver receiver;// = make_unique<Receiver>(io,socket_ptr);
        Sender sender;
        Seq sequence_number;// = 0;
        Seqs received_seqs;
        int received_seqs_lim;// = 40;
        Seq received;// = -1;
        Packets unacked_packets;
        Tick tick;// = 0;
        Instance_Id instance_id_;

    public:
        Instance_Id instance_id() const {
            return instance_id_;
        }
        Connection(io_service& io,
                const Connection_Address& connection_address,
                Instance_Id instance_id,
                int received_seqs_lim) :
            Connection(io,
                    connection_address.local_port,
                    connection_address.remote_host,
                    connection_address.remote_port,
                    instance_id,
                    received_seqs_lim) {}
        Connection(io_service& io,
                std::string local_port, // port to listen on
                std::string host, // host to connect to to send stuff to
                std::string port, // port to connect to
                Instance_Id instance_id,
                int received_seqs_lim) :
            socket_ptr(std::make_shared<udp_socket>(io, udp_endpoint(asio::ip::udp::v4(),std::stoi(local_port)))),
            receiver(io,socket_ptr),
            sender(io,socket_ptr,host,port),
            sequence_number(0),
            received_seqs_lim(received_seqs_lim),
            received(-1),
            tick(0),
            instance_id_(instance_id)
    {

    }

        bool available() {
            return receiver.available();
        }

        Packet_Payloads receive() {
            Packet_Payloads usable_payloads;
            Packet p = receiver.receive<Packet>();
            Packet_Header& header = p.header;
            Packet_Payloads& payloads = p.payloads;
            std::cout << int(instance_id_) << " received from sender id " << int(header.sender_id) << "\n";
            assert(header.sender_id < 8);

            if (!Packet_Header::sequence_more_recent(header.sequence_number, received)) {
                std::cout << int(instance_id_) << " dropping duplicate/old packet " << int(header.sequence_number)  << " from " << int(header.sender_id) << "\n";
            } else {
                //if (header.sequence_number == 3 || header.sequence_number == 4
                        //|| header.sequence_number == 5) return usable_payloads;
                std::cout << int(instance_id_) << " received new packet " << int(header.sequence_number) << ", last received was " << int(received) << "\n";
                //received.emplace_back(header.sequence_number);
                //std::cout << "packet:" << header.sequence_number << ", adding to received\n";
                received = header.sequence_number;

                // update received_seqs to say we have received this packet

                // now when we get packet, need to check through all of payloads to say "we got this packet in case it was piggybacked"
                // add for each payload seq_number to received_seqs so that when next send back a packet, the other end can see that did "get" packets say 3,4 even if instead they were on the back of packet 5
                Seqs duplicates;
                for (const auto& payload: payloads) {
                    assert(payload.valid() && "Invalid payload type");
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

                for (const auto& seq_num: received_payload_seqs) {
                    received_seqs.emplace_back(seq_num);
                    if (!received_seqs.empty() && received_seqs.size() > received_seqs_lim) {
                        received_seqs.pop_front();
                    }
                }
                //std::cout << " to " << pr(received_seqs) << "\n";

                // acknowledge that we sent these packets and they have been acked
                // packets are in p.header.lost
                Seqs just_received_seqs;
                for (const Seq& seq_num: header.received_seqs) {
                    //std::cout << "Sender knows other end received " << int(seq_num) << ", removing it from received_seqs\n";
                    just_received_seqs.emplace_back(seq_num);
                    //erase(unacked_packets, seq_num);
                    //erase(received_seqs, seq_num);
                    // need to remove each packet with this seq_num from unacked_packets
                }
                //std::cout << int(instance_id_) << " (sender) knows remote end received " << pr(just_received_seqs) << "\n";

                // remove from unacked packets packets that have a sequence number
                // that is in "just_received_seqs", ie. we just got an ack for them
                // don't need to hold them in buffer anymore
                //std::cout << "Unacked packets before erase: ";
                for (const auto& pack: unacked_packets) {
                    //std::cout << int(pack.header.sequence_number) << ", ";
                }
                //std::cout << "\n";
                unacked_packets.erase(std::remove_if(
                            unacked_packets.begin(),
                            unacked_packets.end(),
                            [&] (const Packet& p) -> bool {
                            return contains(just_received_seqs, p.header.sequence_number);
                            }),
                        unacked_packets.end()
                        );
                //std::cout << "Unacked packets after erase: ";
                for (const auto& pack: unacked_packets) {
                    //std::cout << int(pack.header.sequence_number) << ", ";
                }
                //std::cout << "\n";
                //std::cout << int(instance_id_) << " receive for this packet over\n";
            }

            // send a message saying "you fuckhead, I didn't get these, where are they"
            return usable_payloads;
        }

        void send(Packet_Payload& payload) {
            // this packet
            Packet p;

            Packet_Header ph;
            ph.sequence_number = sequence_number;
            ph.received_seqs = received_seqs;
            ph.sender_id = instance_id_;
            std::cout << int(instance_id_) << " setting sender_id to " << int(ph.sender_id) << "\n";
            assert(ph.sender_id < 8);

            payload.sequence_number = sequence_number;

            p.header = ph;
            p.payloads.emplace_back(payload);
            // this packet built with this payload

            assert(p.header.sender_id < 8);
            Packet to_send = Packet::piggyback(p, unacked_packets);
            assert(to_send.header.sender_id < 8);

            // add packet to list of unacked packets
            unacked_packets.emplace_back(p);
            assert(p.header.sender_id < 8);
            assert(unacked_packets.back().header.sender_id < 8);

            const int payload_size = to_send.payloads.size();
            if (payload_size > 10) {
                std::cout << "Large payload size of " << payload_size << "\n";
            }

            //Packet::append_payloads(p, unacked_packets);
            auto serial = Sender::serialize(to_send);
            // DO NOT SWAP order of append_payloads and this line below

            // this should not really be a for loop
            // ie. should be one for client, and this whole
            // thing should be called once per client per server
            std::cout << int(instance_id_) << " sending packet seq_num:" << int(ph.sequence_number) << " (" << serial.size() << ") to " << sender.port << " (id:" << int(to_send.header.sender_id) << ")\n";
            sender.send(serial);
            ++sequence_number;
            ++tick;
        }

};

static std::string instance_type; // server/client etc.
static Instance_Id instance_id; // 0-255, server usually 0

static const std::string type_server = "server";
static const std::string type_client = "client";
static const std::string type_local = "local"; // no networking

static io_service io;

static std::string local_port;
// port I receive stuff on

std::vector<Connection_Address> connection_addresses;

static int received_seqs_lim = 20;

int main(int argc, char* argv[]) {

    if (argc < 2 || (argc > 2 && argc % 2 + 1 == 0)) {
        std::cerr << "Args of form: ./exec instance_type id (addr port){2,}\n";
        std::cerr << "instance_type is " << type_server << "," << type_client;
        std::cerr << " or " << type_local << " and id 0-255" << "\n";
        exit(1);
    }

    instance_type = std::string(argv[1]);

    if (instance_type != type_server &&
            instance_type != type_client &&
            instance_type != type_local) {
        std::cerr << "instance_type is " << type_server << "," << type_client;
        std::cerr << " or " << type_local << "\n";
        exit(1);
    }

    if (instance_type == type_local) {
    } else {

        int potential_id = std::stoi(argv[2]);
        if (potential_id < 0 || potential_id > 255) {
            std::cerr << "Id must be from 0 to num instances-1 and must be in byte range (it's an array index\n";
            exit(1);
        }
        instance_id = potential_id;

        /*
        for (int i=3; i<argc; i+=2) {
            // argv[i] = address // eg localhost
            unsigned short port = std::stoi(argv[i+1]);
            addresses.emplace_back(argv[i], argv[i+1]);
        }

        if (instance_id >= addresses.size()) {
            std::cerr << "Instance id must be 0 to number of different addresses-1\n";
            exit(1);
        } else if ((instance_id == 0) && (!(instance_type == type_server))) {
            std::cerr << "Server instance id must be 0\n";
            exit(1);
        }

        local_port = addresses[instance_id].second;
        std::cout << "My local_port " << local_port << "\n";
        //socket_ptr = std::make_shared<udp_socket>(io, udp_endpoint(asio::ip::udp::v4(),local_port));

        if (instance_type == type_server) {
            for (int i=0; i<addresses.size(); ++i) {
                auto& address = addresses[i];
                if (i == instance_id) {
                    continue;
                    // don't send stuff to myself, that would be dumb
                }
                // first:address, second:port
                //senders.emplace_back(io, socket_ptr, address.first, address.second);
            }
        } else if (instance_type == type_client) {
            //senders.emplace_back(io, socket_ptr, addresses[0].first, addresses[0].second);
        }
        */

    }
    std::cout << "\n";

    /*
    // In server case create a connection per client
    Connection c2(io, socket_ptr,
    addresses[0].first, addresses[0].second,
    received_seqs_lim);
    */

    int tps = 100;
    int sleep_time = 1000000/tps;
    auto big = 10;

    auto gen_payload = [&] () -> Packet_Payload {
        static Tick tick = 0;
        Packet_Payload payload(Packet_Payload::Type::Input, tick++);
        Forces fs;
        fs.emplace_back(Force(tick++,v3(69.348284523f,722334.20000223f,-432.3425f),Force::Type::Force));
        payload.forces = fs;
        return payload;
    };

    std::vector<Connection> connections;

    if (instance_type == type_server) {
        //for (const auto& connection_address: connections_addresses) {
        for (int i=0; i<connection_addresses.size(); ++i) {
            if (i == instance_id) {
                continue;
                // don't want server connecting to self
            }
            const auto& connection_address = connection_addresses[i];
            std::cout << "Making connection to " << connection_address.remote_port << "\n";
            connections.emplace_back(io,
                    connection_address,
                    instance_id, received_seqs_lim);
            std::cout << "Success\n";
        }
        std::cout << "Server has " << connections.size() << " connections\n";
        for (int i=0; i<big; ++i) {
            sleep_us(sleep_time);
            for (auto& con: connections) {
                while (con.available()) {
                    Packet_Payloads payloads = con.receive();
                    //std::cout << int(con.instance_id()) << " received ticks ("<<payloads.size() <<")\n";
                    for (const auto& payload: payloads) {
                        //std::cout << "Tick:" << payload.tick << "\n";
                    }
                }
                Packet_Payload payload = gen_payload();
                con.send(payload);
            }
        }
    } else if (instance_type == type_client) {
        Connection con(io,
                connection_addresses[0],
                instance_id, received_seqs_lim);
        for (int i=0; i<big; ++i) {
            sleep_us(sleep_time);
            auto p = gen_payload();
            con.send(p);
            //p = gen_payload();
            //con.send(p);
            Packet_Payloads payloads = con.receive();
            //std::cout << int(con.instance_id()) << " received ticks ("<<payloads.size() <<")\n";
            for (const auto& payload: payloads) {
                //std::cout << "Tick:" << payload.tick << "\n";
            }
        }
    }
    /*
            Forces fs;
            fs.emplace_back(Force(tick,v3(69.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(74.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(79.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(84.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(89.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(94.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(99.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(104.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(109.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(114.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(119.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(124.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(129.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(134.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(139.0f,72.0f,0.0f),Force::Type::Force));
            fs.emplace_back(Force(tick,v3(144.0f,72.0f,0.0f),Force::Type::Force));
            payload.forces = fs;
            */

}

    /*
    int c = 0;

    std::unique_ptr<Receiver> receiver_ptr = make_unique<Receiver>(io,socket_ptr);
    Seq sequence_number = 0;
    Seqs received_seqs;
    int received_seqs_lim = 40;
    Seq received = -1;
    Packets unacked_packets;
    uint32_t tick = 0;

    if (instance_type == type_server) {

        while (1) {
            sleep_ms(1000 * 1);
            while (receiver_ptr->available()) {
                Packet p(receiver_ptr->receive<Packet>());
                Packet_Header& header = p.header;
                Packet_Payloads& payloads = p.payloads;
                if (header.sequence_number == 3 || header.sequence_number == 4) {
                    continue;
                };
                std::cout << "Server received ";

                //if (contains(received, header.sequence_number)) {
                if (!Packet_Header::sequence_more_recent(header.sequence_number, received)) {
                    std::cout << "duplicate or old packet:" << int(header.sequence_number) << "\n";
                } else {
                    //received.emplace_back(header.sequence_number);
                    //std::cout << "packet:" << header.sequence_number << ", adding to received\n";
                    std::cout << "Most recent received from " << int(received);
                    received = header.sequence_number;
                    std::cout << " to " << int(received) << "\n";

                    //received_seqs_add(
                            //received_seqs,
                            //header.sequence_number,
                            //received_seqs_lim);

                    // update received_seqs to say we have received this packet
                    
                    // now when we get packet, need to check through all of payloads to say "we got this packet in case it was piggybacked"
                    // add for each payload seq_number to received_seqs so that when next send back a packet, the other end can see that did "get" packets say 3,4 even if instead they were on the back of packet 5
                    Packet_Payloads usable_payloads;
                    for (const auto& payload: payloads) {
                        const bool already_received = 
                            contains(received_seqs, payload.sequence_number);
                        // this payload is a duplicate
                        if (already_received) {
                            std::cout << "Received duplicate payload " << int(payload.sequence_number) << "\n";
                        } else {
                            std::cout << "Received new payload " << int(payload.sequence_number) << "\n";
                            usable_payloads.emplace_back(payload);
                        }
                    }
                    for (const auto& payload: usable_payloads) {
                        received_seqs.emplace_back(payload.sequence_number);
                        if (!received_seqs.empty() && received_seqs.size() > received_seqs_lim) {
                            received_seqs.pop_front();
                        }
                    }
                    
                    // packets we missed
                    //for (
                      //Seq i = old_received + 1;
                      //Packet_Header::sequence_more_recent(new_received,i);
                      //++i)
                      //{
                      //lost.emplace_back(i);
                      //std::cout << "Receiver didn't get packet " << int(i) << "\n";
                      //}
//
                    // acknowledge that we sent these packets and they have been acked
                    // packets are in p.header.lost
                    Seqs just_received_seqs;
                    for (const Seq& seq_num: header.received_seqs) {
                        std::cout << "Sender knows other end received " << int(seq_num) << ", removing it from received_seqs\n";
                        just_received_seqs.emplace_back(seq_num);
                        //erase(unacked_packets, seq_num);
                        //erase(received_seqs, seq_num);
                        // need to remove each packet with this seq_num from unacked_packets
                    }
                    // remove from unacked packets packets that have a sequence number
                    // that is in "just_received_seqs", ie. we just got an ack for them
                    // don't need to hold them in buffer anymore
                    std::cout << "Unacked packets before erase: ";
                    for (const auto& pack: unacked_packets) {
                        std::cout << pack.header.sequence_number << ", ";
                    }
                    std::cout << "\n";
                    unacked_packets.erase(std::remove_if(
                                unacked_packets.begin(),
                                unacked_packets.end(),
                                [&] (const Packet& p) -> bool {
                                    return contains(just_received_seqs, p.header.sequence_number);
                                }),
                                unacked_packets.end()
                    );
                    std::cout << "Unacked packets after erase: ";
                    for (const auto& pack: unacked_packets) {
                        std::cout << pack.header.sequence_number << ", ";
                    }
                    std::cout << "\n";
                }

                // send a message saying "you fuckhead, I didn't get these, where are they"
            }
        }
    } else if (instance_type == type_client) {
        while (c++ < 7) {
            sleep_ms(1000 * 2);

            // this packet
            Packet p;

            Packet_Header ph;
            ph.sequence_number = sequence_number;
            ph.received_seqs = received_seqs;

            Packet_Payload payload;
            payload.tick = tick;
            payload.type = Packet_Payload::Type::Input;
            payload.sequence_number = sequence_number;
            Forces fs;
            fs.emplace_back(Force(tick,v3(69.0f,72.0f,0.0f),Force::Type::Force));
            payload.forces = fs;

            p.header = ph;
            p.payloads.emplace_back(payload);
            // this packet built with this payload

            Packet to_send = Packet::piggyback(p, unacked_packets);

            // add packet to list of unacked packets
            unacked_packets.emplace_back(p);

            const int payload_size = to_send.payloads.size();
            if (payload_size > 10) {
                std::cout << "Large payload size of " << payload_size << "\n";
            }

            //Packet::append_payloads(p, unacked_packets);
            auto serial = Sender::serialize(to_send);
            // DO NOT SWAP order of append_payloads and this line below

            // this should not really be a for loop
            // ie. should be one for client, and this whole
            // thing should be called once per client per server
            for (auto& sender: senders) {
                std::cout << "Client sent " << int(to_send.header.sequence_number) << " to " << sender.port << " (" << serial.size() << ") with payload size " << payload_size << "\n";
                sender.send(serial);
            }
            ++sequence_number;
            ++tick;

        }
    }

}
*/
    /*
                Packet p;
                Packet_Header ph;
                ph.sequence_number = 0;*/

                /*
                if (!received.empty() && received.size() >= 4) {
                    pr(received);
                    std::cout << "Removing " << received.front() << " from queue\n";
                    received.pop_front();
                    pr(received);
                }
        while (1) {
            if (receiver_ptr->available()) {
                Packet p(receiver_ptr->receive<Packet>());
                Packet_Header& header = p.header;
                std::cout << "Server received packet header:" << printH(header) << "\n";
                std::cout << "Server processing...\n";
                sleep_ms(2500);
                std::cout << "Server done processing\n";
            }
        }
    } else if (instance_type == type_client) {
        int tick = 0;
        uint16_t sequence_number = 0;
        uint16_t remote_sequence_number = 0;
        //uint32_t lost = -1;
        Seqs lost;
        Packets unacked_packets;

        for (int i=0; i<4; ++i) {
            // need queue of uint16_t Sequence numbers that are the prior received packets
            // lost 
            // for every value in the queue (which is a Seq number)
            // lost[i] = contains(queue, remote_sequence_number-i)
            // or for every ele in queue, index=remote_sequence_number - ele
            // lost[index] = 1
            Packet_Header header(sequence_number, remote_sequence_number,
                    lost, instance_id);

            std::cout << "Client sending " << printH(header) << "\n";
            Packet_Payloads payloads(1);
            Packet packet(header, payloads);
            concat(unacked_packets, packet);

            auto serial = Sender::serialize(packet);
            for (auto& sender: senders) {
                std::cout << "Sent to " << sender.port << " with unacked packet size " << unacked_packets.size() << "\n";
                sender.send(serial);
            }

            ++sequence_number; // inc Seq number
            //lost = lost << 1; // shift, ie. most recent packet not acked

            sleep_ms(1000);
            while (receiver_ptr->available()) {
                Packet p(receiver_ptr->receive<Packet>());
                Packet_Header& header = p.header;
                std::cout << "Client received " << printH(header) << "\n";
                if (Packet_Header::sequence_more_recent(
                            header.sequence_number, remote_sequence_number)) {
                    // packet more recent
                    std::cout << "More recent, remote Seq number from " << remote_sequence_number;
                    remote_sequence_number = header.sequence_number;
                    for (int i=0; i<32; ++i) {
                        
                    }
                    std::cout << " to " << remote_sequence_number << "\n";
                } else {
                    std::cout << "Received old or duped packet\n";
                }
                // if not already in queue of acked, add to queue
            // check for receive of anything
            // if receive update lost, ie. set the bit
            // and remove the payload for that tick from unacked_payloads
            }


       */


    /*
    void static write_bit(uint32_t& bitfield, const int& index, const bool& value) {
        bitfield ^= (-value ^ bitfield) & (1 << index);
    }

    bool static read_bit(const uint32_t& bitfield, const int& index) {
        return (bitfield >> index) & 1;
    }
    */

        /*
        Forces fs;
        fs.emplace_back(Force(tick,v3(69.0f,72.0f,0.0f),Force::Type::Force));

        Packet_Payload payload(tick, fs);
        payloads.emplace_back(payload);

        // header and payloads now invalid -- move cons
        //Packet packet(std::move(header), std::move(payloads));

        // copy cons
        */

    /*
    if (instance_type== "server") {
        while (1) {
            if (receiver_ptr->available()) {
                Forces fs;
                fs = receiver_ptr->receive<Forces>();
                std::cout << "Server received message:\n";
                for (const auto& f: fs) {
                    std::cout << "" << f.id << "\n";
                }
            }
        }
    } else if (instance_type== "client") {
        // send from port 2001 to address:port

        Forces fs;
        fs.emplace_back(Force(69,v3(69.0f,72.0f,0.0f),Force::Type::Force));
        fs.emplace_back(Force(71,v3(69.0f,72.0f,0.0f),Force::Type::Force));

        auto serial = Sender::serialize(fs);
        for (auto& sender: senders) {
            sender.send(serial);
        }
    }
    */


// lol I'm retarded this is so complex I'm doing it like the other - ie. the Input_Payload
/*
struct State_Payload {
    // essentially the challenge here is that this is attempting to efficiently
    // encode a P_State's relevant parts
        // parse output
        State_Payload sp;
        const int size = ids.size();
        int data_index = 0;
        for (int i=0; i<size; ++i) {
            const Id& id = sp.ids[i];
            const uint8_t type_number = sp.instance_type[i];
            vv3 outs = decode_one(type_number, data_index);

        }

    uint32_t tick; // initially tied to Seq number
    // application level number, so can tell at what tick this was sent on
    // can tell if too old or not etc
    
    // for now just doing the absolute data items
    vId ids; // ids that the following relate to
    std::vector<uint8_t> instance_type;
    // 1->pos,
    // 2->mom
    // 4->orient // store fq as v3, using x^2+y^2+z^2+w^2 = 1, for len 1 quat
    // 8->ang_mom
    // summation tells you what is in the data
    // assert(ids.size() == instance_type.size());
    vv3 data; // length of this can be > ids, as if instance_type[0]=3, then data
    // will have a pos and mom component

    static const uint8_t pos = 1;
    static const uint8_t orient = 4;

    void encode(const Id& id, const uint8_t& instance_type const vv3& data_in) {
        ids.emplace_back(id);
        instance_type.emplace_back(instance_type;
        concat(data, data_in);
    }

    // decode needs to be like keep counter i into id/instance_type,
    vv3 decode_one(uint8_t type_number, int& data_index) {
        vv3 ret;
        ret.reserve(1);
        for (uint8_t j=8; j>0; --j) {
            if (powers_of_two[j] >= type_number) {
                type_number -= powers_of_two[j];
                ret.emplace_back(data[data_index]);
                ++data_index;
            }
        }
        return ret;
    }

    template<class Archive>
        void serialize(Archive& archive) {
            archive(tick, ids, instance_type, data);       
        }
};
*/
