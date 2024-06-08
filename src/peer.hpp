#pragma once

#include "message.hpp"
#include "torrent.hpp"

#include <optional>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

namespace bittorrent {

class Peer {
   public:
    Peer(std::string ip, uint16_t port);
    ~Peer();

    std::error_code establish_connection();
    std::error_code close_connection();

    std::error_code handshake(std::vector<uint8_t> const& info_hash_raw);
    std::error_code download_file(const Torrent& torrent, std::string const& file_path);

    std::optional<Message> read_message_raw(std::error_code& ec);
    std::error_code send_message(Message const& message);

    int socket_fd;

    std::string ip_;
    uint16_t port_;

    bool is_choked;

   private:
    std::error_code createSocket();
    std::error_code closeSocket();
    std::error_code connect();
};
}  // namespace bittorrent