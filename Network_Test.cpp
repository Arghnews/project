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
#include "Packet_Header.hpp"

struct Packet;
struct Packet_Header;
struct Packet_Payload;

void sleep_ms(long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(std::max(0l,ms)));
}

void sleep_us(long us) {
    std::this_thread::sleep_for(std::chrono::microseconds(std::max(0l,us)));
}

typedef std::deque<Packet_Payload> Packet_Payloads;
typedef std::deque<Packet> Packets;

struct Connection_Address;

typedef std::vector<Connection_Address> Connection_Addresses;

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
    // used for clients as they need the remote/local ports swapped
    Connection_Address(
            std::string local_port,
            std::string remote_host,
            std::string remote_port) :
        local_port(local_port),
        remote_host(remote_host),
        remote_port(remote_port) {}
    Connection_Address clientify() {
        return Connection_Address(remote_port, remote_host, local_port);
    }
};

Connection_Address clientify(Connection_Addresses& addrs, int index) {
    auto addr = addrs[index].clientify();
    assert(addrs.size() > 0 && "Should have a connection address to clientify");
    addr.remote_host = addrs.front().remote_host;
    return addr;
}

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
            assert(header.sender_id < 8);

            if (!Packet_Header::sequence_more_recent(header.sequence_number, received)) {
                std::cout << int(instance_id_) << " dropping duplicate/old packet " << int(header.sequence_number)  << " from " << int(header.sender_id) << "\n";
            } else {
                //if (header.sequence_number == 3 || header.sequence_number == 4
                        //|| header.sequence_number == 5) return usable_payloads;
                std::cout << int(instance_id_) << " received new packet " << int(header.sequence_number) << " from " << int(header.sender_id) << ", last received was " << int(received) << "\n";
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

        void send(Packet_Payload& payload) {
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
            std::cout << int(instance_id_) << " sending packet seq_num:" << int(ph.sequence_number) << " (" << serial.size() << ") to " << sender.port << "\n";
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

static Connection_Addresses connection_addresses;

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

        for (int i=3; i<argc; i+=3) {
            std::string local_port = argv[i+0];
            std::string remote_host = argv[i+1];
            std::string remote_port = argv[i+2];
            connection_addresses.emplace_back(
                    Connection_Address(local_port, remote_host, remote_port));
        }

    }
    std::cout << "\n";

    int tps = 100;
    int sleep_time = 1000000/tps;
    auto big = 100;

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
                clientify(connection_addresses, instance_id),
                instance_id, received_seqs_lim);
        for (int i=0; i<big; ++i) {
            sleep_us(sleep_time);
            auto p = gen_payload();
            con.send(p);
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
}
