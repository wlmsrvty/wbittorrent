#include "peer.hpp"
#include "error.hpp"
#include "lib/utils.hpp"
#include "message.hpp"
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

constexpr unsigned long BLOCK_SIZE = 16 * 1024 / 2;

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

Peer::Peer(std::string ip, uint16_t port)
    : socket_fd(-1), ip_(ip), port_(port), is_choked(true) {}

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

    std::error_code ec = send_all(socket_fd, message.size(),
                                  reinterpret_cast<u_int8_t*>(message.data()));
    if (ec) return ec;

    // Receive the server's response:
    unsigned char response[68];
    ec = recv_all(socket_fd, sizeof(response), response);
    if (ec) return ec;
    std::string response_str =
        std::string(reinterpret_cast<char*>(response), sizeof(response));

    std::string peer_id = parse_peer_id(response_str);
    // std::cout << "Peer ID: " << peer_id << std::endl;

    return {};
}

std::optional<Message> Peer::read_message_raw(std::error_code& ec) {
    // message format: <length><message_id><payload>

    uint8_t length[4];
    ec = recv_all(socket_fd, sizeof(length), length);
    if (ec) return {};

    uint32_t message_length =
        ntohl(*reinterpret_cast<uint32_t*>(length)) -
        1;  // payload length

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

std::error_code Peer::download_file(Torrent const& torrent,
                                    std::string const& file_path) {
    if (socket_fd == -1)
        return errors::make_error_code(
            errors::error_code_enum::peer_no_connection);

    // wait for bitfield message
    std::error_code ec;
    auto get_message = read_message_raw(ec);
    if (get_message.has_value() == false) return ec;
    Message message = get_message.value();

    if (message.type != message_type::bitfield) {
        return errors::make_error_code(
            errors::error_code_enum::message_expected_bitfield);
    }

    // TODO: read bitfield message to check available pieces for this peer

    // send interested message
    message = Message::INTERESTED;
    ec = send_message(message);
    if (ec) return ec;

    // receive unchoke message
    get_message = read_message_raw(ec);
    if (get_message.has_value() == false) return ec;
    message = get_message.value();

    if (message.type != message_type::unchoke) {
        return errors::make_error_code(
            errors::error_code_enum::message_type_not_handled);
    }
    is_choked = false;

    // unsigned long number_blocks_in_piece = torrent.piece_length / BLOCK_SIZE;

    // download pieces

    // which piece
    uint32_t piece_index = 0;

    // which block in piece
    uint32_t block_index = 0;
    uint32_t block_offset = 0;           // begin
    uint32_t block_length = BLOCK_SIZE;  // length

    // download first piece
    uint32_t piece_length = torrent.piece_length;
    std::vector<uint8_t> piece_data(piece_length);

    while (block_index * BLOCK_SIZE < piece_length) {
        // make request
        // std::cout << "make request with " << piece_index << " " <<
        // block_offset
        //           << " " << block_length << std::endl;
        message =
            Message::make_request(piece_index, block_offset, block_length);
        ec = send_message(message);
        if (ec) return ec;
        // std::cout << "sent request" << std::endl;

        // receive block piece
        get_message = read_message_raw(ec);
        if (get_message.has_value() == false) return ec;
        message = get_message.value();
        // std::cout << "received message" << std::endl;

        // check message
        if (message.type == message_type::choke) break;

        if (message.type != message_type::piece) {
            std::cerr << "not piece message received" << std::endl;
            break;
        }

        Block block = message.parse_block();
        // std::cout << "received piece block: " << block.index << " "
        //           << block.begin << std::endl;
        // std::cout << "block length: " << block.data.size() << std::endl;
        // std::cout << "piece length: " << piece_length << std::endl;

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
        std::cerr << "piece hash does not match" << std::endl;
    }

    // write piece to file

    std::ofstream file(file_path, std::ios::binary);
    file.write(reinterpret_cast<char*>(piece_data.data()), piece_data.size());
    file.close();

    std::cout << "Piece 0 downloaded to " << file_path << std::endl;

    return {};
}



}  // namespace bittorrent