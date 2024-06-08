#include "message.hpp"
#include <arpa/inet.h>
#include <cassert>

namespace bittorrent {
Message::Message(message_type type, std::vector<uint8_t>&& payload)
    : type(type), payload(std::move(payload)) {}

Message::Message(message_type type) : type(type) {}

static void push_uint32_t(std::vector<uint8_t>& vec, uint32_t value) {
    vec.push_back(reinterpret_cast<uint8_t*>(&value)[0]);
    vec.push_back(reinterpret_cast<uint8_t*>(&value)[1]);
    vec.push_back(reinterpret_cast<uint8_t*>(&value)[2]);
    vec.push_back(reinterpret_cast<uint8_t*>(&value)[3]);
}

std::vector<uint8_t> Message::serialize() const {
    // message format: <length><message_id><payload>
    // length is a 4-byte big-endian integer

    std::vector<uint8_t> message;
    message.reserve(payload.size() + 5);

    uint32_t length = htonl(payload.size() + 1);
    push_uint32_t(message, length);

    message.push_back(static_cast<uint8_t>(type));

    if (payload.size() > 0) {
        message.insert(message.end(), payload.begin(), payload.end());
    }

    return message;
}


Message Message::make_request(uint32_t index, uint32_t begin, uint32_t length) {
    Message message(message_type::request);

    uint32_t index_n = htonl(index);
    uint32_t begin_n = htonl(begin);
    uint32_t length_n = htonl(length);

    push_uint32_t(message.payload, index_n);
    push_uint32_t(message.payload, begin_n);
    push_uint32_t(message.payload, length_n);

    return message;
}

Block Message::parse_block() const {
    // payload format: <index><begin><block>
    assert(type == message_type::piece);
    Block block;
    block.index = ntohl(*reinterpret_cast<uint32_t const*>(payload.data()));
    block.begin = ntohl(*reinterpret_cast<uint32_t const*>(payload.data() + 4));
    block.data = std::vector<uint8_t>(payload.begin() + 8, payload.end());
    return block;
}

const Message Message::INTERESTED = Message(message_type::interested);

}  // namespace bittorrent