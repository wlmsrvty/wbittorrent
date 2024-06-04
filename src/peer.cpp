#include "peer.hpp"
#include "error.hpp"
#include <arpa/inet.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>

namespace bittorrent {

using namespace std::string_literals;

Peer::Peer(std::string ip, uint16_t port)
    : socket_fd(-1), ip_(ip), port_(port) {}

Peer::~Peer() { this->closeSocket(); }

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

std::error_code Peer::createSocket() {
    if (socket_fd != -1) return {};
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd <= 0)
        return errors::make_error_code(errors::error_code_enum::peer_socket);
    socket_fd = fd;
    return {};
}

std::error_code Peer::closeSocket() {
    if (socket_fd == -1) return {};
    if (::close(socket_fd) != 0) {
        socket_fd = -1;
        return errors::make_error_code(errors::error_code_enum::peer_close);
    }
    socket_fd = -1;
    return {};
}

std::error_code Peer::connect() {
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_);
    server_address.sin_addr.s_addr = inet_addr(ip_.c_str());

    if (::connect(socket_fd, (struct sockaddr*)&server_address,
                  sizeof(server_address)) < 0)
        return errors::make_error_code(errors::error_code_enum::peer_connect);
    return {};
}

std::error_code Peer::establish_connection() {
    std::error_code ec = createSocket();
    if (ec) return ec;
    return connect();
}

std::error_code Peer::close_connection() { return closeSocket(); }

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

std::error_code Peer::handshake(std::vector<uint8_t> const& info_hash_raw) {
    // Send the message to server:
    std::string message =
        handshake_message(info_hash_raw, "00112233445566778899");

    if (send(socket_fd, message.c_str(), message.size(), 0) < 0)
        return errors::make_error_code(errors::error_code_enum::peer_send);

    // Receive the server's response:
    unsigned char response[68];
    if (recv(socket_fd, response, sizeof(response), 0) < 0)
        return errors::make_error_code(errors::error_code_enum::peer_recv);
    std::string response_str =
        std::string(reinterpret_cast<char*>(response), sizeof(response));

    std::string peer_id = parse_peer_id(response_str);
    std::cout << "Peer ID: " << peer_id << std::endl;

    return {};
}

}  // namespace bittorrent