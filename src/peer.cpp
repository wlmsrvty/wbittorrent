#include "peer.hpp"
#include "error.hpp"
#include "lib/utils.hpp"
#include "message.hpp"
#include "spdlog/spdlog.h"
#include "torrent.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>

namespace bittorrent {

using namespace std::string_literals;

constexpr unsigned long BLOCK_SIZE = 16 * 1024;
constexpr char const* PEER_ID = "00112233445566778899";

// ===== Helper functions =====

static std::vector<u_int8_t> recv_all(int socket_fd, size_t size,
                                      std::error_code& ec) {
    std::vector<u_int8_t> buffer(size);
    size_t received = 0;
    while (received < size) {
        ssize_t r =
            recv(socket_fd, buffer.data() + received, size - received, 0);
        if (r < 0) {
            ec = errors::make_error_code(errors::error_code_enum::peer_recv);
            return {};
        }
        received += r;
    }
    return buffer;
}

static std::error_code recv_all(int socket_fd, size_t size, u_int8_t* buffer) {
    size_t received = 0;
    while (received < size) {
        ssize_t r = recv(socket_fd, buffer + received, size - received, 0);
        if (r < 0)
            return errors::make_error_code(errors::error_code_enum::peer_recv);
        received += r;
    }
    return {};
}

static std::error_code send_all(int socket_fd, size_t size, u_int8_t* buffer) {
    size_t sent = 0;
    while (sent < size) {
        ssize_t r = send(socket_fd, buffer + sent, size - sent, 0);
        if (r < 0)
            return errors::make_error_code(errors::error_code_enum::peer_send);
        sent += r;
    }
    return {};
}

// ===== Peer =====

Peer::Peer(std::string ip, uint16_t port, Torrent const& torrent)
    : socket_fd(-1), ip_(ip), port_(port), is_choked(true), torrent(torrent) {}

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

static std::pair<std::string, std::vector<uint8_t>> parse_peer_id(
    std::string const& message) {
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

    return {ss.str(),
            std::vector<uint8_t>(peer_id_raw.begin(), peer_id_raw.end())};
}

std::error_code Peer::handshake(std::vector<uint8_t> const& info_hash_raw) {
    // Send the message to server:
    std::string message = handshake_message(info_hash_raw, PEER_ID);

    std::error_code ec = send_all(socket_fd, message.size(),
                                  reinterpret_cast<u_int8_t*>(message.data()));
    if (ec) return ec;

    // Receive the server's response:
    unsigned char response[68];
    ec = recv_all(socket_fd, sizeof(response), response);
    if (ec) return ec;
    std::string response_str =
        std::string(reinterpret_cast<char*>(response), sizeof(response));

    auto [peer_id, peer_id_raw] = parse_peer_id(response_str);

    this->peer_id = peer_id;
    this->peer_id_raw = peer_id_raw;

    return {};
}

std::optional<Message> Peer::read_message_raw(std::error_code& ec) {
    // message format: <length><message_id><payload>

    uint8_t length[4];
    ec = recv_all(socket_fd, sizeof(length), length);
    if (ec) return {};

    uint32_t message_length =
        ntohl(*reinterpret_cast<uint32_t*>(length)) - 1;  // payload length

    std::vector<u_int8_t> message_id_buffer = recv_all(socket_fd, 1, ec);
    if (ec) return {};
    uint8_t message_id = message_id_buffer[0];

    Message message(static_cast<message_type>(message_id));

    // keepalive message
    if (message_length == 0) return message;

    std::vector<uint8_t> payload = recv_all(socket_fd, message_length, ec);

    if (message_id > static_cast<std::underlying_type_t<message_type>>(
                         message_type::cancel)) {
        ec = errors::make_error_code(
            errors::error_code_enum::message_type_not_handled);
        return {};
    }

    message.payload = std::move(payload);

    return message;
}

std::error_code Peer::send_message(Message const& message) {
    std::vector<uint8_t> message_raw = message.serialize();

    std::error_code ec =
        send_all(socket_fd, message_raw.size(), message_raw.data());
    if (ec) return ec;

    return {};
}

std::error_code Peer::interested_unchoke() {
    if (socket_fd == -1)
        return errors::make_error_code(
            errors::error_code_enum::peer_no_connection);

    if (!is_choked) return {};

    std::error_code ec = send_message(Message::INTERESTED);
    if (ec) return ec;

    auto get_message = read_message_raw(ec);
    if (get_message.has_value() == false) return ec;

    Message message = get_message.value();
    if (message.type != message_type::unchoke) {
        return errors::make_error_code(
            errors::error_code_enum::message_type_not_handled);
    }

    is_choked = false;
    return {};
}

std::error_code Peer::recv_bitfield() {
    if (socket_fd == -1)
        return errors::make_error_code(
            errors::error_code_enum::peer_no_connection);

    std::error_code ec;
    auto get_message = read_message_raw(ec);
    if (get_message.has_value() == false) return ec;
    Message message = get_message.value();

    if (message.type != message_type::bitfield) {
        return errors::make_error_code(
            errors::error_code_enum::message_expected_bitfield);
    }

    return {};
}

std::error_code Peer::download_file(std::string const& file_path) {
    if (socket_fd == -1)
        return errors::make_error_code(
            errors::error_code_enum::peer_no_connection);

    // wait for bitfield message
    std::error_code ec;
    ec = recv_bitfield();
    if (ec) return ec;

    // TODO: read bitfield message to check available pieces for this peer

    // send interested message
    ec = interested_unchoke();
    if (ec) return ec;

    // TODO: download all pieces
    (void)file_path;

    return {};
}

std::vector<uint8_t> Peer::download_piece(size_t piece_index,
                                          std::error_code& ec) {
    if (socket_fd == -1) {
        ec = errors::make_error_code(
            errors::error_code_enum::peer_no_connection);
        return {};
    }

    auto pieces = torrent.piece_hashes();

    if (piece_index >= torrent.piece_hashes().size()) {
        ec = errors::make_error_code(
            errors::error_code_enum::piece_invalid_index);
        return {};
    }

    if (is_choked) {
        ec = interested_unchoke();
        if (ec) return {};
    }

    // which block in piece
    uint32_t block_index = 0;
    uint32_t block_offset = 0;           // begin
    uint32_t block_length = BLOCK_SIZE;  // length

    // get piece length
    uint32_t piece_length = torrent.piece_length;
    if (piece_index == pieces.size() - 1)
        piece_length = torrent.length % torrent.piece_length;

    spdlog::debug("Peer {}: Downloading piece {}, piece_length {}", ip_, piece_index, piece_length);

    // std::cout << "Piece length: " << piece_length << std::endl;

    std::vector<uint8_t> piece_data(piece_length);

    while (block_index * BLOCK_SIZE < piece_length) {
        // std::cout << "Downloading piece " << piece_index << " block "
        //           << block_index << std::endl;
        // std::cout << "block_offset: " << block_offset << std::endl;
        // std::cout << "block length: " << block_length << std::endl;

        // last block size
        if (block_offset + BLOCK_SIZE > piece_length)
            block_length = piece_length - block_offset;

        // make request
        auto message =
            Message::make_request(piece_index, block_offset, block_length);
        ec = send_message(message);
        if (ec) return {};

        // receive block piece
        auto get_message = read_message_raw(ec);
        if (get_message.has_value() == false) return {};
        message = get_message.value();

        // check message
        if (message.type == message_type::choke) break;

        if (message.type != message_type::piece) {
            std::cerr << "not piece message received" << std::endl;
            break;
        }

        Block block = message.parse_block();

        spdlog::debug("Peer {}: Piece {}, block {}, block length {}, downloaded", ip_, piece_index,
                      block_index, block_length);
        std::copy(block.data.begin(), block.data.end(),
                  piece_data.begin() + block.begin);

        block_index++;
        block_offset += BLOCK_SIZE;
    }

    // check piece hash
    std::vector<uint8_t> piece_hash =
        utils::sha1_hash(piece_data.data(), piece_data.size());

    std::vector<uint8_t> expected_piece_hash =
        std::vector<uint8_t>(torrent.pieces.begin() + piece_index * 20,
                             torrent.pieces.begin() + (piece_index + 1) * 20);

    if (piece_hash != expected_piece_hash) {
        ec = errors::make_error_code(
            errors::error_code_enum::piece_hash_mismatch);
        return {};
    }

    spdlog::debug("Peer {}: Piece {} downloaded", ip_, piece_index);

    return piece_data;
}

}  // namespace bittorrent