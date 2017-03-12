#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <vector>
#include <memory>

#include "Util.hpp"
#include "Packet_Header.hpp"
#include "Packet_Payload.hpp"
#include "Packet.hpp"
#include "Archiver.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"

struct Connection_Address;

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
        Instance_Id instance_id() const;
        Connection(io_service& io,
                const Connection_Address& connection_address,
                Instance_Id instance_id,
                int received_seqs_lim);
        Connection(io_service& io,
                std::string local_port, // port to listen on
                std::string host, // host to connect to to send stuff to
                std::string port, // port to connect to
                Instance_Id instance_id,
                int received_seqs_lim);

        bool available();

        Packet_Payloads receive();

        void send(Packet_Payload& payload);

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

    Connection_Address clientify() const {
        return Connection_Address(remote_port, remote_host, local_port);
    }

    Connection_Address static clientify(const Connection_Addresses& addrs, int index) {
        auto addr = addrs[index].clientify();
        assert(addrs.size() > 0 && "Should have a connection address to clientify");
        addr.remote_host = addrs.front().remote_host;
        return addr;
    }

};

#endif
