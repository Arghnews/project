#include "Util.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include "cereal/types/deque.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <sstream>
#include <string>
#include <memory>

#include "Archiver.hpp"
#include "Force.hpp"
#include "Shot.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"

static std::string type;
static std::string server = "server";
static std::string client = "client";

static unsigned short local_port; // port that the socket_ptr binds to
// ie port that stuff gets sent from and read from
static std::shared_ptr<udp_socket> socket_ptr;
static std::vector<std::pair<std::string,std::string>> addresses; // address, port

static io_service io;
static std::unique_ptr<Receiver> receiver_ptr;
static std::vector<Sender> senders;
static std::string my_id;

// returns true when s1 is a more recent sequence number than s2
// where seq numbers are like ids that go up
bool sequence_more_recent(const uint16_t& s1, const uint16_t& s2,
        const uint16_t& max=uint16_t(-1))
{
    return (s1 > s2) && (s1 - s2 <= max/2) ||
        (s2 > s1) && (s2 - s1 > max/2);
}

void write_bit(uint32_t& bitfield, const int& index, const bool& value) {
    bitfield ^= (-value ^ bitfield) & (1 << index);
}

bool read_bit(const uint32_t& bitfield, const int& index) {
    return (bitfield >> index) & 1;
}

class Packet_Header {
    // header
    uint16_t sequence_number;
    uint16_t ack_number;
    uint32_t ack_bitfield; // corresponds to prior 32

};

class Packet_Payload {
    uint32_t tick; // initially tied to seq number
    // application level number, so can tell at what tick this was sent on
    // can tell if too old or not etc
    
    // plan is really to union absolute with relative and have a byte to
    // say whether one, other or both beforehand ie. Id -> 2 = [abs],[delta]
    
    vId ids; // ids that the following relate to
    std::vector<uint8_t> types;
    // 1->pos,
    // 2->mom
    // 4->orient // store fq as v3, using x^2+y^2+z^2+w^2 = 1, for len 1 quat
    // 8->ang_mom
    // summation tells you what is in the data
    // assert(ids.size() == types.size());
    vv3 data; // length of this can be > ids, as if types[0]=3, then data
    // will have a pos and mom component
    
    // payload
    /*
    Forces forces;
    Shots shots;
    template<class Archive>
        void serialize(Archive& archive) {
            archive(sequence_number, ack_number,
                ack_bitfield, forces, shots);
        }
    */
};

class Packet {
    Packet_Header header;
    Packet_Payload payload;
};

int main(int argc, char* argv[]) {

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

    if (argc < 2) {
        std::cout << "Not enough arguments - please provide client/server as first param\n";
        exit(1);
    }

    // host, port
    type = std::string(argv[1]);
    if (type == server) {
        if (argc < 5 || argc % 2 == 0) {
            std::cout << "To run server: ./server server [server_receive_port] [client_addr] [client_port]... - at least is needed\n";
            exit(1);
        }

        local_port = std::stoi(argv[2]);
        for (int i=3; i<argc; i+=2) {
            std::string addr = argv[i];
            std::string port = argv[i+1];
            auto address = std::make_pair(addr,port);
            addresses.push_back(address);
        }
    } else if (type == client) {
        if (argc != 5) {
            std::cout << "To run client: ./client client [client_receive_port] [server_host] [server_port] at least is needed\n";
            exit(1);
        }
        local_port = std::stoi(argv[2]);
        std::string addr = argv[3];
        std::string port = argv[4];
        auto address = std::make_pair(addr,port);
        addresses.push_back(address);
        my_id = local_port;
    } else {
        std::cout << "Did not recognise type, choose from either " << server << " or " << client << "\n";
        exit(1);
    }

    socket_ptr = std::make_shared<udp_socket>(io, udp_endpoint(asio::ip::udp::v4(), local_port));

    std::cout << "Local port: " << local_port << "\n";
    receiver_ptr = make_unique<Receiver>(io,socket_ptr);

    for (const auto& address: addresses) {
        // first:address, second:port
        std::cout << "Local port: " << local_port << " and address " << address.first << " and port " << address.second << "\n";
        senders.emplace_back(io, socket_ptr, address.first, address.second);
    }

    if (type == "server") {
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
    } else if (type == "client") {
        // send from port 2001 to address:port

        Forces fs;
        fs.emplace_back(Force(69,v3(69.0f,72.0f,0.0f),Force::Type::Force));
        fs.emplace_back(Force(71,v3(69.0f,72.0f,0.0f),Force::Type::Force));

        auto serial = Sender::serialize(fs);
        for (auto& sender: senders) {
            sender.send(serial);
        }
    }


}
