#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <system_error>

namespace bittorrent {

class Peer {
    public:
        Peer(std::string ip, uint16_t port);
        ~Peer();

        std::error_code createSocket();
        std::error_code closeSocket();
        std::error_code connect();

        std::error_code establish_connection();
        std::error_code close_connection();

        std::error_code handshake(std::vector<uint8_t> const& info_hash_raw);

        int socket_fd;

        std::string ip_;
        uint16_t port_;
};
}