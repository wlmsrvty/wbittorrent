#pragma once

#include <cstdint>
#include <vector>

namespace bittorrent {

enum class message_type {
    choke = 0,
    unchoke = 1,
    interested = 2,
    not_interested = 3,
    have = 4,
    bitfield = 5,
    request = 6,
    piece = 7,
    cancel = 8,
};

struct Block {
    uint32_t index;
    uint32_t begin;
    std::vector<uint8_t> data;
};

struct Message {
    Message(message_type type, std::vector<uint8_t>&& payload);
    Message(message_type type);

    static Message make_request(uint32_t index, uint32_t begin,
                                uint32_t length);

    std::vector<uint8_t> serialize() const;
    Block parse_block() const;

    message_type type;
    std::vector<uint8_t> payload;

    static const Message INTERESTED;
};

}  // namespace bittorrent
