#include "peer.hpp"
#include <arpa/inet.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>

namespace bittorrent {

using namespace std::string_literals;

Peer::Peer(std::string ip, uint16_t port) : ip_(ip), port_(port) {}
inline std::string const Peer::ip_get() const { return ip_; }
inline uint16_t const Peer::port_get() const { return port_; }

static std::string handshake_message(std::vector<uint8_t> const& info_hash_raw,
                                     std::string const& peer_id) {
    auto digest_str =
        std::string(reinterpret_cast<char const*>(info_hash_raw.data()),
                    info_hash_raw.size() * sizeof(uint8_t));
    std::string handshake_message =
        "\x13"s +  // character 19
        "BitTorrent protocol"s + "\x00\x00\x00\x00\x00\x00\x00\x00"s +
        // make sure to use string literals for \x00
        // and not c style string
        // string_literals ensures std::string{str, len} is used
        digest_str + peer_id;
    return handshake_message;
}

static std::string parse_peer_id(std::string const& message) {
    static std::string const header = "\x13"s +  // character 19
                                      "BitTorrent protocol"s +
                                      "\x00\x00\x00\x00\x00\x00\x00\x00"s;
    std::string digest_str = message.substr(header.size(), 20);
    std::string peer_id_raw = message.substr(header.size() + 20, 20);

    // get peer_id in hex format
    std::stringstream ss;
    for (unsigned char c : peer_id_raw) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(c);
    }
    return ss.str();
}

void Peer::handshake(std::vector<uint8_t> const& info_hash_raw) const {
    int const socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd <= 0) {
        std::cerr << "Failed creating socket" << std::endl;
        return;
    }

    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_get());
    server_address.sin_addr.s_addr = inet_addr(ip_get().c_str());

    if (connect(socket_fd, (struct sockaddr*)&server_address,
                sizeof(server_address)) < 0) {
        std::cerr << "Failed to connect to peer" << std::endl;
        return;
    }

    // Send the message to server:
    std::string message =
        handshake_message(info_hash_raw, "00112233445566778899");

    if (send(socket_fd, message.c_str(), message.size(), 0) < 0) {
        std::cerr << "Unable to send message" << std::endl;
        return;
    }

    // Receive the server's response:
    unsigned char response[68];
    if (recv(socket_fd, response, sizeof(response), 0) < 0) {
        std::cerr << "Unable to receive message" << std::endl;
        return;
    }
    std::string response_str =
        std::string(reinterpret_cast<char*>(response), sizeof(response));

    std::string peer_id = parse_peer_id(response_str);
    std::cout << "Peer ID: " << peer_id << std::endl;

    if (close(socket_fd) != 0) {
        std::cerr << "Error closing socket"
                  << "\n";
        return;
    }
}

}  // namespace bittorrent