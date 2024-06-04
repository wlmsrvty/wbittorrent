#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace bittorrent {

class Peer {
    public:
        Peer(std::string ip, uint16_t port);

        const std::string ip_get() const;
        const uint16_t port_get() const;

        void handshake(std::vector<uint8_t> const& info_hash_raw) const;

    private:
        std::string ip_;
        uint16_t port_;
};
}