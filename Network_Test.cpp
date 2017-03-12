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
#include "Packet_Payload.hpp"
#include "Packet.hpp"
#include "Connection.hpp"

void static parse_args(
        int argc,
        char* argv[],
        std::string& instance_type,
        Instance_Id& instance_id,
        Connection_Addresses& connection_addresses
        );

static std::string instance_type; // server/client etc.
static Instance_Id instance_id; // 0-255, server usually 0

static const std::string type_server = "server";
static const std::string type_client = "client";
static const std::string type_local = "local"; // no networking

static io_service io;

static Connection_Addresses connection_addresses;

static int received_seqs_lim = 20;

int main(int argc, char* argv[]) {

    parse_args(argc, argv,
            instance_type,
            instance_id,
            connection_addresses);

    int tps = 100;
    int sleep_time = 1000000/tps;
    auto big = 100;

    auto gen_payload = [&] () -> Packet_Payload {
        static Tick tick = 0;
        Packet_Payload payload(Packet_Payload::Type::Input, tick++);
        Forces fs;
        fs.emplace_back(Force(tick++,v3(69.348284523f,722334.20000223f,-432.3425f),Force::Type::Force));
        fs.emplace_back(Force(tick++,v3(69.348284523f,722334.20000223f,-432.3425f),Force::Type::Force));
        fs.emplace_back(Force(tick++,v3(69.348284533f,722334.20000223f,-452.3425f),Force::Type::Force));
        fs.emplace_back(Force(tick++,v3(69.348284523f,722334.20000223f,-432.3425f),Force::Type::Force));
        fs.emplace_back(Force(tick++,v3(69.348284524f,722334.20000223f,-432.3425f),Force::Type::Force));
        fs.emplace_back(Force(tick++,v3(69.348284524f,722334.20000223f,-411.3425f),Force::Type::Force));
        fs.emplace_back(Force(tick++,v3(69.348284523f,722334.20000223f,-440.3425f),Force::Type::Force));
        fs.emplace_back(Force(tick++,v3(69.348284489f,722334.20000223f,-432.3425f),Force::Type::Force));
        payload.forces = fs;
        return payload;
    };

    // if want printouts, uncomment the //// lines in Connection.cpp

    Connections connections;

    // setup connections
    if (instance_type == type_server) {
        for (int i=0; i<connection_addresses.size(); ++i) {
            if (i == instance_id) {
                continue;
                // don't want server connecting to self
            }
            const auto& connection_address = connection_addresses[i];
            //std::cout << "Making connection to " << connection_address.remote_port << "\n";
            connections.emplace_back(io,
                    connection_address,
                    instance_id, received_seqs_lim);
            //std::cout << "Success\n";
        }
    } else if (instance_type == type_client) {
        // add server with ports swapped and remote_host as server address
        connections.emplace_back(io,
                Connection_Address::clientify(connection_addresses, instance_id),
                instance_id, received_seqs_lim);
    }

    if (instance_type == type_server) {
        //for (connst auto& connection_address: connections_addresses) {
        std::cout << "Server has " << connections.size() << " connections\n";
        for (int i=0; i<big; ++i) {
            sleep_us(sleep_time);
            for (auto& conn: connections) {
                while (conn.available()) {
                    Packet_Payloads payloads = conn.receive();
                    //std::cout << int(conn.instance_id()) << " received ticks ("<<payloads.size() <<")\n";
                    for (const auto& payload: payloads) {
                        //std::cout << "Tick:" << payload.tick << "\n";
                    }
                }
                Packet_Payload payload = gen_payload();
                conn.send(payload);
            }
        }
    } else if (instance_type == type_client) {
        assert(connections.size() == 1 && "Client should only have one connection");
        for (int i=0; i<big; ++i) {
            Connection& conn = connections[0];
            sleep_us(sleep_time);
            auto p = gen_payload();
            conn.send(p);
            conn.send(p);
            //p = gen_payload();
            //conn.send(p);
            Packet_Payloads payloads = conn.receive();
            //std::cout << int(conn.instance_id()) << " received ticks ("<<payloads.size() <<")\n";
            for (const auto& payload: payloads) {
                //std::cout << "Tick:" << payload.tick << "\n";
            }
        }
    }
}

void static parse_args(
        int argc,
        char* argv[],
        std::string& instance_type,
        Instance_Id& instance_id,
        Connection_Addresses& connection_addresses
        ) {
    if (argc < 9) {
        std::cerr << "Args of form: ./exec [client|server] id (local_port remote_host remote_port){2,}\n";
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

        int start_of_rest = 3;
        if ((argc-start_of_rest) % 3 != 0 || (argc-start_of_rest) < 6) {
            std::cerr << "Arguments at end should [local_port remote_host remote_port] triplets\n";
            exit(1);
        }
        for (int i=start_of_rest; i<argc; i+=3) {
            std::string local_port = argv[i+0];
            std::string remote_host = argv[i+1];
            std::string remote_port = argv[i+2];
            connection_addresses.emplace_back(
                    Connection_Address(local_port, remote_host, remote_port));
        }

    }
    std::cout << "\n";
}

