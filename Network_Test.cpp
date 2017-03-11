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

struct Packet_Header {
    // header
    Seq sequence_number;
    Seqs lost; // corresponds to prior 32
    Instance_Id sender_id; // use last two digits of port
    // packets
    template<class Archive>
        void serialize(Archive& archive) {
            archive(sequence_number, lost, sender_id);       
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
            const Seqs& lost,
            const Instance_Id& sender_id
            ) :
        sequence_number(sequence_number),
        lost(lost),
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

    enum class Type : uint8_t {
        Input, State
    };

    private:

        Packet_Payload(
                const uint32_t& tick,
                const Type& packet_type) :
            tick(tick),
            packet_type(packet_type)
    {
    }

    public:
        Packet_Payload() {}
        uint32_t tick; // initially tied to Seq number
        // application level number, so can tell at what tick this was sent on
        // can tell if too old or not etc // think I need this? maybe not really

        Type packet_type;

        Forces forces;
        Shots shots;
            
        std::vector<Id_v3> positions;
        std::vector<Id_fq> orients;
        std::vector<Id_v3> momentums;
        std::vector<Id_v3> ang_momentums;

        template<class Archive>
            void serialize(Archive& archive) {
                archive(tick, forces, shots, positions, orients, momentums, ang_momentums, packet_type);
                //archive(tick, forces, shots, packet_type);
            }
};

struct Packet {
    Packet_Header header;
    std::vector<Packet_Payload> payloads;
    Packet() {}
    Packet(Packet_Header& header,
            std::vector<Packet_Payload>& payloads) :
        header(header), payloads(payloads) {
            std::cout << "Copy constructor called\n";

    }
    Packet(Packet_Header&& header,
            std::vector<Packet_Payload>&& payloads) :
        header(header), payloads(payloads) {
            std::cout << "Move constructor called\n";

    }
    template<class Archive>
        void serialize(Archive& archive) {
            archive(header, payloads);       
        }
};

static std::string instance_type; // server/client etc.
static Instance_Id instance_id; // 0-255, server usually 0

static const std::string type_server = "server";
static const std::string type_client = "client";
static const std::string type_local = "local"; // no networking

static std::shared_ptr<udp_socket> socket_ptr;
static std::vector<std::pair<std::string,std::string>> addresses; // address, port

static io_service io;
static std::unique_ptr<Receiver> receiver_ptr;
static std::vector<Sender> senders;

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

        uint16_t local_port = std::stoi(addresses[instance_id].second);
        std::cout << "My local_port " << local_port << "\n";
        socket_ptr = std::make_shared<udp_socket>(io, udp_endpoint(asio::ip::udp::v4(),local_port));

        receiver_ptr = make_unique<Receiver>(io,socket_ptr);

        if (instance_type == type_server) {
            for (int i=0; i<addresses.size(); ++i) {
                auto& address = addresses[i];
                if (i == instance_id) {
                    continue;
                    // don't send stuff to myself, that would be dumb
                }
                // first:address, second:port
                senders.emplace_back(io, socket_ptr, address.first, address.second);
            }
        } else if (instance_type == type_client) {
            senders.emplace_back(io, socket_ptr, addresses[0].first, addresses[0].second);
        }

        for (auto& s: senders) {
            std::cout << "In senders: " << s.host << "," << s.port << "\n";
        }
    }

    auto pr = [] (std::deque<Seq>& q) {
        std::cout << "Size " << q.size() << ": ";
        for (const auto& i: q) {
            std::cout << i << ", ";
        }
        std::cout << "\n";
    };

    int c = 0;

    Seq sequence_number = 0;
    Seqs lost;
    Seq received = -1;

    if (instance_type == type_server) {

        while (1) {
            sleep_ms(1000 * 1);
            while (receiver_ptr->available()) {
                Packet p(receiver_ptr->receive<Packet>());
                Packet_Header& header = p.header;
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
                    Seq old_received = received;

                    std::cout << "Most recent received from " << int(received);
                    received = p.header.sequence_number;
                    std::cout << " to " << int(received) << "\n";

                    Seq new_received = received;

                    // packets we missed
                    for (
                            Seq i = old_received + 1;
                            Packet_Header::sequence_more_recent(new_received,i);
                            ++i)
                    {
                        lost.emplace_back(i);
                        std::cout << "Lost packet " << int(i) << "\n";
                    }

                    // acknowledge that we sent these packets but they got lost
                    // packets are in p.header.lost
                    for (const Seq& seq_num: header.lost) {
                        std::cout << "So we lost " << int(seq_num) << "\n";
                    }
                }

                // send a message saying "you fuckhead, I didn't get these, where are they"
                /*Packet p;
                Packet_Header ph;
                ph.sequence_number = 0;*/

                /*
                if (!received.empty() && received.size() >= 4) {
                    pr(received);
                    std::cout << "Removing " << received.front() << " from queue\n";
                    received.pop_front();
                    pr(received);
                }*/
            }
        }
    } else if (instance_type == type_client) {
        while (c++ < 6) {
            sleep_ms(1000 * 2);
            Packet p;
            Packet_Header ph;
            ph.sequence_number = sequence_number;
            ph.lost = lost;
            p.header = ph;

            auto serial = Sender::serialize(p);
            for (auto& sender: senders) {
                std::cout << "Client sent " << int(p.header.sequence_number) << " to " << sender.port << " (" << sizeof(serial) << " vs " << serial.size() << ")\n";
                sender.send(serial);
            }
            ++sequence_number;

        }
    }

    /*
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
            std::vector<Packet_Payload> payloads(1);
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
}
