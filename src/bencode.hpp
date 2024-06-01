#pragma once

#include "error.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/nonstd/expected.hpp"

namespace bittorrent {
class Bencode {
   public:
    static nonstd::expected<nlohmann::json, bittorrent::errors::Error>
    decode_bencoded_value(std::string const& encoded_value);
};

}  // namespace bittorrent