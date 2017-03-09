#include "Util.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include "cereal/instance_type/deque.hpp"
#include "cereal/instance_type/vector.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <sstream>
#include <string>
#include <memory>
#include <algorithm>

#include "Archiver.hpp"
#include "Force.hpp"
#include "Shot.hpp"
#include "P_State.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"

static const std::vector<uint8_t> powers_of_two = {
    0, 1, 2, 4, 8, 16, 32, 64, 128
};

struct Packet_Header {
    // header
    uint16_t sequence_number;
    uint16_t ack_number;
    uint32_t ack_bitfield; // corresponds to prior 32
    Instance_Id sender_id; // use last two digits of port
    // packets
    template<class Archive>
        void serialize(Archive& archive) {
            archive(sequence_number, ack_number, ack_bitfield, sender_id);       
        }

    // returns true when s1 is a more recent sequence number than s2
    // where seq numbers are like ids that go up
    bool static sequence_more_recent(const uint16_t& s1, const uint16_t& s2,
            const uint16_t& max=uint16_t_max) {
        return (s1 > s2) && (s1 - s2 <= max/2) ||
            (s2 > s1) && (s2 - s1 > max/2);
    }

    void static write_bit(uint32_t& bitfield, const int& index, const bool& value) {
        bitfield ^= (-value ^ bitfield) & (1 << index);
    }

    bool static read_bit(const uint32_t& bitfield, const int& index) {
        return (bitfield >> index) & 1;
    }
    
    //Packet_Header() : Packet_Header(1, 0, short_max) {}
    Packet_Header(
            const uint16_t& sequence_number,
            const uint16_t& ack_number,
            const uint32_t& ack_bitfield,
            const Instance_Id& sender_id
            ) :
        sequence_number(sequence_number),
        ack_number(ack_number),
        ack_bitfield(ack_bitfield),
        sender_id(sender_id) {
        }
};

struct Payload {
    enum class Packet_Type : uint8_t {
        Input, State
    };

    uint32_t tick; // initially tied to seq number
    // application level number, so can tell at what tick this was sent on
    // can tell if too old or not etc // think I need this? maybe not really
    
    Packet_Type instance_type
    
    Forces forces;
    Shots shots;
    
    P_States p_states;

    template<class Archive>
        void serialize(Archive& archive) {
            archive(tick, forces, shots, p_states, instance_type;
        }
};

struct Packet {
    Packet_Header header;
    std::vector<Payload> payloads;
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

static unsigned short local_port; // port that the socket_ptr binds to
// ie port that stuff gets sent from and read from
static std::shared_ptr<udp_socket> socket_ptr;
static std::vector<std::pair<std::string,std::string>> addresses; // address, port

static io_service io;
static std::unique_ptr<Receiver> receiver_ptr;
static std::vector<Sender> senders;

int main(int argc, char* argv[]) {

    if (argc < 2 || (argc > 2 && argc % 2 + 1 != 0)) {
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

    int potential_id = std::stoi(argv[2]);
    if (potential_id < 0 || potential_id > 255) {
        std::cerr << "Id must be from 0 to num instances-1 and must be in byte range (it's an array index\n";
        exit(1);
    }
    instance_id = potential_id;


    socket_ptr = std::make_shared<udp_socket>(io, udp_endpoint(asio::ip::udp::v4(), std::stoi(addresses[instance_id].second)));

    std::cout << "Local port: " << local_port << "\n";
    receiver_ptr = make_unique<Receiver>(io,socket_ptr);

    for (const auto& address: addresses) {
        // first:address, second:port
        std::cout << "Local port: " << local_port << " and address " << address.first << " and port " << address.second << "\n";
        senders.emplace_back(io, socket_ptr, address.first, address.second);
    }

    /*
    uint32_t val = 17;
    std::cout << "Before " << val << "\n";
    write_bit(val, 1, true); // now 19
    std::cout << "After " << val << "\n";
    */
    /*
    uint32_t val = 63;
    for (int i=0; i<32; ++i) {
        std::cout << read_bit(val, i) << "\n";
    }*/

    if (instance_type== type_server) {
    
    } else if (instance_type== type_client) {
        Packet_Header packet(1, 0, uint16_t_max, 1);
    }

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

    uint32_t tick; // initially tied to seq number
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
