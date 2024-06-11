#pragma once

#include "error.hpp"
#include "lib/nlohmann/json.hpp"

namespace bittorrent {
class Bencode {
   public:
    static nlohmann::json decode_bencoded_value(
        std::string const& encoded_value, std::error_code& ec);

    static std::string encode_bencoded_value(nlohmann::json const& value);
};

}  // namespace bittorrent